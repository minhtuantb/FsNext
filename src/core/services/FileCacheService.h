#pragma once

#include "core/models/FileItem.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace fsnext {

class FshareApi;
class FileService;
class FileCacheDB;
class FileSyncWorker;
class TransferOrchestrator;

// Cache-first coordinator that sits between FileManagerViewModel and the
// network.  Every read operation serves from the local SQLite cache
// immediately, then schedules a background refresh via FileSyncWorker.
// Write operations go to the API first; the cache is updated optimistically
// and corrected on failure.
//
// Usage (from AppContext after login):
//   m_cacheService->setCurrentUser(user.id);
//   m_cacheService->loadFolderTree();
class FileCacheService : public QObject
{
    Q_OBJECT

public:
    explicit FileCacheService(FshareApi            *api,
                              FileService          *service,
                              TransferOrchestrator *orchestrator = nullptr,
                              QObject              *parent       = nullptr);

    // Test-only constructor — opens the SQLite cache at `dbPath` (e.g.
    // ":memory:") instead of the platform AppData location. Prefer the
    // three-arg form in production code.
    FileCacheService(FshareApi     *api,
                     FileService   *service,
                     const QString &dbPath,
                     QObject       *parent);

    ~FileCacheService() override = default;

    // Call immediately after a successful login.
    void setCurrentUser(const QString &userId);

    // Call on logout — flushes the pending sync queue.
    void onUserLoggedOut();

    // ── Read operations (cache-first) ────────────────────────
    void loadFolderTree();

    // sortKey: "name" | "date" | "size" | "type"
    // typeFilter: "all" | "file" | "folder"
    void listFiles(const QString &folderId,
                   const QString &sortKey    = QStringLiteral("name"),
                   bool           sortAsc    = true,
                   const QString &typeFilter = QStringLiteral("all"));

    // Instant local FTS5 search across the entire cached tree (recursive).
    void searchLocal(const QString &keyword);

    // Local search + parallel API call for items not yet cached.
    void searchFiles(const QString &keyword);

    // ── Write operations (write-through) ────────────────────
    void createFolder(const QString &name, const QString &parentId);
    void renameFile(const QString &linkcode, const QString &newName);
    void deleteFiles(const QStringList &linkcodes);
    void moveFiles(const QStringList &linkcodes, const QString &targetFolderId);
    void copyFiles(const QStringList &linkcodes, const QString &targetFolderId);

    // File settings — delegated straight to FileService (no cache side-effect)
    void getFileInfo(const QString &url);
    void changeSecure(const QStringList &linkcodes, bool secure);
    void setPassword(const QStringList &linkcodes, const QString &password);
    void setDirectLink(const QStringList &linkcodes, bool enabled);

    // ── Download URL (direct link for streaming / download) ─────
    // Calls createDownloadSession to obtain a direct download URL.
    // Emits downloadUrlReady on success, downloadUrlFailed on error.
    void getDownloadUrl(const QString &linkcode, const QString &password = {});

    // Manual refresh — drop the folder_sync row for this folder and
    // re-enqueue it at high priority. Used by the "refresh" toolbar button
    // so the user can force a re-fetch when they suspect stale cache.
    void refreshFolder(const QString &folderId);

    // Re-sync the folder an upload just landed in.
    // `uploadPath` is the Fshare PATH used by createUploadSession
    // ("/" for root, "/Sub" for a sub-folder). Maps the path to a cached
    // folderId (root → empty string) and enqueues a high-priority sync
    // via FileSyncWorker so the file manager picks up the new file.
    void refreshUploadFolder(const QString &uploadPath);

    // ── Local file tracking (download/upload → local path) ─────
    void recordLocalFile(const QString &linkcode, const QString &localPath,
                         const QString &fileName, int64_t fileSize,
                         const QString &transferType);
    QHash<QString, QString> getLocalPaths(const QStringList &linkcodes);
    QString getLocalPath(const QString &linkcode);
    void removeLocalFile(const QString &linkcode);

    // ── Cache helpers ─────────────────────────────────────────
    int  cachedCount(const QString &folderId,
                     const QString &typeFilter = QStringLiteral("all"));
    bool isFolderSynced(const QString &folderId) const;

signals:
    void folderTreeLoaded(const QVector<fsnext::FileItem> &folders);
    void fileListLoaded(const QVector<fsnext::FileItem> &files);
    void searchResultsLoaded(const QVector<fsnext::FileItem> &files, bool isLocal);
    void operationComplete(const QString &message);
    void operationFailed(const QString &error);
    void isLoadingChanged(bool loading);
    void syncProgressChanged(const QString &folderId, int itemsSynced);
    void localFileRecorded(const QString &linkcode, const QString &localPath);
    void downloadUrlReady(const QString &linkcode, const QString &url);
    void downloadUrlFailed(const QString &linkcode, const QString &error);

    // Emitted immediately before a cache refresh is enqueued on FileSyncWorker
    // (i.e. after a write op completes/fails, or when refreshFolder is called).
    // Primarily an observability hook for unit tests to verify the
    // onWriteOperationComplete state machine without needing a real API.
    void cacheRefreshScheduled(const QString &folderId, bool highPriority);

private:
    void emitCurrentFolderFromCache();
    void initDB();

    // Post-operation cache invalidation — connected once in constructor
    void onWriteOperationComplete(const QString &msg);
    void onWriteOperationFailed(const QString &err);

    // Shared constructor tail used by both public ctors.
    static void wireConnections(FileCacheService *self,
                                FileService      *service,
                                FileSyncWorker   *sync);

    // Emit cacheRefreshScheduled then enqueue on FileSyncWorker. Single
    // choke-point so tests can observe every refresh the service schedules.
    void scheduleRefresh(const QString &folderId, bool highPriority);

    FshareApi    *m_api     = nullptr;
    FileService  *m_service = nullptr;
    FileCacheDB  *m_db      = nullptr;
    FileSyncWorker *m_sync  = nullptr;

    QString m_userId;
    QString m_currentFolderId;
    QString m_currentSortKey   = QStringLiteral("name");
    bool    m_currentSortAsc   = true;
    QString m_currentFilter    = QStringLiteral("all");

    // Operation tracking: which folder(s) should be re-synced after the
    // current write operation completes or fails.
    enum class WriteOp { None, Create, Rename, Delete, Move, Copy };
    WriteOp   m_pendingOp           = WriteOp::None;
    QString   m_pendingSourceFolder;   // folder to re-sync on completion
    QString   m_pendingTargetFolder;   // second folder (for move/copy)

    // Pass-through operations (changeSecure, setPassword, setDirectLink,
    // getFileInfo) share the same operationComplete/Failed signals as write
    // ops.  This counter prevents their completion from consuming the pending
    // write-op's state.
    int m_settingsInFlight = 0;

    static constexpr int kStaleSecs = 60;
};

} // namespace fsnext
