#include "FileListModel.h"
#include "core/util/FileTypeHelper.h"

#include <QSet>

namespace fsnext {

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const FileItem &f = m_items[index.row()];

    switch (role) {
    case IdRole:            return static_cast<qint64>(f.id);
    case LinkcodeRole:      return f.linkcode;
    case NameRole:          return f.name;
    case TypeRole:          return f.type;
    case SizeRole:          return static_cast<qint64>(f.size);
    case PathRole:          return f.path;
    case SecureRole:        return f.secure;
    case IsPublicRole:      return f.isPublic;
    case DirectlinkRole:    return f.directlink;
    case HasPasswordRole:   return f.hasPassword;
    case ParentIdRole:      return f.parentId;
    case DownloadCountRole: return f.downloadCount;
    case DescriptionRole:   return f.description;
    case CreatedRole:       return f.created;
    case ModifiedRole:      return f.modified;
    case IsFolderRole:      return f.isFolder();
    case IsFileRole:        return f.isFile();
    case ExtensionRole:     return FileTypeHelper::extension(f.name);
    case FileCategoryRole:  return FileTypeHelper::category(FileTypeHelper::extension(f.name));
    case LocalPathRole:     return m_localPaths.value(f.linkcode);
    case IsDownloadedRole:  return m_localPaths.contains(f.linkcode);
    default:                return {};
    }
}

QHash<int, QByteArray> FileListModel::roleNames() const
{
    return {
        { IdRole,            "fileId"        },
        { LinkcodeRole,      "linkcode"      },
        { NameRole,          "name"          },
        { TypeRole,          "type"          },
        { SizeRole,          "size"          },
        { PathRole,          "path"          },
        { SecureRole,        "secure"        },
        { IsPublicRole,      "isPublic"      },
        { DirectlinkRole,    "directlink"    },
        { HasPasswordRole,   "hasPassword"   },
        { ParentIdRole,      "parentId"      },
        { DownloadCountRole, "downloadCount" },
        { DescriptionRole,   "description"   },
        { CreatedRole,       "created"       },
        { ModifiedRole,      "modified"      },
        { IsFolderRole,      "isFolder"      },
        { IsFileRole,        "isFile"        },
        { ExtensionRole,     "extension"     },
        { FileCategoryRole,  "fileCategory"  },
        { LocalPathRole,     "localPath"     },
        { IsDownloadedRole,  "isDownloaded"  },
    };
}

void FileListModel::rebuildLinkcodeIndex()
{
    m_linkcodeIndex.clear();
    m_linkcodeIndex.reserve(m_items.size());
    for (int i = 0; i < m_items.size(); ++i)
        m_linkcodeIndex.insert(m_items[i].linkcode, i);
}

void FileListModel::resetItems(const QVector<FileItem> &items)
{
    beginResetModel();
    m_items = items;
    m_localPaths.clear();
    rebuildLinkcodeIndex();
    endResetModel();
    emit countChanged();
}

void FileListModel::mergeItems(const QVector<FileItem> &items)
{
    if (items.isEmpty()) return;

    // Use the existing linkcode index for O(1) dedup — no secondary QSet scan.
    QVector<FileItem> toAdd;
    toAdd.reserve(items.size());
    for (const FileItem &f : items) {
        if (!m_linkcodeIndex.contains(f.linkcode))
            toAdd.append(f);
    }

    if (toAdd.isEmpty()) return;

    const int first = m_items.size();
    const int last  = first + toAdd.size() - 1;
    beginInsertRows({}, first, last);
    m_items.append(toAdd);
    // Extend the index incrementally — appending never shifts existing rows,
    // so we don't need to rebuild from scratch.
    for (int i = 0; i < toAdd.size(); ++i)
        m_linkcodeIndex.insert(toAdd[i].linkcode, first + i);
    endInsertRows();
    emit countChanged();
}

int FileListModel::removeByLinkcodes(const QStringList &linkcodes)
{
    if (linkcodes.isEmpty() || m_items.isEmpty()) return 0;

    const QSet<QString> removing(linkcodes.cbegin(), linkcodes.cend());

    // Pass 1: collect all row indices to drop (ascending order).
    QVector<int> rows;
    rows.reserve(linkcodes.size());
    for (int i = 0; i < m_items.size(); ++i) {
        if (removing.contains(m_items[i].linkcode))
            rows.append(i);
    }
    if (rows.isEmpty()) return 0;

    // Pass 2: collapse consecutive indices into [first, last] ranges so we
    // can emit one beginRemoveRows per contiguous block instead of one per
    // row. For 10K-file folders with bulk deletes, this cuts view refresh
    // cost from O(N) signals to O(ranges).
    struct Range { int first; int last; };
    QVector<Range> ranges;
    ranges.reserve(rows.size());
    {
        int s = rows.first();
        int e = s;
        for (int k = 1; k < rows.size(); ++k) {
            if (rows[k] == e + 1) {
                e = rows[k];
            } else {
                ranges.append({s, e});
                s = e = rows[k];
            }
        }
        ranges.append({s, e});
    }

    // Pass 3: remove ranges in reverse order so earlier indices stay stable.
    int removed = 0;
    for (int r = ranges.size() - 1; r >= 0; --r) {
        const Range &rg = ranges[r];
        const int count = rg.last - rg.first + 1;
        beginRemoveRows({}, rg.first, rg.last);
        m_items.remove(rg.first, count);
        endRemoveRows();
        removed += count;
    }

    // Indices shifted for every row after the first removal — a full rebuild
    // is simpler and still O(N), same as collecting the removals was.
    rebuildLinkcodeIndex();
    emit countChanged();
    return removed;
}

const FileItem *FileListModel::findByLinkcode(const QString &linkcode) const
{
    const int row = m_linkcodeIndex.value(linkcode, -1);
    if (row < 0 || row >= m_items.size()) return nullptr;
    return &m_items[row];
}

bool FileListModel::hasFolderNamed(const QString &name) const
{
    if (name.isEmpty()) return false;
    for (const FileItem &f : m_items) {
        if (f.isFolder() && f.name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QVariantMap FileListModel::getItemAsVariant(const QString &linkcode) const
{
    const FileItem *f = findByLinkcode(linkcode);
    if (!f) return {};

    const QString ext = FileTypeHelper::extension(f->name);
    return {
        { QStringLiteral("linkcode"),      f->linkcode },
        { QStringLiteral("name"),          f->name },
        { QStringLiteral("isFolder"),      f->isFolder() },
        { QStringLiteral("size"),          static_cast<qint64>(f->size) },
        { QStringLiteral("extension"),     ext },
        { QStringLiteral("fileCategory"),  FileTypeHelper::category(ext) },
        { QStringLiteral("secure"),        f->secure },
        { QStringLiteral("hasPassword"),   f->hasPassword },
        { QStringLiteral("directlink"),    f->directlink },
        { QStringLiteral("downloadCount"), f->downloadCount },
        { QStringLiteral("created"),       f->created },
        { QStringLiteral("modified"),      f->modified },
        { QStringLiteral("isDownloaded"),  m_localPaths.contains(f->linkcode) },
        { QStringLiteral("localPath"),     m_localPaths.value(f->linkcode) },
    };
}

void FileListModel::setLocalPaths(const QHash<QString, QString> &paths)
{
    m_localPaths = paths;
    if (!m_items.isEmpty())
        emit dataChanged(index(0), index(m_items.size() - 1),
                         { LocalPathRole, IsDownloadedRole });
}

void FileListModel::updateLocalPath(const QString &linkcode, const QString &localPath)
{
    if (localPath.isEmpty()) {
        m_localPaths.remove(linkcode);
    } else {
        m_localPaths.insert(linkcode, localPath);
    }
    // O(1) row lookup via the linkcode index. Called after every download
    // completion, so the previous linear scan was directly hot on the UI
    // thread for 10K-file folders.
    const int row = m_linkcodeIndex.value(linkcode, -1);
    if (row >= 0 && row < m_items.size())
        emit dataChanged(index(row), index(row), { LocalPathRole, IsDownloadedRole });
}

} // namespace fsnext
