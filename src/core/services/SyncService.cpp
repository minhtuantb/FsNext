#include "SyncService.h"
#include "TransferService.h"
#include "core/api/FshareApi.h"
#include "core/models/TransferTask.h"
#include "core/repositories/SyncRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QThread>
#include <QPointer>
#include <QUuid>
#include <QtConcurrent>

namespace fsnext {

// The pure walk, skip-list predicates, and ignore-pattern parser now live in
// SyncScanner.{h,cpp} so they can run off-main and be unit-tested without the
// FshareApi/TransferService/SQLite dependency graph.  This file owns only the
// stateful, main-thread half of the scan (diff + persist + enqueue).

SyncService::SyncService(TransferService *transfer, FshareApi *api,
                         SyncRepository *repo, QObject *parent)
    : QObject(parent)
    , m_transfer(transfer)
    , m_api(api)
    , m_repo(repo)
    , m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this,      &SyncService::onPathChanged);

    m_rescanTimer.setInterval(kRescanIntervalMs);
    connect(&m_rescanTimer, &QTimer::timeout, this, &SyncService::rescanAll);

    if (m_transfer) {
        connect(m_transfer, &TransferService::syncUploadFinished,
                this,       &SyncService::onUploadFinished);
        // Per-folder progress aggregation: TransferService fires
        // taskProgressChanged for every byte tick; we filter by tasks we
        // tracked in m_taskToFolder, recompute the folder's totals, and
        // re-emit a per-folder signal that the VM listens to.  Doing this
        // bookkeeping here (not in the VM) keeps QML out of per-tick
        // hot-path threading.
        connect(m_transfer, &TransferService::taskProgressChanged, this,
            [this](const QString &id, int64_t bytes, int64_t total,
                   double speed, const QString &eta) {
                if (!m_taskToFolder.contains(id)) return;
                TaskProgress p;
                p.bytesDone  = bytes;
                p.bytesTotal = total;
                p.speedBps   = speed;
                // eta string from TransferService is human-formatted ("~12s"
                // / "—") — parse the leading int when possible so VM can
                // re-format consistently.  -1 means unknown.
                bool ok = false;
                const qint64 etaSecs = eta.split(QChar(' ')).first()
                                          .remove(QChar('~')).remove(QChar('s'))
                                          .toLongLong(&ok);
                p.etaSecs = ok ? etaSecs : -1;
                m_taskProgress.insert(id, p);
                emitFolderProgress(m_taskToFolder.value(id));
            });
    }
}

SyncService::~SyncService() = default;

// ─────────────────────────────────────────────────────────────────────────────
// User lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void SyncService::setUserId(const QString &userId)
{
    if (m_userId == userId) return;
    m_userId = userId;
    m_folders.clear();
    m_files.clear();
    m_createdSubdirs.clear();
    // Drop any task↔folder state from the previous session — stale ids
    // could otherwise trigger a "pause folder" on someone else's tasks
    // after a fast logout/login.
    m_taskToFolder.clear();
    m_folderToTasks.clear();
    m_taskProgress.clear();
    rebuildWatcher();
    m_rescanTimer.stop();

    if (m_userId.isEmpty()) {
        emit foldersChanged();
        return;
    }

    if (m_repo) {
        m_repo->setUserId(m_userId);
        m_folders = m_repo->loadFolders();
        for (const SyncFolder &f : m_folders) {
            const QVector<SyncFileEntry> files = m_repo->loadFiles(f.id);
            auto &map = m_files[f.id];
            for (const SyncFileEntry &e : files) map.insert(e.relPath, e);
        }
        // Master toggle is per-user: load AFTER setUserId so the right key
        // is consulted.  Defaults to true (see SyncRepository::autoSyncEnabled).
        const bool wasEnabled = m_autoSyncEnabled;
        m_autoSyncEnabled = m_repo->autoSyncEnabled();
        if (wasEnabled != m_autoSyncEnabled)
            emit autoSyncEnabledChanged(m_autoSyncEnabled);
    }
    rebuildWatcher();
    m_rescanTimer.start();
    emit foldersChanged();

    // Kick an immediate scan so the UI isn't blank at login.
    for (const SyncFolder &f : m_folders)
        if (f.enabled) scanFolderInternal(f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

QVector<SyncFileEntry> SyncService::filesOf(const QString &folderId) const
{
    QVector<SyncFileEntry> out;
    if (!m_files.contains(folderId)) return out;
    const auto &map = m_files.value(folderId);
    out.reserve(map.size());
    for (const auto &e : map) out.append(e);
    // Stable order by relPath for predictable UI.
    std::sort(out.begin(), out.end(), [](const SyncFileEntry &a, const SyncFileEntry &b) {
        return a.relPath.toLower() < b.relPath.toLower();
    });
    return out;
}

SyncFolder *SyncService::findFolder(const QString &folderId)
{
    for (auto &f : m_folders) if (f.id == folderId) return &f;
    return nullptr;
}
const SyncFolder *SyncService::findFolderConst(const QString &folderId) const
{
    for (const auto &f : m_folders) if (f.id == folderId) return &f;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Folder CRUD
// ─────────────────────────────────────────────────────────────────────────────

bool SyncService::addFolder(const QString &localPath, QString *errorOut)
{
    auto setErr = [errorOut](const QString &msg) { if (errorOut) *errorOut = msg; };

    if (m_userId.isEmpty()) { setErr(QObject::tr("Vui lòng đăng nhập trước")); return false; }

    if (m_folders.size() >= kMaxFolders) {
        setErr(QObject::tr("Đã đạt giới hạn %1 thư mục đồng bộ").arg(kMaxFolders));
        return false;
    }

    const QString normalized = QDir(localPath).absolutePath();
    QFileInfo fi(normalized);
    if (!fi.exists() || !fi.isDir()) {
        setErr(QObject::tr("Thư mục không tồn tại"));
        return false;
    }

    for (const SyncFolder &f : m_folders) {
        if (QDir(f.localPath).absolutePath().compare(normalized, Qt::CaseInsensitive) == 0) {
            setErr(QObject::tr("Thư mục này đã được đồng bộ"));
            return false;
        }
    }

    SyncFolder nf;
    nf.id                = QUuid::createUuid().toString(QUuid::WithoutBraces);
    nf.localPath         = normalized;
    nf.fshareFolderName  = QFileInfo(normalized).fileName();   // leaf name
    if (nf.fshareFolderName.isEmpty())
        nf.fshareFolderName = QStringLiteral("Sync");
    nf.enabled           = true;
    nf.deleteAfterUpload = false;
    nf.createdAt         = QDateTime::currentDateTime();
    // Per-folder settings stay at the documented SyncFolder defaults
    // (recursive walk, no user ignore patterns, kSpeedLimitBps throttle) —
    // this overload exists for v5-style callers that never collected the
    // extra fields from the user.
    nf.watchSubfolders   = true;
    nf.ignorePatterns    = QString{};
    nf.speedLimitBps     = kSpeedLimitBps;

    m_folders.append(nf);
    if (m_repo) m_repo->saveFolder(nf);

    // Activity log: surface "thư mục đã thêm" so the user sees their action
    // reflected before the first file even uploads.  Folder label = leaf so
    // the entry is human-scannable in the recent-activity feed.
    if (m_repo) {
        SyncActivityEntry act;
        act.folderId    = nf.id;
        act.folderLabel = QFileInfo(nf.localPath).fileName();
        act.kind        = SyncActivityKind::FolderAdded;
        act.at          = QDateTime::currentDateTime();
        m_repo->appendActivity(act);
        emit activityChanged();
    }

    rebuildWatcher();
    emit foldersChanged();
    // The sync-root folder on Fshare is created lazily by the scan's worker
    // (ensureSubdirsThenEnqueue) — once, before the first upload — so there
    // is no race between folder creation and upload-session start.
    //
    // User just added this folder → treat its initial scan as Normal priority
    // so the uploads show visible progress instead of being parked behind the
    // Background timer-rescan queue.
    scanFolderInternal(nf, TransferPriority::Normal);
    return true;
}

bool SyncService::addFolder(const QString &localPath,
                            bool watchSubfolders,
                            const QString &ignorePatterns,
                            qint64 speedLimitBps,
                            bool deleteAfterUpload,
                            QString *errorOut)
{
    // Routes through the legacy single-arg overload to share its path
    // normalisation + duplicate / cap / existence checks, then patches the
    // new fields on the folder it just appended.  Keeps the validation in
    // one place — important because the legacy overload's errors (cap hit,
    // dup, missing dir) are user-facing strings we don't want to drift.
    const int sizeBefore = m_folders.size();
    if (!addFolder(localPath, errorOut)) return false;
    if (m_folders.size() <= sizeBefore) return false;     // defensive

    SyncFolder &nf = m_folders.last();
    nf.watchSubfolders   = watchSubfolders;
    nf.ignorePatterns    = ignorePatterns;
    nf.speedLimitBps     = speedLimitBps > 0 ? speedLimitBps : 0;   // 0 = unlimited
    nf.deleteAfterUpload = deleteAfterUpload;
    if (m_repo) m_repo->saveFolder(nf);
    // Refresh the watcher in case watchSubfolders=false demoted the scope
    // away from recursive — the legacy overload's rebuildWatcher() ran
    // before we wrote the field.
    rebuildWatcher();
    emit foldersChanged();
    return true;
}

void SyncService::removeFolder(const QString &folderId)
{
    const int idx = [&]{
        for (int i = 0; i < m_folders.size(); ++i)
            if (m_folders[i].id == folderId) return i;
        return -1;
    }();
    if (idx < 0) return;

    // Snapshot label BEFORE removing so the activity entry has a human-
    // readable folder name even after m_folders no longer contains it.
    const QString removedLabel = QFileInfo(m_folders[idx].localPath).fileName();

    m_folders.remove(idx);
    m_files.remove(folderId);
    m_createdSubdirs.remove(folderId);
    // Tear down the per-folder task bookkeeping last; emits a final 0/0
    // progress so the QML row stops drawing a stale bar after removal.
    forgetFolderTasks(folderId);
    if (m_repo) {
        m_repo->deleteAllFiles(folderId);
        m_repo->deleteFolder(folderId);
        SyncActivityEntry act;
        act.folderId    = folderId;
        act.folderLabel = removedLabel;
        act.kind        = SyncActivityKind::FolderRemoved;
        act.at          = QDateTime::currentDateTime();
        m_repo->appendActivity(act);
        emit activityChanged();
    }
    rebuildWatcher();
    emit foldersChanged();
}

void SyncService::setFolderEnabled(const QString &folderId, bool enabled)
{
    SyncFolder *f = findFolder(folderId);
    if (!f || f->enabled == enabled) return;
    f->enabled = enabled;
    if (m_repo) m_repo->saveFolder(*f);
    emit foldersChanged();
    // Detach/reattach watchers so disabled folders don't keep OS handles open
    // (on Windows this prevents the user from moving/deleting the directory).
    rebuildWatcher();
    // Re-enabling a folder is a user action — run its catch-up scan at Normal
    // priority so pending uploads don't wait behind the Background queue.
    if (enabled) scanFolderInternal(*f, TransferPriority::Normal);
}

void SyncService::setDeleteAfterUpload(const QString &folderId, bool deleteAfter)
{
    SyncFolder *f = findFolder(folderId);
    if (!f || f->deleteAfterUpload == deleteAfter) return;
    f->deleteAfterUpload = deleteAfter;
    if (m_repo) m_repo->saveFolder(*f);
    emit foldersChanged();
}

void SyncService::setWatchSubfolders(const QString &folderId, bool on)
{
    SyncFolder *f = findFolder(folderId);
    if (!f || f->watchSubfolders == on) return;
    f->watchSubfolders = on;
    if (m_repo) m_repo->saveFolder(*f);
    emit foldersChanged();
    // Scope changed: the OS watcher list must drop subdir entries (or grow
    // back) and the file map needs a re-scan so newly in-scope or out-of-
    // scope files transition Pending/Missing correctly.
    rebuildWatcher();
    scanFolderInternal(*f, TransferPriority::Normal);
}

void SyncService::setIgnorePatterns(const QString &folderId, const QString &patterns)
{
    SyncFolder *f = findFolder(folderId);
    if (!f || f->ignorePatterns == patterns) return;
    f->ignorePatterns = patterns;
    if (m_repo) m_repo->saveFolder(*f);
    emit foldersChanged();
    // New patterns may flip some files from Pending → skipped (Missing) or
    // surface freshly-allowed ones — re-scan so the effect is visible
    // without waiting for the 5-min timer tick.
    scanFolderInternal(*f, TransferPriority::Normal);
}

void SyncService::setSpeedLimitBps(const QString &folderId, qint64 bps)
{
    SyncFolder *f = findFolder(folderId);
    if (!f) return;
    const qint64 clamped = bps > 0 ? bps : 0;             // 0 = unlimited
    if (f->speedLimitBps == clamped) return;
    f->speedLimitBps = clamped;
    if (m_repo) m_repo->saveFolder(*f);
    emit foldersChanged();
    // No rescan needed — already in-flight tasks keep their original cap
    // (TransferTask is a value-snapshot); new tasks will pick up the
    // updated rate via ensureSubdirsThenEnqueue's read of f->speedLimitBps.
}

void SyncService::scanFolder(const QString &folderId)
{
    // Explicit user click on "Đồng bộ ngay" — Normal priority so the uploads
    // run ahead of any backlogged timer-rescan work.
    const SyncFolder *f = findFolderConst(folderId);
    if (f) scanFolderInternal(*f, TransferPriority::Normal);
}

void SyncService::resetFolderState(const QString &folderId)
{
    m_files.remove(folderId);
    if (m_repo) m_repo->deleteAllFiles(folderId);
    emit folderFilesChanged(folderId);
    // User just cleared state and is waiting to see the folder repopulate —
    // Normal priority matches the explicit-user-action heuristic above.
    const SyncFolder *f = findFolderConst(folderId);
    if (f) scanFolderInternal(*f, TransferPriority::Normal);
}

SyncService::PreviewResult SyncService::previewScan(const QString &localPath,
                                                    bool watchSubfolders,
                                                    const QString &ignorePatterns) const
{
    PreviewResult out;
    QFileInfo rootFi(localPath);
    if (localPath.isEmpty() || !rootFi.exists() || !rootFi.isDir()) {
        out.errorMessage = QObject::tr("Thư mục không tồn tại");
        return out;
    }

    // Share the exact walk the live scan uses (SyncScanner::scanFilesystem) so
    // the two can't drift — a divergence here would make the preview lie to
    // the user.  Oversized files come back flagged; the preview excludes them
    // (UX-wise they're "won't upload" so they shouldn't inflate the estimate),
    // matching the historical behaviour where the scan logged them as Failed.
    ScanSnapshot snap;
    snap.localPath       = localPath;
    snap.watchSubfolders = watchSubfolders;
    snap.ignorePatterns  = ignorePatterns;

    const ScanResult r = scanFilesystem(snap, kMaxFileSize);
    for (const ScannedFile &sf : r.files) {
        if (sf.oversized) continue;
        ++out.fileCount;
        out.totalBytes += sf.size;
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Scanning
// ─────────────────────────────────────────────────────────────────────────────
//
// The skip-list predicates (scanShouldSkipFile/scanShouldSkipDir) and the pure
// BFS (scanFilesystem) live in SyncScanner.cpp.  See scanFolderInternal /
// applyScanResult below for the stateful main-thread half.

QVector<SyncActivityEntry> SyncService::loadActivity() const
{
    return m_repo ? m_repo->loadActivity() : QVector<SyncActivityEntry>{};
}

void SyncService::clearActivity()
{
    if (m_repo) m_repo->clearActivity();
    emit activityChanged();
}

void SyncService::emitFolderProgress(const QString &folderId)
{
    if (folderId.isEmpty()) return;
    const auto &tasks = m_folderToTasks.value(folderId);
    qint64 done = 0, total = 0, etaMax = -1;
    double speed = 0.0;
    for (const QString &id : tasks) {
        const TaskProgress &p = m_taskProgress.value(id);
        done  += p.bytesDone;
        total += p.bytesTotal;
        speed += p.speedBps;
        if (p.etaSecs > etaMax) etaMax = p.etaSecs;
    }
    emit folderProgressChanged(folderId, done, total, speed, etaMax);
}

void SyncService::forgetFolderTasks(const QString &folderId)
{
    if (folderId.isEmpty()) return;
    const auto tasks = m_folderToTasks.take(folderId);
    for (const QString &id : tasks) {
        m_taskToFolder.remove(id);
        m_taskProgress.remove(id);
    }
    emit folderProgressChanged(folderId, 0, 0, 0.0, -1);
}

void SyncService::pauseFolder(const QString &folderId)
{
    // CRASH_AUDIT M4: m_folderToTasks is only ever touched on the object's
    // thread — TransferService progress signals are delivered queued (main),
    // and pause/resume come from QML (main).  Assert the invariant so any
    // future cross-thread caller trips in a debug build instead of racing.
    Q_ASSERT(QThread::currentThread() == this->thread());
    if (!m_transfer) return;
    const auto tasks = m_folderToTasks.value(folderId);
    for (const QString &id : tasks) m_transfer->pauseTask(id);
}

void SyncService::resumeFolder(const QString &folderId)
{
    Q_ASSERT(QThread::currentThread() == this->thread());   // see pauseFolder (M4)
    if (!m_transfer) return;
    const auto tasks = m_folderToTasks.value(folderId);
    for (const QString &id : tasks) m_transfer->resumeTask(id);
}

void SyncService::retryFailed(const QString &folderId)
{
    // For every file currently marked Failed, flip it back to Pending and
    // run a full scanFolderInternal — the scan loop already re-enqueues
    // Pending entries through the normal createUploadSession + libcurl
    // path so we don't duplicate any of that wiring here.
    if (!m_files.contains(folderId)) return;
    auto &map = m_files[folderId];
    int retried = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->state == SyncFileState::Failed) {
            it->state = SyncFileState::Pending;
            it->errorMessage.clear();
            if (m_repo) m_repo->saveFile(*it);
            ++retried;
        }
    }
    if (retried == 0) return;
    emit folderFilesChanged(folderId);
    const SyncFolder *f = findFolderConst(folderId);
    if (f) scanFolderInternal(*f, TransferPriority::Normal);
}

void SyncService::setAutoSyncEnabled(bool enabled)
{
    if (m_autoSyncEnabled == enabled) return;
    m_autoSyncEnabled = enabled;
    if (m_repo) m_repo->setAutoSyncEnabled(enabled);
    emit autoSyncEnabledChanged(enabled);
    // Re-enabling fires an immediate catch-up scan so the user sees motion
    // without waiting for the 5-min timer or a fresh watcher event.  Disable
    // path leaves any in-flight uploads alone — caller said "pause new", not
    // "cancel everything".
    if (enabled) {
        for (const SyncFolder &f : m_folders) {
            if (f.enabled) scanFolderInternal(f, TransferPriority::Normal);
        }
    }
}

// ── M18 reentrancy guard ──────────────────────────────────────────────────
// All access on the main thread (markScanInFlight at the top of
// scanFolderInternal, clear at the tail of applyScanResult) → no mutex.

bool SyncService::markScanInFlight(const QString &folderId)
{
    if (m_scanInFlight.contains(folderId)) {
        // A walk for this folder is already running off-main.  Remember the
        // request so it isn't lost — clearScanInFlight will fire exactly one
        // coalesced rescan once the in-flight walk settles (watcher/timer
        // storms collapse to a single follow-up instead of stacking N walks).
        m_scanDirty.insert(folderId);
        return false;
    }
    m_scanInFlight.insert(folderId);
    return true;
}

void SyncService::clearScanInFlight(const QString &folderId)
{
    m_scanInFlight.remove(folderId);
    if (m_scanDirty.remove(folderId)) {
        // Coalesced follow-up: run at Background priority — the original
        // priority belonged to whatever triggered the now-finished walk, and a
        // dirty re-scan is a catch-up, not a fresh user action.
        if (const SyncFolder *f = findFolderConst(folderId))
            scanFolderInternal(*f, TransferPriority::Background);
    }
}

ScanSnapshot SyncService::makeScanSnapshot(const SyncFolder &folder)
{
    // Copy ONLY the fields the pure walk reads.  Snapshotting here (main
    // thread, before the worker starts) means a concurrent config edit can't
    // tear the inputs mid-walk; the edit's own setter fires a fresh scan that
    // picks up the new config — same value-snapshot contract as TransferTask.
    ScanSnapshot snap;
    snap.localPath       = folder.localPath;
    snap.watchSubfolders = folder.watchSubfolders;
    snap.ignorePatterns  = folder.ignorePatterns;
    return snap;
}

void SyncService::scanFolderInternal(const SyncFolder &folder, TransferPriority prio)
{
    if (!m_autoSyncEnabled) return;                    // master pause beats per-folder
    if (!folder.enabled) return;

    // Guard against overlapping walks of the SAME folder (watcher + timer +
    // user click can all fire near-simultaneously now that the walk is async).
    if (!markScanInFlight(folder.id)) return;          // coalesced → dirty flag set

    // Snapshot the walk inputs, then run the BLOCKING filesystem traversal on a
    // worker so a slow network mount can't freeze the UI.  The result is
    // marshalled back to the main thread, where applyScanResult does the diff +
    // persist + enqueue against m_files/m_repo/m_createdSubdirs (all main-only).
    const ScanSnapshot snap = makeScanSnapshot(folder);
    const QString      folderId = folder.id;
    const qint64       maxFileSize = kMaxFileSize;
    QPointer<SyncService> guard(this);

    QtConcurrent::run([guard, snap, folderId, prio, maxFileSize]() {
        ScanResult r = scanFilesystem(snap, maxFileSize);
        if (!guard) return;                            // service gone mid-walk
        QMetaObject::invokeMethod(guard.data(),
            [guard, folderId, prio, r]() {
                if (auto *self = guard.data())
                    self->applyScanResult(folderId, prio, r);
            },
            Qt::QueuedConnection);
    });
}

void SyncService::applyScanResult(const QString &folderId, TransferPriority prio,
                                  const ScanResult &result)
{
    Q_ASSERT(QThread::currentThread() == this->thread());

    // Re-validate: the world may have moved while the walk ran on a worker —
    // user logged out, folder removed, auto-sync paused, or this folder
    // disabled.  Any of those → drop the result and release the guard.
    const SyncFolder *fConst = findFolderConst(folderId);
    if (m_userId.isEmpty() || !fConst || !m_autoSyncEnabled || !fConst->enabled) {
        clearScanInFlight(folderId);
        return;
    }
    const SyncFolder folder = *fConst;        // value copy: safe across the emits below

    // Root vanished between snapshot and walk → surface the same toast the
    // old synchronous path emitted, then bail.
    if (!result.rootExists) {
        emit folderMissing(folder.id, folder.localPath);
        clearScanInFlight(folderId);
        return;
    }

    const QString rootFsharePath = QStringLiteral("/") + folder.fshareFolderName;
    auto &map   = m_files[folder.id];
    auto &cache = m_createdSubdirs[folder.id];

    QVector<PendingUpload> pending;          // to enqueue AFTER subdirs exist
    QSet<QString>          newRelDirs;       // relDirs not yet in cache

    // Diff the walked files against our tracked state — identical logic to the
    // old in-walk version, just driven by ScannedFile instead of QFileInfo.
    for (const ScannedFile &sf : result.files) {
        if (sf.oversized) {
            SyncFileEntry oversized;
            oversized.folderId     = folder.id;
            oversized.relPath      = sf.relPath;
            oversized.size         = sf.size;
            oversized.mtime        = sf.mtime;
            oversized.state        = SyncFileState::Failed;
            oversized.errorMessage = QObject::tr("File vượt quá giới hạn 1 GB");
            map[sf.relPath] = oversized;
            if (m_repo) m_repo->saveFile(oversized);
            continue;
        }

        SyncFileEntry prev = map.value(sf.relPath);
        const bool hasPrev = !prev.relPath.isEmpty();

        if (hasPrev && prev.state == SyncFileState::Synced &&
            prev.size == sf.size && prev.mtime == sf.mtime) {
            continue;
        }
        if (hasPrev && prev.state == SyncFileState::Uploading)
            continue;

        SyncFileEntry e;
        e.folderId = folder.id;
        e.relPath  = sf.relPath;
        e.size     = sf.size;
        e.mtime    = sf.mtime;
        e.state    = SyncFileState::Uploading;
        map[sf.relPath] = e;
        if (m_repo) m_repo->saveFile(e);

        PendingUpload pu;
        pu.absPath = sf.absPath;
        pu.relPath = sf.relPath;
        pu.relDir  = sf.relDir;
        pending.append(pu);

        // Track any ancestor paths of relDir that aren't cached yet so we
        // can create them on Fshare parent→child in a single worker.
        if (!sf.relDir.isEmpty() && !cache.contains(sf.relDir)) {
            QString accum;
            const QStringList segs = sf.relDir.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            for (const QString &seg : segs) {
                accum = accum.isEmpty() ? seg : (accum + QLatin1Char('/') + seg);
                if (!cache.contains(accum)) newRelDirs.insert(accum);
            }
        }
    }

    // Mark previously-tracked files that have vanished from disk as Missing
    // (but keep the linkcode — the Fshare copy is still good).
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (result.seenRel.contains(it.key())) continue;
        if (it->state != SyncFileState::Missing) {
            it->state = SyncFileState::Missing;
            if (m_repo) m_repo->saveFile(*it);
        }
    }

    SyncFolder *mut = findFolder(folder.id);
    if (mut) {
        mut->lastScanAt = QDateTime::currentDateTime();
        if (m_repo) m_repo->saveFolder(*mut);
    }
    emit folderFilesChanged(folder.id);

    // Refresh watcher AFTER the scan so newly-discovered subdirs are watched.
    rebuildWatcher();

    if (pending.isEmpty()) {
        emit folderSynced(folder.id, 0);
        clearScanInFlight(folderId);
        return;
    }

    // Sort new subdir paths shallow→deep so parents are created before
    // children (Fshare rejects child creation when parent doesn't exist).
    QStringList newDirsList = newRelDirs.values();
    std::sort(newDirsList.begin(), newDirsList.end(),
              [](const QString &a, const QString &b) {
                  const int da = a.count(QLatin1Char('/'));
                  const int db = b.count(QLatin1Char('/'));
                  return da != db ? da < db : a < b;
              });

    // The cache uses "" as a sentinel for "sync root exists on Fshare" so we
    // never re-create it across scans. Without this the first upload's
    // createUploadSession would race against (nonexistent) root.
    const bool needsRoot = !cache.contains(QString());
    ensureSubdirsThenEnqueue(folder, newDirsList, pending, needsRoot, prio);

    // Release the per-folder guard last.  ensureSubdirsThenEnqueue is itself
    // fire-and-forget async, so a coalesced dirty rescan kicked here can run
    // concurrently with the subdir-creation worker without conflict.
    clearScanInFlight(folderId);
}

void SyncService::ensureSubdirsThenEnqueue(const SyncFolder &folder,
                                            const QStringList &newRelDirs,
                                            const QVector<PendingUpload> &pending,
                                            bool needsRoot,
                                            TransferPriority prio)
{
    if (pending.isEmpty()) return;

    const QString folderId       = folder.id;
    const QString rootName       = folder.fshareFolderName;
    const QString rootFsharePath = QStringLiteral("/") + rootName;
    // Snapshot the per-folder rate cap NOW, on the main thread.  If the user
    // edits the speed limit while these tasks are in-flight the change only
    // affects the NEXT scan — already-queued tasks keep their original cap,
    // matching TransferTask's value-snapshot semantics.
    const qint64  taskSpeedLimit = folder.speedLimitBps;
    FshareApi *api = m_api;

    QPointer<SyncService> guard(this);

    QtConcurrent::run([guard, api, folderId, rootName, rootFsharePath,
                       newRelDirs, pending, needsRoot, prio, taskSpeedLimit]() {
        bool rootCreated = false;

        // 0) Ensure the sync-root folder exists on Fshare before any upload.
        //    "Already exists" is benign — treat as success so idempotent.
        if (needsRoot && api) {
            const auto res = api->createFolderInPath(rootName, QStringLiteral("/"));
            if (res.isError()) {
                qDebug() << "[SyncService] createFolderInPath(root)" << rootName
                         << "=>" << res.error().message << "(treating as created)";
            }
            rootCreated = true;
        }

        // 1) Create any missing subfolders on Fshare, parent→child.
        QSet<QString> createdOk;
        for (const QString &rel : newRelDirs) {
            if (!api) break;
            const int slash = rel.lastIndexOf(QLatin1Char('/'));
            const QString leaf   = slash < 0 ? rel : rel.mid(slash + 1);
            const QString parent = slash < 0
                ? rootFsharePath
                : rootFsharePath + QLatin1Char('/') + rel.left(slash);
            const auto res = api->createFolderInPath(leaf, parent);
            if (res.isError()) {
                qDebug() << "[SyncService] createFolderInPath(" << leaf
                         << "in" << parent << ") =>"
                         << res.error().message << "(treating as created)";
            }
            createdOk.insert(rel);
        }

        // 2) Marshal back to the main thread to enqueue the uploads and
        //    record which subdirs are now known-good.
        if (!guard) return;
        QMetaObject::invokeMethod(guard.data(),
            [guard, folderId, rootFsharePath, pending, createdOk, rootCreated, prio,
             taskSpeedLimit]() {
                auto *self = guard.data();
                if (!self) return;

                // If the folder was removed or the user logged out while our
                // worker was running on Fshare, silently drop the result —
                // don't enqueue ghost uploads under a different session.
                if (self->m_userId.isEmpty() || !self->findFolderConst(folderId))
                    return;

                auto &cache = self->m_createdSubdirs[folderId];
                if (rootCreated) cache.insert(QString()); // "" sentinel
                for (const QString &rel : createdOk) cache.insert(rel);

                auto &map = self->m_files[folderId];
                for (const PendingUpload &p : pending) {
                    const QString fsharePath = p.relDir.isEmpty()
                        ? rootFsharePath
                        : rootFsharePath + QLatin1Char('/') + p.relDir;

                    const QString taskId = self->m_transfer
                        ? self->m_transfer->addSyncUpload(p.absPath, fsharePath,
                                                          folderId, p.relPath,
                                                          taskSpeedLimit, prio)
                        : QString{};
                    if (taskId.isEmpty() && map.contains(p.relPath)) {
                        auto &e = map[p.relPath];
                        e.state        = SyncFileState::Failed;
                        e.errorMessage = QObject::tr("Không thể thêm vào hàng đợi tải lên");
                        if (self->m_repo) self->m_repo->saveFile(e);
                    } else if (!taskId.isEmpty()) {
                        // Remember the task so progress aggregation,
                        // pauseFolder/resumeFolder, and forgetFolderTasks
                        // can find it.  Seeded with a zero-progress
                        // snapshot so a folder's pill flips to "Uploading"
                        // immediately, not after the first byte tick.
                        self->m_taskToFolder.insert(taskId, folderId);
                        self->m_folderToTasks[folderId].insert(taskId);
                        self->m_taskProgress.insert(taskId, TaskProgress{});
                    }
                }
                self->emitFolderProgress(folderId);
                emit self->folderFilesChanged(folderId);
            },
            Qt::QueuedConnection);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Watcher + timer
// ─────────────────────────────────────────────────────────────────────────────

void SyncService::rebuildWatcher()
{
    const QStringList old = m_watcher->directories();
    if (!old.isEmpty()) m_watcher->removePaths(old);

    // QFileSystemWatcher on Windows does NOT recurse, so we explicitly add
    // every subdirectory under each enabled sync folder. Manual BFS lets us
    // prune whole skipped subtrees (.git et al.) which QDirIterator cannot do.
    // This keeps the watcher cap in mind (Windows allows a few hundred paths
    // per watcher realistically) — very deep/wide trees may still exceed it.
    // In that case the 5-minute rescan timer still catches updates; only the
    // "immediate" fire-on-change behaviour degrades.
    QStringList fresh;
    QSet<QString> visited;                           // canonical-path cycle guard
    for (const SyncFolder &f : m_folders) {
        if (!f.enabled) continue;
        // QFileInfo::isDir() is a stat() syscall and CAN block on a slow /
        // disconnected network mount (UNC path, mapped drive over VPN, etc.)
        // — long enough to freeze the GUI for the user-visible duration of
        // the OS SMB timeout.  We accept that risk here because removing the
        // check just shifts the same syscall into QFileSystemWatcher::addPaths
        // and we lose the explicit "skip nonexistent root" branch.  If a
        // future regression points at GUI hangs during sync-folder load,
        // refactor rebuildWatcher() to run on a worker thread and post the
        // resulting path list back via QMetaObject::invokeMethod.
        if (!QFileInfo(f.localPath).isDir()) continue;
        fresh.append(f.localPath);

        // watchSubfolders=false → register only the root. Saves OS watcher
        // slots (Windows has a low per-watcher cap) and matches the scan
        // contract: we don't enumerate file changes the user opted out of.
        if (!f.watchSubfolders) continue;

        QStringList queue;
        queue.append(f.localPath);
        while (!queue.isEmpty()) {
            const QString dirAbs = queue.takeLast();
            const QString canon  = QFileInfo(dirAbs).canonicalFilePath();
            if (canon.isEmpty() || visited.contains(canon)) continue;
            visited.insert(canon);

            const QFileInfoList subs = QDir(dirAbs).entryInfoList(
                QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo &sub : subs) {
                if (scanShouldSkipDir(sub.fileName())) continue;
                const QString path = sub.absoluteFilePath();
                fresh.append(path);
                queue.append(path);
            }
        }
    }
    if (!fresh.isEmpty()) m_watcher->addPaths(fresh);
}

void SyncService::onPathChanged(const QString &path)
{
    // Master pause: ignore filesystem signals — the next user toggle-on
    // (or the periodic rescan) will pick up any changes that happened
    // while paused.  We DO NOT remove the watcher entries on pause so
    // resume doesn't have to walk every tree again to re-arm.
    if (!m_autoSyncEnabled) return;

    // A directory that belongs to any enabled sync folder changed on disk —
    // rescan the whole folder it sits in. Using startsWith against the sync
    // root's absolute path avoids having to list every watched subdir.
    //
    // This code path fires when the user just dropped / edited / removed a
    // file under a watched folder, which is the textbook Normal-priority
    // case (see TransferPriority::Normal docs): the user expects the upload
    // to start shortly, so it must jump ahead of the Background timer queue.
    const QString changed = QDir(path).absolutePath();
    for (const SyncFolder &f : m_folders) {
        const QString root = QDir(f.localPath).absolutePath();
        if (changed.startsWith(root, Qt::CaseInsensitive)) {
            scanFolderInternal(f, TransferPriority::Normal);
            return;
        }
    }
}

void SyncService::rescanAll()
{
    if (!m_autoSyncEnabled) return;        // master pause skips timer rescans too
    for (const SyncFolder &f : m_folders)
        if (f.enabled) scanFolderInternal(f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Upload callbacks
// ─────────────────────────────────────────────────────────────────────────────

void SyncService::onUploadFinished(const QString &syncFolderId, const QString &relPath,
                                    bool success, const QString &linkcode, const QString &error)
{
    // Clear out the task↔folder mapping for whichever task just finished.
    // syncUploadFinished doesn't carry a taskId, but at this point exactly
    // one taskId in the folder's set maps to (folderId, relPath); cheapest
    // way to find it is a small linear scan (kMaxFolders × inflight).
    if (m_folderToTasks.contains(syncFolderId)) {
        auto &ids = m_folderToTasks[syncFolderId];
        for (auto it = ids.begin(); it != ids.end(); ) {
            // We don't have a back-pointer relPath→taskId, but the
            // alternative — adding one — bloats the maps for one rare path.
            // Iterate the set and consult TransferService::findTask for the
            // mismatched task's relPath.  In the typical case the set is
            // tiny (≤ orchestrator slot cap) so this is cheap.
            const QString &id = *it;
            const TransferTask t = m_transfer ? m_transfer->findTask(id)
                                              : TransferTask{};
            if (t.syncRelPath == relPath) {
                m_taskToFolder.remove(id);
                m_taskProgress.remove(id);
                it = ids.erase(it);
            } else {
                ++it;
            }
        }
        emitFolderProgress(syncFolderId);
    }

    if (!m_files.contains(syncFolderId)) return;
    auto &map = m_files[syncFolderId];
    if (!map.contains(relPath)) return;

    SyncFileEntry &e = map[relPath];
    if (success) {
        e.state        = SyncFileState::Synced;
        e.linkcode     = linkcode;
        e.uploadedAt   = QDateTime::currentDateTime();
        e.errorMessage.clear();
    } else {
        e.state        = SyncFileState::Failed;
        e.errorMessage = error;
    }
    if (m_repo) m_repo->saveFile(e);
    emit folderFilesChanged(syncFolderId);

    // Resolve the owning folder once — used both for delete-after-upload
    // and for the activity-log "folderLabel" field (the user reads the
    // local folder leaf, not the UUID).
    const SyncFolder *f = findFolderConst(syncFolderId);
    const QString folderLabel = f ? QFileInfo(f->localPath).fileName() : QString{};

    if (!success) {
        // Persist a "Failed" entry so the user can see WHY a file didn't
        // upload after the toast disappears.  Bounded by kActivityCap so
        // the log never grows unbounded.
        if (m_repo) {
            SyncActivityEntry act;
            act.folderId    = syncFolderId;
            act.folderLabel = folderLabel;
            act.relPath     = relPath;
            act.sizeBytes   = e.size;
            act.kind        = SyncActivityKind::Failed;
            act.message     = error;
            act.at          = QDateTime::currentDateTime();
            m_repo->appendActivity(act);
            emit activityChanged();
        }
        emit syncError(syncFolderId, error);
        return;
    }

    // Success path — log the upload BEFORE the optional delete-local so the
    // entry remains accurate even if the local file is then gone.
    if (m_repo) {
        SyncActivityEntry act;
        act.folderId    = syncFolderId;
        act.folderLabel = folderLabel;
        act.relPath     = relPath;
        act.sizeBytes   = e.size;
        act.kind        = SyncActivityKind::Uploaded;
        act.at          = QDateTime::currentDateTime();
        m_repo->appendActivity(act);
        emit activityChanged();
    }

    // Apply "delete local after upload" opt-in.
    if (f && f->deleteAfterUpload) {
        const QString full = QDir(f->localPath).absoluteFilePath(relPath);
        if (QFile::remove(full)) {
            e.state = SyncFileState::Missing;
            if (m_repo) m_repo->saveFile(e);
            emit folderFilesChanged(syncFolderId);
            // Surface the delete as its own activity entry — different kind
            // so the UI can tint it (lock icon vs check) and the user
            // knows local was removed, not just the upload succeeded.
            if (m_repo) {
                SyncActivityEntry del;
                del.folderId    = syncFolderId;
                del.folderLabel = folderLabel;
                del.relPath     = relPath;
                del.sizeBytes   = e.size;
                del.kind        = SyncActivityKind::DeletedLocal;
                del.at          = QDateTime::currentDateTime();
                m_repo->appendActivity(del);
                emit activityChanged();
            }
        } else {
            qWarning() << "[SyncService] Could not remove after upload:" << full;
        }
    }

    emit folderSynced(syncFolderId, /*filesUploaded=*/1);
}

} // namespace fsnext
