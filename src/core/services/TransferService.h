#pragma once

#include "core/models/TransferTask.h"
#include "core/models/TransferState.h"
#include "core/transfer/TransferPriority.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QThread>
#include <cstdint>

namespace fsnext {

class FshareApi;
class SettingsRepository;
class HistoryRepository;
class DownloadEngine;
class UploadEngine;
class FolderExpander;
class TransferOrchestrator;

class TransferService : public QObject
{
    Q_OBJECT

public:
    explicit TransferService(FshareApi *api, SettingsRepository *settings,
                             TransferOrchestrator *orchestrator,
                             HistoryRepository *history = nullptr, QObject *parent = nullptr);
    ~TransferService() override;

    // Load history for a user (call after login). Loads just the first
    // page (kHistoryMaxPerType items) for immediate display; older pages
    // are fetched lazily via loadHistoryPage() as the user scrolls.
    void loadHistory(const QString &userId);

    // Fetch a page of history rows straight from the DB — used by the
    // view-models' infinite-scroll handler. `offset` is 0-based; newest
    // row is offset=0. Returns an empty vector if the repository is null
    // or the user has no rows at that offset.
    QVector<TransferTask> loadHistoryPage(TransferType type,
                                          int limit,
                                          int offset) const;

    // Total row count for the current user + type. Drives the "load more"
    // chip's visibility (hide when the model already holds all rows).
    int historyCount(TransferType type) const;

    // Single-file download
    void addDownload(const QString &url, const QString &password, const QString &savePath);

    // Folder download — crawls the tree asynchronously, then enqueues all files.
    void addFolderDownload(const QString &folderUrl, const QString &password,
                           const QString &savePath);

    // Cancel an in-progress folder scan (no-op if groupId is unknown).
    void cancelFolderScan(const QString &groupId);

    // Upload
    void addUpload(const QString &localPath, const QString &folderId,
                   const QString &password    = QString{},
                   const QString &description = QString{},
                   bool secured   = false,
                   bool directLink = false);

    // Upload variant used by SyncService. Same as addUpload() but:
    //   • isSyncTask=true so UploadEngine applies speedLimitBps throttle
    //   • always secured=true (sync files are set to private on Fshare)
    //   • folderId is a PATH like "/MyFolder"
    //   • returns the generated task-id so the caller can bookkeep the mapping
    //     between local relPath → task → linkcode in SyncRepository.
    // Returns empty string if the file could not be enqueued (e.g. missing).
    // `priority` lets SyncService distinguish event-driven (user just added a
    // file → Normal) from timer-rescan (→ Background).  Defaults to Background
    // so the generic path does not starve user-initiated work.
    QString addSyncUpload(const QString &localPath,
                          const QString &fsharePath,
                          const QString &syncFolderId,
                          const QString &syncRelPath,
                          int64_t speedLimitBps,
                          TransferPriority priority = TransferPriority::Background);

    // Task control
    void pauseTask(const QString &id);
    void resumeTask(const QString &id);
    void cancelTask(const QString &id);
    void pauseAll();
    void resumeAll();

    // Queue reordering (Upload/Queued tasks only)
    void moveTaskFirst(const QString &id);
    void moveTaskLast(const QString &id);
    void moveTaskUp(const QString &id);
    void moveTaskDown(const QString &id);

    // Accessors
    QVector<TransferTask> activeTasks() const;
    QVector<TransferTask> completedTasks() const;
    TransferTask findTask(const QString &id) const;

signals:
    void taskAdded(const TransferTask &task);
    void taskProgressChanged(const QString &id, int64_t bytes, int64_t total, double speed, const QString &eta);
    void taskStateChanged(const QString &id, TransferState state);
    void taskCompleted(const QString &id);
    void taskFailed(const QString &id, const QString &error);

    // Folder scan lifecycle signals
    void folderScanStarted(const QString &groupId, const QString &folderName);
    void folderScanProgress(const QString &groupId, int foldersScanned, int filesFound);
    void folderScanCompleted(const QString &groupId, int totalFiles);
    void folderScanFailed(const QString &groupId, const QString &error);

    // Emitted when an API call returns an auth error (HTTP 201/202).
    void sessionExpired(const QString &message);

    // Emitted when the order of queued upload tasks changes.
    void taskOrderChanged();

    // Emitted when a transfer completes with a resolved linkcode + local path.
    // Used by FileCacheService to persist the linkcode → local file mapping.
    void transferRecordReady(const QString &linkcode, const QString &localPath,
                             const QString &fileName, int64_t fileSize,
                             const QString &transferType);

    // Emitted after an Upload task completes successfully.  Carries the
    // Fshare folder path the file landed in ("/" for root, "/Sub" otherwise)
    // so the file manager can invalidate the matching cached folder.
    void uploadCompleted(const QString &folderPath);

    // Emitted when the post-upload setFilePassword call fails: the file was
    // uploaded but is NOT password-protected. Lets the UI warn the user instead
    // of leaving them to assume the file is secured. (P7)
    void filePasswordSetFailed(const QString &fileName, const QString &message);

    // Emitted alongside taskCompleted/taskFailed for tasks flagged as sync
    // uploads. Carries enough routing information for SyncService to update
    // its per-file state without having to snoop generic upload signals.
    //   success=true  → linkcode is the new Fshare linkcode
    //   success=false → linkcode is empty, error holds the message
    void syncUploadFinished(const QString &syncFolderId,
                            const QString &syncRelPath,
                            bool success,
                            const QString &linkcode,
                            const QString &error);

private:
    // Slot dispatchers — called when TransferOrchestrator grants a slot via
    // its dispatchReady signal.  Each performs the session-create + engine-
    // spawn work for one task.  They replace the old startNextInQueue /
    // startNextUpload which also owned slot-cap enforcement (now delegated).
    void onDispatchReady(const QString &id, fsnext::TransferClass cls,
                         fsnext::TransferPriority prio);
    void dispatchDownload(const QString &taskId);
    void dispatchUpload  (const QString &taskId);

    void onDownloadProgress(const QString &taskId, int64_t bytes, int64_t total, double speed, const QString &eta);
    void onDownloadComplete(const QString &taskId, const QString &filePath);
    void onDownloadFailed(const QString &taskId, const QString &error);

    FshareApi            *m_api          = nullptr;
    SettingsRepository   *m_settings     = nullptr;
    HistoryRepository    *m_history      = nullptr;
    TransferOrchestrator *m_orch         = nullptr;
    QString               m_currentUserId;

    QVector<TransferTask>           m_tasks;
    QVector<TransferTask>           m_completed;
    QMap<QString, DownloadEngine*>  m_engines;
    QMap<QString, UploadEngine*>    m_uploadEngines;
    QMap<QString, QThread*>         m_threads;
    QMap<QString, FolderExpander*>  m_expanders;   // groupId → expander
    // Remember the priority each task was enqueued with so we can re-enqueue
    // after an upload-session auto-renewal (see onUploadSessionExpired).
    QMap<QString, TransferPriority> m_priorities;

    void onUploadProgress(const QString &taskId, int64_t bytes, int64_t total, double speed, const QString &eta);
    void onUploadComplete(const QString &taskId, const QString &linkcode);
    void onUploadFailed(const QString &taskId, const QString &error);
    void onUploadSessionExpired(const QString &taskId);

    // Spawn the upload engine + worker thread for a task whose upload session
    // URL (task.realUrl) is already resolved. Shared by the fresh-upload path
    // (after createUploadSession) and the resume path (dispatchUpload reusing a
    // retained realUrl). The engine resumes from the server offset when the
    // task carries non-zero progress (see UploadEngine::startUpload).
    void spawnUploadEngine(const TransferTask &task);

    // ── ADR D12 — progress-persistence wiring ──────────────────────────────
    // Snapshot the current state of every in-flight task to the history DB so
    // a crash mid-transfer resumes from the most recent checkpoint instead of
    // byte 0.  Triggered by m_progressFlushTimer at ≈5 s cadence and once on
    // taskStateChanged for every state transition (paused / resumed / failed).
    void persistProgressSnapshots();
    // Resume tasks loaded from loadInFlight().  Called by loadHistory() after
    // it has populated the history list, so the active list is rebuilt from
    // any rows that didn't reach Complete state before the previous shutdown.
    void resumeInFlightTasks(const QVector<TransferTask> &inflight,
                             const QHash<QString, QString> &snapshots);

    QTimer  m_progressFlushTimer;
    static constexpr int kProgressFlushIntervalMs = 5000;
};

} // namespace fsnext
