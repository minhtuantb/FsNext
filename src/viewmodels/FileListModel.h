#pragma once

#include "core/models/FileItem.h"
#include <QAbstractListModel>
#include <QHash>
#include <QVariantMap>
#include <QVector>

namespace fsnext {

// QAbstractListModel backed by QVector<FileItem>.
// Exposes all FileItem fields as named roles for QML ListView/GridView.
class FileListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        LinkcodeRole,
        NameRole,
        TypeRole,
        SizeRole,
        PathRole,
        SecureRole,
        IsPublicRole,
        DirectlinkRole,
        HasPasswordRole,
        ParentIdRole,
        DownloadCountRole,
        DescriptionRole,
        CreatedRole,
        ModifiedRole,
        IsFolderRole,
        IsFileRole,
        ExtensionRole,
        FileCategoryRole,
        LocalPathRole,
        IsDownloadedRole,
    };
    Q_ENUM(Roles)

    explicit FileListModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Bulk replace — emits modelReset for the full list swap.
    void resetItems(const QVector<FileItem> &items);

    // Merge API search results into existing local results (dedup by linkcode).
    void mergeItems(const QVector<FileItem> &items);

    // Optimistic remove — returns removed count.
    int removeByLinkcodes(const QStringList &linkcodes);

    // Accessors
    int count() const { return m_items.size(); }
    const QVector<FileItem> &items() const { return m_items; }

    // Lookup a single item by linkcode (returns nullptr if not found).
    const FileItem *findByLinkcode(const QString &linkcode) const;

    // Return a single item as QVariantMap (all roles) for QML consumption.
    Q_INVOKABLE QVariantMap getItemAsVariant(const QString &linkcode) const;

    // Case-insensitive check for a folder with the given name in the current
    // list. Used by FileManagerPage's create-folder dialog to validate
    // against duplicates without having to materialise the whole list as
    // a QVariantList in QML (which was O(N) in both directions for 10K
    // folders). Scans m_items once with no allocations.
    Q_INVOKABLE bool hasFolderNamed(const QString &name) const;

    // Local file path tracking — populated from local_files DB table.
    void setLocalPaths(const QHash<QString, QString> &paths);
    void updateLocalPath(const QString &linkcode, const QString &localPath);

signals:
    void countChanged();

private:
    // Rebuild m_linkcodeIndex from m_items (full O(N) scan). Called after
    // any operation that could have invalidated stable row indices — reset,
    // merge, remove. Per-row updates (updateLocalPath) instead use the
    // existing index directly for O(1) lookup.
    void rebuildLinkcodeIndex();

    QVector<FileItem>       m_items;
    QHash<QString, QString> m_localPaths;     // linkcode → local file path
    // linkcode → row index. Kept in sync with m_items; lets updateLocalPath
    // and any future single-item lookups run in O(1) instead of O(N), which
    // matters for 10K-file folders where linear scans accumulate into jank.
    QHash<QString, int>     m_linkcodeIndex;
};

} // namespace fsnext
