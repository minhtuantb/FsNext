#include "TransferListModel.h"

namespace fsnext {

TransferListModel::TransferListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TransferListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_tasks.size();
}

QVariant TransferListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size())
        return {};

    const TransferTask &task = m_tasks.at(index.row());

    switch (static_cast<Roles>(role)) {
    case FileNameRole:         return task.fileName;
    case FileSizeRole:         return static_cast<qint64>(task.fileSize);
    case ProgressRole:         return task.progress;
    case SpeedRole:            return task.speed;
    case EtaRole:              return task.eta;
    case StatusRole:           return static_cast<int>(task.state);
    case LinkCodeRole:         return task.linkcode;
    case ErrorMessageRole:     return task.errorMessage;
    case TaskIdRole:           return task.id;
    case BytesTransferredRole: return static_cast<qint64>(task.bytesTransferred);
    case LocalPathRole:
        // Uploads store the on-disk source in sourcePath; localPath is only
        // meaningful for downloads. Funnel both through the same QML role so
        // the shared transfer delegate doesn't have to branch on type.
        return task.type == TransferType::Upload ? task.sourcePath : task.localPath;
    case GroupIdRole:          return task.groupId;
    case FolderPathRole:       return task.folderPath;
    case CompletedAtRole:      return task.completedAt;
    case FolderIdRole:         return task.folderId;
    }
    return {};
}

QHash<int, QByteArray> TransferListModel::roleNames() const
{
    return {
        { FileNameRole,         "fileName"         },
        { FileSizeRole,         "fileSize"         },
        { ProgressRole,         "progress"         },
        { SpeedRole,            "speed"            },
        { EtaRole,              "eta"              },
        { StatusRole,           "status"           },
        { LinkCodeRole,         "linkCode"         },
        { ErrorMessageRole,     "errorMessage"     },
        { TaskIdRole,           "taskId"           },
        { BytesTransferredRole, "bytesTransferred" },
        { LocalPathRole,        "localPath"        },
        { GroupIdRole,          "groupId"          },
        { FolderPathRole,       "folderPath"       },
        { CompletedAtRole,      "completedAt"      },
        { FolderIdRole,         "folderId"         },
    };
}

void TransferListModel::addTask(const TransferTask &task)
{
    const int row = m_tasks.size();
    beginInsertRows(QModelIndex(), row, row);
    m_tasks.append(task);
    endInsertRows();
    emit countChanged();
}

// Insert at row 0 — used by the history models so newly-archived items
// appear on top (matches "Recent Activity" convention).
void TransferListModel::prependTask(const TransferTask &task)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_tasks.prepend(task);
    endInsertRows();
    emit countChanged();
}

// Bulk-append at the end with a single beginInsertRows range. Callers
// (history pager) rely on this being a single QAbstractItemModel notify
// cycle so the attached ListView re-positions once, not N times.
void TransferListModel::appendTasks(const QVector<TransferTask> &tasks)
{
    if (tasks.isEmpty()) return;
    const int first = m_tasks.size();
    const int last  = first + tasks.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    m_tasks.reserve(m_tasks.size() + tasks.size());
    for (const TransferTask &t : tasks) m_tasks.append(t);
    endInsertRows();
    emit countChanged();
}

void TransferListModel::removeTask(const QString &id)
{
    const int row = indexOfTask(id);
    if (row < 0) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_tasks.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void TransferListModel::clear()
{
    if (m_tasks.isEmpty()) return;
    beginResetModel();
    m_tasks.clear();
    endResetModel();
    emit countChanged();
}

// Merge-style: update only progress fields, preserve fileName/fileSize/linkcode/etc.
void TransferListModel::updateProgress(const QString &id, qint64 bytesTransferred, qint64 totalBytes,
                                       double speed, const QString &eta)
{
    const int row = indexOfTask(id);
    if (row < 0) return;

    TransferTask &task = m_tasks[row];
    task.bytesTransferred = bytesTransferred;
    if (totalBytes > 0) task.fileSize = totalBytes;
    task.speed = speed;
    task.eta = eta;
    task.progress = task.progressPercent();

    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {ProgressRole, SpeedRole, EtaRole, FileSizeRole, BytesTransferredRole});
}

// Merge-style: update state only, preserve all other fields
void TransferListModel::updateState(const QString &id, TransferState state, const QString &errorMessage)
{
    const int row = indexOfTask(id);
    if (row < 0) return;

    TransferTask &task = m_tasks[row];
    task.state = state;
    if (!errorMessage.isEmpty()) task.errorMessage = errorMessage;

    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {StatusRole, ErrorMessageRole});
}

// Mark Complete in place: stamps progress=100, state=Complete, records
// completedAt for the sweep timer, and stores the final linkcode (critical
// for uploads — the share code is only known at this point).
void TransferListModel::markCompleted(const QString &id, const QString &linkcode, qint64 timestampMs)
{
    const int row = indexOfTask(id);
    if (row < 0) return;

    TransferTask &task = m_tasks[row];
    task.state           = TransferState::Complete;
    task.progress        = 100.0;
    task.bytesTransferred = task.fileSize;   // cosmetic alignment for the progress bar
    task.speed           = 0.0;
    task.eta.clear();
    task.completedAt     = timestampMs;
    if (!linkcode.isEmpty())
        task.linkcode = linkcode;

    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {StatusRole, ProgressRole, SpeedRole, EtaRole,
                                BytesTransferredRole, LinkCodeRole, CompletedAtRole});
}

// Full replace — use when you have the complete new task data
void TransferListModel::replaceTask(const QString &id, const TransferTask &task)
{
    const int row = indexOfTask(id);
    if (row < 0) return;
    m_tasks[row] = task;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

int TransferListModel::indexOfTask(const QString &id) const
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks.at(i).id == id) return i;
    }
    return -1;
}

} // namespace fsnext
