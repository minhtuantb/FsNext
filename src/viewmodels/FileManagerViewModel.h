#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

#include "viewmodels/FileListModel.h"
#include "viewmodels/FolderPickerModel.h"
#include "viewmodels/FolderTreeModel.h"

namespace fsnext {

class FileCacheService;
class BatchFileResolver;

// ViewModel for the File Manager screen.
//
// Exposes to QML:
//   - folderTreeModel — QAbstractListModel for sidebar tree of folders
//   - fileListModel   — QAbstractListModel for current folder contents
//   - breadcrumbs     — [{id, name}] hierarchy path (folder chain, NOT nav history)
//   - sort / filter   — applied locally against the cache
//   - selection       — multi-select with toggle / selectAll / clear
//   - search          — instant local FTS5 + parallel API search
class FileManagerViewModel : public QObject
{
    Q_OBJECT

    // ── Data ──────────────────────────────────────────────────────────────
    Q_PROPERTY(FolderTreeModel*   folderTreeModel   READ folderTreeModel   CONSTANT)
    Q_PROPERTY(FileListModel*     fileListModel     READ fileListModel     CONSTANT)
    Q_PROPERTY(FolderPickerModel* folderPickerModel READ folderPickerModel CONSTANT)
    Q_PROPERTY(bool             isLoading       READ isLoading       NOTIFY isLoadingChanged)
    Q_PROPERTY(bool             isSyncing       READ isSyncing       NOTIFY isSyncingChanged)

    // Keep folderTree/fileList as QVariantList for backward compat with existing QML
    Q_PROPERTY(QVariantList folderTree READ folderTree NOTIFY folderTreeChanged)
    Q_PROPERTY(QVariantList fileList   READ fileList   NOTIFY fileListChanged)

    // ── Sort & filter ──────────────────────────────────────────────────────
    Q_PROPERTY(QString sortKey      READ sortKey      WRITE setSortKey      NOTIFY sortKeyChanged)
    Q_PROPERTY(bool    sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(QString typeFilter   READ typeFilter   WRITE setTypeFilter   NOTIFY typeFilterChanged)

    // ── Selection ──────────────────────────────────────────────────────────
    Q_PROPERTY(QStringList selectedLinkcodes READ selectedLinkcodes NOTIFY selectionChanged)
    Q_PROPERTY(int         selectedCount     READ selectedCount     NOTIFY selectionChanged)
    Q_PROPERTY(bool        hasSelection      READ hasSelection      NOTIFY selectionChanged)

    // ── Navigation ─────────────────────────────────────────────────────────
    Q_PROPERTY(QString      currentFolderId READ currentFolderId NOTIFY currentFolderChanged)
    Q_PROPERTY(QVariantList breadcrumbs     READ breadcrumbs     NOTIFY currentFolderChanged)
    Q_PROPERTY(bool         canGoBack       READ canGoBack       NOTIFY historyChanged)
    Q_PROPERTY(bool         canGoForward    READ canGoForward    NOTIFY historyChanged)

    // ── View mode ──────────────────────────────────────────────────────────
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)

    // ── Statistics ─────────────────────────────────────────────────────────
    Q_PROPERTY(int  totalCount READ totalCount NOTIFY totalCountChanged)

    // ── Search ─────────────────────────────────────────────────────────────
    Q_PROPERTY(bool    isSearchMode  READ isSearchMode  NOTIFY searchModeChanged)
    Q_PROPERTY(QString searchKeyword READ searchKeyword NOTIFY searchModeChanged)

public:
    explicit FileManagerViewModel(FileCacheService  *service,
                                    BatchFileResolver *resolver = nullptr,
                                    QObject           *parent   = nullptr);
    ~FileManagerViewModel() override = default;

    // ── Model accessors (for QML binding) ─────────────────────────────────
    FileListModel     *fileListModel()     const { return m_fileListModel; }
    FolderTreeModel   *folderTreeModel()   const { return m_folderTreeModel; }
    FolderPickerModel *folderPickerModel() const { return m_folderPickerModel; }

    // ── Backward-compat QVariantList accessors ────────────────────────────
    QVariantList folderTree() const;
    QVariantList fileList()   const;

    // ── Property readers ──────────────────────────────────────────────────
    bool         isLoading()   const { return m_isLoading; }
    bool         isSyncing()   const { return m_isSyncing; }

    QString sortKey()       const { return m_sortKey; }
    bool    sortAscending() const { return m_sortAsc; }
    QString typeFilter()    const { return m_typeFilter; }

    QStringList selectedLinkcodes() const;
    int         selectedCount()     const { return m_selected.size(); }
    bool        hasSelection()      const { return !m_selected.isEmpty(); }

    QString      currentFolderId() const;
    QVariantList breadcrumbs()     const;
    bool         canGoBack()       const;
    bool         canGoForward()    const;

    QString viewMode()    const { return m_viewMode; }
    int    totalCount()   const { return m_totalCount; }
    bool   isSearchMode() const { return m_searchMode; }
    QString searchKeyword() const { return m_searchKeyword; }

    // ── Sort & filter (also callable from QML via WRITE) ──────────────────
    void setSortKey(const QString &key);
    void setSortAscending(bool asc);
    void setTypeFilter(const QString &filter);

    // ── View mode ────────────────────────────────────────────────────────
    void setViewMode(const QString &mode);

    // ── Navigation ────────────────────────────────────────────────────────
    Q_INVOKABLE void navigateTo(const QString &folderId, const QString &folderName);
    Q_INVOKABLE void navigateBack();
    Q_INVOKABLE void navigateForward();
    Q_INVOKABLE void loadFolder(const QString &id);

    // Drop cache for the current folder and re-fetch from the API. Wired to
    // the Refresh toolbar button so users can recover from stale state.
    Q_INVOKABLE void refreshCurrentFolder();

    // ── Selection ────────────────────────────────────────────────────────
    Q_INVOKABLE void toggleSelection(const QString &linkcode, bool multiSelect = true);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    // Replace the entire selection with the given set of linkcodes. Called
    // from QML keyboard / single-click handlers that want C++ state to match
    // the visible highlight (otherwise bulk ops via selectedLinkcodes see an
    // empty set while the user is staring at a highlighted row).
    Q_INVOKABLE void setSelected(const QStringList &linkcodes);

    // ── Search ───────────────────────────────────────────────────────────
    Q_INVOKABLE void searchRecursive(const QString &keyword);
    Q_INVOKABLE void searchFiles(const QString &keyword);
    Q_INVOKABLE void clearSearch();

    // ── CRUD ─────────────────────────────────────────────────────────────
    Q_INVOKABLE void createFolder(const QString &name, const QString &parentId);
    Q_INVOKABLE void renameFile(const QString &linkcode, const QString &name);
    Q_INVOKABLE void deleteFiles(const QStringList &linkcodes);
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void moveFiles(const QStringList &linkcodes, const QString &to);
    Q_INVOKABLE void copyFiles(const QStringList &linkcodes, const QString &to);

    // ── File settings ────────────────────────────────────────────────────
    Q_INVOKABLE void getFileInfo(const QString &url);
    Q_INVOKABLE void changeSecure(const QStringList &linkcodes, bool secure);
    Q_INVOKABLE void setPassword(const QStringList &linkcodes, const QString &pwd);
    Q_INVOKABLE void setDirectLink(const QStringList &linkcodes, bool enabled);
    Q_INVOKABLE void copyLinks(const QStringList &linkcodes);

    // ── Local file actions ───────────────────────────────────────────────
    Q_INVOKABLE void openInExplorer(const QString &localPath);
    Q_INVOKABLE void openLocalFile(const QString &localPath);

    // ── Stream link (video) ─────────────────────────────────────────────
    Q_INVOKABLE void getStreamLink(const QString &linkcode);

    // Stream a direct URL through the user's default media player (writes a
    // temp .m3u8 playlist and hands it to the OS — Qt.openUrlExternally on
    // https:// URLs would incorrectly dispatch to the browser).
    //
    // fileName is optional; when provided it becomes the playlist title so
    // the media player's window bar shows something meaningful.
    Q_INVOKABLE void playStreamUrl(const QString &url, const QString &fileName = QString{});

    // ── Batch resolve ───────────────────────────────────────────────────
    Q_INVOKABLE void resolveLinks(const QStringList &urls);
    Q_INVOKABLE void cancelResolve();

signals:
    void folderTreeChanged();
    void fileListChanged();
    void isLoadingChanged();
    void isSyncingChanged();

    void sortKeyChanged();
    void sortAscendingChanged();
    void typeFilterChanged();

    void selectionChanged();
    void currentFolderChanged();
    void historyChanged();
    void viewModeChanged();
    void totalCountChanged();
    void searchModeChanged();

    void linksCopied(int count);
    void operationMessage(const QString &msg, bool isError);
    void resolveProgress(int completed, int total);
    void resolveCompleted(int total, int succeeded, int failed);

    void streamLinkReady(const QString &linkcode, const QString &url);
    void streamLinkError(const QString &message);

private:
    void reloadCurrentFolder();
    void refreshTotalCount();
    void enrichWithLocalPaths();

    struct NavEntry { QString id; QString name; };

    FileCacheService  *m_service  = nullptr;
    BatchFileResolver *m_resolver = nullptr;
    FileListModel     *m_fileListModel     = nullptr;
    FolderTreeModel   *m_folderTreeModel   = nullptr;
    FolderPickerModel *m_folderPickerModel = nullptr;

    bool         m_isLoading = false;
    bool         m_isSyncing = false;

    // View mode
    QString m_viewMode = QStringLiteral("list");

    // Sort & filter
    QString m_sortKey    = QStringLiteral("name");
    bool    m_sortAsc    = true;
    QString m_typeFilter = QStringLiteral("all");

    // Selection
    QSet<QString> m_selected;

    // Navigation history (back/forward)
    QVector<NavEntry> m_history;
    int               m_histIdx = -1;

    // Statistics
    int m_totalCount = 0;

    // Search
    bool    m_searchMode    = false;
    QString m_searchKeyword;

    // fileListLoaded fires twice per folder visit — once from local cache, once
    // after the background sync completes — and often the second payload is
    // byte-identical to the first. We keep a lightweight content digest of
    // the last accepted payload so the second emission can short-circuit the
    // reset+enrich+refresh pipeline when nothing actually changed.
    // Key: currentFolderId() ; Value: digest over (linkcode|modified|size).
    QHash<QString, quint64> m_fileListDigest;
    // Folder currently represented in m_fileListModel. Without this, a nav
    // sequence A→B→A can short-circuit on A's cached digest while the model
    // still contains B's rows.
    QString                 m_loadedFolderId;
};

} // namespace fsnext
