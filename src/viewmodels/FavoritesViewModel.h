#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

#include "viewmodels/FileListModel.h"

namespace fsnext {

class FshareApi;
class FileCacheService;
class FolderTreeModel;

// ViewModel for the Favorites page.
//
// Two modes:
//   1. Root — shows the user's favorite files/folders (from API)
//   2. Folder browsing — user clicked a favorite folder, show its contents
//      with breadcrumb navigation (reuses FileCacheService for folder listing)
class FavoritesViewModel : public QObject
{
    Q_OBJECT

    // ── Data ──────────────────────────────────────────────────
    Q_PROPERTY(FileListModel* fileListModel READ fileListModel CONSTANT)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(int  totalCount READ totalCount NOTIFY totalCountChanged)

    // ── Navigation ────────────────────────────────────────────
    Q_PROPERTY(bool isInFolder READ isInFolder NOTIFY navigationChanged)
    Q_PROPERTY(QVariantList breadcrumbs READ breadcrumbs NOTIFY navigationChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationChanged)

    // ── Selection ─────────────────────────────────────────────
    Q_PROPERTY(QStringList selectedLinkcodes READ selectedLinkcodes NOTIFY selectionChanged)
    Q_PROPERTY(int  selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)

    // ── Filter ────────────────────────────────────────────────
    Q_PROPERTY(QString extFilter READ extFilter WRITE setExtFilter NOTIFY extFilterChanged)
    // Fulltext name search (client-side substring, diacritic-insensitive).
    // Applies on top of whatever items the page currently shows — both the
    // favorites root and any sub-folder the user has drilled into. Unlike
    // extFilter (server-side hint that triggered a re-fetch), nameFilter never
    // round-trips the server, so it stays in sync with the user's keystrokes
    // and is reliable for partial matches like "lan" → "Lan toả.mp4".
    Q_PROPERTY(QString nameFilter READ nameFilter WRITE setNameFilter NOTIFY nameFilterChanged)

public:
    explicit FavoritesViewModel(FshareApi        *api,
                                 FileCacheService *cacheService,
                                 QObject          *parent = nullptr);
    ~FavoritesViewModel() override = default;

    // ── Model accessor ───────────────────────────────────────
    FileListModel *fileListModel() const { return m_fileListModel; }

    // ── Property readers ─────────────────────────────────────
    bool isLoading()  const { return m_isLoading; }
    int  totalCount() const { return m_totalCount; }

    bool         isInFolder()  const { return !m_folderStack.isEmpty(); }
    QVariantList breadcrumbs() const;
    bool         canGoBack()   const { return !m_folderStack.isEmpty(); }

    QStringList selectedLinkcodes() const;
    int  selectedCount() const { return m_selected.size(); }
    bool hasSelection()  const { return !m_selected.isEmpty(); }

    QString extFilter() const { return m_extFilter; }
    void setExtFilter(const QString &filter);

    QString nameFilter() const { return m_nameFilter; }
    void setNameFilter(const QString &filter);

    // ── Favorites CRUD ───────────────────────────────────────
    Q_INVOKABLE void loadFavorites();
    Q_INVOKABLE void addToFavorite(const QString &linkcode);
    Q_INVOKABLE void removeFromFavorite(const QString &linkcode);

    // ── Folder navigation ────────────────────────────────────
    Q_INVOKABLE void navigateToFolder(const QString &folderId, const QString &folderName);
    Q_INVOKABLE void navigateBack();
    Q_INVOKABLE void navigateToRoot();

    // ── Selection ────────────────────────────────────────────
    Q_INVOKABLE void toggleSelection(const QString &linkcode);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    // Replace the entire selection with the given set of linkcodes. See
    // FileManagerViewModel::setSelected for rationale (keyboard / single-click
    // handlers in QML need C++ state to mirror the visible highlight).
    Q_INVOKABLE void setSelected(const QStringList &linkcodes);

    // ── File actions ─────────────────────────────────────────
    Q_INVOKABLE void copyLinks(const QStringList &linkcodes);
    Q_INVOKABLE void getStreamLink(const QString &linkcode);
    Q_INVOKABLE void openInExplorer(const QString &localPath);
    Q_INVOKABLE void openLocalFile(const QString &localPath);

    // Stream a direct URL through the user's default media player (writes a
    // temp .m3u8 playlist and hands it to the OS — Qt.openUrlExternally on
    // https:// URLs would incorrectly dispatch to the browser).
    //
    // fileName is optional; when provided it becomes the playlist title so
    // the media player's window bar shows something meaningful.
    Q_INVOKABLE void playStreamUrl(const QString &url, const QString &fileName = QString{});

    // ── File settings (delegate to cache service) ────────────
    Q_INVOKABLE void changeSecure(const QStringList &linkcodes, bool secure);
    Q_INVOKABLE void setPassword(const QStringList &linkcodes, const QString &pwd);
    Q_INVOKABLE void setDirectLink(const QStringList &linkcodes, bool enabled);
    Q_INVOKABLE void deleteFiles(const QStringList &linkcodes);
    Q_INVOKABLE void renameFile(const QString &linkcode, const QString &name);

signals:
    void isLoadingChanged();
    void totalCountChanged();
    void navigationChanged();
    void selectionChanged();
    void extFilterChanged();
    void nameFilterChanged();

    void linksCopied(int count);
    void operationMessage(const QString &msg, bool isError);
    void streamLinkReady(const QString &linkcode, const QString &url);
    void streamLinkError(const QString &message);

private:
    void setLoading(bool v);
    void refreshTotalCount();
    // Set the visible model from `source`, after applying the current
    // nameFilter as a case- and diacritic-insensitive substring match on
    // `FileItem::name`. Caches `source` in m_allItems so a later filter
    // change re-applies without going back to the server / cache.
    void applyAndShow(const QVector<FileItem> &source);
    // Normalise a string for fuzzy matching: trim → lowercase → strip
    // Vietnamese combining marks (so "Lồng tiếng" matches "long tieng" and
    // vice versa). Defined inline to keep parity with the BadWordFilter's
    // stripDiacritics helper without sharing state.
    static QString normaliseForSearch(const QString &s);

    struct FolderEntry { QString id; QString name; };

    FshareApi        *m_api     = nullptr;
    FileCacheService *m_cache   = nullptr;
    FileListModel    *m_fileListModel = nullptr;

    bool m_isLoading = false;
    int  m_totalCount = 0;

    // Folder navigation stack (empty = showing favorites root)
    QVector<FolderEntry> m_folderStack;

    // Selection
    QSet<QString> m_selected;

    // Filter
    QString m_extFilter;
    QString m_nameFilter;
    // Unfiltered snapshot of whatever items the visible model was last
    // populated with (favorites root OR sub-folder contents). Lets a
    // keystroke-driven nameFilter re-apply without round-tripping the
    // server or cache. Cleared on every fresh load.
    QVector<FileItem> m_allItems;
};

} // namespace fsnext
