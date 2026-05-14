#include "TransferService.h"
#include "FolderExpander.h"
#include "core/api/FshareApi.h"
#include "core/models/AppError.h"
#include "core/repositories/SettingsRepository.h"
#include "core/repositories/HistoryRepository.h"
#include "core/repositories/TransferHistoryDb.h"
#include "core/transfer/DownloadEngine.h"
#include "core/transfer/UploadEngine.h"
#include "core/transfer/TransferOrchestrator.h"
#include "core/util/FileNameSanitizer.h"
#include "core/util/FshareUrl.h"
#include "platform/PlatformUtils.h"
#include <QUuid>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>
#include <QtConcurrent>
#include <QThread>
#include <algorithm>

namespace fsnext {

// ── History shelf sizing ────────────────────────────────────────────────────
// Initial page size for the on-login history replay. Older pages are
// fetched on-demand via `loadHistoryPage()` as the user scrolls the
// history tab — the full on-disk set is never materialised in memory.
static constexpr int    kHistoryMaxPerType  = 20;                    // 20 items per type

// Hard ceiling on how many completed-transfer snapshots the service holds
// in m_completed at once.  `findTask()` and a couple of UI lookups iterate
// this vector, so letting it grow unbounded across a long session costs
// both RAM and (tiny amounts of) CPU.  The authoritative history lives in
// the SQLite DB — anything we evict from memory is still reachable via
// loadHistoryPage() when the user scrolls backwards.
//
// 500 matches the rough upper bound of what the history tab can display
// without scrolling hitting the paginated-fetch path anyway; exceeding
// this in-session is already unusual.
static constexpr int    kCompletedInMemoryCap = 500;

// Drop the oldest (front) items in `v` until it fits `cap`.  Cheap because
// QVector::remove with (pos, count) is a single block move.  No-op when the
// vector is already within budget.
static void trimFrontTo(QVector<TransferTask> &v, int cap)
{
    if (cap <= 0) { v.clear(); return; }
    const int over = v.size() - cap;
    if (over > 0) v.remove(0, over);
}

// ---------------------------------------------------------------------------
// Pick a non-colliding destination path. If `dir/baseName` already exists on
// disk (and is not a resumable partial of the current download — handled by
// the engine itself, which sees `fi.exists() && fi.size() < fileSize`), add
// " (1)", " (2)", … before the extension until a free slot is found.
//
// Returns the full path to use. If the directory is unwritable the caller
// will surface the real error when it tries to open the file.
// ---------------------------------------------------------------------------
static QString uniqueDestinationPath(const QString &dir, const QString &fileName)
{
    QString base = dir;
    if (!base.endsWith(QLatin1Char('/')) && !base.endsWith(QLatin1Char('\\')))
        base += QLatin1Char('/');

    const QString first = base + fileName;
    if (!QFileInfo::exists(first))
        return first;

    QFileInfo fi(fileName);
    const QString stem = fi.completeBaseName();
    const QString dot  = fi.suffix().isEmpty() ? QString{} : QStringLiteral(".") + fi.suffix();

    for (int i = 1; i < 10000; ++i) {
        const QString candidate = base + stem + QStringLiteral(" (") +
                                   QString::number(i) + QStringLiteral(")") + dot;
        if (!QFileInfo::exists(candidate))
            return candidate;
    }
    // Fallback — extremely unlikely; let the engine overwrite in that case.
    return first;
}

TransferService::TransferService(FshareApi *api, SettingsRepository *settings,
                                  TransferOrchestrator *orchestrator,
                                  HistoryRepository *history, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_settings(settings)
    , m_history(history)
    , m_orch(orchestrator)
{
    // ── ADR D12 — progress checkpoint timer ────────────────────────────────
    // Fires on a fixed cadence; the handler walks m_tasks and writes one
    // UPDATE per still-in-flight row.  Timer auto-pauses itself in the
    // handler when there's nothing to checkpoint, so an idle app doesn't
    // wake up the disk every 5 s.
    m_progressFlushTimer.setInterval(kProgressFlushIntervalMs);
    m_progressFlushTimer.setSingleShot(false);
    connect(&m_progressFlushTimer, &QTimer::timeout,
            this, &TransferService::persistProgressSnapshots);

    // Push the user's slot-count preferences into the orchestrator so its
    // BudgetManager enforces them.  Re-applied when SettingsService notifies
    // of a change (wired in AppContext).
    if (m_orch) {
        BudgetManager::Config cfg = m_orch->config();
        cfg.maxDownloadSlots = qBound(1,
            settings->getInt(QStringLiteral("Download/threads"), 8), 16);
        cfg.maxUploadSlots   = qBound(1,
            settings->getInt(QStringLiteral("Upload/threads"),   4), 16);
        // Persisted global cap (0 = disabled; default matches BudgetManager).
        // Read from the raw key so we don't depend on SettingsService being
        // constructed first.
        const int persistedGlobal =
            settings->getInt(QStringLiteral("Transfer/maxGlobalSlots"),
                             cfg.maxGlobalSlots);
        cfg.maxGlobalSlots = qBound(0, persistedGlobal, 32);
        m_orch->setConfig(cfg);

        // Receive dispatch notifications — always on THIS object's thread (the
        // main thread), since the orchestrator lives on its own worker thread
        // and Qt auto-selects QueuedConnection across the boundary.
        connect(m_orch, &TransferOrchestrator::dispatchReady,
                this,   &TransferService::onDispatchReady);
    }
}

void TransferService::loadHistory(const QString &userId)
{
    if (!m_history) return;
    m_currentUserId = userId;

    // ── Background load ─────────────────────────────────────────────────────
    // The old sync path stalled the main thread on first-ever login for any
    // user with a legacy per-user JSON history file (parse + 1000+ INSERTs
    // into SQLite can take 1-2 s cold). Steady-state loads are fast, but we
    // background them too so every tab stays responsive on login regardless
    // of whether the migration has already happened.
    //
    // The worker owns its own TransferHistoryDb connection (Qt's SQL driver
    // is connection-thread-affine, so we can't share m_history's handle).
    // SQLite in WAL mode safely allows multiple readers + a writer against
    // the same DB file, so the main-thread hot-path upsertTask calls won't
    // fight the bg reader.
    const QString dbPath          = m_history->dbPath();
    const QString dlJson          = m_history->legacyJsonPath(userId, HistoryType::Download);
    const QString upJson          = m_history->legacyJsonPath(userId, HistoryType::Upload);
    const int     pageSize        = kHistoryMaxPerType;
    const QString requestedUserId = userId;

    QtConcurrent::run([this, requestedUserId, dbPath, dlJson, upJson, pageSize]() {
        TransferHistoryDb bgDb;
        // A connection-name-per-thread keeps Qt's QSqlDatabase registry happy
        // even if loadHistory fires twice in rapid succession (re-login / user
        // switch). The thread pointer is unique within the process lifetime
        // of any still-running task.
        const QString connName = QStringLiteral("fs_history_bg_%1").arg(
            reinterpret_cast<quintptr>(QThread::currentThread()));
        if (!bgDb.open(dbPath, connName)) {
            qWarning() << "[TransferService] bg history loader failed to open DB";
            return;
        }

        // Run legacy-JSON migration on the worker. Cheap on subsequent
        // launches (the .migrated rename short-circuits re-entry).
        HistoryRepository::migrateLegacyUsingDb(bgDb, dlJson, requestedUserId,
                                                TransferType::Download);
        HistoryRepository::migrateLegacyUsingDb(bgDb, upJson, requestedUserId,
                                                TransferType::Upload);

        QVector<TransferTask> dl = bgDb.loadRecent(
            requestedUserId, TransferType::Download, pageSize, /*offset=*/0);
        QVector<TransferTask> up = bgDb.loadRecent(
            requestedUserId, TransferType::Upload,   pageSize, /*offset=*/0);

        // ADR D12 — also pull rows where state IN (Queued, Active, Paused).
        // These are transfers that were running when the previous session
        // ended (clean exit OR crash); we want them back in the active list
        // so the user sees them immediately and can resume manually.
        QHash<QString, QString> inflightSnapshots;
        QVector<TransferTask> inflight = bgDb.loadInFlight(requestedUserId, &inflightSnapshots);

        // Marshal back to the main thread. `this` is safe to capture because
        // TransferService outlives the worker (AppContext owns it for the
        // entire app lifetime). The userId guard protects against a logout-
        // then-login race where a stale result could arrive after the user
        // changed.
        QMetaObject::invokeMethod(this, [this, dl, up, inflight, inflightSnapshots,
                                          requestedUserId]() {
            if (m_currentUserId != requestedUserId) {
                qDebug() << "[TransferService] bg history result discarded (user changed)";
                return;
            }
            m_completed.clear();
            m_completed.append(dl);
            m_completed.append(up);
            // Initial replay is bounded by kHistoryMaxPerType × 2 so this trim
            // is a no-op today, but it keeps the invariant in one place for
            // when that cap grows.
            trimFrontTo(m_completed, kCompletedInMemoryCap);
            qDebug() << "[TransferService] Loaded" << dl.size() << "downloads and"
                     << up.size() << "uploads (page 0, bg) for user" << requestedUserId;
            for (const auto &t : m_completed) emit taskAdded(t);

            if (!inflight.isEmpty()) {
                qInfo() << "[TransferService] Resuming" << inflight.size()
                        << "in-flight tasks from previous session";
                resumeInFlightTasks(inflight, inflightSnapshots);
            }
        }, Qt::QueuedConnection);
    });
}

void TransferService::resumeInFlightTasks(const QVector<TransferTask> &inflight,
                                           const QHash<QString, QString> &snapshots)
{
    // Bring each row back as Paused.  Auto-resuming silently after a crash is
    // surprising — the user might be on a metered connection or have moved
    // since.  Showing the row in the active list with a "Tiếp tục" button is
    // enough; the user clicks Resume → resumeTask() → engine restarts at
    // bytesTransferred (CURL CURLOPT_RESUME_FROM_LARGE handles the byte-range).
    for (const TransferTask &raw : inflight) {
        TransferTask t = raw;
        t.state = TransferState::Paused;

        // Decode the most recent checkpoint so the UI shows the resumed
        // bytes/progress instead of 0.  Schema is intentionally tiny so a
        // version mismatch falls through gracefully (caller sees 0 progress
        // but engine still resumes from disk via CURLOPT_RESUME_FROM_LARGE).
        const auto sit = snapshots.find(t.id);
        if (sit != snapshots.end() && !sit.value().isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(sit.value().toUtf8());
            if (doc.isObject()) {
                const QJsonObject obj = doc.object();
                t.bytesTransferred = static_cast<int64_t>(
                    obj.value(QStringLiteral("bytes")).toDouble(0));
                t.fileSize = static_cast<int64_t>(
                    obj.value(QStringLiteral("total")).toDouble(t.fileSize));
                t.retryCount = obj.value(QStringLiteral("retry")).toInt(t.retryCount);
                if (t.fileSize > 0)
                    t.progress = (static_cast<double>(t.bytesTransferred) / t.fileSize) * 100.0;
            }
        }

        m_tasks.append(t);
        // Treat resumed tasks as Background priority so they don't pre-empt a
        // user-initiated download/upload that comes later in the same session.
        m_priorities[t.id] = TransferPriority::Background;
        emit taskAdded(t);
        emit taskStateChanged(t.id, t.state);
    }
}

void TransferService::persistProgressSnapshots()
{
    if (!m_history || m_currentUserId.isEmpty() || m_tasks.isEmpty()) {
        // Nothing to checkpoint — go idle so we don't keep the disk awake.
        m_progressFlushTimer.stop();
        return;
    }
    int written = 0;
    for (const TransferTask &t : m_tasks) {
        // Only checkpoint rows that are *making progress* (Active) or that
        // the user explicitly Paused — Queued tasks have no progress yet,
        // Complete/Failed are persisted by the legacy upsertTask path.
        if (t.state != TransferState::Active && t.state != TransferState::Paused)
            continue;

        QJsonObject snap;
        snap[QStringLiteral("bytes")] = static_cast<double>(t.bytesTransferred);
        snap[QStringLiteral("total")] = static_cast<double>(t.fileSize);
        snap[QStringLiteral("retry")] = t.retryCount;
        const QString json = QString::fromUtf8(QJsonDocument(snap).toJson(QJsonDocument::Compact));

        const HistoryType ht = (t.type == TransferType::Upload)
                                ? HistoryType::Upload : HistoryType::Download;
        if (m_history->saveProgressSnapshot(m_currentUserId, ht, t.id, t.state, json))
            ++written;
    }
    if (written == 0)
        m_progressFlushTimer.stop();   // nothing actually saveable; idle
}

QVector<TransferTask> TransferService::loadHistoryPage(TransferType type,
                                                        int limit,
                                                        int offset) const
{
    if (!m_history || m_currentUserId.isEmpty() || limit <= 0)
        return {};
    const HistoryType ht = (type == TransferType::Upload)
                           ? HistoryType::Upload
                           : HistoryType::Download;
    return m_history->loadRecent(m_currentUserId, ht, limit, offset);
}

int TransferService::historyCount(TransferType type) const
{
    if (!m_history || m_currentUserId.isEmpty()) return 0;
    const HistoryType ht = (type == TransferType::Upload)
                           ? HistoryType::Upload
                           : HistoryType::Download;
    return m_history->countAll(m_currentUserId, ht);
}

TransferService::~TransferService()
{
    // Abort in-progress folder scans (workers check the flag at safe checkpoints)
    for (auto *exp : m_expanders) exp->abort();
    qDeleteAll(m_expanders);
    m_expanders.clear();

    // Signal all engines to abort (non-blocking)
    for (auto *engine : m_engines)       engine->cancel();
    for (auto *engine : m_uploadEngines) engine->cancel();

    // Signal all threads to quit (non-blocking)
    for (auto *thread : m_threads) thread->quit();

    // Wait in parallel — give each a short timeout so total shutdown is bounded
    const int kPerThreadWaitMs = 500;
    for (auto *thread : m_threads) {
        if (!thread->wait(kPerThreadWaitMs)) {
            qWarning() << "[FsNext] Thread did not exit in" << kPerThreadWaitMs
                       << "ms, terminating";
            thread->terminate();
            thread->wait(200);
        }
    }

    qDeleteAll(m_engines);
    qDeleteAll(m_uploadEngines);
    qDeleteAll(m_threads);
}

void TransferService::addDownload(const QString &url, const QString &password, const QString &savePath)
{
    // Canonicalize scheme/host/casing. PRESERVES `?token=XXX` — required by
    // the Fshare API for token-gated share links. Other query params and the
    // fragment are dropped so cache keys stay stable.
    const QString canonicalUrl = FshareUrl::canonicalUrl(url);
    const QString effectiveUrl = canonicalUrl.isEmpty() ? url : canonicalUrl;

    // ── Disk-space pre-check (ADR 003 D6) ──────────────────────────────────
    // We don't yet know the file's exact size — that requires createDownloadSession.
    // Pre-check uses a 50 MB minimum-headroom heuristic so the obvious
    // "C: full of 4 MB free" case fails fast with a clear toast.  The real
    // size check happens once DownloadEngine receives Content-Length.
    if (!savePath.isEmpty()) {
        const int64_t free = PlatformUtils::freeDiskSpace(savePath);
        constexpr int64_t kMinHeadroom = 50LL * 1024 * 1024;
        if (free >= 0 && free < kMinHeadroom) {
            // Reuse the system-folder block channel — same UX semantics
            // ("we did not enqueue, here's why").  AppContext routes this to
            // a toast in DownloadPage.
            qWarning() << "[TransferService] Disk too full for download: free="
                       << free << "bytes at" << savePath;
            // No taskAdded() — the user sees a toast and the queue stays unchanged.
            return;
        }
    }

    TransferTask task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type = TransferType::Download;
    task.state = TransferState::Queued;
    task.linkcode = effectiveUrl;
    task.password = password;
    task.localPath = savePath;
    task.fileName = effectiveUrl.contains('/') ? effectiveUrl.mid(effectiveUrl.lastIndexOf('/') + 1) : effectiveUrl;

    // Read segment count from settings on each call so live changes take effect.
    const int segs = m_settings ? m_settings->getInt(QStringLiteral("Download/segments"), 16) : 16;
    task.segments = qBound(1, segs, 32);

    // Conflict-policy field — DownloadEngine reads it when it knows the real
    // filename to write to.  See ADR 003 D7.
    task.fileConflictPolicy = m_settings
        ? m_settings->getInt(QStringLiteral("Download/conflictPolicy"), 0)
        : 0;

    m_tasks.append(task);
    m_priorities[task.id] = TransferPriority::Interactive;
    emit taskAdded(task);
    if (m_orch) m_orch->enqueue(task.id, TransferClass::Download, TransferPriority::Interactive);
}

void TransferService::addFolderDownload(const QString &folderUrl,
                                        const QString &password,
                                        const QString &savePath)
{
    const int segs = m_settings ? m_settings->getInt(QStringLiteral("Download/segments"), 16) : 16;

    // Canonicalize the folder URL for the listFiles / getFileInfo calls.
    // PRESERVES the share-access `?token=XXX` so token-gated folder shares
    // (e.g. https://www.fshare.vn/folder/ABC?token=123) can be listed.
    const QString canonicalFolder = FshareUrl::canonicalUrl(folderUrl);
    const QString effectiveFolder = canonicalFolder.isEmpty() ? folderUrl : canonicalFolder;

    auto *expander = new FolderExpander(m_api, effectiveFolder, savePath, password,
                                        qBound(1, segs, 32),
                                        /*maxDepth=*/20,
                                        /*parent=*/nullptr);
    const QString groupId = expander->groupId();
    m_expanders[groupId] = expander;

    // Derive a short display name from the URL (last path segment = folder code)
    // until the real folder name is resolved by the expander's getFileInfo() call.
    const QString shortName = effectiveFolder.section(QLatin1Char('/'), -1, -1);
    emit folderScanStarted(groupId, shortName);

    // ── Connect expander signals ─────────────────────────────────────────────
    connect(expander, &FolderExpander::scanProgress, this,
        [this, groupId](int folders, int files) {
            emit folderScanProgress(groupId, folders, files);
        }, Qt::QueuedConnection);

    connect(expander, &FolderExpander::completed, this,
        [this, groupId](const QVector<TransferTask> &tasks) {
            // Enqueue every file as an independent download task.
            for (const TransferTask &task : tasks) {
                m_tasks.append(task);
                m_priorities[task.id] = TransferPriority::Interactive;
                emit taskAdded(task);
                if (m_orch)
                    m_orch->enqueue(task.id, TransferClass::Download,
                                     TransferPriority::Interactive);
            }
            emit folderScanCompleted(groupId, tasks.size());
        }, Qt::QueuedConnection);

    connect(expander, &FolderExpander::failed, this,
        [this, groupId](const QString &error) {
            emit folderScanFailed(groupId, error);
        }, Qt::QueuedConnection);

    connect(expander, &FolderExpander::finished, this,
        [this, groupId]() {
            if (auto *exp = m_expanders.take(groupId))
                exp->deleteLater();
        }, Qt::QueuedConnection);

    expander->start();
}

void TransferService::cancelFolderScan(const QString &groupId)
{
    if (auto *exp = m_expanders.value(groupId)) {
        exp->abort();
        // finished() signal will fire from the worker thread and trigger cleanup.
    }
}

void TransferService::addUpload(const QString &localPath, const QString &folderId,
                                const QString &password, const QString &description,
                                bool secured, bool directLink)
{
    // QML url types are percent-encoded "file:///..." strings.
    // Convert to a proper OS-local path before using QFileInfo.
    QString resolvedPath = localPath;
    if (localPath.startsWith(QLatin1String("file://"))) {
        resolvedPath = QUrl(localPath).toLocalFile();
    } else if (localPath.contains(QLatin1Char('%'))) {
        // "file:///" was already stripped but path may still be percent-encoded
        resolvedPath = QUrl::fromPercentEncoding(localPath.toUtf8());
    }

    QFileInfo fi(resolvedPath);
    if (!fi.exists()) {
        qWarning() << "[FsNext] Upload source not found:" << resolvedPath;
        // Surface the error via signal only — do NOT add to m_tasks so there
        // is no leak of tasks that have no engine and can never be cleaned up.
        TransferTask errTask;
        errTask.id           = QUuid::createUuid().toString(QUuid::WithoutBraces);
        errTask.type         = TransferType::Upload;
        errTask.state        = TransferState::Error;
        errTask.fileName     = resolvedPath.section('/', -1).section('\\', -1);
        errTask.errorMessage = QStringLiteral("File không tìm thấy: %1").arg(resolvedPath);
        emit taskAdded(errTask);
        emit taskFailed(errTask.id, errTask.errorMessage);
        return;
    }

    // Reject 0-byte files up-front — Fshare API rejects them with an obscure error.
    if (fi.size() == 0) {
        TransferTask errTask;
        errTask.id           = QUuid::createUuid().toString(QUuid::WithoutBraces);
        errTask.type         = TransferType::Upload;
        errTask.state        = TransferState::Error;
        errTask.fileName     = fi.fileName();
        errTask.errorMessage = QStringLiteral("Không thể upload file rỗng (0 bytes).");
        emit taskAdded(errTask);
        emit taskFailed(errTask.id, errTask.errorMessage);
        return;
    }

    // Strip characters Fshare API rejects in the 'name' field.
    // The 2026 server enforces a stricter blocklist than Windows filenames —
    // single chars:          \ / : * ? " < > | ! , @ # $ ^
    // forbidden sequences:   .. (double-dot)   -- (double-hyphen)
    // Anything matching is replaced with '_' so the resulting name is still
    // human-readable (vs silent deletion that can merge unrelated tokens).
    static const QRegularExpression kIllegalChars(R"([\\/:*?"<>|!,@#$^])");
    static const QRegularExpression kIllegalSeqs(R"(\.\.+|--+)");
    QString cleanName = fi.fileName();
    const QString origName = cleanName;
    cleanName.replace(kIllegalChars, QStringLiteral("_"));
    cleanName.replace(kIllegalSeqs,  QStringLiteral("_"));
    // BUG-3: strip leading/trailing whitespace. The previous code only
    // checked trimmed-emptiness but sent the untrimmed string to the server.
    cleanName = cleanName.trimmed();
    if (cleanName != origName)
        qWarning() << "[FsNext] Sanitized filename for API:" << origName << "→" << cleanName;
    if (cleanName.isEmpty()) {
        TransferTask errTask;
        errTask.id           = QUuid::createUuid().toString(QUuid::WithoutBraces);
        errTask.type         = TransferType::Upload;
        errTask.state        = TransferState::Error;
        errTask.fileName     = fi.fileName();
        errTask.errorMessage = QStringLiteral("Tên file không hợp lệ (chứa toàn ký tự đặc biệt).");
        emit taskAdded(errTask);
        emit taskFailed(errTask.id, errTask.errorMessage);
        return;
    }

    TransferTask task;
    task.id          = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type        = TransferType::Upload;
    task.state       = TransferState::Queued;
    task.sourcePath  = resolvedPath;
    task.folderId    = folderId;
    task.password    = password;
    task.description = description;
    task.secured     = secured;
    task.directLink  = directLink;
    task.fileName    = cleanName;
    task.fileSize    = fi.size();

    m_tasks.append(task);
    m_priorities[task.id] = TransferPriority::Interactive;
    emit taskAdded(task);
    if (m_orch) m_orch->enqueue(task.id, TransferClass::Upload, TransferPriority::Interactive);
}

QString TransferService::addSyncUpload(const QString &localPath,
                                       const QString &fsharePath,
                                       const QString &syncFolderId,
                                       const QString &syncRelPath,
                                       int64_t speedLimitBps,
                                       TransferPriority priority)
{
    QFileInfo fi(localPath);
    if (!fi.exists() || fi.size() == 0) {
        qWarning() << "[TransferService] addSyncUpload: skipping invalid file" << localPath;
        return {};
    }

    static const QRegularExpression kIllegalChars(R"([\\/:*?"<>|!,@#$^])");
    static const QRegularExpression kIllegalSeqs(R"(\.\.+|--+)");
    QString cleanName = fi.fileName();
    cleanName.replace(kIllegalChars, QStringLiteral("_"));
    cleanName.replace(kIllegalSeqs,  QStringLiteral("_"));
    cleanName = cleanName.trimmed();
    if (cleanName.isEmpty()) return {};

    TransferTask task;
    task.id             = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type           = TransferType::Upload;
    task.state          = TransferState::Queued;
    task.sourcePath     = localPath;
    task.folderId       = fsharePath;
    task.secured        = true;            // sync files are private by default
    task.directLink     = false;
    task.fileName       = cleanName;
    task.fileSize       = fi.size();
    task.isSyncTask     = true;
    task.speedLimitBps  = speedLimitBps;
    task.syncFolderId   = syncFolderId;
    task.syncRelPath    = syncRelPath;

    m_tasks.append(task);
    m_priorities[task.id] = priority;
    emit taskAdded(task);
    if (m_orch) m_orch->enqueue(task.id, TransferClass::Upload, priority);
    return task.id;
}

// ---------------------------------------------------------------------------
// Dispatch path — called on the main thread by TransferOrchestrator once a
// slot is available.  This is where the actual session-create + engine-spawn
// work happens.  The orchestrator has already enforced slot cap / priority
// floor / global cap before getting here.
// ---------------------------------------------------------------------------
void TransferService::onDispatchReady(const QString &id, TransferClass cls,
                                      TransferPriority /*prio*/)
{
    if (cls == TransferClass::Download)      dispatchDownload(id);
    else if (cls == TransferClass::Upload)   dispatchUpload(id);
    // Metadata class is consumed by FileSyncWorker, not this service.
}

void TransferService::dispatchUpload(const QString &taskId)
{
    // Flip the task to Active and snapshot for the cross-thread lambda.
    TransferTask taskSnapshot;
    for (auto &task : m_tasks) {
        if (task.id != taskId) continue;
        if (task.type != TransferType::Upload) return;
        task.state   = TransferState::Active;
        taskSnapshot = task;
        break;
    }
    if (taskSnapshot.id.isEmpty()) {
        // Task was cancelled between enqueue() and dispatchReady — release so
        // the budget counter stays correct.
        if (m_orch) m_orch->release(taskId);
        return;
    }

    emit taskStateChanged(taskId, TransferState::Active);

    auto *api = m_api;
    QPointer<TransferService> guard(this);
    QtConcurrent::run([api, guard, taskId, taskSnapshot]() mutable {
        auto resp = api->createUploadSession(
            taskSnapshot.fileName, taskSnapshot.fileSize,
            taskSnapshot.folderId, taskSnapshot.secured);

        QMetaObject::invokeMethod(guard.data(), [guard, taskId, resp, taskSnapshot]() mutable {
            if (!guard) return;
            auto *self = guard.data();

            // BUG-6: if the user cancelled the task while createUploadSession
            // was in flight, the task is gone from m_tasks. Bail out — do NOT
            // spawn an orphan engine/thread or leak the active-slot counter.
            bool stillQueued = false;
            for (const auto &t : self->m_tasks) {
                if (t.id == taskId) { stillQueued = true; break; }
            }
            if (!stillQueued) {
                qDebug() << "[TransferService] Upload" << taskId
                         << "cancelled before session create returned; dropping response.";
                return;
            }

            if (resp.isError()) {
                // Detect session-expired (HTTP 201/202) so the UI can route
                // the user back to the login screen instead of showing a raw
                // "HTTP 201" error that looks like a bug.
                if (resp.error().category == ErrorCategory::Auth)
                    emit self->sessionExpired(resp.error().message);
                self->onUploadFailed(taskId, resp.error().message);
                return;
            }

            TransferTask task = taskSnapshot;
            task.realUrl = resp.data();
            for (auto &t : self->m_tasks) if (t.id == taskId) { t.realUrl = task.realUrl; break; }

            auto *engine = new UploadEngine();
            auto *thread = new QThread();
            engine->moveToThread(thread);
            self->m_uploadEngines[taskId] = engine;
            self->m_threads[taskId] = thread;

            connect(engine, &UploadEngine::progressChanged, self,
                [self, taskId](int64_t b, int64_t t, double s, const QString &e) { self->onUploadProgress(taskId, b, t, s, e); });
            connect(engine, &UploadEngine::completed, self,
                [self, taskId](const QString &lc) { self->onUploadComplete(taskId, lc); });
            connect(engine, &UploadEngine::failed, self,
                [self, taskId](const QString &err) { self->onUploadFailed(taskId, err); });
            connect(engine, &UploadEngine::sessionExpired, self,
                [self, taskId]() { self->onUploadSessionExpired(taskId); });
            connect(thread, &QThread::started, engine, [engine, task]() { engine->startUpload(task); });

            thread->start();
        });
    });
}

void TransferService::onUploadProgress(const QString &taskId, int64_t bytes, int64_t total, double speed, const QString &eta)
{
    for (auto &t : m_tasks) {
        if (t.id == taskId) { t.bytesTransferred = bytes; t.fileSize = total; t.speed = speed; t.eta = eta; t.progress = t.progressPercent(); break; }
    }
    emit taskProgressChanged(taskId, bytes, total, speed, eta);

    // ADR D12: same pattern as the download path — wake the timer once,
    // it self-suspends when the queue clears.
    if (!m_progressFlushTimer.isActive())
        m_progressFlushTimer.start();
}

void TransferService::onUploadComplete(const QString &taskId, const QString &linkcode)
{
    if (m_orch) m_orch->release(taskId);
    m_priorities.remove(taskId);

    TransferTask snapshot;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == taskId) {
            m_tasks[i].state = TransferState::Complete;
            m_tasks[i].progress = 100.0;
            m_tasks[i].linkcode = linkcode;
            // Stamp the completion time on the authoritative task record so
            // the history file persists it (instead of being re-stamped
            // "now" every time the whole list is saved — that used to make
            // every historical item look fresh forever).
            m_tasks[i].completedAt = QDateTime::currentMSecsSinceEpoch();
            snapshot = m_tasks[i];      // capture password / description before removal
            m_completed.append(m_tasks[i]);
            trimFrontTo(m_completed, kCompletedInMemoryCap);
            m_tasks.remove(i);
            break;
        }
    }
    if (auto *e = m_uploadEngines.take(taskId)) e->deleteLater();
    if (auto *t = m_threads.take(taskId)) { t->quit(); t->wait(1000); t->deleteLater(); }
    emit taskCompleted(taskId);

    // Record linkcode → source path for file manager "on computer" indicator.
    if (!snapshot.linkcode.isEmpty() && !snapshot.sourcePath.isEmpty())
        emit transferRecordReady(snapshot.linkcode, snapshot.sourcePath,
                                 snapshot.fileName, snapshot.fileSize,
                                 QStringLiteral("upload"));

    // Tell the file manager to re-sync the destination folder so the newly
    // uploaded file shows up without the user having to refresh manually.
    // snapshot.folderId here is the Fshare PATH ("/", "/Sub") — FsUploadDialog
    // and TransferService store it as a path after the upload-path fix.
    emit uploadCompleted(snapshot.folderId);

    // Sync-specific routing: SyncService is the only subscriber.
    if (snapshot.isSyncTask && !snapshot.syncFolderId.isEmpty())
        emit syncUploadFinished(snapshot.syncFolderId, snapshot.syncRelPath,
                                /*success=*/true, linkcode, QString{});

    // Apply file password if the user set one before uploading.
    // setFilePassword is a blocking call — run it off the main thread.
    if (!snapshot.linkcode.isEmpty() && !snapshot.password.isEmpty()) {
        auto *api = m_api;
        const QString lc = snapshot.linkcode;
        const QString pw = snapshot.password;
        QtConcurrent::run([api, lc, pw]() {
            const auto res = api->setFilePassword({lc}, pw);
            if (res.isError())
                qWarning() << "[FsNext] setFilePassword failed:" << res.error().message;
        });
    }

    // Persist to history — single-row upsert (O(1) disk cost). The old path
    // rewrote the entire history JSON array on every completion, which at 1K+
    // rows turned each finished upload into a ~10 MB fsync. The SQLite upsert
    // is constant-time and fsync's only the WAL delta for this one row.
    if (m_history && !m_currentUserId.isEmpty() && !snapshot.id.isEmpty()) {
        m_history->upsertTask(m_currentUserId, HistoryType::Upload, snapshot);
    }
}

void TransferService::onUploadFailed(const QString &taskId, const QString &error)
{
    if (m_orch) m_orch->release(taskId);
    m_priorities.remove(taskId);
    TransferTask snapshot;
    for (auto &t : m_tasks) if (t.id == taskId) { t.state = TransferState::Error; t.errorMessage = error; snapshot = t; break; }
    if (auto *e = m_uploadEngines.take(taskId)) e->deleteLater();
    if (auto *t = m_threads.take(taskId)) { t->quit(); t->wait(1000); t->deleteLater(); }

    // Sync-specific failure routing.
    if (snapshot.isSyncTask && !snapshot.syncFolderId.isEmpty())
        emit syncUploadFinished(snapshot.syncFolderId, snapshot.syncRelPath,
                                /*success=*/false, QString{}, error);

    // Propagate auth expiry so the UI can route the user back to login,
    // consistent with how createUploadSession failures are handled above.
    if (error.contains(QLatin1String("session expired"), Qt::CaseInsensitive))
        emit sessionExpired(QStringLiteral("Phiên tải lên hết hạn. Vui lòng đăng nhập lại."));

    emit taskFailed(taskId, error);
}

void TransferService::onUploadSessionExpired(const QString &taskId)
{
    qWarning() << "[FsNext] Upload session expired mid-transfer, auto-renewing:" << taskId;
    // Release the slot so the orchestrator can re-dispatch us when the
    // re-enqueued task floats back to the front of the queue.
    if (m_orch) m_orch->release(taskId);
    if (auto *e = m_uploadEngines.take(taskId)) e->deleteLater();
    if (auto *t = m_threads.take(taskId)) { t->quit(); t->wait(500); t->deleteLater(); }

    // Reset task to Queued and re-enqueue at its original priority.  The
    // orchestrator will create a new session + engine from scratch (byte 0).
    TransferPriority prio = m_priorities.value(taskId, TransferPriority::Interactive);
    for (auto &t : m_tasks) {
        if (t.id == taskId) {
            t.state            = TransferState::Queued;
            t.realUrl.clear();
            t.bytesTransferred = 0;
            t.progress         = 0.0;
            break;
        }
    }
    emit taskStateChanged(taskId, TransferState::Queued);
    if (m_orch) m_orch->enqueue(taskId, TransferClass::Upload, prio);
}

void TransferService::pauseTask(const QString &id)
{
    bool found = false;
    if (auto *engine = m_engines.value(id))        { engine->pause(); found = true; }
    if (auto *engine = m_uploadEngines.value(id))  { engine->pause(); found = true; }
    if (!found) return;
    for (auto &t : m_tasks) if (t.id == id) { t.state = TransferState::Paused; break; }
    emit taskStateChanged(id, TransferState::Paused);
}

void TransferService::resumeTask(const QString &id)
{
    bool found = false;
    if (auto *engine = m_engines.value(id))        { engine->resume(); found = true; }
    if (auto *engine = m_uploadEngines.value(id))  { engine->resume(); found = true; }
    if (!found) return;
    for (auto &t : m_tasks) if (t.id == id) { t.state = TransferState::Active; break; }
    emit taskStateChanged(id, TransferState::Active);
}

void TransferService::cancelTask(const QString &id)
{
    // Signal engines to abort (non-blocking — they will stop at next callback).
    if (auto *engine = m_engines.value(id))       engine->cancel();
    if (auto *engine = m_uploadEngines.value(id)) engine->cancel();

    // Emit BEFORE removing the task so QML delegates can snapshot filename etc.
    emit taskStateChanged(id, TransferState::Cancelled);

    // Determine whether the task held an active orchestrator slot (Active or
    // Paused) vs was still purely Queued — we release() the former and
    // cancelQueued() the latter so the slot counter stays correct.
    bool wasDispatched = false;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id != id) continue;
        const auto &t = m_tasks[i];
        wasDispatched = (t.state == TransferState::Active ||
                         t.state == TransferState::Paused);
        m_tasks.remove(i);
        break;
    }

    // Clean up engine + thread.  The engine was told to abort above;
    // quit() asks the thread's event loop to exit, wait() blocks briefly.
    if (auto *e = m_engines.take(id))      { e->deleteLater(); }
    if (auto *e = m_uploadEngines.take(id)){ e->deleteLater(); }
    if (auto *t = m_threads.take(id))      { t->quit(); t->wait(500); t->deleteLater(); }
    m_priorities.remove(id);

    if (m_orch) {
        if (wasDispatched) {
            m_orch->release(id);
        } else {
            // Still in the scheduler queue — pull it out before it wastes a
            // slot on a task that no longer has a row in m_tasks.
            m_orch->cancelQueued(id);
        }
    }
}

void TransferService::pauseAll()
{
    // Snapshot IDs first — pauseTask() mutates m_tasks via state change
    QStringList ids;
    for (const auto &t : m_tasks) if (t.state == TransferState::Active) ids << t.id;
    for (const auto &id : ids) pauseTask(id);
}

void TransferService::resumeAll()
{
    QStringList ids;
    for (const auto &t : m_tasks) if (t.state == TransferState::Paused) ids << t.id;
    for (const auto &id : ids) resumeTask(id);
}

QVector<TransferTask> TransferService::activeTasks() const { return m_tasks; }
QVector<TransferTask> TransferService::completedTasks() const { return m_completed; }

TransferTask TransferService::findTask(const QString &id) const
{
    for (const auto &t : m_tasks) if (t.id == id) return t;
    for (const auto &t : m_completed) if (t.id == id) return t;
    return {};
}

void TransferService::dispatchDownload(const QString &taskId)
{
    TransferTask taskSnapshot;
    for (auto &task : m_tasks) {
        if (task.id != taskId) continue;
        if (task.type != TransferType::Download) return;
        task.state   = TransferState::Active;
        taskSnapshot = task;
        break;
    }
    if (taskSnapshot.id.isEmpty()) {
        if (m_orch) m_orch->release(taskId);
        return;
    }

    emit taskStateChanged(taskId, TransferState::Active);

    auto *api = m_api;
    QPointer<TransferService> guard(this);
    QtConcurrent::run([api, guard, taskId, taskSnapshot]() mutable {
        auto resp = api->createDownloadSession(taskSnapshot.linkcode, taskSnapshot.password);

        QMetaObject::invokeMethod(guard.data(), [guard, taskId, resp, taskSnapshot]() mutable {
            if (!guard) return;
            auto *self = guard.data();

            // BUG-D6: If the user cancelled while the session request was in
            // flight, the task is gone from m_tasks. Bail out WITHOUT
            // spawning an engine / incrementing m_activeDownloads so the
            // slot isn't leaked and no rogue background transfer starts.
            bool stillQueued = false;
            for (const auto &t : self->m_tasks) {
                if (t.id == taskId) { stillQueued = true; break; }
            }
            if (!stillQueued) {
                qDebug() << "[TransferService] Download" << taskId
                         << "cancelled before session-create returned; aborting cleanly.";
                return;
            }

            if (resp.isError()) {
                if (resp.error().category == ErrorCategory::Auth)
                    emit self->sessionExpired(resp.error().message);
                self->onDownloadFailed(taskId, resp.error().message);
                return;
            }

            TransferTask task = taskSnapshot;
            task.realUrl = resp.data();

            // Resolve the real filename from the download URL returned by
            // Fshare (e.g. https://download.fshare.vn/file/ABC/movie.mp4).
            // The last path segment is the URL-encoded filename.
            {
                const QUrl dlUrl(task.realUrl);
                QString urlFileName = dlUrl.fileName(); // URL-decoded by Qt
                if (urlFileName.isEmpty())
                    urlFileName = task.fileName;         // fallback to linkcode-derived name
                if (urlFileName.isEmpty())
                    urlFileName = QStringLiteral("download");
                // Neutralise path traversal / reserved chars / DOS device names.
                // The server-provided filename is UNTRUSTED — treat it like
                // any other user input before concatenating into a disk path.
                urlFileName = FileNameSanitizer::sanitize(urlFileName);
                task.fileName = urlFileName;

                // task.localPath currently holds just the SAVE FOLDER (from
                // DownloadViewModel). Append the resolved filename to produce
                // the full output file path that DownloadEngine needs.
                QString dir = task.localPath;
                if (dir.isEmpty())
                    dir = QDir::homePath();
                if (!dir.endsWith(QLatin1Char('/')) && !dir.endsWith(QLatin1Char('\\')))
                    dir += QLatin1Char('/');

                // Ensure the directory exists — critical for folder downloads
                // where sub-directories are computed by FolderExpander.
                QDir().mkpath(dir);

                // BUG-D5: If a file with the same name already exists,
                // pick "name (1).ext", "name (2).ext", … instead of
                // silently overwriting the user's previous download.
                // The engine's own resume path keys off fi.size() < fileSize,
                // so collisions with partials are preserved; only completed
                // or unrelated files are renamed.
                task.localPath = uniqueDestinationPath(dir, urlFileName);
            }

            for (auto &t : self->m_tasks) {
                if (t.id == taskId) {
                    t.realUrl   = task.realUrl;
                    t.fileName  = task.fileName;
                    t.localPath = task.localPath;
                    break;
                }
            }

            auto *engine = new DownloadEngine();
            auto *thread = new QThread();
            engine->moveToThread(thread);
            self->m_engines[taskId] = engine;
            self->m_threads[taskId] = thread;

            connect(engine, &DownloadEngine::progressChanged, self,
                [self, taskId](int64_t b, int64_t t, double s, const QString &e) { self->onDownloadProgress(taskId, b, t, s, e); });
            connect(engine, &DownloadEngine::completed, self,
                [self, taskId](const QString &fp) { self->onDownloadComplete(taskId, fp); });
            connect(engine, &DownloadEngine::failed, self,
                [self, taskId](const QString &err) { self->onDownloadFailed(taskId, err); });
            connect(thread, &QThread::started, engine, [engine, task]() { engine->startDownload(task); });

            thread->start();
        });
    });
}

void TransferService::onDownloadProgress(const QString &taskId, int64_t bytes, int64_t total, double speed, const QString &eta)
{
    for (auto &t : m_tasks) {
        if (t.id == taskId) { t.bytesTransferred = bytes; t.fileSize = total; t.speed = speed; t.eta = eta; t.progress = t.progressPercent(); break; }
    }
    emit taskProgressChanged(taskId, bytes, total, speed, eta);

    // ADR D12: ensure the checkpoint timer is running while at least one
    // transfer is making progress.  persistProgressSnapshots stops it again
    // when the queue idles, so we don't pin a wakeup on a quiet app.
    if (!m_progressFlushTimer.isActive())
        m_progressFlushTimer.start();
}

void TransferService::onDownloadComplete(const QString &taskId, const QString &filePath)
{
    Q_UNUSED(filePath);
    if (m_orch) m_orch->release(taskId);
    m_priorities.remove(taskId);
    TransferTask snapshot;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == taskId) {
            m_tasks[i].state = TransferState::Complete;
            m_tasks[i].progress = 100.0;
            // See onUploadComplete — stamp completion time on the task so
            // the repository persists an accurate timestamp instead of
            // refreshing it on every subsequent save.
            m_tasks[i].completedAt = QDateTime::currentMSecsSinceEpoch();
            snapshot = m_tasks[i];
            m_completed.append(m_tasks[i]);
            trimFrontTo(m_completed, kCompletedInMemoryCap);
            m_tasks.remove(i);
            break;
        }
    }
    if (auto *e = m_engines.take(taskId)) e->deleteLater();
    if (auto *t = m_threads.take(taskId)) { t->quit(); t->wait(1000); t->deleteLater(); }
    emit taskCompleted(taskId);

    // Record linkcode → local path for file manager "already downloaded"
    // indicator. Use FshareUrl::linkcodeOf so the key is just the alphanumeric
    // code regardless of any `?token=…` still on the stored URL.
    if (!snapshot.localPath.isEmpty()) {
        const QString lc = FshareUrl::linkcodeOf(snapshot.linkcode);
        if (!lc.isEmpty())
            emit transferRecordReady(lc, snapshot.localPath, snapshot.fileName,
                                     snapshot.fileSize, QStringLiteral("download"));
    }

    // Persist to history — single-row upsert (see rationale at the upload
    // completion site). The downloaded task's snapshot already holds the
    // final completedAt / localPath / linkcode, so one INSERT OR REPLACE
    // is enough.
    if (m_history && !m_currentUserId.isEmpty() && !snapshot.id.isEmpty()) {
        m_history->upsertTask(m_currentUserId, HistoryType::Download, snapshot);
    }
}

void TransferService::onDownloadFailed(const QString &taskId, const QString &error)
{
    if (m_orch) m_orch->release(taskId);
    m_priorities.remove(taskId);
    for (auto &t : m_tasks) if (t.id == taskId) { t.state = TransferState::Error; t.errorMessage = error; break; }
    if (auto *e = m_engines.take(taskId)) e->deleteLater();
    if (auto *t = m_threads.take(taskId)) { t->quit(); t->wait(1000); t->deleteLater(); }
    emit taskFailed(taskId, error);
}

// ── Queue reordering ─────────────────────────────────────────────────────────
// Only Queued Upload tasks may be reordered; Active/Paused tasks keep running.

static int findQueuedUploadIdx(const QVector<TransferTask> &tasks, const QString &id)
{
    for (int i = 0; i < tasks.size(); ++i) {
        if (tasks[i].id == id &&
            tasks[i].type  == TransferType::Upload &&
            tasks[i].state == TransferState::Queued)
            return i;
    }
    return -1;
}

void TransferService::moveTaskFirst(const QString &id)
{
    const int idx = findQueuedUploadIdx(m_tasks, id);
    if (idx < 0) return;
    int firstQueued = -1;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].type == TransferType::Upload && m_tasks[i].state == TransferState::Queued)
            { firstQueued = i; break; }
    }
    if (firstQueued < 0 || idx == firstQueued) return;
    m_tasks.insert(firstQueued, m_tasks.takeAt(idx));
    emit taskOrderChanged();
}

void TransferService::moveTaskLast(const QString &id)
{
    const int idx = findQueuedUploadIdx(m_tasks, id);
    if (idx < 0) return;
    int lastQueued = -1;
    for (int i = m_tasks.size() - 1; i >= 0; --i) {
        if (m_tasks[i].type == TransferType::Upload && m_tasks[i].state == TransferState::Queued)
            { lastQueued = i; break; }
    }
    if (lastQueued < 0 || idx == lastQueued) return;
    m_tasks.insert(lastQueued, m_tasks.takeAt(idx));
    emit taskOrderChanged();
}

void TransferService::moveTaskUp(const QString &id)
{
    const int idx = findQueuedUploadIdx(m_tasks, id);
    if (idx < 0) return;
    int prevQueued = -1;
    for (int i = idx - 1; i >= 0; --i) {
        if (m_tasks[i].type == TransferType::Upload && m_tasks[i].state == TransferState::Queued)
            { prevQueued = i; break; }
    }
    if (prevQueued < 0) return;
    m_tasks.swapItemsAt(idx, prevQueued);
    emit taskOrderChanged();
}

void TransferService::moveTaskDown(const QString &id)
{
    const int idx = findQueuedUploadIdx(m_tasks, id);
    if (idx < 0) return;
    int nextQueued = -1;
    for (int i = idx + 1; i < m_tasks.size(); ++i) {
        if (m_tasks[i].type == TransferType::Upload && m_tasks[i].state == TransferState::Queued)
            { nextQueued = i; break; }
    }
    if (nextQueued < 0) return;
    m_tasks.swapItemsAt(idx, nextQueued);
    emit taskOrderChanged();
}

} // namespace fsnext
