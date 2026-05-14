#include "FileManagerViewModel.h"

#include "FileListModel.h"
#include "FolderPickerModel.h"
#include "FolderTreeModel.h"
#include "core/services/BatchFileResolver.h"
#include "core/services/FileCacheService.h"
#include "platform/PlatformUtils.h"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QUrl>
#include <QVariantMap>
#include <QtConcurrent>

#include "core/api/FshareApi.h"

namespace fsnext {

// ── helpers ───────────────────────────────────────────────────────────────────

static QVariantMap fileItemToVariant(const FileItem &f)
{
    QVariantMap m;
    m[QStringLiteral("id")]            = QString::number(f.id);
    m[QStringLiteral("linkcode")]      = f.linkcode;
    m[QStringLiteral("name")]          = f.name;
    m[QStringLiteral("type")]          = f.type;
    m[QStringLiteral("size")]          = static_cast<qint64>(f.size);
    m[QStringLiteral("path")]          = f.path;
    m[QStringLiteral("secure")]        = f.secure;
    m[QStringLiteral("public")]        = f.isPublic;
    m[QStringLiteral("directlink")]    = f.directlink;
    m[QStringLiteral("hasPassword")]   = f.hasPassword;
    m[QStringLiteral("parentId")]      = f.parentId;
    m[QStringLiteral("downloadCount")] = f.downloadCount;
    m[QStringLiteral("created")]       = f.created;
    m[QStringLiteral("modified")]      = f.modified;
    m[QStringLiteral("isFolder")]      = f.isFolder();
    m[QStringLiteral("description")]   = f.description;
    return m;
}

static QVariantList toVariantList(const QVector<FileItem> &items)
{
    QVariantList list;
    list.reserve(items.size());
    for (const FileItem &f : items) list.append(fileItemToVariant(f));
    return list;
}

// ── constructor ───────────────────────────────────────────────────────────────

FileManagerViewModel::FileManagerViewModel(FileCacheService  *service,
                                           BatchFileResolver *resolver,
                                           QObject           *parent)
    : QObject(parent)
    , m_service(service)
    , m_resolver(resolver)
    , m_fileListModel(new FileListModel(this))
    , m_folderTreeModel(new FolderTreeModel(this))
    , m_folderPickerModel(new FolderPickerModel(this))
{
    if (!m_service) return;

    connect(m_service, &FileCacheService::isLoadingChanged, this, [this](bool loading) {
        if (m_isLoading == loading) return;
        m_isLoading = loading;
        emit isLoadingChanged();
    });

    connect(m_service, &FileCacheService::folderTreeLoaded,
            this, [this](const QVector<FileItem> &folders) {
                m_folderTreeModel->resetFolders(folders);
                // Keep the upload dialog's picker in sync with the tree. The
                // rebuild is internally digest-guarded so re-emissions with
                // identical content are cheap.
                m_folderPickerModel->rebuild(folders);
                emit folderTreeChanged();
                // Breadcrumbs are built from the folder tree's parent chain,
                // so re-notify the QML bindings whenever the tree changes —
                // otherwise the breadcrumb bar would only refresh on folder
                // navigation and stay stale when background sync discovers
                // new sub-folders for the path we're already viewing.
                emit currentFolderChanged();
            });

    // fileListLoaded is emitted twice per folder visit:
    //   1. Immediately from cache  (fast path)
    //   2. After background sync finishes (refreshed data)
    //
    // For large folders (10K+ files) the sync payload is usually identical
    // to the cache payload — the API just confirmed nothing changed. We
    // digest (linkcode|modified|size) for each row and skip the whole
    // reset/enrich/notify pipeline when the digest matches the previous
    // payload for this folder. We still update m_isSyncing (it flips to
    // false after sync completes) so the indicator can hide.
    connect(m_service, &FileCacheService::fileListLoaded,
            this, [this](const QVector<FileItem> &files) {
                const QString folderId = currentFolderId();

                // FNV-1a 64-bit over a compact fingerprint string per row.
                // Cheap to compute (single pass, no allocations for the hash
                // itself), and collision-resistant enough for a "did the list
                // shape change" check.
                quint64 digest = 1469598103934665603ULL; // FNV offset basis
                const quint64 prime = 1099511628211ULL;
                auto mixBytes = [&](const QByteArray &b) {
                    for (char c : b) {
                        digest ^= static_cast<unsigned char>(c);
                        digest *= prime;
                    }
                    digest ^= '|';
                    digest *= prime;
                };
                for (const FileItem &f : files) {
                    mixBytes(f.linkcode.toUtf8());
                    mixBytes(f.modified.toUtf8());
                    mixBytes(QByteArray::number(static_cast<qint64>(f.size)));
                }
                // Fold file count in so empty vs single-empty still differ.
                digest ^= static_cast<quint64>(files.size());
                digest *= prime;

                const bool sameAsLast =
                    m_loadedFolderId == folderId &&
                    m_fileListDigest.value(folderId, 0ULL) == digest &&
                    m_fileListModel->count() == files.size();

                if (!sameAsLast) {
                    m_fileListDigest.insert(folderId, digest);
                    m_loadedFolderId = folderId;
                    m_fileListModel->resetItems(files);
                    enrichWithLocalPaths();
                    emit fileListChanged();
                    refreshTotalCount();
                }

                const bool syncing = !m_service->isFolderSynced(folderId);
                if (syncing != m_isSyncing) {
                    m_isSyncing = syncing;
                    emit isSyncingChanged();
                }
            });

    // Real-time update: when a transfer completes while viewing the same folder,
    // update the isDownloaded indicator immediately without a full reload.
    connect(m_service, &FileCacheService::localFileRecorded,
            this, [this](const QString &linkcode, const QString &localPath) {
                m_fileListModel->updateLocalPath(linkcode, localPath);
            });

    // Search results: merge API results into local results (dedup by linkcode)
    connect(m_service, &FileCacheService::searchResultsLoaded,
            this, [this](const QVector<FileItem> &files, bool isLocal) {
                if (isLocal) {
                    // Local FTS5 results replace the list
                    m_fileListModel->resetItems(files);
                } else {
                    // API results: merge new items not already in the local list
                    m_fileListModel->mergeItems(files);
                }
                emit fileListChanged();
                refreshTotalCount();
            });

    connect(m_service, &FileCacheService::operationComplete,
            this, [this](const QString &msg) {
                emit operationMessage(msg, false);
            });

    connect(m_service, &FileCacheService::operationFailed,
            this, [this](const QString &err) {
                emit operationMessage(err, true);
            });

    // ── Download URL (stream link) wiring ──
    connect(m_service, &FileCacheService::downloadUrlReady,
            this, [this](const QString &linkcode, const QString &url) {
                QGuiApplication::clipboard()->setText(url);
                emit linksCopied(1);
                emit streamLinkReady(linkcode, url);
            });

    connect(m_service, &FileCacheService::downloadUrlFailed,
            this, [this](const QString &, const QString &err) {
                emit streamLinkError(err);
            });

    // ── BatchFileResolver wiring ──
    if (m_resolver) {
        connect(m_resolver, &BatchFileResolver::itemResolved,
                this, [this](const FileItem &item) {
                    m_fileListModel->mergeItems({item});
                    emit fileListChanged();
                    refreshTotalCount();
                });

        connect(m_resolver, &BatchFileResolver::batchProgress,
                this, &FileManagerViewModel::resolveProgress);

        connect(m_resolver, &BatchFileResolver::batchCompleted,
                this, &FileManagerViewModel::resolveCompleted);
    }
}

// ── backward-compat QVariantList accessors ────────────────────────────────────

QVariantList FileManagerViewModel::folderTree() const
{
    return toVariantList(m_folderTreeModel->items());
}

QVariantList FileManagerViewModel::fileList() const
{
    return toVariantList(m_fileListModel->items());
}

// ── navigation ────────────────────────────────────────────────────────────────

void FileManagerViewModel::navigateTo(const QString &folderId, const QString &folderName)
{
    // Discard any forward history when the user navigates to a new place
    if (m_histIdx < m_history.size() - 1)
        m_history.resize(m_histIdx + 1);

    m_history.append({folderId, folderName});
    m_histIdx = m_history.size() - 1;

    clearSelection();
    clearSearch();

    emit currentFolderChanged();
    emit historyChanged();

    reloadCurrentFolder();
}

void FileManagerViewModel::navigateBack()
{
    if (!canGoBack()) return;
    --m_histIdx;
    clearSelection();
    clearSearch();
    emit currentFolderChanged();
    emit historyChanged();
    reloadCurrentFolder();
}

void FileManagerViewModel::navigateForward()
{
    if (!canGoForward()) return;
    ++m_histIdx;
    clearSelection();
    clearSearch();
    emit currentFolderChanged();
    emit historyChanged();
    reloadCurrentFolder();
}

void FileManagerViewModel::loadFolder(const QString &id)
{
    navigateTo(id, QString());
}

void FileManagerViewModel::refreshCurrentFolder()
{
    if (!m_service) return;
    // User-initiated refresh: invalidate our digest so the next fileListLoaded
    // pass always reaches the model, even if the payload happens to match what
    // we last saw byte-for-byte. The user clicked Refresh expecting to see
    // the list re-render.
    m_fileListDigest.remove(currentFolderId());
    m_service->refreshFolder(currentFolderId());
    // Also re-issue the normal reload so the UI shows a loading state and
    // the sidebar tree is refreshed when at root.
    reloadCurrentFolder();
}

// ── sort & filter ─────────────────────────────────────────────────────────────

void FileManagerViewModel::setSortKey(const QString &key)
{
    if (m_sortKey == key) return;
    m_sortKey = key;
    emit sortKeyChanged();
    reloadCurrentFolder();
}

void FileManagerViewModel::setSortAscending(bool asc)
{
    if (m_sortAsc == asc) return;
    m_sortAsc = asc;
    emit sortAscendingChanged();
    reloadCurrentFolder();
}

void FileManagerViewModel::setTypeFilter(const QString &filter)
{
    if (m_typeFilter == filter) return;
    m_typeFilter = filter;
    emit typeFilterChanged();
    reloadCurrentFolder();
}

// ── view mode ────────────────────────────────────────────────────────────────

void FileManagerViewModel::setViewMode(const QString &mode)
{
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    emit viewModeChanged();
}

// ── local file actions ───────────────────────────────────────────────────────

void FileManagerViewModel::openInExplorer(const QString &localPath)
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openInExplorer(localPath);
}

void FileManagerViewModel::openLocalFile(const QString &localPath)
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openFile(localPath);
}

// ── stream link (video) ──────────────────────────────────────────────────────

void FileManagerViewModel::getStreamLink(const QString &linkcode)
{
    if (linkcode.isEmpty() || !m_service) return;

    // Request the direct download URL from the API (async).
    // On success the URL is copied to clipboard and emitted via streamLinkReady;
    // the user can then open it in VLC / system media player.
    m_service->getDownloadUrl(linkcode);
}

void FileManagerViewModel::playStreamUrl(const QString &url, const QString &fileName)
{
    if (url.isEmpty()) return;
    const bool ok = PlatformUtils::playStreamUrl(url, fileName);
    if (!ok) {
        emit operationMessage(tr("Không mở được trình phát mặc định"), true);
    }
}

// ── batch resolve ────────────────────────────────────────────────────────────

void FileManagerViewModel::resolveLinks(const QStringList &urls)
{
    if (!m_resolver || urls.isEmpty()) return;
    m_resolver->resolve(urls, 3);
}

void FileManagerViewModel::cancelResolve()
{
    if (m_resolver) m_resolver->cancel();
}

// ── selection ─────────────────────────────────────────────────────────────────

QStringList FileManagerViewModel::selectedLinkcodes() const
{
    return QStringList(m_selected.cbegin(), m_selected.cend());
}

void FileManagerViewModel::toggleSelection(const QString &linkcode, bool multiSelect)
{
    if (!multiSelect) {
        const bool alreadyOnly = m_selected.size() == 1 && m_selected.contains(linkcode);
        m_selected.clear();
        if (!alreadyOnly) m_selected.insert(linkcode);
    } else {
        if (m_selected.contains(linkcode)) m_selected.remove(linkcode);
        else                               m_selected.insert(linkcode);
    }
    emit selectionChanged();
}

void FileManagerViewModel::selectAll()
{
    const auto &items = m_fileListModel->items();
    for (const FileItem &f : items) {
        if (!f.linkcode.isEmpty()) m_selected.insert(f.linkcode);
    }
    emit selectionChanged();
}

void FileManagerViewModel::clearSelection()
{
    if (m_selected.isEmpty()) return;
    m_selected.clear();
    emit selectionChanged();
}

// ── search ────────────────────────────────────────────────────────────────────

void FileManagerViewModel::searchRecursive(const QString &keyword)
{
    if (keyword.trimmed().isEmpty()) { clearSearch(); return; }

    m_searchMode    = true;
    m_searchKeyword = keyword;
    emit searchModeChanged();

    if (m_service) m_service->searchLocal(keyword);
}

void FileManagerViewModel::searchFiles(const QString &keyword)
{
    if (keyword.trimmed().isEmpty()) { clearSearch(); return; }

    m_searchMode    = true;
    m_searchKeyword = keyword;
    emit searchModeChanged();

    if (m_service) m_service->searchFiles(keyword);
}

void FileManagerViewModel::clearSearch()
{
    if (!m_searchMode) return;
    m_searchMode    = false;
    m_searchKeyword.clear();
    emit searchModeChanged();
}

// ── CRUD ──────────────────────────────────────────────────────────────────────

void FileManagerViewModel::createFolder(const QString &name, const QString &parentId)
{
    if (m_service) m_service->createFolder(name, parentId);
}

void FileManagerViewModel::renameFile(const QString &linkcode, const QString &name)
{
    if (m_service) m_service->renameFile(linkcode, name);
}

void FileManagerViewModel::deleteFiles(const QStringList &linkcodes)
{
    if (!m_service) return;

    // Optimistic: remove from the visible model immediately
    m_fileListModel->removeByLinkcodes(linkcodes);
    QSet<QString> removing(linkcodes.cbegin(), linkcodes.cend());
    m_selected.subtract(removing);
    emit fileListChanged();
    emit selectionChanged();

    m_service->deleteFiles(linkcodes);
}

void FileManagerViewModel::deleteSelected()
{
    if (!hasSelection()) return;
    deleteFiles(selectedLinkcodes());
}

void FileManagerViewModel::moveFiles(const QStringList &linkcodes, const QString &to)
{
    if (m_service) m_service->moveFiles(linkcodes, to);
}

void FileManagerViewModel::copyFiles(const QStringList &linkcodes, const QString &to)
{
    if (m_service) m_service->copyFiles(linkcodes, to);
}

// ── file settings ─────────────────────────────────────────────────────────────

void FileManagerViewModel::getFileInfo(const QString &url)
{
    if (m_service) m_service->getFileInfo(url);
}

void FileManagerViewModel::changeSecure(const QStringList &linkcodes, bool secure)
{
    if (m_service) m_service->changeSecure(linkcodes, secure);
}

void FileManagerViewModel::setPassword(const QStringList &linkcodes, const QString &pwd)
{
    if (m_service) m_service->setPassword(linkcodes, pwd);
}

void FileManagerViewModel::setDirectLink(const QStringList &linkcodes, bool enabled)
{
    if (m_service) m_service->setDirectLink(linkcodes, enabled);
}

void FileManagerViewModel::copyLinks(const QStringList &linkcodes)
{
    if (linkcodes.isEmpty()) return;
    QStringList urls;
    urls.reserve(linkcodes.size());
    for (const QString &lc : linkcodes) {
        // Check if the item is a folder — use /folder/ URL pattern
        const FileItem *item = m_fileListModel->findByLinkcode(lc);
        if (item && item->isFolder())
            urls.append(QStringLiteral("https://www.fshare.vn/folder/") + lc);
        else
            urls.append(QStringLiteral("https://www.fshare.vn/file/") + lc);
    }
    QGuiApplication::clipboard()->setText(urls.join(QLatin1Char('\n')));
    emit linksCopied(linkcodes.size());
}

// ── navigation helpers ────────────────────────────────────────────────────────

QString FileManagerViewModel::currentFolderId() const
{
    return (m_histIdx >= 0 && m_histIdx < m_history.size())
           ? m_history[m_histIdx].id
           : QString();
}

QVariantList FileManagerViewModel::breadcrumbs() const
{
    // Build breadcrumbs from the folder hierarchy (parent chain), NOT from
    // navigation history. This ensures breadcrumbs show the actual folder
    // path (Root > Parent > Current) rather than a confusing history trail.
    const QString fid = currentFolderId();
    if (fid.isEmpty()) return {};

    const auto path = m_folderTreeModel->buildBreadcrumbPath(fid);
    QVariantList crumbs;
    crumbs.reserve(path.size());
    for (const auto &[id, name] : path) {
        QVariantMap c;
        c[QStringLiteral("id")]   = id;
        c[QStringLiteral("name")] = name;
        crumbs.append(c);
    }
    return crumbs;
}

bool FileManagerViewModel::canGoBack()    const { return m_histIdx > 0; }
bool FileManagerViewModel::canGoForward() const { return m_histIdx < m_history.size() - 1; }

// ── private ───────────────────────────────────────────────────────────────────

void FileManagerViewModel::reloadCurrentFolder()
{
    if (!m_service) return;
    const QString fid = currentFolderId();
    if (fid.isEmpty()) {
        // Root view — Windows Explorer style: sidebar tree + level-1 content
        // (both files and folders). loadFolderTree feeds the sidebar;
        // listFiles("") fetches level-1 items for the main pane.
        m_service->loadFolderTree();
        m_service->listFiles(QString(), m_sortKey, m_sortAsc, m_typeFilter);
    } else {
        m_service->listFiles(fid, m_sortKey, m_sortAsc, m_typeFilter);
    }
}

void FileManagerViewModel::refreshTotalCount()
{
    const int n = m_fileListModel->count();
    if (m_totalCount == n) return;
    m_totalCount = n;
    emit totalCountChanged();
}

void FileManagerViewModel::enrichWithLocalPaths()
{
    if (!m_service) return;

    const auto &items = m_fileListModel->items();
    QStringList linkcodes;
    linkcodes.reserve(items.size());
    for (const FileItem &f : items) {
        if (f.isFile() && !f.linkcode.isEmpty())
            linkcodes << f.linkcode;
    }
    if (linkcodes.isEmpty()) return;

    auto paths = m_service->getLocalPaths(linkcodes);

    // Lazy verification: remove stale entries where file no longer exists
    for (auto it = paths.begin(); it != paths.end(); ) {
        if (!QFile::exists(it.value())) {
            m_service->removeLocalFile(it.key());
            it = paths.erase(it);
        } else {
            ++it;
        }
    }

    m_fileListModel->setLocalPaths(paths);
}

} // namespace fsnext
