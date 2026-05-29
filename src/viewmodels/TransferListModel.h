#pragma once

#include "core/models/TransferTask.h"
#include "core/models/TransferState.h"
#include <QAbstractListModel>
#include <QVector>

namespace fsnext {

class TransferListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    int count() const { return m_tasks.size(); }

signals:
    void countChanged();

public:
    enum Roles {
        FileNameRole         = Qt::UserRole + 1,
        FileSizeRole         = Qt::UserRole + 2,
        ProgressRole         = Qt::UserRole + 3,
        SpeedRole            = Qt::UserRole + 4,
        EtaRole              = Qt::UserRole + 5,
        StatusRole           = Qt::UserRole + 6,
        LinkCodeRole         = Qt::UserRole + 7,
        ErrorMessageRole     = Qt::UserRole + 8,
        TaskIdRole           = Qt::UserRole + 9,   // UUID — use for pause/resume/cancel
        BytesTransferredRole = Qt::UserRole + 10,  // raw bytes (not derived from progress %)
        // Download: save destination. Upload: source file (so QML can always
        // read `model.localPath` as "the file on this machine" regardless of
        // transfer direction).
        LocalPathRole        = Qt::UserRole + 11,
        GroupIdRole          = Qt::UserRole + 12,  // UUID shared by all files from one folder download
        FolderPathRole       = Qt::UserRole + 13,  // display path, e.g. "My Photos/2024"
        CompletedAtRole      = Qt::UserRole + 14,  // ms-since-epoch; 0 if not yet completed
        FolderIdRole         = Qt::UserRole + 15,  // Upload: destination folder id on Fshare
    };
    Q_ENUM(Roles)

    explicit TransferListModel(QObject *parent = nullptr);
    ~TransferListModel() override = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Mutation API — full replace (used when adding new task)
    void addTask(const TransferTask &task);
    // Insert at the top (row 0). Used by history models that want newest-first order.
    void prependTask(const TransferTask &task);
    // Bulk append at the tail (preserves input order). Used by the history
    // pager: a single beginInsertRows range covers the whole page, so QML
    // re-layouts the list view once instead of once per row.
    void appendTasks(const QVector<TransferTask> &tasks);
    void removeTask(const QString &id);
    void clear();

    // Merge-style updates (preserve existing fields, only update what's specified)
    void updateProgress(const QString &id, qint64 bytesTransferred, qint64 totalBytes,
                        double speed, const QString &eta);
    void updateState(const QString &id, TransferState state, const QString &errorMessage = {});

    // Mark an existing active task as Complete without removing it. Stamps
    // completedAt with `timestampMs` and optionally records the final
    // `linkcode` (upload flow: the share code only becomes available at
    // completion). Used by the soft-archive lifecycle — completed items
    // linger in the active list briefly before migrating to history.
    void markCompleted(const QString &id, const QString &linkcode, qint64 timestampMs);

    // Replace whole task (use carefully — loses fields not set in new task)
    void replaceTask(const QString &id, const TransferTask &task);

    // Read-only access — the view-models need to iterate the active list to
    // decide which completed items are ripe for archival.
    const QVector<TransferTask> &tasks() const { return m_tasks; }

    // QML-callable row lookup by task id. Returns -1 if the task isn't in
    // this model.  Used by Pages' focusTask(id) to drive
    // ListView.positionViewAtIndex when the HUD asks for scroll-to.
    Q_INVOKABLE int rowOfTask(const QString &id) const { return indexOfTask(id); }

private:
    int indexOfTask(const QString &id) const;

    QVector<TransferTask> m_tasks;
};

} // namespace fsnext
