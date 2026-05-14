#include "FavoritesViewModel.h"

#include "FileListModel.h"
#include "core/api/FshareApi.h"
#include "core/services/FileCacheService.h"
#include "platform/PlatformUtils.h"

#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QVariantMap>
#include <QtConcurrent>

namespace fsnext {

// ── constructor ──────────────────────────────────────────────────────────────

FavoritesViewModel::FavoritesViewModel(FshareApi        *api,
                                       FileCacheService *cacheService,
                                       QObject          *parent)
    : QObject(parent)
    , m_api(api)
    , m_cache(cacheService)
    , m_fileListModel(new FileListModel(this))
{
    if (!m_cache) return;

    // When browsing inside a favorite folder, FileCacheService delivers the
    // folder contents via this signal (cache-first, then background refresh).
    connect(m_cache, &FileCacheService::fileListLoaded,
            this, [this](const QVector<FileItem> &files) {
                // Only update if we're in folder-browsing mode
                if (!isInFolder()) return;
                m_fileListModel->resetItems(files);
                refreshTotalCount();
            });

    // Forward operation results to QML
    connect(m_cache, &FileCacheService::operationComplete,
            this, [this](const QString &msg) {
                emit operationMessage(msg, false);
            });

    connect(m_cache, &FileCacheService::operationFailed,
            this, [this](const QString &err) {
                emit operationMessage(err, true);
            });

    // Download URL (stream link) wiring
    connect(m_cache, &FileCacheService::downloadUrlReady,
            this, [this](const QString &linkcode, const QString &url) {
                QGuiApplication::clipboard()->setText(url);
                emit linksCopied(1);
                emit streamLinkReady(linkcode, url);
            });

    connect(m_cache, &FileCacheService::downloadUrlFailed,
            this, [this](const QString &, const QString &err) {
                emit streamLinkError(err);
            });
}

// ── property helpers ─────────────────────────────────────────────────────────

void FavoritesViewModel::setLoading(bool v)
{
    if (m_isLoading == v) return;
    m_isLoading = v;
    emit isLoadingChanged();
}

void FavoritesViewModel::refreshTotalCount()
{
    const int n = m_fileListModel->count();
    if (m_totalCount == n) return;
    m_totalCount = n;
    emit totalCountChanged();
}

// ── Favorites CRUD ───────────────────────────────────────────────────────────

void FavoritesViewModel::loadFavorites()
{
    // Go back to root view
    m_folderStack.clear();
    emit navigationChanged();

    setLoading(true);

    FshareApi *api = m_api;
    const QString extFilter = m_extFilter;
    QPointer<FavoritesViewModel> guard(this);

    QtConcurrent::run([api, extFilter, guard]() {
        auto resp = api->listFavorites(extFilter);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            guard->setLoading(false);
            if (resp.isSuccess()) {
                guard->m_fileListModel->resetItems(resp.data());
                guard->refreshTotalCount();
            } else {
                emit guard->operationMessage(resp.error().message, true);
            }
        });
    });
}

void FavoritesViewModel::addToFavorite(const QString &linkcode)
{
    if (linkcode.isEmpty() || !m_api) return;

    FshareApi *api = m_api;
    QPointer<FavoritesViewModel> guard(this);

    QtConcurrent::run([api, linkcode, guard]() {
        auto resp = api->changeFavorite(linkcode, true);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            if (resp.isSuccess()) {
                emit guard->operationMessage(tr("Đã thêm vào yêu thích"), false);
            } else {
                emit guard->operationMessage(resp.error().message, true);
            }
        });
    });
}

void FavoritesViewModel::removeFromFavorite(const QString &linkcode)
{
    if (linkcode.isEmpty() || !m_api) return;

    // Optimistic: remove from visible list immediately
    m_fileListModel->removeByLinkcodes({linkcode});
    m_selected.remove(linkcode);
    emit selectionChanged();
    refreshTotalCount();

    FshareApi *api = m_api;
    QPointer<FavoritesViewModel> guard(this);

    QtConcurrent::run([api, linkcode, guard]() {
        auto resp = api->changeFavorite(linkcode, false);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, linkcode]() {
            if (!guard) return;
            if (resp.isSuccess()) {
                emit guard->operationMessage(tr("Đã bỏ yêu thích"), false);
            } else {
                // Revert: reload the full list
                emit guard->operationMessage(resp.error().message, true);
                guard->loadFavorites();
            }
        });
    });
}

// ── Folder navigation ────────────────────────────────────────────────────────

void FavoritesViewModel::navigateToFolder(const QString &folderId, const QString &folderName)
{
    if (folderId.isEmpty()) return;

    m_folderStack.append({folderId, folderName});
    clearSelection();
    emit navigationChanged();

    // Use FileCacheService to load the folder contents
    if (m_cache)
        m_cache->listFiles(folderId, QStringLiteral("name"), true, {});
}

void FavoritesViewModel::navigateBack()
{
    if (m_folderStack.isEmpty()) return;

    m_folderStack.removeLast();
    clearSelection();
    emit navigationChanged();

    if (m_folderStack.isEmpty()) {
        // Back to favorites root — reload favorites list
        loadFavorites();
    } else {
        // Navigate to the previous folder in the stack
        const auto &top = m_folderStack.last();
        if (m_cache)
            m_cache->listFiles(top.id, QStringLiteral("name"), true, {});
    }
}

void FavoritesViewModel::navigateToRoot()
{
    if (m_folderStack.isEmpty()) return;

    m_folderStack.clear();
    clearSelection();
    emit navigationChanged();

    loadFavorites();
}

// ── Breadcrumbs ──────────────────────────────────────────────────────────────

QVariantList FavoritesViewModel::breadcrumbs() const
{
    QVariantList crumbs;
    crumbs.reserve(m_folderStack.size());
    for (const auto &entry : m_folderStack) {
        QVariantMap c;
        c[QStringLiteral("id")]   = entry.id;
        c[QStringLiteral("name")] = entry.name;
        crumbs.append(c);
    }
    return crumbs;
}

// ── Selection ────────────────────────────────────────────────────────────────

QStringList FavoritesViewModel::selectedLinkcodes() const
{
    return QStringList(m_selected.cbegin(), m_selected.cend());
}

void FavoritesViewModel::toggleSelection(const QString &linkcode)
{
    if (m_selected.contains(linkcode))
        m_selected.remove(linkcode);
    else
        m_selected.insert(linkcode);
    emit selectionChanged();
}

void FavoritesViewModel::selectAll()
{
    const auto &items = m_fileListModel->items();
    for (const FileItem &f : items) {
        if (!f.linkcode.isEmpty())
            m_selected.insert(f.linkcode);
    }
    emit selectionChanged();
}

void FavoritesViewModel::clearSelection()
{
    if (m_selected.isEmpty()) return;
    m_selected.clear();
    emit selectionChanged();
}

// ── File actions ─────────────────────────────────────────────────────────────

void FavoritesViewModel::copyLinks(const QStringList &linkcodes)
{
    if (linkcodes.isEmpty()) return;
    QStringList urls;
    urls.reserve(linkcodes.size());
    for (const QString &lc : linkcodes) {
        const FileItem *item = m_fileListModel->findByLinkcode(lc);
        if (item && item->isFolder())
            urls.append(QStringLiteral("https://www.fshare.vn/folder/") + lc);
        else
            urls.append(QStringLiteral("https://www.fshare.vn/file/") + lc);
    }
    QGuiApplication::clipboard()->setText(urls.join(QLatin1Char('\n')));
    emit linksCopied(linkcodes.size());
}

void FavoritesViewModel::getStreamLink(const QString &linkcode)
{
    if (linkcode.isEmpty() || !m_cache) return;
    m_cache->getDownloadUrl(linkcode);
}

void FavoritesViewModel::openInExplorer(const QString &localPath)
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openInExplorer(localPath);
}

void FavoritesViewModel::openLocalFile(const QString &localPath)
{
    if (localPath.isEmpty()) return;
    PlatformUtils::openFile(localPath);
}

void FavoritesViewModel::playStreamUrl(const QString &url, const QString &fileName)
{
    if (url.isEmpty()) return;
    const bool ok = PlatformUtils::playStreamUrl(url, fileName);
    if (!ok) {
        emit operationMessage(tr("Không mở được trình phát mặc định"), true);
    }
}

// ── File settings ────────────────────────────────────────────────────────────

void FavoritesViewModel::changeSecure(const QStringList &linkcodes, bool secure)
{
    if (m_cache) m_cache->changeSecure(linkcodes, secure);
}

void FavoritesViewModel::setPassword(const QStringList &linkcodes, const QString &pwd)
{
    if (m_cache) m_cache->setPassword(linkcodes, pwd);
}

void FavoritesViewModel::setDirectLink(const QStringList &linkcodes, bool enabled)
{
    if (m_cache) m_cache->setDirectLink(linkcodes, enabled);
}

void FavoritesViewModel::deleteFiles(const QStringList &linkcodes)
{
    if (!m_cache) return;

    // Optimistic: remove from visible model
    m_fileListModel->removeByLinkcodes(linkcodes);
    QSet<QString> removing(linkcodes.cbegin(), linkcodes.cend());
    m_selected.subtract(removing);
    emit selectionChanged();
    refreshTotalCount();

    m_cache->deleteFiles(linkcodes);
}

void FavoritesViewModel::renameFile(const QString &linkcode, const QString &name)
{
    if (m_cache) m_cache->renameFile(linkcode, name);
}

// ── Filter ───────────────────────────────────────────────────────────────────

void FavoritesViewModel::setExtFilter(const QString &filter)
{
    if (m_extFilter == filter) return;
    m_extFilter = filter;
    emit extFilterChanged();

    // Reload favorites with the new filter (only when at root)
    if (!isInFolder())
        loadFavorites();
}

} // namespace fsnext
