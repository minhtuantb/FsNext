#include "FolderTreeModel.h"

#include <QHash>

namespace fsnext {

FolderTreeModel::FolderTreeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FolderTreeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant FolderTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return {};

    const int idx = m_visible[index.row()];
    if (idx < 0 || idx >= m_all.size()) return {};
    const FileItem &f = m_all[idx];

    switch (role) {
    case LinkcodeRole:    return f.linkcode;
    case NameRole:        return f.name;
    case PathRole:        return f.path;
    case ParentIdRole:    return f.parentId;
    case DepthRole:       return m_visibleDepths.value(index.row(), 0);
    case HasChildrenRole: return m_childrenOfParent.contains(f.linkcode)
                               && !m_childrenOfParent.value(f.linkcode).isEmpty();
    default:              return {};
    }
}

QHash<int, QByteArray> FolderTreeModel::roleNames() const
{
    return {
        { LinkcodeRole,    "linkcode"    },
        { NameRole,        "name"        },
        { PathRole,        "path"        },
        { ParentIdRole,    "parentId"    },
        { DepthRole,       "depth"       },
        { HasChildrenRole, "hasChildren" },
    };
}

namespace {

// FNV-1a 64-bit over a compact fingerprint of each folder. Keeps enough
// fields that a rename or reparent flips the digest, but stays bounded in
// cost so we can afford to recompute it on every emission.
quint64 computeFoldersDigest(const QVector<FileItem> &folders)
{
    quint64 h = 1469598103934665603ULL;
    constexpr quint64 prime = 1099511628211ULL;
    auto mix = [&](const QByteArray &b) {
        for (char c : b) {
            h ^= static_cast<unsigned char>(c);
            h *= prime;
        }
        h ^= '|';
        h *= prime;
    };
    for (const FileItem &f : folders) {
        mix(f.linkcode.toUtf8());
        mix(f.name.toUtf8());
        mix(f.parentId.toUtf8());
        mix(f.path.toUtf8());
    }
    h ^= static_cast<quint64>(folders.size());
    h *= prime;
    return h;
}

} // namespace

void FolderTreeModel::resetFolders(const QVector<FileItem> &folders)
{
    // Fast path 1 — fingerprint hasn't changed. Guaranteed no visible
    // difference, so we skip the whole rebuild + modelReset.
    const quint64 digest = computeFoldersDigest(folders);
    if (digest == m_lastFoldersDigest && folders.size() == m_all.size())
        return;

    // Fast path 2 — the tree shape is identical (same set of (linkcode,
    // parentId) pairs in the same order) but some rows differ in name or
    // path (e.g. a single folder was renamed). We update in place and emit
    // targeted dataChanged per-row instead of resetting the whole model.
    // This preserves QML scroll position and keeps any per-row animations
    // attached to their QQmlDelegate instances.
    auto sameShape = [&]() -> bool {
        if (folders.size() != m_all.size()) return false;
        for (int i = 0; i < folders.size(); ++i) {
            if (folders[i].linkcode != m_all[i].linkcode) return false;
            if (folders[i].parentId != m_all[i].parentId) return false;
        }
        return true;
    }();

    if (sameShape) {
        for (int i = 0; i < folders.size(); ++i) {
            const FileItem &nf = folders[i];
            FileItem       &of = m_all[i];
            if (nf.name != of.name || nf.path != of.path) {
                of.name = nf.name;
                of.path = nf.path;
                // The visible row may not equal i because DFS reorders; find
                // the visible index that points here.
                const int visibleRow = m_visible.indexOf(i);
                if (visibleRow >= 0) {
                    emit dataChanged(index(visibleRow), index(visibleRow),
                                     { NameRole, PathRole });
                }
            }
        }
        m_lastFoldersDigest = digest;
        return;
    }

    // Slow path — genuine structural change (insert, delete, reparent).
    m_all = folders;
    m_indexByLinkcode.clear();
    m_indexByLinkcode.reserve(folders.size());
    for (int i = 0; i < m_all.size(); ++i)
        m_indexByLinkcode[m_all[i].linkcode] = i;

    m_childrenOfParent.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        const QString &pid = m_all[i].parentId;
        m_childrenOfParent[pid].append(i);
    }

    rebuildVisible();
    m_lastFoldersDigest = digest;
}

void FolderTreeModel::rebuildVisible()
{
    beginResetModel();
    m_visible.clear();
    m_visibleDepths.clear();

    // DFS walk starting from top-level folders (parentId == "" or "0").
    // Every folder is emitted — the Move/Copy dialog renders them as a
    // flat indented list (no expand/collapse).
    //
    // Defensive guards (added 2026-05-28 — FM-H3 audit):
    //   • kMaxDepth caps recursion depth so a malformed cache (folder with
    //     parentId pointing deeper than any real Fshare account ever has)
    //     can't blow the stack.
    //   • `visited` detects cycles — a server quirk or tampered cache that
    //     leaves a folder's parentId pointing at itself (or a descendant)
    //     would otherwise loop forever and stack-overflow.
    constexpr int kMaxDepth = 64;
    QSet<QString> visited;

    auto addSubtree = [&](auto &self, int idx, int depth) -> void {
        if (idx < 0 || idx >= m_all.size()) return;
        if (depth > kMaxDepth) {
            qWarning() << "[FolderTreeModel] depth cap reached at idx" << idx
                       << "linkcode" << m_all[idx].linkcode
                       << "— suspect malformed folder tree, truncating";
            return;
        }
        const QString &lc = m_all[idx].linkcode;
        if (!lc.isEmpty() && visited.contains(lc)) {
            qWarning() << "[FolderTreeModel] cycle detected at linkcode" << lc
                       << "(parentId=" << m_all[idx].parentId << ") — skipping";
            return;
        }
        if (!lc.isEmpty()) visited.insert(lc);

        m_visible.append(idx);
        m_visibleDepths.append(depth);

        auto it = m_childrenOfParent.find(lc);
        if (it == m_childrenOfParent.end()) return;
        for (int childIdx : it.value())
            self(self, childIdx, depth + 1);
    };

    auto appendRoots = [&](const QString &pid) {
        auto it = m_childrenOfParent.find(pid);
        if (it == m_childrenOfParent.end()) return;
        for (int idx : it.value())
            addSubtree(addSubtree, idx, 0);
    };
    appendRoots(QString());
    appendRoots(QStringLiteral("0"));

    endResetModel();
    emit countChanged();
}

QVector<QPair<QString, QString>> FolderTreeModel::buildBreadcrumbPath(const QString &folderId) const
{
    if (folderId.isEmpty()) return {};

    QVector<QPair<QString, QString>> path;
    QString current = folderId;
    int safety = 100;

    while (!current.isEmpty() && --safety > 0) {
        auto it = m_indexByLinkcode.find(current);
        if (it == m_indexByLinkcode.end()) break;
        const FileItem &f = m_all[it.value()];
        path.prepend({f.linkcode, f.name});
        current = f.parentId;
    }

    return path;
}

} // namespace fsnext
