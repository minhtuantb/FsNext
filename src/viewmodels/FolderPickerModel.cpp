#include "FolderPickerModel.h"

#include <QCoreApplication>

namespace fsnext {

FolderPickerModel::FolderPickerModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Start with just the root option so the dialog is functional before the
    // folder tree has loaded.
    const QString rootLabel = tr("/ (Thư mục gốc)");
    const QString rootId    = QStringLiteral("/");
    m_rows.append({ rootLabel, rootId });
    m_labels.append(rootLabel);
    m_ids.append(rootId);
    m_indexById.insert(rootId, 0);
}

int FolderPickerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant FolderPickerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    switch (role) {
    case LabelRole:    return r.label;
    case FolderIdRole: return r.id;
    case Qt::DisplayRole: return r.label; // default-bound comboboxes
    }
    return {};
}

QHash<int, QByteArray> FolderPickerModel::roleNames() const
{
    return {
        { LabelRole,    "label"    },
        { FolderIdRole, "folderId" },
    };
}

namespace {

// FNV-1a 64-bit over the concatenated (linkcode|path|name) of each folder.
// Any rename, reparent, add, or removal flips the digest — which is exactly
// when the picker labels need to be rebuilt.
quint64 digestOf(const QVector<FileItem> &folders)
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
        if (f.isFile()) continue; // only folders matter for the picker
        mix(f.linkcode.toUtf8());
        mix(f.path.toUtf8());
        mix(f.name.toUtf8());
    }
    h ^= static_cast<quint64>(folders.size());
    h *= prime;
    return h;
}

} // namespace

void FolderPickerModel::rebuild(const QVector<FileItem> &folders)
{
    const quint64 digest = digestOf(folders);
    if (digest == m_lastDigest && !m_rows.isEmpty()) return;
    m_lastDigest = digest;

    // Precompute the new row list before touching the model so view consumers
    // only ever see a consistent state.
    QVector<Row> next;
    next.reserve(folders.size() + 1);
    next.append({ tr("/ (Thư mục gốc)"), QStringLiteral("/") });

    // Reusable indent buffer — avoids constructing a fresh QString for every
    // indent depth on every row. Depth is bounded to a sane ceiling so a
    // pathological tree can't balloon memory.
    constexpr int kMaxIndent = 32;
    static const QString kIndentUnit = QStringLiteral("    ");
    QString indentBuf;
    indentBuf.reserve(kIndentUnit.size() * kMaxIndent);

    for (const FileItem &f : folders) {
        if (f.isFile()) continue;
        const QString &path = !f.path.isEmpty() ? f.path : f.name;
        if (path.isEmpty()) continue;

        // Depth = number of '/' separators minus one (leading '/'). Counting
        // characters once with a manual loop is materially faster than the
        // regex that QML had to run.
        int depth = 0;
        for (const QChar c : path)
            if (c == QLatin1Char('/')) ++depth;
        depth = std::max(0, depth - 1);
        if (depth > kMaxIndent) depth = kMaxIndent;

        indentBuf.clear();
        for (int d = 0; d < depth; ++d) indentBuf.append(kIndentUnit);

        Row r;
        r.label = indentBuf + (f.name.isEmpty() ? path : f.name);
        r.id    = !f.linkcode.isEmpty() ? f.linkcode : path;
        next.append(std::move(r));
    }

    // Diff against current rows. If the labels/ids line up 1:1 we can emit
    // targeted dataChanged rather than a full modelReset. Otherwise reset.
    bool sameShape = (next.size() == m_rows.size());
    if (sameShape) {
        for (int i = 0; i < next.size(); ++i) {
            if (next[i].id != m_rows[i].id) { sameShape = false; break; }
        }
    }

    if (sameShape) {
        bool anyChanged = false;
        for (int i = 0; i < next.size(); ++i) {
            if (next[i].label != m_rows[i].label) {
                m_rows[i] = next[i];
                m_labels[i] = next[i].label;
                emit dataChanged(index(i), index(i), { LabelRole });
                anyChanged = true;
            }
        }
        if (anyChanged) emit labelsChanged();
        return;
    }

    const int oldSize = m_rows.size();
    beginResetModel();
    m_rows.swap(next);
    m_labels.clear();
    m_ids.clear();
    m_labels.reserve(m_rows.size());
    m_ids.reserve(m_rows.size());
    m_indexById.clear();
    m_indexById.reserve(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        m_labels.append(m_rows[i].label);
        m_ids.append(m_rows[i].id);
        m_indexById.insert(m_rows[i].id, i);
    }
    endResetModel();

    emit labelsChanged();
    if (oldSize != m_rows.size()) emit countChanged();
}

QString FolderPickerModel::idAt(int row) const
{
    if (row < 0 || row >= m_rows.size()) return QStringLiteral("/");
    return m_rows[row].id;
}

int FolderPickerModel::indexOfId(const QString &folderId) const
{
    return m_indexById.value(folderId, 0);
}

} // namespace fsnext
