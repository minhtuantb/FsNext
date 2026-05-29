#include "DownloadViewModel.h"
#include "TransferListModel.h"
#include "core/services/AuthService.h"
#include "core/services/SettingsService.h"
#include "core/services/TransferService.h"
#include "core/util/FormatUtil.h"
#include "core/util/FshareUrl.h"
#include "platform/PlatformUtils.h"

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QGuiApplication>
#include <QMimeData>
#include <QRegularExpression>

namespace fsnext {

// Kept in sync with UploadViewModel — Complete items linger in the active
// list for 1 hour so users get confirmation, then migrate to history.
static constexpr qint64 kCompleteTtlMs   = 60LL * 60LL * 1000LL;
static constexpr int    kSweepIntervalMs = 60 * 1000;

// Shared with UploadViewModel — see that file for the rationale on coalescing
// raw taskProgressChanged events into a 200 ms (≈5 Hz) flush cadence.
static constexpr int    kProgressCoalesceMs = 200;

DownloadViewModel::DownloadViewModel(TransferService *transferService,
                                     SettingsService *settingsService,
                                     AuthService     *authService,
                                     QObject         *parent)
    : QObject(parent)
    , m_service(transferService)
    , m_settingsService(settingsService)
    , m_auth(authService)
    , m_model(new TransferListModel(this))
    , m_historyModel(new TransferListModel(this))
{
    if (m_settingsService) {
        connect(m_settingsService, &SettingsService::settingsChanged, this,
                &DownloadViewModel::defaultSaveFolderChanged);
    }

    // Soft-archive sweep — runs once a minute, moves aged Complete items
    // out of the active list and prepends them to history (newest-first).
    m_sweepTimer.setInterval(kSweepIntervalMs);
    m_sweepTimer.setSingleShot(false);
    connect(&m_sweepTimer, &QTimer::timeout, this, &DownloadViewModel::sweepCompleted);
    m_sweepTimer.start();

    // Progress-flush timer — re-armed on each incoming taskProgressChanged.
    m_progressFlushTimer.setInterval(kProgressCoalesceMs);
    m_progressFlushTimer.setSingleShot(true);
    connect(&m_progressFlushTimer, &QTimer::timeout,
            this, &DownloadViewModel::flushPendingProgress);

    // On logout, flush all Complete items so the new session starts clean.
    if (m_auth) {
        connect(m_auth, &AuthService::isLoggedInChanged, this, [this]() {
            if (m_auth && !m_auth->isLoggedIn())
                flushCompletedToHistory();
        });
    }

    if (!m_service) return;

    connect(m_service, &TransferService::taskAdded, this, [this](const TransferTask &task) {
        if (task.type != TransferType::Download) return;
        // Historical items replayed from disk are already Complete — route
        // them into the bounded history list. See UploadViewModel for the
        // full rationale (freeze-on-tab-switch with large history files).
        if (task.state == TransferState::Complete) {
            prependHistory(task);
            return;
        }
        m_model->addTask(task);
        refreshRunState();
    });

    connect(m_service, &TransferService::taskProgressChanged, this,
        [this](const QString &id, int64_t bytes, int64_t total, double speed, const QString &eta) {
            // Buffer latest snapshot; flush timer drains ≤5 Hz. See the
            // coalescing notes in UploadViewModel for the full rationale.
            m_pendingProgress[id] = {bytes, total, speed, eta};
            if (!m_progressFlushTimer.isActive())
                m_progressFlushTimer.start();
        });

    connect(m_service, &TransferService::taskCompleted, this, [this](const QString &id) {
        const TransferTask snapshot = m_service->findTask(id);
        // Keep the completed item visible in the active list — user sees a
        // persistent ✓ confirmation. The sweep timer (or logout, or the
        // dismiss button) will move it to history after the TTL.
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

    connect(m_service, &TransferService::taskStateChanged, this,
        [this](const QString &id, TransferState state) {
            // Complete is stamped by taskCompleted (with timestamp) — skip
            // here so we don't wipe the completedAt the sweep relies on.
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

    // ── Folder scan signals ──────────────────────────────────────────────────
    connect(m_service, &TransferService::folderScanStarted, this,
        [this](const QString &groupId, const QString &folderName) {
            m_activeScans++;
            m_scanGroupId    = groupId;
            m_scanFolderName = folderName;
            m_scanFoundFiles = 0;
            emit isScanningChanged();
            emit scanProgressChanged();
        });

    connect(m_service, &TransferService::folderScanProgress, this,
        [this](const QString & /*groupId*/, int /*folders*/, int filesFound) {
            m_scanFoundFiles = filesFound;
            emit scanProgressChanged();
        });

    connect(m_service, &TransferService::folderScanCompleted, this,
        [this](const QString &groupId, int /*totalFiles*/) {
            if (m_activeScans > 0) m_activeScans--;
            if (m_scanGroupId == groupId) {
                m_scanFolderName.clear();
                m_scanGroupId.clear();
            }
            emit isScanningChanged();
        });

    connect(m_service, &TransferService::folderScanFailed, this,
        [this](const QString &groupId, const QString & /*error*/) {
            if (m_activeScans > 0) m_activeScans--;
            if (m_scanGroupId == groupId) {
                m_scanFolderName.clear();
                m_scanGroupId.clear();
            }
            emit isScanningChanged();
        });
}

TransferListModel *DownloadViewModel::model()        const { return m_model; }
TransferListModel *DownloadViewModel::historyModel() const { return m_historyModel; }
QString            DownloadViewModel::totalSpeed()   const { return m_totalSpeed; }

bool DownloadViewModel::hasMoreHistory() const
{
    if (!m_historyModel) return false;
    // Pre-probe state: treat "unknown" as "assume more". Mirrors
    // UploadViewModel — see that class for the full rationale.
    if (m_historyTotalRows < 0) return true;
    return m_historyTotalRows > m_historyModel->count();
}

void DownloadViewModel::refreshHistoryCursor()
{
    const bool wasMore = hasMoreHistory();
    const int total = m_service ? m_service->historyCount(TransferType::Download) : 0;
    m_historyTotalRows = total;
    if (wasMore != hasMoreHistory()) emit hasMoreHistoryChanged();
}

void DownloadViewModel::loadMoreHistory()
{
    if (!m_service || !m_historyModel) return;
    const int offset = m_historyModel->count();
    QVector<TransferTask> page =
        m_service->loadHistoryPage(TransferType::Download, kHistoryPageSize, offset);
    if (!page.isEmpty())
        m_historyModel->appendTasks(page);
    refreshHistoryCursor();
}

QString DownloadViewModel::defaultSaveFolder() const
{
    return m_settingsService ? m_settingsService->effectiveDownloadFolder() : QString{};
}

// ---------------------------------------------------------------------------
// addDownload — accepts one or more URLs separated by newlines.
// Folder URLs (fshare.vn/folder/...) are dispatched to addFolderDownload;
// file URLs go to the regular single-file addDownload path.
// ---------------------------------------------------------------------------
void DownloadViewModel::addDownload(const QString &urls,
                                    const QString &folder,
                                    const QString &password)
{
    if (!m_service) return;

    // Resolve target folder once for all URLs in this batch
    QString targetFolder = folder.trimmed();
    if (targetFolder.isEmpty()) {
        targetFolder = m_settingsService
                         ? m_settingsService->effectiveDownloadFolder()
                         : QDir::homePath();
    } else if (m_settingsService && m_settingsService->downloadFolder() != targetFolder) {
        m_settingsService->setDownloadFolder(targetFolder);
    }

    // Block downloads into protected system directories
    if (PlatformUtils::isSystemFolder(targetFolder)) {
        emit downloadBlocked(
            tr("Cannot download to system folder: \"%1\".\n"
               "Please choose a different destination.")
            .arg(QDir::toNativeSeparators(targetFolder)));
        return;
    }

    // Ensure the base save directory exists
    QDir dir(targetFolder);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral(".")))
            qWarning() << "[FsNext] Failed to create download folder:" << targetFolder;
    }

    const QStringList urlList = urls.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList invalid;
    for (const QString &rawUrl : urlList) {
        const QString trimmed = rawUrl.trimmed();
        if (trimmed.isEmpty()) continue;

        // Parse + canonicalize so token/fragment/case variants reach the
        // API (and the linkcode cache) in a single, stable form.
        const auto parsed = FshareUrl::parse(trimmed);
        if (parsed.kind == FshareUrl::Kind::Invalid) {
            invalid.append(trimmed);
            continue;
        }

        // Use canonicalUrl (not just the bare code) — preserves the
        // share-access ?token= required by the Fshare API for token-gated
        // folder listings and file session creation.
        const QString canonical = FshareUrl::canonicalUrl(trimmed);
        if (parsed.kind == FshareUrl::Kind::Folder)
            m_service->addFolderDownload(canonical, password, targetFolder);
        else
            m_service->addDownload(canonical, password, targetFolder);
    }

    if (!invalid.isEmpty()) {
        emit downloadBlocked(
            tr("Không nhận diện được %1 liên kết (chỉ chấp nhận fshare.vn/file/ hoặc /folder/):\n%2")
            .arg(invalid.size())
            .arg(invalid.join(QLatin1Char('\n'))));
    }
}

void DownloadViewModel::cancelFolderScan(const QString &groupId)
{
    if (m_service) m_service->cancelFolderScan(groupId);
}

void DownloadViewModel::pauseTask(const QString &id)    { if (m_service) m_service->pauseTask(id); }
void DownloadViewModel::resumeTask(const QString &id)   { if (m_service) m_service->resumeTask(id); }
void DownloadViewModel::cancelTask(const QString &id)   { if (m_service) m_service->cancelTask(id); }
void DownloadViewModel::pauseAll()                      { if (m_service) m_service->pauseAll(); }
void DownloadViewModel::resumeAll()                     { if (m_service) m_service->resumeAll(); }
void DownloadViewModel::clearHistory()
{
    if (m_historyModel) m_historyModel->clear();
    if (m_historyTotalRows != -1) {
        m_historyTotalRows = -1;
        emit hasMoreHistoryChanged();
    }
}

// ---------------------------------------------------------------------------
// Soft-archive lifecycle
// ---------------------------------------------------------------------------

void DownloadViewModel::dismissCompleted(const QString &taskId)
{
    archiveCompleted(taskId);
}

void DownloadViewModel::revealInFolder(const QString &localPath) const
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openInExplorer(localPath);
}

void DownloadViewModel::openLocalFile(const QString &localPath) const
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openFile(localPath);
}

QString DownloadViewModel::normalizeShareUrl(const QString &linkcodeOrUrl)
{
    // Download tasks store the FULL share URL in `linkcode` (e.g.
    // "http://www.fshare.vn/file/IBO7PP5CYKN9"); uploads store a bare
    // 12-char code. Prefixing unconditionally produced the double-domain
    // bug ".../file/http://...". Detect "is this already a URL" first —
    // same guard UploadViewModel::openShareLinkInBrowser uses. Also
    // normalise http → https for the canonical share page.
    if (linkcodeOrUrl.startsWith(QStringLiteral("http://"),  Qt::CaseInsensitive)
        || linkcodeOrUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        QString s = linkcodeOrUrl;
        if (s.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive))
            s.replace(0, 7, QStringLiteral("https://"));
        return s;
    }
    return QStringLiteral("https://www.fshare.vn/file/") + linkcodeOrUrl;
}

void DownloadViewModel::openShareUrl(const QString &linkcodeOrUrl) const
{
    if (linkcodeOrUrl.isEmpty()) return;
    const QUrl url(normalizeShareUrl(linkcodeOrUrl));
    if (url.isValid()) QDesktopServices::openUrl(url);
}

void DownloadViewModel::copyShareLink(const QString &linkcodeOrUrl)
{
    if (linkcodeOrUrl.isEmpty()) return;
    const QString url = normalizeShareUrl(linkcodeOrUrl);
    if (QClipboard *cb = QGuiApplication::clipboard())
        cb->setText(url);
    emit shareLinkCopied(url);
}

void DownloadViewModel::archiveCompleted(const QString &id)
{
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

void DownloadViewModel::prependHistory(const TransferTask &task)
{
    m_historyModel->prependTask(task);
    // The old in-memory cap (kHistoryMaxItems) was dropped in the SQLite-
    // backed pager world — evicting the tail would break scroll position for
    // rows the user has already paginated into view. See UploadViewModel's
    // prependHistory for the full rationale.
    const bool wasMore = hasMoreHistory();
    if (m_historyTotalRows >= 0) m_historyTotalRows++;
    if (wasMore != hasMoreHistory()) emit hasMoreHistoryChanged();
}

void DownloadViewModel::sweepCompleted()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 cutoff = now - kCompleteTtlMs;
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

void DownloadViewModel::flushCompletedToHistory()
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

// ---------------------------------------------------------------------------
// isFolderUrl — detect https://www.fshare.vn/folder/<code> (any case / variants)
// ---------------------------------------------------------------------------
bool DownloadViewModel::isFolderUrl(const QString &url)
{
    return FshareUrl::isFolderUrl(url);
}

// ---------------------------------------------------------------------------
// clipboardText — plain-text contents of the system clipboard.
// Used by Main.qml's Ctrl+V handler to route Fshare links into Download.
// ---------------------------------------------------------------------------
QString DownloadViewModel::clipboardText() const
{
    const QClipboard *cb = QGuiApplication::clipboard();
    if (!cb) return {};
    const QMimeData *md = cb->mimeData();
    if (md && md->hasText())
        return md->text();
    return cb->text();
}

// ---------------------------------------------------------------------------
// extractFshareLinks — scan arbitrary text for fshare.vn/file|folder URLs.
// Splits on whitespace so a pasted blob with surrounding prose still yields
// clean, one-per-line output ready for addDownload() or the add-dialog.
// ---------------------------------------------------------------------------
QString DownloadViewModel::extractFshareLinks(const QString &text) const
{
    if (text.isEmpty()) return {};
    // Match http(s)://(www.)?fshare.vn/(file|folder)/<code>(?query) up to a
    // whitespace or quote boundary. Case-insensitive — legacy share pages
    // sometimes capitalise the host.
    static const QRegularExpression kFshareRx(
        QStringLiteral(R"((?i)\bhttps?://(?:www\.)?fshare\.vn/(?:file|folder)/[^\s"'<>]+)"));

    QStringList hits;
    auto it = kFshareRx.globalMatch(text);
    while (it.hasNext()) {
        const QString url = it.next().captured(0).trimmed();
        if (!url.isEmpty() && !hits.contains(url))
            hits.append(url);
    }
    return hits.join(QLatin1Char('\n'));
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DownloadViewModel::updateTotalSpeed()
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
// flushPendingProgress — mirrors UploadViewModel. Swaps the pending buffer
// out so progress events arriving during the flush land in a fresh buffer
// and re-arm the timer independently, rather than being lost to a
// mid-iteration clear().
// ---------------------------------------------------------------------------
void DownloadViewModel::flushPendingProgress()
{
    if (m_pendingProgress.isEmpty()) return;
    QHash<QString, ProgressSnapshot> batch;
    batch.swap(m_pendingProgress);
    for (auto it = batch.cbegin(); it != batch.cend(); ++it) {
        const ProgressSnapshot &p = it.value();
        m_model->updateProgress(it.key(), p.bytes, p.total, p.speed, p.eta);
        m_speedMap[it.key()] = p.speed;
    }
    updateTotalSpeed();
}

void DownloadViewModel::refreshRunState()
{
    // See UploadViewModel::refreshRunState for the full rationale. Paused
    // and Queued both map to "paused" — the toggle's purpose is to answer
    // "is there pausable work running, or pending work I can kick off?"
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
