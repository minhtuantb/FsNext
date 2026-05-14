#pragma once

#include "core/models/FileItem.h"
#include <QAbstractListModel>
#include <QHash>
#include <QVector>

namespace fsnext {

// Flat list model of folders rendered as an indented DFS walk.
// Used by the Move/Copy dialog's destination picker — every folder is always
// visible, indented by depth. (The previous sidebar "folder tree" UI has been
// removed in favour of breadcrumb-driven navigation; expand/collapse state
// was tied to that sidebar and is no longer needed.)
class FolderTreeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        LinkcodeRole = Qt::UserRole + 1,
        NameRole,
        PathRole,
        ParentIdRole,
        DepthRole,       // Nesting depth (0 = top-level)
        HasChildrenRole, // True if any other folder has parentId == linkcode
    };
    Q_ENUM(Roles)

    explicit FolderTreeModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace the entire folder list. Internally optimised: if the content
    // fingerprint (linkcode|name|parentId|path per row) is identical to the
    // last accepted payload, returns without touching anything. If only a
    // subset of row data changed while the tree shape stayed identical, emits
    // targeted dataChanged() per-row instead of a full modelReset. Only when
    // the set of folders genuinely changes (add/remove/reparent) do we fall
    // back to the full rebuild path.
    void resetFolders(const QVector<FileItem> &folders);

    int count() const { return m_visible.size(); }
    const QVector<FileItem> &items() const { return m_all; }

    // Build a hierarchy path [root, ..., folderId] from the folder tree.
    // Returns list of {linkcode, name} pairs from root down to the given folder.
    QVector<QPair<QString, QString>> buildBreadcrumbPath(const QString &folderId) const;

signals:
    void countChanged();

private:
    // Rebuild m_visible from m_all (full DFS, every folder included).
    void rebuildVisible();

    QVector<FileItem> m_all;                         // every folder in the tree
    QHash<QString, int> m_indexByLinkcode;           // linkcode → m_all index
    QHash<QString, QVector<int>> m_childrenOfParent; // parentId → m_all indices

    QVector<int>  m_visible;                         // DFS-ordered indices into m_all
    QVector<int>  m_visibleDepths;                   // depth per visible row

    // Digest of the last accepted payload. Used by resetFolders() to short-
    // circuit no-op re-emissions from FileCacheService (which fires
    // folderTreeLoaded on both cache-hit and sync-complete even when the
    // folder set is unchanged).
    quint64 m_lastFoldersDigest = 0;
};

} // namespace fsnext
