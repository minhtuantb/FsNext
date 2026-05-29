#include "FileCacheService.h"

#include "FileService.h"
#include "core/api/FshareApi.h"
#include "core/cache/FileCacheDB.h"
#include "core/cache/FileSyncWorker.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QPointer>
#include <QStandardPaths>
#include <QtConcurrent>

namespace fsnext {

static qint64 nowSecs()
{
    return QDateTime::currentSecsSinceEpoch();
}

// ── construction ──────────────────────────────────────────────────────────────

FileCacheService::FileCacheService(FshareApi            *api,
                                   FileService          *service,
                                   TransferOrchestrator *orchestrator,
                                   QObject              *parent)
    : QObject(parent)
    , m_api(api)
    , m_service(service)
    , m_db(new FileCacheDB(this))
    , m_sync(new FileSyncWorker(api, m_db, orchestrator, this))
{
    wireConnections(this, m_service, m_sync);
    initDB();
}

FileCacheService::FileCacheService(FshareApi     *api,
                                   FileService   *service,
                                   const QString &dbPath,
                                   QObject       *parent)
    : QObject(parent)
    , m_api(api)
    , m_service(service)
    , m_db(new FileCacheDB(this))
    , m_sync(new FileSyncWorker(api, m_db, /*orchestrator=*/nullptr, this))
{
    wireConnections(this, m_service, m_sync);
    if (!m_db->open(dbPath))
        qWarning() << "[FileCacheService] Failed to open cache DB at" << dbPath;
}

// Shared constructor tail — signal/slot wiring used by both the production and
// test constructors.
void FileCacheService::wireConnections(FileCacheService *self,
                                       FileService      *service,
                                       FileSyncWorker   *sync)
{
    // ── Forward loading state from FileService ───────────────────────────
    QObject::connect(service, &FileService::isLoadingChanged,
                     self,    &FileCacheService::isLoadingChanged);

    // ── Single permanent connections for write-operation completion ──────
    // Instead of SingleShotConnection per operation (race-prone), we use
    // permanent connections that check m_pendingOp to decide what to do.
    QObject::connect(service, &FileService::operationComplete,
                     self,    &FileCacheService::onWriteOperationComplete);
    QObject::connect(service, &FileService::operationFailed,
                     self,    &FileCacheService::onWriteOperationFailed);

    // ── React to background sync events ───────────────────────────────────
    // In addition to refreshing the current-folder file list, each batch
    // completion also re-emits the full folder tree. Without this, the
    // FolderTreeModel only knows root-level folders (what loadFolderTree
    // fetched at startup) and any sub-folder the user navigates into is
    // missing from the parent-chain lookup — breaking the breadcrumb past
    // one level of depth.
    auto reemitFolderTree = [self]() {
        if (!self->m_db->isOpen() || self->m_userId.isEmpty()) return;
        emit self->folderTreeLoaded(self->m_db->getFolderTree(self->m_userId));
    };

    QObject::connect(sync, &FileSyncWorker::firstBatchSynced,
                     self, [self, reemitFolderTree](const QString &folderId) {
                         if (folderId == self->m_currentFolderId)
                             self->emitCurrentFolderFromCache();
                         reemitFolderTree();
                     });

    QObject::connect(sync, &FileSyncWorker::folderSynced,
                     self, [self, reemitFolderTree](const QString &folderId) {
                         if (folderId == self->m_currentFolderId)
                             self->emitCurrentFolderFromCache();
                         reemitFolderTree();
                     });

    QObject::connect(sync, &FileSyncWorker::syncError,
                     self, [](const QString &folderId, const QString &err) {
                         qWarning() << "[FileCacheService] sync error for" << folderId << ":" << err;
                     });
}

// ── init ──────────────────────────────────────────────────────────────────────

void FileCacheService::initDB()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dbPath  = QDir(dataDir).filePath(QStringLiteral("filecache.db"));

    if (!m_db->open(dbPath))
        qWarning() << "[FileCacheService] Failed to open cache DB at" << dbPath;
    else
        qDebug() << "[FileCacheService] Cache DB opened at" << dbPath;
}

// ── user lifecycle ────────────────────────────────────────────────────────────

void FileCacheService::setCurrentUser(const QString &userId)
{
    const bool firstLogin = m_userId.isEmpty() && !userId.isEmpty();
    m_userId = userId;
    qDebug() << "[FileCacheService] Current user set to" << userId;

    // If the File Manager page tried to load a folder before auth completed
    // (common on cold start — the QML Component.onCompleted fires during
    // startup, racing with the silent OAuth refresh), the initial listFiles()
    // ran with an empty m_userId and skipped the cache path, and the live API
    // call failed with "Not logged in yet!". Now that we have a user id, kick
    // off one refresh of the currently-shown folder so the UI catches up.
    if (firstLogin && m_sync) {
        m_sync->enqueueFolder(m_userId, m_currentFolderId, /*highPriority=*/true);
        emitCurrentFolderFromCache();
    }
}

void FileCacheService::onUserLoggedOut()
{
    m_sync->cancelAll();
    m_userId.clear();
    m_currentFolderId.clear();
    m_pendingOp = WriteOp::None;
}

// ── write-operation tracking ──────────────────────────────────────────────────

void FileCacheService::onWriteOperationComplete(const QString &msg)
{
    // Forward to QML via operationComplete
    emit operationComplete(msg);

    // Settings pass-through ops (changeSecure, setPassword, etc.) share the
    // same FileService signal.  Don't let them consume the pending write-op.
    if (m_settingsInFlight > 0) {
        --m_settingsInFlight;
        return;
    }

    // Post-operation cache invalidation based on what we were doing
    if (m_pendingOp == WriteOp::None || m_userId.isEmpty())
        return;

    const WriteOp op = m_pendingOp;
    m_pendingOp = WriteOp::None;

    // An empty folder id means the account root — still a valid target for
    // enqueueFolder (the sync worker hits /api/fileops/list/home for it). The
    // earlier isEmpty() guards meant root-level creates/renames/deletes never
    // triggered a cache refresh, so users saw nothing change even on API
    // success.
    switch (op) {
    case WriteOp::Create:
        // Re-sync the parent folder to pick up the new server-assigned linkcode
        scheduleRefresh(m_pendingSourceFolder, true);
        break;
    case WriteOp::Rename:
    case WriteOp::Delete:
        // Re-sync current folder to confirm server state
        scheduleRefresh(m_currentFolderId, true);
        break;
    case WriteOp::Move:
        // Re-sync both source and target folders
        scheduleRefresh(m_pendingSourceFolder, true);
        if (m_pendingTargetFolder != m_pendingSourceFolder)
            scheduleRefresh(m_pendingTargetFolder, false);
        break;
    case WriteOp::Copy:
        // Only the target folder has new items
        scheduleRefresh(m_pendingTargetFolder, false);
        break;
    default:
        break;
    }

    m_pendingSourceFolder.clear();
    m_pendingTargetFolder.clear();
}

void FileCacheService::onWriteOperationFailed(const QString &err)
{
    // Forward to QML
    emit operationFailed(err);

    // Settings pass-through — same guard as onWriteOperationComplete.
    if (m_settingsInFlight > 0) {
        --m_settingsInFlight;
        return;
    }

    // On failure, revert optimistic cache changes by re-syncing
    if (m_pendingOp == WriteOp::None || m_userId.isEmpty())
        return;

    m_pendingOp = WriteOp::None;

    // Re-sync current folder (root included) to restore correct state
    scheduleRefresh(m_currentFolderId, true);

    m_pendingSourceFolder.clear();
    m_pendingTargetFolder.clear();
}

void FileCacheService::scheduleRefresh(const QString &folderId, bool highPriority)
{
    emit cacheRefreshScheduled(folderId, highPriority);
    if (m_sync)
        m_sync->enqueueFolder(m_userId, folderId, highPriority);
}

// ── read operations ───────────────────────────────────────────────────────────

void FileCacheService::loadFolderTree()
{
    // 1. Instant cache hit
    if (m_db->isOpen() && !m_userId.isEmpty()) {
        const auto cached = m_db->getFolderTree(m_userId);
        if (!cached.isEmpty())
            emit folderTreeLoaded(cached);
    }

    // 2. Always refresh from API (folder tree is small, cheap to re-fetch)
    FshareApi    *api    = m_api;
    FileCacheDB  *db     = m_db;
    FileSyncWorker *sync = m_sync;
    const QString userId = m_userId;
    QPointer<FileCacheService> guard(this);

    QtConcurrent::run([api, db, sync, userId, guard]() {
        auto resp = api->listFolders({});
        QMetaObject::invokeMethod(guard.data(), [guard, resp, db, sync, userId]() {
            if (!guard) return;
            if (resp.isSuccess()) {
                if (db->isOpen() && !userId.isEmpty())
                    db->upsertFiles(userId, resp.data());

                // Emit the FULL folder tree from DB (root-level folders from
                // the API refresh PLUS any previously-synced sub-folders) so
                // the folder-tree model doesn't lose nested folders it learned
                // about when the user navigated into them. This is required
                // for the breadcrumb path to resolve more than one level deep.
                const auto fullTree = (db->isOpen() && !userId.isEmpty())
                    ? db->getFolderTree(userId)
                    : resp.data();
                emit guard->folderTreeLoaded(fullTree);

                // Kick background sync of all folders (low priority)
                if (!userId.isEmpty())
                    sync->enqueueTree(userId, resp.data());
            } else {
                emit guard->operationFailed(resp.error().message);
            }
        });
    });
}

void FileCacheService::listFiles(const QString &folderId,
                                 const QString &sortKey,
                                 bool           sortAsc,
                                 const QString &typeFilter)
{
    m_currentFolderId = folderId;
    m_currentSortKey  = sortKey;
    m_currentSortAsc  = sortAsc;
    m_currentFilter   = typeFilter;

    if (!m_db->isOpen() || m_userId.isEmpty()) {
        // No cache — fall through to the live FileService. Guard against
        // FileCacheService being destructed before the SingleShotConnection
        // fires (cold-start race: user clicks a folder before auth completes,
        // then quits the app while the API call is still in flight). Without
        // the QPointer the `emit this->fileListLoaded(...)` below would
        // dereference a dangling `this`.
        QPointer<FileCacheService> guard(this);
        connect(m_service, &FileService::fileListLoaded,
                this, [guard](const QVector<FileItem> &files) {
                    if (!guard) return;
                    emit guard->fileListLoaded(files);
                }, Qt::SingleShotConnection);
        m_service->listFiles(folderId);
        return;
    }

    // 1. Serve cached data immediately (may be empty on first visit)
    emitCurrentFolderFromCache();

    // 2. Determine if a background refresh is needed
    const FolderSyncState state = m_db->getSyncState(m_userId, folderId);
    const bool stale = !state.exists || (nowSecs() - state.syncedAt > kStaleSecs);

    if (stale || !state.isComplete) {
        // High-priority: current folder should be fetched before anything else
        m_sync->enqueueFolder(m_userId, folderId, /*highPriority=*/true);
    }
}

void FileCacheService::searchLocal(const QString &keyword)
{
    if (keyword.trimmed().isEmpty()) return;
    if (!m_db->isOpen() || m_userId.isEmpty()) return;

    const auto results = m_db->searchFts(m_userId, keyword);
    emit searchResultsLoaded(results, /*isLocal=*/true);
}

void FileCacheService::searchFiles(const QString &keyword)
{
    if (keyword.trimmed().isEmpty()) return;

    // 1. Instant local search across the entire cached tree
    if (m_db->isOpen() && !m_userId.isEmpty()) {
        const auto local = m_db->searchFts(m_userId, keyword);
        emit searchResultsLoaded(local, /*isLocal=*/true);
    }

    // 2. API search for items potentially not yet cached
    FshareApi    *api    = m_api;
    FileCacheDB  *db     = m_db;
    const QString userId = m_userId;
    QPointer<FileCacheService> guard(this);

    // Use direct API call instead of FileService to avoid signal conflicts
    QtConcurrent::run([api, db, userId, keyword, guard]() {
        auto resp = api->searchFiles(keyword, 1);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, db, userId]() {
            if (!guard) return;
            if (resp.isSuccess()) {
                // Merge new API results into cache
                if (db->isOpen() && !userId.isEmpty())
                    db->upsertFiles(userId, resp.data());
                emit guard->searchResultsLoaded(resp.data(), /*isLocal=*/false);
            }
            // Silently ignore API search errors — local results are already shown
        });
    });
}

// ── write operations ──────────────────────────────────────────────────────────

void FileCacheService::createFolder(const QString &name, const QString &parentId)
{
    m_pendingOp           = WriteOp::Create;
    m_pendingSourceFolder = parentId;
    m_service->createFolder(name, parentId);
}

void FileCacheService::renameFile(const QString &linkcode, const QString &newName)
{
    // Optimistic cache update
    if (m_db->isOpen())
        m_db->renameFile(linkcode, newName);

    m_pendingOp = WriteOp::Rename;
    m_service->renameFile(linkcode, newName);
}

void FileCacheService::deleteFiles(const QStringList &linkcodes)
{
    // Optimistic: remove from cache immediately
    if (m_db->isOpen())
        m_db->removeFiles(linkcodes);

    m_pendingOp = WriteOp::Delete;
    m_service->deleteFiles(linkcodes);
}

void FileCacheService::moveFiles(const QStringList &linkcodes,
                                 const QString     &targetFolderId)
{
    // Optimistic: update parent in cache
    if (m_db->isOpen())
        m_db->updateParent(linkcodes, targetFolderId);

    m_pendingOp           = WriteOp::Move;
    m_pendingSourceFolder = m_currentFolderId;
    m_pendingTargetFolder = targetFolderId;
    m_service->moveFiles(linkcodes, targetFolderId);
}

void FileCacheService::copyFiles(const QStringList &linkcodes,
                                 const QString     &targetFolderId)
{
    m_pendingOp           = WriteOp::Copy;
    m_pendingTargetFolder = targetFolderId;
    m_service->copyFiles(linkcodes, targetFolderId);
}

void FileCacheService::refreshFolder(const QString &folderId)
{
    if (m_userId.isEmpty() || !m_sync) return;
    if (m_db->isOpen())
        m_db->clearFolderSync(m_userId, folderId);
    m_sync->enqueueFolder(m_userId, folderId, /*highPriority=*/true);
}

void FileCacheService::refreshUploadFolder(const QString &uploadPath)
{
    if (m_userId.isEmpty() || !m_sync) return;

    // Normalise the path the same way FshareApi::createUploadSession does, so
    // "/", "", and "0" all collapse to root.
    QString path = uploadPath.trimmed();
    if (path.isEmpty() || path == QStringLiteral("0"))
        path = QStringLiteral("/");
    else if (!path.startsWith(QLatin1Char('/')))
        path = QLatin1Char('/') + path;

    // Resolve the target folderId:
    //   root path "/"  → file-manager root uses empty-string folderId
    //   sub-folder     → look it up in the cached folder tree by path match
    QString folderId;
    if (path != QStringLiteral("/") && m_db->isOpen()) {
        const auto folders = m_db->getFolderTree(m_userId);
        for (const FileItem &f : folders) {
            if (f.path == path || f.path == path.mid(1)) {
                folderId = f.linkcode;
                break;
            }
        }
    }

    // Enqueue a high-priority re-sync of the target folder.
    // firstBatchSynced / folderSynced signals already trigger
    // emitCurrentFolderFromCache when it matches m_currentFolderId, so the
    // file manager refreshes automatically if the user is viewing it.
    m_sync->enqueueFolder(m_userId, folderId, /*highPriority=*/true);

    // Also refresh the folder tree so newly created sub-folders (if any) and
    // updated folder metadata propagate to the sidebar.
    loadFolderTree();
}

void FileCacheService::getFileInfo(const QString &url)              { ++m_settingsInFlight; m_service->getFileInfo(url); }
void FileCacheService::changeSecure(const QStringList &lc, bool s)  { ++m_settingsInFlight; m_service->changeSecure(lc, s); }
void FileCacheService::setPassword(const QStringList &lc, const QString &p) { ++m_settingsInFlight; m_service->setPassword(lc, p); }
void FileCacheService::setDirectLink(const QStringList &lc, bool e) { ++m_settingsInFlight; m_service->setDirectLink(lc, e); }

// ── download URL ─────────────────────────────────────────────────────────────

void FileCacheService::getDownloadUrl(const QString &linkcode, const QString &password)
{
    if (linkcode.isEmpty() || !m_api) return;

    const QString fileUrl = QStringLiteral("https://www.fshare.vn/file/") + linkcode;
    FshareApi *api = m_api;
    QPointer<FileCacheService> guard(this);

    QtConcurrent::run([api, fileUrl, password, linkcode, guard]() {
        auto resp = api->createDownloadSession(fileUrl, password);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, linkcode]() {
            if (!guard) return;
            if (resp.isSuccess() && !resp.data().isEmpty()) {
                emit guard->downloadUrlReady(linkcode, resp.data());
            } else {
                const QString err = resp.isSuccess()
                    ? QStringLiteral("Empty download URL")
                    : resp.error().message;
                emit guard->downloadUrlFailed(linkcode, err);
            }
        });
    });
}

// ── local file tracking ──────────────────────────────────────────────────────

void FileCacheService::recordLocalFile(const QString &linkcode, const QString &localPath,
                                       const QString &fileName, int64_t fileSize,
                                       const QString &transferType)
{
    if (m_userId.isEmpty() || linkcode.isEmpty() || localPath.isEmpty()) return;
    if (!m_db->isOpen()) return;
    m_db->upsertLocalFile(m_userId, linkcode, localPath, fileName, fileSize, transferType);
    emit localFileRecorded(linkcode, localPath);
}

QHash<QString, QString> FileCacheService::getLocalPaths(const QStringList &linkcodes)
{
    if (m_userId.isEmpty() || !m_db->isOpen()) return {};
    return m_db->getLocalPaths(m_userId, linkcodes);
}

QString FileCacheService::getLocalPath(const QString &linkcode)
{
    if (m_userId.isEmpty() || !m_db->isOpen()) return {};
    return m_db->getLocalPath(m_userId, linkcode);
}

void FileCacheService::removeLocalFile(const QString &linkcode)
{
    if (m_userId.isEmpty() || !m_db->isOpen()) return;
    m_db->removeLocalFile(m_userId, linkcode);
}

// ── helpers ───────────────────────────────────────────────────────────────────

void FileCacheService::emitCurrentFolderFromCache()
{
    if (!m_db->isOpen() || m_userId.isEmpty())
        return;
    // Root (empty folderId) is also served from cache: queryFiles filters by
    // parent_id='' which matches items ingested from the root listing endpoint.
    const auto files = m_db->queryFiles(m_userId, m_currentFolderId,
                                        m_currentSortKey, m_currentSortAsc,
                                        m_currentFilter);
    qInfo().noquote() << "[FileCacheService] emitCurrentFolderFromCache folderId="
                      << m_currentFolderId << "filter=" << m_currentFilter
                      << "count=" << files.size();
    emit fileListLoaded(files);
}

int FileCacheService::cachedCount(const QString &folderId, const QString &typeFilter)
{
    if (!m_db->isOpen() || m_userId.isEmpty()) return 0;
    return m_db->countFiles(m_userId, folderId, typeFilter);
}

bool FileCacheService::isFolderSynced(const QString &folderId) const
{
    if (!m_db->isOpen() || m_userId.isEmpty()) return false;
    return m_db->getSyncState(m_userId, folderId).isComplete;
}

} // namespace fsnext
