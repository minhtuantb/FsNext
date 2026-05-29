#include "UploadViewModel.h"
#include "TransferListModel.h"
#include "core/services/TransferService.h"
#include "core/services/AuthService.h"
#include "core/util/FormatUtil.h"
#include "platform/PlatformUtils.h"

#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStringList>
#include <QUrl>

namespace fsnext {

// How long a completed item stays visible in the active list before it
// is migrated to the history list. Chosen so users get a persistent
// visual confirmation after a transfer finishes, without letting
// completed items accumulate indefinitely in the primary view.
static constexpr qint64 kCompleteTtlMs   = 60LL * 60LL * 1000LL;   // 1 hour
static constexpr int    kSweepIntervalMs = 60 * 1000;              // check every minute

// Progress updates are batched into 200 ms windows (≈5 Hz). Progress bars
// don't visually benefit from faster updates, and at 10+ active transfers
// the un-throttled signal rate (10-100 Hz) dominates main-thread CPU.
static constexpr int    kProgressCoalesceMs = 200;

// NOTE: The old in-memory kHistoryMaxItems cap was retired when the
// history backend moved to SQLite. With paged loading from disk, the UI
// can grow the history model as the user scrolls; the DB is the source
// of truth and the only row budget that matters. See prependHistory
// and loadMoreHistory for the replacement strategy.

UploadViewModel::UploadViewModel(TransferService *service, AuthService *auth, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_auth(auth)
    , m_model(new TransferListModel(this))
    , m_historyModel(new TransferListModel(this))
{
    // ── Soft-archive sweep timer ────────────────────────────────────────────
    // Runs once a minute; moves Complete items older than the TTL out of
    // the active list and into the history list (newest-on-top).
    m_sweepTimer.setInterval(kSweepIntervalMs);
    m_sweepTimer.setSingleShot(false);
    connect(&m_sweepTimer, &QTimer::timeout, this, &UploadViewModel::sweepCompleted);
    m_sweepTimer.start();

    // ── Progress-flush timer (single-shot, re-armed on each incoming event) ─
    // Buffers raw taskProgressChanged events into m_pendingProgress; this
    // timer fires kProgressCoalesceMs after the first buffered event and
    // applies the coalesced snapshot to the model + recomputes totalSpeed.
    m_progressFlushTimer.setInterval(kProgressCoalesceMs);
    m_progressFlushTimer.setSingleShot(true);
    connect(&m_progressFlushTimer, &QTimer::timeout,
            this, &UploadViewModel::flushPendingProgress);

    // ── Session boundary ────────────────────────────────────────────────────
    // On logout, flush all completed items so a subsequent login starts
    // with a clean active list. Login itself doesn't need special handling
    // — the model is already empty.
    if (m_auth) {
        connect(m_auth, &AuthService::isLoggedInChanged, this, [this]() {
            if (m_auth && !m_auth->isLoggedIn())
                flushCompletedToHistory();
        });
    }

    if (!m_service) return;

    connect(m_service, &TransferService::taskAdded, this, [this](const TransferTask &task) {
        if (task.type != TransferType::Upload) return;
        // Historical items replayed from disk arrive already marked Complete.
        // Route them straight into the bounded history list instead of the
        // active model — otherwise the "Đang tải" tab would show thousands
        // of finished uploads (and freeze while rendering them).
        if (task.state == TransferState::Complete) {
            // Cache linkcode so the history-row copy-link button still works.
            if (!task.linkcode.isEmpty())
                m_linkcodeMap[task.id] = task.linkcode;
            prependHistory(task);
            return;
        }
        m_model->addTask(task);
        refreshRunState();
    });

    connect(m_service, &TransferService::taskProgressChanged, this,
        [this](const QString &id, int64_t bytes, int64_t total, double speed, const QString &eta) {
            // Buffer the latest snapshot (overwrites older one for same id).
            // The timer drains the buffer at most every kProgressCoalesceMs.
            m_pendingProgress[id] = {bytes, total, speed, eta};
            if (!m_progressFlushTimer.isActive())
                m_progressFlushTimer.start();
        });

    connect(m_service, &TransferService::taskCompleted, this, [this](const QString &id) {
        const TransferTask snapshot = m_service->findTask(id);
        // Cache linkcode so copyLink still works after the task eventually
        // migrates out of the active model (findTask may have dropped it).
        if (!snapshot.linkcode.isEmpty())
            m_linkcodeMap[id] = snapshot.linkcode;

        // Keep the item visible in the active list with a Complete badge.
        // It migrates to history after kCompleteTtlMs (sweep timer) or on
        // logout/dismiss — see sweepCompleted() / dismissCompleted().
        m_model->markCompleted(id, snapshot.linkcode,
                               QDateTime::currentMSecsSinceEpoch());
        m_speedMap.remove(id);
        updateTotalSpeed();
        refreshRunState();
    });

    connect(m_service, &TransferService::taskFailed, this,
        [this](const QString &id, const QString &error) {
            m_model->updateState(id, TransferState::Error, error);
            m_speedMap.remove(id);
            updateTotalSpeed();
            refreshRunState();
        });

    // P7: the file uploaded but applying its password failed — warn the user so
    // they don't assume a public file is protected.
    connect(m_service, &TransferService::filePasswordSetFailed, this,
        [this](const QString &fileName, const QString &message) {
            emit uploadError(tr("Đã tải lên \"%1\" nhưng không đặt được mật khẩu: %2. "
                                "File hiện KHÔNG được bảo vệ.")
                                 .arg(fileName, message));
        });

    connect(m_service, &TransferService::taskStateChanged, this,
        [this](const QString &id, TransferState state) {
            // Complete is handled exclusively by taskCompleted (which stamps
            // completedAt for the sweep). Forwarding it through updateState
            // would clobber the timestamp-carrying fields and prevent the
            // soft-archive lifecycle from working.
            if (state == TransferState::Complete) return;
            m_model->updateState(id, state);
            if (state == TransferState::Paused   ||
                state == TransferState::Cancelled ||
                state == TransferState::Error) {
                m_speedMap.remove(id);
                updateTotalSpeed();
            }
            refreshRunState();
        });
}

TransferListModel *UploadViewModel::model()        const { return m_model; }
TransferListModel *UploadViewModel::historyModel() const { return m_historyModel; }
QString            UploadViewModel::totalSpeed()   const { return m_totalSpeed; }

bool UploadViewModel::hasMoreHistory() const
{
    if (!m_historyModel) return false;
    // Before the first DB count, assume there's more so the UI can show the
    // "load more" affordance on a clean login without waiting for a probe.
    if (m_historyTotalRows < 0) return true;
    return m_historyTotalRows > m_historyModel->count();
}

void UploadViewModel::refreshHistoryCursor()
{
    const bool wasMore = hasMoreHistory();
    const int total = m_service ? m_service->historyCount(TransferType::Upload) : 0;
    m_historyTotalRows = total;
    if (wasMore != hasMoreHistory()) emit hasMoreHistoryChanged();
}

void UploadViewModel::loadMoreHistory()
{
    if (!m_service || !m_historyModel) return;
    const int offset = m_historyModel->count();
    QVector<TransferTask> page =
        m_service->loadHistoryPage(TransferType::Upload, kHistoryPageSize, offset);
    if (!page.isEmpty()) {
        // Cache linkcodes so the history-row copy-link button keeps working
        // for scroll-loaded rows too (same invariant as the login replay path).
        for (const TransferTask &t : page)
            if (!t.linkcode.isEmpty()) m_linkcodeMap[t.id] = t.linkcode;
        m_historyModel->appendTasks(page);
    }
    refreshHistoryCursor();
}

// ---------------------------------------------------------------------------
// addUpload — validates quota before dispatching to TransferService.
// Files that fail quota checks are signalled via uploadError() and skipped.
// ---------------------------------------------------------------------------
void UploadViewModel::addUpload(const QStringList &files,
                                const QString &folderId,
                                const QString &desc,
                                const QString &password,
                                bool secured,
                                bool directLink)
{
    if (!m_service)
        return;

    // Running total of bytes already accepted in THIS batch. The quota check is
    // cumulative: ten files that each fit individually but together exceed free
    // space must not all be enqueued, or the tail would fail at the server with
    // an obscure error. Tracked per webspace pool (secured uploads draw on a
    // separate VIP quota from normal ones).
    int64_t committed = 0;

    // Snapshot the user-quota fields ONCE per batch. The user object can't
    // change between files added in the same call (the AuthService writes
    // happen on the same thread we're on), and currentUser() is heavier than
    // a struct read — it copies a User by value out of the service. The old
    // per-file call repeated that copy for every file in the batch.
    const bool    loggedIn     = m_auth && m_auth->isLoggedIn();
    const User    user         = loggedIn ? m_auth->currentUser() : User{};
    const bool    userIsVip    = loggedIn && user.isVip();
    const int64_t webFree      = loggedIn ? user.webspaceFree()       : -1;
    const int64_t webSecFree   = loggedIn ? user.webspaceSecureFree() : -1;

    // NOTE: The size-of-disk loop below is synchronous on the calling thread.
    // QFileInfo::size() on a local file is a stat() call (microseconds), so for
    // typical batches this is negligible. For uploads sourced from a slow share
    // (SMB/NFS) each stat can take tens of ms and a large drag-drop could stutter
    // the UI. If telemetry confirms the stutter, hoist this whole loop into
    // QtConcurrent::run + post the accept/reject decisions back via invokeMethod.
    // Keeping it sync today preserves the existing same-thread ordering contract
    // (callers see uploadError emissions and queued tasks before addUpload returns).

    for (const QString &file : files) {
        // --- P1-3: Quota / permission check ---
        if (loggedIn) {
            // Resolve file:/// URL → local path so we can read the file size.
            QString localPath = file;
            if (file.startsWith(QLatin1String("file://")))
                localPath = QUrl(file).toLocalFile();
            else if (file.contains(QLatin1Char('%')))
                localPath = QUrl::fromPercentEncoding(file.toUtf8());

            const QFileInfo fi(localPath);
            if (fi.exists() && fi.size() > 0) {
                const int64_t needed = fi.size();

                if (secured) {
                    if (!userIsVip) {
                        emit uploadError(tr("Chỉ tài khoản VIP mới có thể upload file bảo mật."));
                        continue;
                    }
                    if (webSecFree >= 0 && committed + needed > webSecFree) {
                        emit uploadError(
                            tr("Không đủ dung lượng bảo mật. Cần thêm %1 MB, còn %2 MB.")
                                .arg((committed + needed) / (1024LL * 1024))
                                .arg(webSecFree  / (1024LL * 1024)));
                        continue;
                    }
                } else {
                    if (webFree >= 0 && committed + needed > webFree) {
                        emit uploadError(
                            tr("Không đủ dung lượng. Cần thêm %1 MB, còn %2 MB.")
                                .arg((committed + needed) / (1024LL * 1024))
                                .arg(webFree  / (1024LL * 1024)));
                        continue;
                    }
                }
                committed += needed;
            }
        }

        m_service->addUpload(file, folderId, password, desc, secured, directLink);
    }
}

void UploadViewModel::pauseTask(const QString &id)    { if (m_service) m_service->pauseTask(id); }
void UploadViewModel::resumeTask(const QString &id)   { if (m_service) m_service->resumeTask(id); }
void UploadViewModel::cancelTask(const QString &id)   { if (m_service) m_service->cancelTask(id); }
void UploadViewModel::pauseAll()                      { if (m_service) m_service->pauseAll(); }
void UploadViewModel::resumeAll()                     { if (m_service) m_service->resumeAll(); }
void UploadViewModel::clearHistory()
{
    if (m_historyModel) m_historyModel->clear();
    // Reset the cursor to "unknown" — the next hasMoreHistory read will
    // assume-more, and the next loadMoreHistory call will refresh from DB.
    if (m_historyTotalRows != -1) {
        m_historyTotalRows = -1;
        emit hasMoreHistoryChanged();
    }
}

// ---------------------------------------------------------------------------
// copyLinkToClipboard — looks up the linkcode in the active task map first,
// then falls back to the cached m_linkcodeMap populated at task completion.
// ---------------------------------------------------------------------------
void UploadViewModel::copyLinkToClipboard(const QString &taskId)
{
    if (m_service) {
        const TransferTask t = m_service->findTask(taskId);
        if (!t.linkcode.isEmpty()) {
            QGuiApplication::clipboard()->setText(t.linkcode);
            return;
        }
    }
    const QString lc = m_linkcodeMap.value(taskId);
    if (!lc.isEmpty())
        QGuiApplication::clipboard()->setText(lc);
}

// ---------------------------------------------------------------------------
// Post-upload local file actions — resolved by scanning both the active and
// history models (both hold full TransferTask rows) so actions work whether
// the item is "just finished" or "from yesterday's history".
// ---------------------------------------------------------------------------

namespace {
TransferTask findTaskIn(const TransferListModel *m, const QString &id) {
    if (!m) return {};
    for (const TransferTask &t : m->tasks()) {
        if (t.id == id) return t;
    }
    return {};
}
} // namespace

void UploadViewModel::revealLocalFile(const QString &taskId)
{
    TransferTask t = findTaskIn(m_model, taskId);
    if (t.id.isEmpty()) t = findTaskIn(m_historyModel, taskId);
    if (t.sourcePath.isEmpty()) return;
    PlatformUtils::openInExplorer(t.sourcePath);
}

void UploadViewModel::openShareLinkInBrowser(const QString &taskId)
{
    // Prefer live linkcode on the task; fall back to the completion-time
    // cache populated by taskCompleted (same invariant copyLinkToClipboard
    // relies on for tasks that migrated to history after a fresh login).
    QString linkcode;
    TransferTask t = findTaskIn(m_model, taskId);
    if (t.id.isEmpty()) t = findTaskIn(m_historyModel, taskId);
    if (!t.linkcode.isEmpty()) linkcode = t.linkcode;
    if (linkcode.isEmpty())    linkcode = m_linkcodeMap.value(taskId);
    if (linkcode.isEmpty())    return;

    // Fshare's createUploadSession response stores the FULL share URL
    // (e.g. "http://www.fshare.vn/file/MAACH7AIIPSK") in linkcode for the
    // current callsites — copyLinkToClipboard wants that as-is. The
    // earlier version of this function blindly prefixed with
    // "https://www.fshare.vn/file/" which produced double-domain URLs
    // like ".../file/http://...". Detect "is this already a URL" before
    // prefixing so both cases (bare 12-char linkcode + full URL) work.
    QUrl url;
    if (linkcode.startsWith(QStringLiteral("http://"),  Qt::CaseInsensitive)
        || linkcode.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        url = QUrl(linkcode);
    } else {
        url = QUrl(QStringLiteral("https://www.fshare.vn/file/") + linkcode);
    }
    QDesktopServices::openUrl(url);
}

bool UploadViewModel::deleteLocalFile(const QString &taskId)
{
    TransferTask t = findTaskIn(m_model, taskId);
    if (t.id.isEmpty()) t = findTaskIn(m_historyModel, taskId);
    if (t.sourcePath.isEmpty()) {
        emit localFileDeleteFailed(taskId, tr("Không tìm thấy file cục bộ."));
        return false;
    }
    QFile f(t.sourcePath);
    if (!f.exists()) {
        emit localFileDeleteFailed(taskId, tr("File cục bộ đã bị xoá hoặc di chuyển."));
        return false;
    }
    if (!f.remove()) {
        emit localFileDeleteFailed(taskId,
            tr("Không xoá được file: %1").arg(f.errorString()));
        return false;
    }
    emit localFileDeleted(taskId);
    return true;
}

QString UploadViewModel::folderIdOf(const QString &taskId) const
{
    TransferTask t = findTaskIn(m_model, taskId);
    if (t.id.isEmpty()) t = findTaskIn(m_historyModel, taskId);
    return t.folderId;
}

// ---------------------------------------------------------------------------
// Soft-archive lifecycle
// ---------------------------------------------------------------------------

void UploadViewModel::dismissCompleted(const QString &taskId)
{
    archiveCompleted(taskId);
}

void UploadViewModel::archiveCompleted(const QString &id)
{
    // Find the task in the active model; only Complete items are archivable
    // via this path (active/paused tasks are cancelled through the service).
    const auto &tasks = m_model->tasks();
    for (int i = 0; i < tasks.size(); ++i) {
        if (tasks.at(i).id != id) continue;
        if (tasks.at(i).state != TransferState::Complete) return;
        const TransferTask snapshot = tasks.at(i);
        m_model->removeTask(id);
        prependHistory(snapshot);
        refreshRunState();
        return;
    }
}

void UploadViewModel::prependHistory(const TransferTask &task)
{
    m_historyModel->prependTask(task);
    // The old ~20-item hard cap was a safety rail for the all-in-memory
    // history model. With the SQLite-backed pager, evicting the tail would
    // fight with scroll-loaded older rows (breaking scroll position and
    // re-fetching the same page on next scroll). Instead we let the history
    // model grow unbounded in-memory — the on-disk DB is the source of truth
    // and bounded by user-driven pagination, not a fixed window.

    // Live archivals may flip hasMoreHistory when total > loaded was exact
    // and this row came from a previously-unloaded offset. Cheap to always
    // re-check; only emits if the answer flipped.
    const bool wasMore = hasMoreHistory();
    if (m_historyTotalRows >= 0) m_historyTotalRows++;  // we just added a row
    if (wasMore != hasMoreHistory()) emit hasMoreHistoryChanged();
}

void UploadViewModel::sweepCompleted()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 cutoff = now - kCompleteTtlMs;

    // Collect ids first to avoid mutating the model while iterating.
    QStringList ripe;
    const auto &tasks = m_model->tasks();
    for (const TransferTask &t : tasks) {
        if (t.state == TransferState::Complete &&
            t.completedAt > 0 && t.completedAt <= cutoff) {
            ripe.append(t.id);
        }
    }
    for (const QString &id : ripe)
        archiveCompleted(id);
}

void UploadViewModel::flushCompletedToHistory()
{
    QStringList ids;
    const auto &tasks = m_model->tasks();
    for (const TransferTask &t : tasks) {
        if (t.state == TransferState::Complete)
            ids.append(t.id);
    }
    for (const QString &id : ids)
        archiveCompleted(id);
}

void UploadViewModel::moveTaskFirst(const QString &id) { if (m_service) m_service->moveTaskFirst(id); }
void UploadViewModel::moveTaskUp(const QString &id)    { if (m_service) m_service->moveTaskUp(id); }
void UploadViewModel::moveTaskDown(const QString &id)  { if (m_service) m_service->moveTaskDown(id); }
void UploadViewModel::moveTaskLast(const QString &id)  { if (m_service) m_service->moveTaskLast(id); }

// ---------------------------------------------------------------------------
// Private helpers — mirrors DownloadViewModel pattern.
// ---------------------------------------------------------------------------

void UploadViewModel::updateTotalSpeed()
{
    double sum = 0.0;
    for (double s : m_speedMap) sum += s;
    const QString formatted = FormatUtil::humanSpeed(sum);
    if (m_totalSpeed != formatted) {
        m_totalSpeed = formatted;
        emit totalSpeedChanged();
    }
}

// ---------------------------------------------------------------------------
// flushPendingProgress — drain the coalescing buffer. See the kProgressCoalesceMs
// rationale at the top of this file; this is the single point where the buffered
// progress snapshots actually hit the model (and therefore trigger delegate
// repaints in QML).
// ---------------------------------------------------------------------------
void UploadViewModel::flushPendingProgress()
{
    if (m_pendingProgress.isEmpty()) return;
    // Swap out so any progress event arriving *during* the flush lands into
    // a fresh buffer and re-arms the timer on its own, rather than getting
    // silently dropped by a mid-iteration clear().
    QHash<QString, ProgressSnapshot> batch;
    batch.swap(m_pendingProgress);
    for (auto it = batch.cbegin(); it != batch.cend(); ++it) {
        const ProgressSnapshot &p = it.value();
        m_model->updateProgress(it.key(), p.bytes, p.total, p.speed, p.eta);
        m_speedMap[it.key()] = p.speed;
    }
    updateTotalSpeed();
}

void UploadViewModel::refreshRunState()
{
    // "running" wins over "paused"; "paused" wins over "idle". Queued items
    // count as pausable (the user may want to pause everything before anything
    // has actually started transferring), so they map to "paused" here too —
    // "paused" really means "there is pending work the user can push on".
    bool hasActive = false;
    bool hasPausable = false;
    for (const TransferTask &t : m_model->tasks()) {
        if (t.state == TransferState::Active) {
            hasActive = true;
            break;
        }
        if (t.state == TransferState::Queued ||
            t.state == TransferState::Paused) {
            hasPausable = true;
        }
    }
    const QString next = hasActive   ? QStringLiteral("running")
                       : hasPausable ? QStringLiteral("paused")
                                     : QStringLiteral("idle");
    if (m_runState != next) {
        m_runState = next;
        emit runStateChanged();
    }
}

} // namespace fsnext
