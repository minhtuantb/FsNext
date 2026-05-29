#pragma once
#include "core/models/SyncFolder.h"
#include "core/services/SyncScanner.h"
#include "core/transfer/TransferPriority.h"
#include <QFileSystemWatcher>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QTimer>
#include <QVector>

namespace fsnext {

class TransferService;
class SyncRepository;
class FshareApi;

// SyncService — the orchestrator for "sync N local folders to Fshare".
//
// Scope:
//   • max 5 folders (hard cap)
//   • recursive walk of the whole tree; relPath uses forward-slashes
//   • file skip-list: .tmp, Thumbs.db, .DS_Store, desktop.ini, ~$*, dotfiles
//   • directory skip-list: .git, node_modules, __pycache__, .venv, .idea, .vs
//   • file > 1 GB is skipped (surfaced as error entry)
//   • per-task throttle = 5 MB/s (fixed, UI not editable)
//   • uploaded files are private (secured=true, no password)
//   • optional: delete local file after successful upload (per-folder toggle)
//
// Folder tree on Fshare mirrors the local tree: a local "sub/a/b" lands under
// "/<rootName>/sub/a/b". Subfolders are created lazily via createFolderInPath
// before the first upload into each path; "already exists" is treated as
// success. An in-memory cache (m_createdSubdirs) avoids re-issuing the same
// create across a session.
//
// Lifetime: owned by AppContext, alive as long as the application.
// Runs on the main thread; QFileSystemWatcher + QTimer are Qt-native and
// safe to touch from signal handlers without extra synchronization.
class SyncService : public QObject {
    Q_OBJECT

public:
    static constexpr int    kMaxFolders     = 5;
    static constexpr int64_t kSpeedLimitBps  = 5LL * 1024 * 1024;        // 5 MB/s
    static constexpr int64_t kMaxFileSize    = 1024LL * 1024 * 1024;     // 1 GB
    static constexpr int    kRescanIntervalMs = 5 * 60 * 1000;           // 5 min

    SyncService(TransferService *transfer, FshareApi *api,
                SyncRepository *repo, QObject *parent = nullptr);
    ~SyncService() override;

    // Must be called when the logged-in user changes. An empty id stops all
    // watchers and clears in-memory state until a real user signs in.
    void setUserId(const QString &userId);

    // Read-only views.
    QVector<SyncFolder>     folders() const { return m_folders; }
    QVector<SyncFileEntry>  filesOf(const QString &folderId) const;
    int  folderCount() const { return m_folders.size(); }

    // ── Master "auto-sync" toggle (v6.0+) ──────────────────────────────────
    // When false, every scan path early-returns and the QFileSystemWatcher
    // callback no-ops — uploads in flight are NOT cancelled, but no new ones
    // are enqueued.  Per-folder enabled flags continue to be respected; this
    // is a global override layered on top, not a replacement.
    bool autoSyncEnabled() const { return m_autoSyncEnabled; }
    void setAutoSyncEnabled(bool enabled);

    // ── Activity log (v6.0+) ───────────────────────────────────────────────
    // Pass-through to SyncRepository so callers don't need to know where
    // activity is stored.  loadActivity() returns newest-first.  clearActivity
    // wipes the per-user log and fires activityChanged so the VM resets its
    // model.  appendActivity is internal-only (file-level callbacks).
    QVector<SyncActivityEntry> loadActivity() const;
    void clearActivity();

    // ── Per-folder transfer control (v6.0+ Phase 3) ────────────────────────
    // pauseFolder/resumeFolder operate on the in-flight TransferService tasks
    // attributed to this folder, NOT the SyncFolder.enabled flag — they're
    // intended as one-shot user actions ("hold this folder's uploads for a
    // minute, I need bandwidth").  retryFailed re-enqueues every file in
    // Failed state for that folder via the normal scan path; cheaper than
    // running a full scanFolder() because we already know which files want
    // a retry.
    void pauseFolder(const QString &folderId);
    void resumeFolder(const QString &folderId);
    void retryFailed(const QString &folderId);

    // Returns false if the cap (kMaxFolders) has already been hit or the path
    // is invalid / duplicates an existing folder. On success emits foldersChanged.
    //
    // The single-arg overload preserves the v5 default settings (recursive
    // walk, no user ignore patterns, kSpeedLimitBps throttle).  Prefer the
    // expanded overload from new callers so per-folder settings flow through
    // from the AddWatchFolderDialog UI without a follow-up update call.
    bool addFolder(const QString &localPath, QString *errorOut = nullptr);
    bool addFolder(const QString &localPath,
                   bool watchSubfolders,
                   const QString &ignorePatterns,
                   qint64 speedLimitBps,
                   bool deleteAfterUpload,
                   QString *errorOut = nullptr);
    void removeFolder(const QString &folderId);
    void setFolderEnabled(const QString &folderId, bool enabled);
    void setDeleteAfterUpload(const QString &folderId, bool deleteAfter);

    // ── Per-folder settings setters (v6.0+) ────────────────────────────────
    // Each one is a small no-op when value is unchanged, persists via
    // SyncRepository, and emits foldersChanged so the QML list re-binds.
    // Mutations that affect scan scope (watchSubfolders / ignorePatterns)
    // additionally rebuild the QFileSystemWatcher and trigger a fresh scan
    // so the effect is observable immediately rather than after the next
    // 5-min rescan tick.
    void setWatchSubfolders(const QString &folderId, bool on);
    void setIgnorePatterns(const QString &folderId, const QString &patterns);
    void setSpeedLimitBps(const QString &folderId, qint64 bps);

    // Force a rescan of one folder now (user pressed "Đồng bộ ngay" in UI).
    void scanFolder(const QString &folderId);

    // Forget all recorded state for a folder so the next scan will re-evaluate
    // every file. Useful when the user re-authorized or the Fshare-side folder
    // was emptied. Does NOT delete files on Fshare.
    void resetFolderState(const QString &folderId);

    // Result of previewScan(): a dry-run estimate the Add-folder wizard shows
    // to set expectations before the user commits (so a 60 GB drop isn't a
    // surprise).  errorMessage is filled when the path is missing / unreadable.
    struct PreviewResult {
        int      fileCount   = 0;
        qint64   totalBytes  = 0;
        QString  errorMessage;
    };

    // Walks `localPath` honouring the SAME skip rules as the live scanner
    // (system skip-list + watchSubfolders + user ignore patterns) but does
    // NOT enqueue anything for upload.  Synchronous & deterministic — the
    // VM wraps the call in QtConcurrent so the UI thread stays responsive
    // on big trees.
    PreviewResult previewScan(const QString &localPath,
                              bool watchSubfolders,
                              const QString &ignorePatterns) const;

signals:
    // List mutations — coarse-grained, views re-read folders() on each.
    void foldersChanged();

    // Per-folder file-list mutation (add/update/remove/state change).
    void folderFilesChanged(const QString &folderId);

    // Toast-worthy events.
    void folderMissing(const QString &folderId, const QString &localPath);
    void folderSynced(const QString &folderId, int filesUploaded);
    void syncError(const QString &folderId, const QString &message);

    // Fired when setAutoSyncEnabled flips the master switch — lets the VM
    // mirror the value into Q_PROPERTY autoSyncEnabled without polling.
    void autoSyncEnabledChanged(bool enabled);

    // Fired whenever appendActivity / clearActivity touches the log so the
    // VM can re-poll SyncRepository::loadActivity() and reset its model.
    void activityChanged();

    // Per-folder aggregated upload progress.  Emitted on every
    // TransferService::taskProgressChanged that belongs to a tracked sync
    // task — VM throttles into its model.  speedBps is the sum across the
    // folder's active tasks; etaSecs is the conservative max() across them
    // (slowest task wins so the UI doesn't promise speeds it can't deliver).
    void folderProgressChanged(const QString &folderId,
                                qint64 bytesDone,
                                qint64 bytesTotal,
                                double speedBps,
                                qint64 etaSecs);

private slots:
    void onPathChanged(const QString &path);
    void onUploadFinished(const QString &syncFolderId, const QString &relPath,
                          bool success, const QString &linkcode, const QString &error);
    void rescanAll();

private:
    // Helpers
    void rebuildWatcher();
    // `prio` controls the priority queued uploads land in on the
    // TransferOrchestrator.  Defaults to Background so the periodic timer
    // rescan does not starve interactive user-initiated work; user actions
    // (add folder, click "sync now", file-watcher fire) pass Normal.
    void scanFolderInternal(const SyncFolder &folder,
                            TransferPriority prio = TransferPriority::Background);

    // ── M18: async scan (walk off-main, apply on-main) ─────────────────────
    // scanFolderInternal snapshots the folder, runs scanFilesystem() (in
    // SyncScanner) on a QtConcurrent worker, then marshals the ScanResult back
    // here so applyScanResult() can do the diff + persist + enqueue on the
    // main thread.  m_files / m_repo / m_createdSubdirs / the watcher / every
    // emit stay main-thread-only.
    static ScanSnapshot makeScanSnapshot(const SyncFolder &folder);
    void applyScanResult(const QString &folderId, TransferPriority prio,
                         const ScanResult &result);

    // Per-folder reentrancy guard.  markScanInFlight returns false (and sets a
    // "dirty" flag) when a walk for that folder is already running, so watcher
    // / timer fires coalesce into exactly one follow-up rescan instead of
    // stacking N overlapping walks.  Both touch m_scanInFlight/m_scanDirty on
    // the main thread only — no mutex needed.
    bool markScanInFlight(const QString &folderId);
    void clearScanInFlight(const QString &folderId);

    SyncFolder *findFolder(const QString &folderId);
    const SyncFolder *findFolderConst(const QString &folderId) const;

    // Recomputes the folderProgressChanged payload from m_taskProgress
    // entries owned by the given folder and emits the signal.  Called on
    // every per-task tick AND on add/remove of taskIds so a folder that
    // just lost its last in-flight task settles back to 0/0.
    void emitFolderProgress(const QString &folderId);

    // Drops every taskId belonging to the folder from m_taskToFolder /
    // m_folderToTasks / m_taskProgress and emits a final 0/0 progress so
    // QML clears the bar.  Called on removeFolder, retryFailed (after
    // cancel), and on the per-task completion path.
    void forgetFolderTasks(const QString &folderId);

    // Upload that scan() decided to enqueue but is parked until the Fshare
    // subfolder chain finishes being created. relDir uses forward slashes; ""
    // means "sync root" and needs no folder creation.
    struct PendingUpload {
        QString absPath;
        QString relPath;
        QString relDir;
    };

    // Creates the Fshare sync root (when needsRoot) plus any missing subfolders
    // (sequential parent→child) inside a QtConcurrent worker, then marshals
    // back to the main thread via QMetaObject::invokeMethod to enqueue uploads.
    // Keeps UI responsive and eliminates the race where an upload's
    // createUploadSession would hit a folder that wasn't created yet.
    void ensureSubdirsThenEnqueue(const SyncFolder &folder,
                                  const QStringList &newRelDirs,
                                  const QVector<PendingUpload> &pending,
                                  bool needsRoot,
                                  TransferPriority prio);

    TransferService          *m_transfer = nullptr;
    FshareApi                *m_api      = nullptr;
    SyncRepository           *m_repo     = nullptr;
    QFileSystemWatcher       *m_watcher  = nullptr;
    QTimer                    m_rescanTimer;

    QVector<SyncFolder>       m_folders;
    // folderId → (relPath → entry) for O(1) lookups during scans.
    QHash<QString, QHash<QString, SyncFileEntry>> m_files;

    // folderId → set of relDirs that have been (or are being) created on
    // Fshare this session. Avoids redundant createFolderInPath calls.
    // Sentinel "" (empty string) tracks the sync ROOT folder specifically.
    QHash<QString, QSet<QString>> m_createdSubdirs;

    // ── M18 reentrancy guard ───────────────────────────────────────────────
    // m_scanInFlight: folderIds whose background walk is running right now.
    // m_scanDirty:    folderIds that asked for a rescan while in-flight — they
    // get exactly one coalesced follow-up walk when the current one settles.
    QSet<QString>             m_scanInFlight;
    QSet<QString>             m_scanDirty;

    QString                   m_userId;

    // Master toggle — defaults to true so v5 callers and fresh users see no
    // behaviour change.  Persisted via SyncRepository, reloaded on every
    // setUserId() so each account's pause state is independent.
    bool                      m_autoSyncEnabled = true;

    // ── Task↔folder bookkeeping (v6.0+) ────────────────────────────────────
    // SyncService is the only thing that knows which TransferService task
    // belongs to which sync folder.  Maintain the mapping here so we can:
    //   • Aggregate per-folder upload progress (sum bytes across taskIds)
    //   • Pause/Resume / Retry "all files in folder X" without dragging
    //     the per-folder task list through the QML side.
    //
    // Cleared on session boundaries via setUserId() — stale ids from a
    // previous user must never bleed into a fresh login.
    QHash<QString, QString>                   m_taskToFolder;     // taskId   → folderId
    QHash<QString, QSet<QString>>             m_folderToTasks;    // folderId → {taskIds}
    // Cache of latest per-task progress snapshot (bytes done, bytes total,
    // speed, eta).  Used to recompute folder aggregates on every tick
    // without re-querying TransferService::activeTasks() — that scan would
    // be O(n) per signal and we get a signal per ~50 ms during upload.
    struct TaskProgress {
        qint64 bytesDone  = 0;
        qint64 bytesTotal = 0;
        double speedBps   = 0.0;
        qint64 etaSecs    = -1;
    };
    QHash<QString, TaskProgress>              m_taskProgress;     // taskId → snapshot
};

} // namespace fsnext
