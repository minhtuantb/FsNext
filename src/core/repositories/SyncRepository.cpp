#include "SyncRepository.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QStringList>

namespace fsnext {

SyncRepository::SyncRepository() = default;

void SyncRepository::setUserId(const QString &userId) { m_userId = userId; }

QString SyncRepository::groupPrefix() const
{
    return QStringLiteral("SyncByUser/") + m_userId;
}

// Hash the relPath into a QSettings-safe key so path separators / weird chars
// don't fight the QSettings group parser.
static QString hashRelPath(const QString &rel)
{
    const QByteArray h = QCryptographicHash::hash(rel.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(h.toHex().left(16));
}

// ─────────────────────────────────────────────────────────────────────────────
// Folders
// ─────────────────────────────────────────────────────────────────────────────

QVector<SyncFolder> SyncRepository::loadFolders() const
{
    QVector<SyncFolder> out;
    if (m_userId.isEmpty()) return out;

    const QString base = groupPrefix();
    const QStringList ids = m_settings.value(base + QStringLiteral("/folders")).toStringList();
    out.reserve(ids.size());
    for (const QString &id : ids) {
        const QString k = base + QStringLiteral("/folder/") + id;
        SyncFolder f;
        f.id                = id;
        f.localPath         = m_settings.value(k + QStringLiteral("/localPath")).toString();
        f.fshareFolderName  = m_settings.value(k + QStringLiteral("/fshareFolderName")).toString();
        f.enabled           = m_settings.value(k + QStringLiteral("/enabled"), true).toBool();
        f.deleteAfterUpload = m_settings.value(k + QStringLiteral("/deleteAfterUpload"), false).toBool();
        const qint64 created  = m_settings.value(k + QStringLiteral("/createdAt"), 0).toLongLong();
        const qint64 lastScan = m_settings.value(k + QStringLiteral("/lastScanAt"), 0).toLongLong();
        if (created  > 0) f.createdAt  = QDateTime::fromSecsSinceEpoch(created);
        if (lastScan > 0) f.lastScanAt = QDateTime::fromSecsSinceEpoch(lastScan);
        // ── Per-folder settings (v6.0+) ─────────────────────────────────────
        // Missing keys = pre-v6 folder: read back the v5 hard-coded defaults
        // (recursive walk, no user ignore patterns, 5 MiB/s throttle) so
        // upgrade is invisible until the user actively edits Settings.
        f.watchSubfolders   = m_settings.value(k + QStringLiteral("/watchSubfolders"), true).toBool();
        f.ignorePatterns    = m_settings.value(k + QStringLiteral("/ignorePatterns"), QString{}).toString();
        f.speedLimitBps     = m_settings.value(k + QStringLiteral("/speedLimitBps"),
                                                static_cast<qint64>(5LL * 1024 * 1024)).toLongLong();
        if (!f.localPath.isEmpty()) out.append(f);
    }
    return out;
}

void SyncRepository::saveFolder(const SyncFolder &folder)
{
    if (m_userId.isEmpty() || folder.id.isEmpty()) return;

    const QString base = groupPrefix();
    QStringList ids = m_settings.value(base + QStringLiteral("/folders")).toStringList();
    if (!ids.contains(folder.id)) ids.append(folder.id);
    m_settings.setValue(base + QStringLiteral("/folders"), ids);

    const QString k = base + QStringLiteral("/folder/") + folder.id;
    m_settings.setValue(k + QStringLiteral("/localPath"),         folder.localPath);
    m_settings.setValue(k + QStringLiteral("/fshareFolderName"),  folder.fshareFolderName);
    m_settings.setValue(k + QStringLiteral("/enabled"),           folder.enabled);
    m_settings.setValue(k + QStringLiteral("/deleteAfterUpload"), folder.deleteAfterUpload);
    if (folder.createdAt.isValid())
        m_settings.setValue(k + QStringLiteral("/createdAt"),  folder.createdAt.toSecsSinceEpoch());
    if (folder.lastScanAt.isValid())
        m_settings.setValue(k + QStringLiteral("/lastScanAt"), folder.lastScanAt.toSecsSinceEpoch());
    // ── Per-folder settings (v6.0+) ──────────────────────────────────────────
    // Always write — even unchanged from default — so the keys exist on disk
    // and a future loader doesn't silently fall back to the hardcoded default
    // when the user explicitly chose that value.
    m_settings.setValue(k + QStringLiteral("/watchSubfolders"),   folder.watchSubfolders);
    m_settings.setValue(k + QStringLiteral("/ignorePatterns"),    folder.ignorePatterns);
    m_settings.setValue(k + QStringLiteral("/speedLimitBps"),
                        static_cast<qint64>(folder.speedLimitBps));
    m_settings.sync();
}

bool SyncRepository::autoSyncEnabled() const
{
    if (m_userId.isEmpty()) return true;     // no user → caller treats as off anyway
    return m_settings.value(groupPrefix() + QStringLiteral("/autoSyncEnabled"),
                             true).toBool();
}

void SyncRepository::setAutoSyncEnabled(bool enabled)
{
    if (m_userId.isEmpty()) return;
    m_settings.setValue(groupPrefix() + QStringLiteral("/autoSyncEnabled"), enabled);
    m_settings.sync();
}

void SyncRepository::deleteFolder(const QString &folderId)
{
    if (m_userId.isEmpty() || folderId.isEmpty()) return;

    const QString base = groupPrefix();
    QStringList ids = m_settings.value(base + QStringLiteral("/folders")).toStringList();
    ids.removeAll(folderId);
    m_settings.setValue(base + QStringLiteral("/folders"), ids);

    m_settings.remove(base + QStringLiteral("/folder/") + folderId);
    m_settings.sync();
}

// ─────────────────────────────────────────────────────────────────────────────
// Files
// ─────────────────────────────────────────────────────────────────────────────

QVector<SyncFileEntry> SyncRepository::loadFiles(const QString &folderId) const
{
    QVector<SyncFileEntry> out;
    if (m_userId.isEmpty() || folderId.isEmpty()) return out;

    const QString base = groupPrefix() + QStringLiteral("/folder/") + folderId;
    const QStringList rels = m_settings.value(base + QStringLiteral("/files")).toStringList();
    out.reserve(rels.size());
    for (const QString &rel : rels) {
        const QString k = base + QStringLiteral("/file/") + hashRelPath(rel);
        SyncFileEntry e;
        e.folderId = folderId;
        e.relPath  = rel;
        e.size     = m_settings.value(k + QStringLiteral("/size"), 0).toLongLong();
        e.mtime    = m_settings.value(k + QStringLiteral("/mtime"), 0).toLongLong();
        e.linkcode = m_settings.value(k + QStringLiteral("/linkcode")).toString();
        const qint64 up = m_settings.value(k + QStringLiteral("/uploadedAt"), 0).toLongLong();
        if (up > 0) e.uploadedAt = QDateTime::fromSecsSinceEpoch(up);
        e.state    = static_cast<SyncFileState>(
            m_settings.value(k + QStringLiteral("/state"),
                             static_cast<int>(SyncFileState::Pending)).toInt());
        e.errorMessage = m_settings.value(k + QStringLiteral("/error")).toString();
        out.append(e);
    }
    return out;
}

void SyncRepository::saveFile(const SyncFileEntry &e)
{
    if (m_userId.isEmpty() || e.folderId.isEmpty() || e.relPath.isEmpty()) return;

    const QString base = groupPrefix() + QStringLiteral("/folder/") + e.folderId;
    QStringList rels = m_settings.value(base + QStringLiteral("/files")).toStringList();
    if (!rels.contains(e.relPath)) rels.append(e.relPath);
    m_settings.setValue(base + QStringLiteral("/files"), rels);

    const QString k = base + QStringLiteral("/file/") + hashRelPath(e.relPath);
    m_settings.setValue(k + QStringLiteral("/size"),     e.size);
    m_settings.setValue(k + QStringLiteral("/mtime"),    e.mtime);
    m_settings.setValue(k + QStringLiteral("/linkcode"), e.linkcode);
    m_settings.setValue(k + QStringLiteral("/state"),    static_cast<int>(e.state));
    m_settings.setValue(k + QStringLiteral("/error"),    e.errorMessage);
    if (e.uploadedAt.isValid())
        m_settings.setValue(k + QStringLiteral("/uploadedAt"), e.uploadedAt.toSecsSinceEpoch());
    // Don't sync() on every file write — the scan loop batches; folder-level
    // save calls will flush. Bulk scans on slow disks would otherwise thrash.
}

void SyncRepository::deleteFile(const QString &folderId, const QString &relPath)
{
    if (m_userId.isEmpty() || folderId.isEmpty() || relPath.isEmpty()) return;

    const QString base = groupPrefix() + QStringLiteral("/folder/") + folderId;
    QStringList rels = m_settings.value(base + QStringLiteral("/files")).toStringList();
    rels.removeAll(relPath);
    m_settings.setValue(base + QStringLiteral("/files"), rels);
    m_settings.remove(base + QStringLiteral("/file/") + hashRelPath(relPath));
    m_settings.sync();
}

void SyncRepository::deleteAllFiles(const QString &folderId)
{
    if (m_userId.isEmpty() || folderId.isEmpty()) return;

    const QString base = groupPrefix() + QStringLiteral("/folder/") + folderId;
    m_settings.setValue(base + QStringLiteral("/files"), QStringList{});
    // Also clear the file/* subtree.
    m_settings.beginGroup(base + QStringLiteral("/file"));
    m_settings.remove(QStringLiteral(""));
    m_settings.endGroup();
    m_settings.sync();
}

// ─────────────────────────────────────────────────────────────────────────────
// Activity log
// ─────────────────────────────────────────────────────────────────────────────
//
// Encoding: each entry is "<unix>\t<kind>\t<folderId>\t<folderLabel>\t<relPath>\t<size>\t<message>"
// (tab-separated, message is the last field so it can contain tabs without
// breaking the parser).  We use \x1f as the entry separator instead of \n
// so messages with embedded newlines roundtrip safely.

static constexpr QChar kRowSep  = QChar(0x1f);
static constexpr QChar kFieldSep = QChar('\t');

static QString encodeActivity(const SyncActivityEntry &e)
{
    // Replace tabs in any field that could conceivably contain one (folderLabel
    // = local folder name on Windows can't contain \t, but defend anyway).
    auto sanitize = [](QString s) {
        s.replace(kFieldSep, QLatin1Char(' '));
        s.replace(kRowSep,   QLatin1Char(' '));
        return s;
    };
    // QString message can contain anything except the row separator (kRowSep)
    // which we strip on decode-time too.
    QString msg = e.message;
    msg.replace(kRowSep, QLatin1Char(' '));

    QStringList fields;
    fields.reserve(7);
    fields << QString::number(e.at.toSecsSinceEpoch())
           << QString::number(static_cast<int>(e.kind))
           << sanitize(e.folderId)
           << sanitize(e.folderLabel)
           << sanitize(e.relPath)
           << QString::number(e.sizeBytes)
           << msg;
    return fields.join(kFieldSep);
}

static bool decodeActivity(const QString &row, SyncActivityEntry &out)
{
    const QStringList parts = row.split(kFieldSep);
    if (parts.size() < 7) return false;
    bool ok = false;
    const qint64 secs = parts[0].toLongLong(&ok); if (!ok) return false;
    const int    knd  = parts[1].toInt(&ok);       if (!ok) return false;
    out.at          = QDateTime::fromSecsSinceEpoch(secs);
    out.kind        = static_cast<SyncActivityKind>(knd);
    out.folderId    = parts[2];
    out.folderLabel = parts[3];
    out.relPath     = parts[4];
    out.sizeBytes   = parts[5].toLongLong();
    // Re-join any remaining fields (just in case the message itself had
    // tabs and the sanitizer was bypassed somehow).
    out.message     = parts.mid(6).join(kFieldSep);
    return true;
}

QVector<SyncActivityEntry> SyncRepository::loadActivity() const
{
    QVector<SyncActivityEntry> out;
    if (m_userId.isEmpty()) return out;
    const QString packed = m_settings.value(groupPrefix() + QStringLiteral("/activity"))
                                       .toString();
    if (packed.isEmpty()) return out;
    const QStringList rows = packed.split(kRowSep, Qt::SkipEmptyParts);
    out.reserve(rows.size());
    for (const QString &row : rows) {
        SyncActivityEntry e;
        if (decodeActivity(row, e)) out.append(e);
    }
    return out;
}

void SyncRepository::appendActivity(const SyncActivityEntry &entry)
{
    if (m_userId.isEmpty()) return;
    QVector<SyncActivityEntry> entries = loadActivity();
    // Newest-first — prepend, then truncate.  Cap is small enough that the
    // O(n) prepend cost (~50 entries × small QString copy) is fine; switching
    // to a QQueue would buy little and complicate the encoded format.
    entries.prepend(entry);
    while (entries.size() > kActivityCap) entries.removeLast();

    QString packed;
    packed.reserve(entries.size() * 96);
    for (int i = 0; i < entries.size(); ++i) {
        if (i > 0) packed.append(kRowSep);
        packed.append(encodeActivity(entries[i]));
    }
    m_settings.setValue(groupPrefix() + QStringLiteral("/activity"), packed);
    m_settings.sync();
}

void SyncRepository::clearActivity()
{
    if (m_userId.isEmpty()) return;
    m_settings.remove(groupPrefix() + QStringLiteral("/activity"));
    m_settings.sync();
}

} // namespace fsnext
