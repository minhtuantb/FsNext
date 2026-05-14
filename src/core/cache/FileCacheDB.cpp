#include "FileCacheDB.h"

#include "core/util/FormatUtil.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

namespace fsnext {

// ── helpers ──────────────────────────────────────────────────────────────────

static qint64 nowSecs()
{
    return QDateTime::currentSecsSinceEpoch();
}

// Qt's SQLite driver maps a null QString to SQL NULL. Rows in TEXT NOT NULL
// columns are stored as the empty string '', so binding a null QString at
// query time would compare against NULL and never match. Use this helper on
// every parent_id / folderId / linkcode binding that may be null (e.g. the
// root folder id, which is represented as QString()).
static inline QString nonNullStr(const QString &s)
{
    return s.isNull() ? QStringLiteral("") : s;
}

using FormatUtil::parseTimestamp;

static FileItem rowToItem(const QSqlQuery &q)
{
    FileItem f;
    f.id           = static_cast<uint64_t>(q.value(QStringLiteral("fshare_id")).toLongLong());
    f.linkcode     = q.value(QStringLiteral("linkcode")).toString();
    f.parentId     = q.value(QStringLiteral("parent_id")).toString();
    f.name         = q.value(QStringLiteral("name")).toString();
    f.type         = q.value(QStringLiteral("type")).toString();
    f.size         = q.value(QStringLiteral("size")).toLongLong();
    f.path         = q.value(QStringLiteral("path")).toString();
    f.secure       = q.value(QStringLiteral("secure")).toInt() != 0;
    f.hasPassword  = q.value(QStringLiteral("has_password")).toInt() != 0;
    f.directlink   = q.value(QStringLiteral("directlink")).toInt() != 0;
    f.isPublic     = q.value(QStringLiteral("is_public")).toInt() != 0;
    f.hashIndex    = q.value(QStringLiteral("hash_index")).toString();
    f.downloadCount = q.value(QStringLiteral("download_count")).toInt();
    f.description  = q.value(QStringLiteral("description")).toString();
    f.tIndex       = q.value(QStringLiteral("t_index")).toString();

    // Store as unix-second strings (same as API; ViewModel / QML converts for display)
    qint64 ca = q.value(QStringLiteral("created_at")).toLongLong();
    qint64 ma = q.value(QStringLiteral("modified_at")).toLongLong();
    if (ca) f.created  = QString::number(ca);
    if (ma) f.modified = QString::number(ma);

    return f;
}

// ── lifecycle ─────────────────────────────────────────────────────────────────

FileCacheDB::FileCacheDB(QObject *parent) : QObject(parent) {}

FileCacheDB::~FileCacheDB()
{
    if (m_db.isOpen()) m_db.close();
    QSqlDatabase::removeDatabase(QStringLiteral("fscache"));
}

bool FileCacheDB::open(const QString &dbPath)
{
    // ":memory:" is SQLite's in-memory database — no filesystem backing, so
    // skip the mkpath step (which would fail on ":/"). Used by unit tests.
    if (dbPath != QStringLiteral(":memory:")) {
        QDir dir = QFileInfo(dbPath).absoluteDir();
        if (!dir.mkpath(QStringLiteral("."))) {
            qWarning() << "[FileCacheDB] Cannot create directory:" << dir.absolutePath();
            return false;
        }
    }

    // Guard against double-open (e.g. unit tests or unexpected re-init).
    if (QSqlDatabase::contains(QStringLiteral("fscache"))) {
        QSqlDatabase::removeDatabase(QStringLiteral("fscache"));
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("fscache"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[FileCacheDB] Cannot open:" << m_db.lastError().text();
        return false;
    }

    return createSchema();
}

bool FileCacheDB::isOpen() const { return m_db.isOpen(); }

// ── schema ────────────────────────────────────────────────────────────────────

bool FileCacheDB::createSchema()
{
    QSqlQuery q(m_db);

    // Performance pragmas. WAL + synchronous=NORMAL is the standard "fast but
    // still crash-safe" profile. The reads we care about most (folder listing,
    // startup tree walk) benefit further from:
    //   • A larger page cache — the raw cost is bytes of RAM, and we're reading
    //     the same pages over and over while the user navigates.
    //   • Temp store in memory — sort/group operations spill to temp tables;
    //     keeping them in RAM is a drop-in latency win for ORDER BY queries.
    //   • mmap_size — lets SQLite satisfy reads directly from a shared mmap of
    //     the DB file, bypassing the page-cache copy for hot pages. On 64-bit
    //     Windows this is safe up to the OS address-space limit. 256 MB is
    //     more than any realistic user's cache DB, so every page fits.
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA cache_size=-32000"));          // ~32 MB page cache
    q.exec(QStringLiteral("PRAGMA temp_store=MEMORY"));
    q.exec(QStringLiteral("PRAGMA mmap_size=268435456"));        // 256 MB mmap window

    // Main files table
    const bool filesOk = q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS files (
            linkcode        TEXT PRIMARY KEY,
            fshare_id       INTEGER NOT NULL DEFAULT 0,
            parent_id       TEXT NOT NULL DEFAULT '',
            name            TEXT NOT NULL,
            type            TEXT NOT NULL,
            size            INTEGER NOT NULL DEFAULT 0,
            path            TEXT NOT NULL DEFAULT '',
            secure          INTEGER NOT NULL DEFAULT 0,
            has_password    INTEGER NOT NULL DEFAULT 0,
            directlink      INTEGER NOT NULL DEFAULT 0,
            is_public       INTEGER NOT NULL DEFAULT 0,
            hash_index      TEXT NOT NULL DEFAULT '',
            download_count  INTEGER NOT NULL DEFAULT 0,
            description     TEXT NOT NULL DEFAULT '',
            created_at      INTEGER NOT NULL DEFAULT 0,
            modified_at     INTEGER NOT NULL DEFAULT 0,
            t_index         TEXT NOT NULL DEFAULT '',
            cached_at       INTEGER NOT NULL DEFAULT 0,
            user_id         TEXT NOT NULL
        )
    )"));
    // Migration: add fshare_id column if upgrading from older schema.
    // PRAGMA table_info probe avoids ALTER failure on fresh databases where
    // the column already exists in the CREATE TABLE statement.
    {
        QSqlQuery info(m_db);
        info.exec(QStringLiteral("PRAGMA table_info(files)"));
        bool hasFshareId = false;
        while (info.next()) {
            if (info.value(1).toString() == QStringLiteral("fshare_id")) {
                hasFshareId = true;
                break;
            }
        }
        if (!hasFshareId)
            q.exec(QStringLiteral("ALTER TABLE files ADD COLUMN fshare_id INTEGER NOT NULL DEFAULT 0"));
    }
    if (!filesOk) {
        qWarning() << "[FileCacheDB] Table 'files' error:" << q.lastError().text();
        return false;
    }

    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_files_parent   ON files(parent_id, user_id)"));
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_files_modified ON files(modified_at DESC)"));
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_files_user     ON files(user_id)"));

    // FTS5 virtual table — probe first, mark flag, create triggers if available
    if (q.exec(QStringLiteral(R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS files_fts USING fts5(
            linkcode UNINDEXED,
            name,
            description,
            path,
            content='files',
            content_rowid='rowid'
        )
    )"))) {
        m_hasFts5 = true;

        q.exec(QStringLiteral(R"(
            CREATE TRIGGER IF NOT EXISTS fts_insert AFTER INSERT ON files BEGIN
                INSERT INTO files_fts(rowid, linkcode, name, description, path)
                VALUES (new.rowid, new.linkcode, new.name, new.description, new.path);
            END
        )"));

        q.exec(QStringLiteral(R"(
            CREATE TRIGGER IF NOT EXISTS fts_delete AFTER DELETE ON files BEGIN
                INSERT INTO files_fts(files_fts, rowid, linkcode, name, description, path)
                VALUES ('delete', old.rowid, old.linkcode, old.name, old.description, old.path);
            END
        )"));

        q.exec(QStringLiteral(R"(
            CREATE TRIGGER IF NOT EXISTS fts_update AFTER UPDATE ON files BEGIN
                INSERT INTO files_fts(files_fts, rowid, linkcode, name, description, path)
                VALUES ('delete', old.rowid, old.linkcode, old.name, old.description, old.path);
                INSERT INTO files_fts(rowid, linkcode, name, description, path)
                VALUES (new.rowid, new.linkcode, new.name, new.description, new.path);
            END
        )"));

        qDebug() << "[FileCacheDB] FTS5 full-text search enabled";
    } else {
        qWarning() << "[FileCacheDB] FTS5 unavailable, falling back to LIKE search";
    }

    // Folder sync-state table
    const bool syncOk = q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS folder_sync (
            linkcode     TEXT    NOT NULL,
            user_id      TEXT    NOT NULL,
            synced_at    INTEGER NOT NULL DEFAULT 0,
            is_complete  INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (linkcode, user_id)
        )
    )"));
    if (!syncOk) {
        qWarning() << "[FileCacheDB] Table 'folder_sync' error:" << q.lastError().text();
        return false;
    }

    // Local file tracking (download/upload → local path mapping)
    const bool localOk = q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS local_files (
            linkcode        TEXT    NOT NULL,
            user_id         TEXT    NOT NULL,
            local_path      TEXT    NOT NULL,
            file_name       TEXT    NOT NULL,
            file_size       INTEGER NOT NULL DEFAULT 0,
            transfer_type   TEXT    NOT NULL,
            completed_at    INTEGER NOT NULL DEFAULT 0,
            verified_at     INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (linkcode, user_id)
        )
    )"));
    if (!localOk) {
        qWarning() << "[FileCacheDB] Table 'local_files' error:" << q.lastError().text();
        return false;
    }
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_local_files_user ON local_files(user_id)"));
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_local_files_path ON local_files(local_path)"));

    // One-shot schema migrations. Bump kCurrentSchemaVersion whenever cache
    // semantics change in a way that makes older rows misleading.
    //   v2: clear folder_sync — earlier root-listing + page-index bugs left
    //       empty folders marked "complete", which suppressed re-fetches.
    //   v3: wipe files + folder_sync — pre-v3 syncs stored items under the
    //       API's numeric "pid" instead of the parent's linkcode, so cached
    //       children could not be queried by the linkcode the UI uses.
    //   v4: normalise type column from Fshare's "0"/"1" to "folder"/"file".
    //       UPDATE in place so existing rows start grouping folders-first in
    //       the main list without needing a full re-fetch.
    constexpr int kCurrentSchemaVersion = 4;
    int currentVersion = 0;
    {
        QSqlQuery v(m_db);
        if (v.exec(QStringLiteral("PRAGMA user_version")) && v.next())
            currentVersion = v.value(0).toInt();
    }
    if (currentVersion < kCurrentSchemaVersion) {
        QSqlQuery mig(m_db);
        if (currentVersion < 2)
            mig.exec(QStringLiteral("DELETE FROM folder_sync"));
        if (currentVersion < 3) {
            mig.exec(QStringLiteral("DELETE FROM files"));
            mig.exec(QStringLiteral("DELETE FROM folder_sync"));
        }
        if (currentVersion < 4) {
            mig.exec(QStringLiteral("UPDATE files SET type='folder' WHERE type='0'"));
            mig.exec(QStringLiteral("UPDATE files SET type='file'   WHERE type='1'"));
        }
        mig.exec(QStringLiteral("PRAGMA user_version=")
                 + QString::number(kCurrentSchemaVersion));
        qDebug() << "[FileCacheDB] Migrated cache schema"
                 << currentVersion << "→" << kCurrentSchemaVersion;
    }

    return true;
}

// ── upsert ────────────────────────────────────────────────────────────────────

void FileCacheDB::upsertFiles(const QString &userId, const QVector<FileItem> &items)
{
    if (items.isEmpty()) return;

    QSqlQuery q(m_db);
    m_db.transaction();

    q.prepare(QStringLiteral(R"(
        INSERT INTO files
            (linkcode, fshare_id, parent_id, name, type, size, path, secure, has_password,
             directlink, is_public, hash_index, download_count, description,
             created_at, modified_at, t_index, cached_at, user_id)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        ON CONFLICT(linkcode) DO UPDATE SET
            fshare_id=excluded.fshare_id,
            parent_id=excluded.parent_id, name=excluded.name, type=excluded.type,
            size=excluded.size, path=excluded.path, secure=excluded.secure,
            has_password=excluded.has_password, directlink=excluded.directlink,
            is_public=excluded.is_public, hash_index=excluded.hash_index,
            download_count=excluded.download_count, description=excluded.description,
            created_at=excluded.created_at, modified_at=excluded.modified_at,
            t_index=excluded.t_index, cached_at=excluded.cached_at,
            user_id=excluded.user_id
    )"));

    const qint64 now = nowSecs();

    for (const FileItem &f : items) {
        q.addBindValue(f.linkcode);
        q.addBindValue(static_cast<qint64>(f.id));
        q.addBindValue(nonNullStr(f.parentId));
        q.addBindValue(f.name);
        q.addBindValue(f.type);
        q.addBindValue(static_cast<qint64>(f.size));
        q.addBindValue(nonNullStr(f.path));
        q.addBindValue(f.secure       ? 1 : 0);
        q.addBindValue(f.hasPassword  ? 1 : 0);
        q.addBindValue(f.directlink   ? 1 : 0);
        q.addBindValue(f.isPublic     ? 1 : 0);
        q.addBindValue(nonNullStr(f.hashIndex));
        q.addBindValue(f.downloadCount);
        q.addBindValue(nonNullStr(f.description));
        q.addBindValue(parseTimestamp(f.created));
        q.addBindValue(parseTimestamp(f.modified));
        q.addBindValue(nonNullStr(f.tIndex));
        q.addBindValue(now);
        q.addBindValue(userId);

        if (!q.exec())
            qWarning() << "[FileCacheDB] upsert failed for" << f.linkcode << q.lastError().text();
    }

    m_db.commit();

    // Sanity count after commit — helps diagnose why queryFiles returns 0 when
    // upsert appeared to succeed.
    QSqlQuery c(m_db);
    if (c.exec(QStringLiteral("SELECT COUNT(*), user_id, parent_id FROM files GROUP BY user_id, parent_id"))) {
        while (c.next()) {
            qInfo().noquote() << "[FileCacheDB] rows user_id=" << c.value(1).toString()
                              << "parent_id='" << c.value(2).toString() << "' count=" << c.value(0).toInt();
        }
    }
}

// ── query ─────────────────────────────────────────────────────────────────────

QVector<FileItem> FileCacheDB::queryFiles(const QString &userId,
                                          const QString &parentId,
                                          const QString &sortKey,
                                          bool           sortAsc,
                                          const QString &typeFilter,
                                          int            limit,
                                          int            offset)
{
    // Build ORDER BY — folders always sort before files when key is name/type
    const QString dir = sortAsc ? QStringLiteral("ASC") : QStringLiteral("DESC");
    QString orderExpr;
    if (sortKey == QStringLiteral("date")) {
        orderExpr = QStringLiteral("modified_at ") + dir;
    } else if (sortKey == QStringLiteral("size")) {
        orderExpr = QStringLiteral("(CASE type WHEN 'folder' THEN 0 ELSE 1 END) ASC, size ") + dir;
    } else if (sortKey == QStringLiteral("type")) {
        orderExpr = QStringLiteral("type ") + dir + QStringLiteral(", name ASC");
    } else { // "name" (default)
        orderExpr = QStringLiteral("(CASE type WHEN 'folder' THEN 0 ELSE 1 END) ASC, name COLLATE NOCASE ") + dir;
    }

    // Type filter clause
    QString typeClause;
    if (typeFilter == QStringLiteral("file"))   typeClause = QStringLiteral(" AND type='file'");
    else if (typeFilter == QStringLiteral("folder")) typeClause = QStringLiteral(" AND type='folder'");

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM files WHERE user_id=? AND parent_id=?%1 ORDER BY %2 LIMIT ? OFFSET ?")
              .arg(typeClause, orderExpr));
    q.addBindValue(userId);
    q.addBindValue(nonNullStr(parentId));
    q.addBindValue(limit);
    q.addBindValue(offset);

    QVector<FileItem> result;
    if (!q.exec()) {
        qWarning() << "[FileCacheDB] queryFiles error:" << q.lastError().text();
        return result;
    }
    while (q.next()) result.append(rowToItem(q));
    return result;
}

int FileCacheDB::countFiles(const QString &userId,
                            const QString &parentId,
                            const QString &typeFilter)
{
    QString typeClause;
    if (typeFilter == QStringLiteral("file"))   typeClause = QStringLiteral(" AND type='file'");
    else if (typeFilter == QStringLiteral("folder")) typeClause = QStringLiteral(" AND type='folder'");

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM files WHERE user_id=? AND parent_id=?%1").arg(typeClause));
    q.addBindValue(userId);
    q.addBindValue(nonNullStr(parentId));
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

// ── full-text search ─────────────────────────────────────────────────────────

QVector<FileItem> FileCacheDB::searchFts(const QString &userId,
                                         const QString &keyword,
                                         int            limit)
{
    QSqlQuery q(m_db);
    QVector<FileItem> result;

    if (m_hasFts5) {
        // Prefix search — append * for partial word matching
        const QString term = keyword.trimmed() + QStringLiteral("*");
        q.prepare(QStringLiteral(R"(
            SELECT f.* FROM files f
            JOIN files_fts fts ON fts.linkcode = f.linkcode
            WHERE fts.files_fts MATCH ? AND f.user_id = ?
            ORDER BY rank
            LIMIT ?
        )"));
        q.addBindValue(term);
        q.addBindValue(userId);
        q.addBindValue(limit);

        if (q.exec()) {
            while (q.next()) result.append(rowToItem(q));
            return result;
        }
        qWarning() << "[FileCacheDB] FTS5 search failed, falling back:" << q.lastError().text();
    }

    // LIKE fallback
    const QString pattern = QStringLiteral("%") + keyword + QStringLiteral("%");
    q.prepare(QStringLiteral(R"(
        SELECT * FROM files
        WHERE user_id=? AND (name LIKE ? OR description LIKE ?)
        ORDER BY modified_at DESC
        LIMIT ?
    )"));
    q.addBindValue(userId);
    q.addBindValue(pattern);
    q.addBindValue(pattern);
    q.addBindValue(limit);

    if (!q.exec()) {
        qWarning() << "[FileCacheDB] LIKE search error:" << q.lastError().text();
        return result;
    }
    while (q.next()) result.append(rowToItem(q));
    return result;
}

// ── folder tree ───────────────────────────────────────────────────────────────

QVector<FileItem> FileCacheDB::getFolderTree(const QString &userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT * FROM files WHERE user_id=? AND type='folder' ORDER BY path ASC, name ASC"));
    q.addBindValue(userId);

    QVector<FileItem> result;
    if (!q.exec()) {
        qWarning() << "[FileCacheDB] getFolderTree error:" << q.lastError().text();
        return result;
    }
    while (q.next()) result.append(rowToItem(q));
    return result;
}

// ── sync state ────────────────────────────────────────────────────────────────

FolderSyncState FileCacheDB::getSyncState(const QString &userId, const QString &folderId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT synced_at, is_complete FROM folder_sync WHERE linkcode=? AND user_id=?"));
    q.addBindValue(nonNullStr(folderId));
    q.addBindValue(userId);

    FolderSyncState state;
    if (q.exec() && q.next()) {
        state.exists     = true;
        state.syncedAt   = q.value(0).toLongLong();
        state.isComplete = q.value(1).toInt() != 0;
    }
    return state;
}

void FileCacheDB::setFolderSyncComplete(const QString &userId, const QString &folderId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO folder_sync (linkcode, user_id, synced_at, is_complete) VALUES (?,?,?,1)
        ON CONFLICT(linkcode, user_id) DO UPDATE SET synced_at=excluded.synced_at, is_complete=1
    )"));
    q.addBindValue(nonNullStr(folderId));
    q.addBindValue(userId);
    q.addBindValue(nowSecs());
    q.exec();
}

void FileCacheDB::setFolderSyncPartial(const QString &userId, const QString &folderId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO folder_sync (linkcode, user_id, synced_at, is_complete) VALUES (?,?,?,0)
        ON CONFLICT(linkcode, user_id) DO UPDATE SET synced_at=excluded.synced_at
    )"));
    q.addBindValue(nonNullStr(folderId));
    q.addBindValue(userId);
    q.addBindValue(nowSecs());
    q.exec();
}

void FileCacheDB::clearFolderSync(const QString &userId, const QString &folderId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM folder_sync WHERE linkcode=? AND user_id=?"));
    q.addBindValue(nonNullStr(folderId));
    q.addBindValue(userId);
    if (!q.exec())
        qWarning() << "[FileCacheDB] clearFolderSync error:" << q.lastError().text();
}

// ── mutations ────────────────────────────────────────────────────────────────

void FileCacheDB::removeFiles(const QStringList &linkcodes)
{
    if (linkcodes.isEmpty()) return;

    QSqlQuery q(m_db);
    m_db.transaction();
    for (const QString &lc : linkcodes) {
        q.prepare(QStringLiteral("DELETE FROM files WHERE linkcode=?"));
        q.addBindValue(lc);
        if (!q.exec())
            qWarning() << "[FileCacheDB] removeFiles error:" << q.lastError().text();
    }
    m_db.commit();
}

void FileCacheDB::renameFile(const QString &linkcode, const QString &newName)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE files SET name=? WHERE linkcode=?"));
    q.addBindValue(newName);
    q.addBindValue(linkcode);
    if (!q.exec())
        qWarning() << "[FileCacheDB] renameFile error:" << q.lastError().text();
}

void FileCacheDB::updateParent(const QStringList &linkcodes, const QString &newParentId)
{
    if (linkcodes.isEmpty()) return;

    QSqlQuery q(m_db);
    m_db.transaction();
    for (const QString &lc : linkcodes) {
        q.prepare(QStringLiteral("UPDATE files SET parent_id=? WHERE linkcode=?"));
        q.addBindValue(newParentId);
        q.addBindValue(lc);
        if (!q.exec())
            qWarning() << "[FileCacheDB] updateParent error:" << q.lastError().text();
    }
    m_db.commit();
}

// ── local file tracking ──────────────────────────────────────────────────────

void FileCacheDB::upsertLocalFile(const QString &userId, const QString &linkcode,
                                  const QString &localPath, const QString &fileName,
                                  int64_t fileSize, const QString &transferType)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO local_files
            (linkcode, user_id, local_path, file_name, file_size, transfer_type, completed_at, verified_at)
        VALUES (?,?,?,?,?,?,?,?)
        ON CONFLICT(linkcode, user_id) DO UPDATE SET
            local_path=excluded.local_path, file_name=excluded.file_name,
            file_size=excluded.file_size, transfer_type=excluded.transfer_type,
            completed_at=excluded.completed_at, verified_at=excluded.verified_at
    )"));
    const qint64 now = nowSecs();
    q.addBindValue(linkcode);
    q.addBindValue(userId);
    q.addBindValue(localPath);
    q.addBindValue(fileName);
    q.addBindValue(static_cast<qint64>(fileSize));
    q.addBindValue(transferType);
    q.addBindValue(now);
    q.addBindValue(now);

    if (!q.exec())
        qWarning() << "[FileCacheDB] upsertLocalFile error:" << q.lastError().text();
}

QString FileCacheDB::getLocalPath(const QString &userId, const QString &linkcode)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT local_path FROM local_files WHERE linkcode=? AND user_id=?"));
    q.addBindValue(linkcode);
    q.addBindValue(userId);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return {};
}

QHash<QString, QString> FileCacheDB::getLocalPaths(const QString &userId,
                                                    const QStringList &linkcodes)
{
    QHash<QString, QString> result;
    if (linkcodes.isEmpty()) return result;

    QSqlQuery q(m_db);
    // Build placeholders for IN clause
    QStringList placeholders;
    placeholders.reserve(linkcodes.size());
    for (int i = 0; i < linkcodes.size(); ++i)
        placeholders << QStringLiteral("?");

    q.prepare(QStringLiteral("SELECT linkcode, local_path FROM local_files WHERE user_id=? AND linkcode IN (%1)")
              .arg(placeholders.join(QLatin1Char(','))));
    q.addBindValue(userId);
    for (const QString &lc : linkcodes)
        q.addBindValue(lc);

    if (!q.exec()) {
        qWarning() << "[FileCacheDB] getLocalPaths error:" << q.lastError().text();
        return result;
    }
    while (q.next())
        result.insert(q.value(0).toString(), q.value(1).toString());
    return result;
}

void FileCacheDB::removeLocalFile(const QString &userId, const QString &linkcode)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM local_files WHERE linkcode=? AND user_id=?"));
    q.addBindValue(linkcode);
    q.addBindValue(userId);
    if (!q.exec())
        qWarning() << "[FileCacheDB] removeLocalFile error:" << q.lastError().text();
}

bool FileCacheDB::verifyLocalFile(const QString &userId, const QString &linkcode)
{
    const QString path = getLocalPath(userId, linkcode);
    if (path.isEmpty()) return false;

    if (QFile::exists(path)) {
        // Update verified_at timestamp
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("UPDATE local_files SET verified_at=? WHERE linkcode=? AND user_id=?"));
        q.addBindValue(nowSecs());
        q.addBindValue(linkcode);
        q.addBindValue(userId);
        q.exec();
        return true;
    }

    // File no longer exists — remove the stale mapping
    removeLocalFile(userId, linkcode);
    return false;
}

// ── maintenance ──────────────────────────────────────────────────────────────

void FileCacheDB::clearUserCache(const QString &userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM files WHERE user_id=?"));
    q.addBindValue(userId);
    q.exec();

    q.prepare(QStringLiteral("DELETE FROM folder_sync WHERE user_id=?"));
    q.addBindValue(userId);
    q.exec();

    q.prepare(QStringLiteral("DELETE FROM local_files WHERE user_id=?"));
    q.addBindValue(userId);
    q.exec();

    qDebug() << "[FileCacheDB] Cache cleared for user" << userId;
}

} // namespace fsnext
