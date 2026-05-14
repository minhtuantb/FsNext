#pragma once

#include "core/models/FileItem.h"
#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVector>

namespace fsnext {

// Flat QAbstractListModel used by FsUploadDialog's destination-folder picker.
// Row 0 is always the "/ (Root)" sentinel; subsequent rows are the user's
// folders in DFS order, pre-indented by depth so the combo-box visually
// hints at hierarchy without needing a tree widget.
//
// Previously the dialog rebuilt this list in QML/JS on every open, iterating
// a QVariantList of 1000s of folders and doing regex-based depth counting
// per row. That blocked the main thread long enough to be visible as
// dropdown "stickiness" for power users. Moving the precompute to C++ and
// exposing it as a role-based model means:
//   • the label + id strings are built once per folder-tree change,
//   • FsSelect binds natively with textRole: "label" (no JS iteration),
//   • label changes flow through targeted dataChanged() from rebuild().
class FolderPickerModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int         count  READ count  NOTIFY countChanged)
    // Precomputed label + id parallel arrays. FsSelect (and any other JS-
    // array-based combo) can bind `model: folderPickerModel.labels` directly
    // and use `folderPickerModel.ids[idx]` on activation, avoiding the QML
    // JS iteration that used to dominate dialog-open cost for power users.
    Q_PROPERTY(QStringList labels READ labels NOTIFY labelsChanged)
    Q_PROPERTY(QStringList ids    READ ids    NOTIFY labelsChanged)

public:
    enum Roles {
        LabelRole    = Qt::UserRole + 1,  // user-visible, indented label
        FolderIdRole,                     // linkcode (or "/" for root)
    };
    Q_ENUM(Roles)

    explicit FolderPickerModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_rows.size(); }

    // Parallel arrays mirrored from m_rows. Rebuilt on every rebuild().
    QStringList labels() const { return m_labels; }
    QStringList ids()    const { return m_ids; }

    // Rebuild the picker list from the current folder tree. Short-circuits
    // if the digest of (id|label) across all rows matches the last rebuild —
    // this is the common case when folderTreeLoaded re-fires with identical
    // data (see FolderTreeModel for the matching digest pattern).
    void rebuild(const QVector<FileItem> &folders);

    // O(1) folder id lookup by row. Returns "/" for out-of-range rows so
    // callers can pass the result straight to upload APIs without a null
    // check.
    Q_INVOKABLE QString idAt(int row) const;

    // Reverse lookup — returns the row that represents `folderId`, or 0
    // (the root row) when the id isn't present. Used by the dialog to
    // restore the previously-selected folder after a rebuild.
    Q_INVOKABLE int indexOfId(const QString &folderId) const;

signals:
    void countChanged();
    void labelsChanged();

private:
    struct Row {
        QString label;  // indented display text
        QString id;     // linkcode or "/"
    };

    QVector<Row>        m_rows;
    QStringList         m_labels;
    QStringList         m_ids;
    QHash<QString, int> m_indexById;
    quint64             m_lastDigest = 0;
};

} // namespace fsnext
