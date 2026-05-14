#include "TransferHistoryDb.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QVariant>

namespace fsnext {

namespace {

constexpr int kTypeDownload = 0;
constexpr int kTypeUpload   = 1;

int typeToInt(TransferType t) {
    return (t == TransferType::Upload) ? kTypeUpload : kTypeDownload;
}

TransferType intToType(int i) {
    return (i == kTypeUpload) ? TransferType::Upload : TransferType::Download;
}

TransferTask rowToTask(const QSqlQuery &q)
{
    TransferTask t;
    t.id          = q.value(QStringLiteral("id")).toString();
    t.type        = intToType(q.value(QStringLiteral("type")).toInt());
    t.state       = static_cast<TransferState>(q.value(QStringLiteral("state")).toInt());
    t.fileName    = q.value(QStringLiteral("file_name")).toString();
    t.fileSize    = q.value(QStringLiteral("file_size")).toLongLong();
    t.linkcode    = q.value(QStringLiteral("linkcode")).toString();
    t.localPath   = q.value(QStringLiteral("local_path")).toString();
    t.sourcePath  = q.value(QStringLiteral("source_path")).toString();
    t.folderId    = q.value(QStringLiteral("folder_id")).toString();
    t.description = q.value(QStringLiteral("description")).toString();
    t.secured     = q.value(QStringLiteral("secured")).toInt() != 0;
    t.directLink  = q.value(QStringLiteral("direct_link")).toInt() != 0;
    t.progress    = q.value(QStringLiteral("progress")).toDouble();
    t.errorMessage = q.value(QStringLiteral("error_message")).toString();
    t.groupId     = q.value(QStringLiteral("group_id")).toString();
    t.folderPath  = q.value(QStringLiteral("folder_path")).toString();
    t.completedAt = q.value(QStringLiteral("completed_at")).toLongLong();
    return t;
}

void bindTaskRow(QSqlQuery &q,
                 const QString &userId,
                 TransferType type,
                 const TransferTask &t)
{
    q.bindValue(QStringLiteral(":id"),            t.id);
    q.bindValue(QStringLiteral(":user_id"),       userId);
    q.bindValue(QStringLiteral(":type"),          typeToInt(type));
    q.bindValue(QStringLiteral(":state"),         static_cast<int>(t.state));
    q.bindValue(QStringLiteral(":file_name"),     t.fileName);
    q.bindValue(QStringLiteral(":file_size"),     static_cast<qint64>(t.fileSize));
    q.bindValue(QStringLiteral(":linkcode"),      t.linkcode);
    q.bindValue(QStringLiteral(":local_path"),    t.localPath);
    q.bindValue(QStringLiteral(":source_path"),   t.sourcePath);
    q.bindValue(QStringLiteral(":folder_id"),     t.folderId);
    q.bindValue(QStringLiteral(":description"),   t.description);
    q.bindValue(QStringLiteral(":secured"),       t.secured ? 1 : 0);
    q.bindValue(QStringLiteral(":direct_link"),   t.directLink ? 1 : 0);
    q.bindValue(QStringLiteral(":progress"),      t.progress);
    q.bindValue(QStringLiteral(":error_message"), t.errorMessage);
    q.bindValue(QStringLiteral(":group_id"),      t.groupId);
    q.bindValue(QStringLiteral(":folder_path"),   t.folderPath);
    q.bindValue(QStringLiteral(":completed_at"),  t.completedAt);
}

// SQLite UPSERT — INSERT...ON CONFLICT(id) DO UPDATE — preserves the
// progress_json column on re-upsert (state transitions, completion writes…).
// Plain `INSERT OR REPLACE` would atomically DELETE+INSERT and reset
// progress_json to '', wiping the most recent ADR-D12 checkpoint.
constexpr const char *kInsertOrReplaceSql = R"(
    INSERT INTO transfer_history (
        id, user_id, type, state,
        file_name, file_size, linkcode,
        local_path, source_path, folder_id,
        description, secured, direct_link,
        progress, error_message,
        group_id, folder_path, completed_at
    ) VALUES (
        :id, :user_id, :type, :state,
        :file_name, :file_size, :linkcode,
        :local_path, :source_path, :folder_id,
        :description, :secured, :direct_link,
        :progress, :error_message,
        :group_id, :folder_path, :completed_at
    )
    ON CONFLICT(id) DO UPDATE SET
        user_id       = excluded.user_id,
        type          = excluded.type,
        state         = excluded.state,
        file_name     = excluded.file_name,
        file_size     = excluded.file_size,
        linkcode      = excluded.linkcode,
        local_path    = excluded.local_path,
        source_path   = excluded.source_path,
        folder_id     = excluded.folder_id,
        description   = excluded.description,
        secured       = excluded.secured,
        direct_link   = excluded.direct_link,
        progress      = excluded.progress,
        error_message = excluded.error_message,
        group_id      = excluded.group_id,
        folder_path   = excluded.folder_path,
        completed_at  = excluded.completed_at
        -- progress_json INTENTIONALLY OMITTED so the most recent
        -- saveProgressSnapshot() write isn't clobbered by a state-change
        -- upsert that doesn't carry the snapshot.
)";

} // namespace

TransferHistoryDb::TransferHistoryDb() = default;

TransferHistoryDb::~TransferHistoryDb()
{
    if (m_db.isOpen()) m_db.close();
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool TransferHistoryDb::open(const QString &dbPath, const QString &connectionName)
{
    // Close the previous handle if we're re-opening on a different path.
    if (m_db.isOpen()) m_db.close();
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);

    m_connectionName = connectionName;

    if (dbPath != QStringLiteral(":memory:")) {
        QDir dir = QFileInfo(dbPath).absoluteDir();
        if (!dir.mkpath(QStringLiteral("."))) {
            qWarning() << "[TransferHistoryDb] Cannot create dir:" << dir.absolutePath();
            return false;
        }
    }

    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[TransferHistoryDb] Cannot open:" << m_db.lastError().text();
        return false;
    }

    // WAL for read-write concurrency; NORMAL sync is fine for history rows
    // where a rare crash would at worst lose the last few seconds of
    // completed-transfer metadata.  mmap + bigger page cache give the
    // paginated history fetch a head-start on cold opens; history volume
    // (up to ~1 K rows per user per type) easily fits in the mmap window.
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA temp_store = MEMORY"));
    pragma.exec(QStringLiteral("PRAGMA cache_size = -16000"));    // ~16 MB
    pragma.exec(QStringLiteral("PRAGMA mmap_size = 67108864"));   // 64 MB

    return createSchema();
}

bool TransferHistoryDb::isOpen() const { return m_db.isOpen(); }

bool TransferHistoryDb::createSchema()
{
    QSqlQuery q(m_db);
    const QString ddl = QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS transfer_history (
            id             TEXT PRIMARY KEY,
            user_id        TEXT NOT NULL,
            type           INTEGER NOT NULL,
            state          INTEGER NOT NULL,
            file_name      TEXT NOT NULL DEFAULT '',
            file_size      INTEGER NOT NULL DEFAULT 0,
            linkcode       TEXT NOT NULL DEFAULT '',
            local_path     TEXT NOT NULL DEFAULT '',
            source_path    TEXT NOT NULL DEFAULT '',
            folder_id      TEXT NOT NULL DEFAULT '',
            description    TEXT NOT NULL DEFAULT '',
            secured        INTEGER NOT NULL DEFAULT 0,
            direct_link    INTEGER NOT NULL DEFAULT 0,
            progress       REAL    NOT NULL DEFAULT 100.0,
            error_message  TEXT    NOT NULL DEFAULT '',
            group_id       TEXT    NOT NULL DEFAULT '',
            folder_path    TEXT    NOT NULL DEFAULT '',
            completed_at   INTEGER NOT NULL DEFAULT 0,
            -- ADR D12: per-task progress checkpoint, debounced ~5s.
            -- Empty string means "no checkpoint" (e.g. completed rows).
            progress_json  TEXT    NOT NULL DEFAULT ''
        )
    )");
    if (!q.exec(ddl)) {
        qWarning() << "[TransferHistoryDb] Schema error:" << q.lastError().text();
        return false;
    }

    // The entire read path filters by (user_id, type) and sorts by
    // completed_at DESC, so this composite index is what makes loadRecent
    // return fast even with millions of rows in the table.
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_history_user_type_completed "
        "ON transfer_history(user_id, type, completed_at DESC)"));

    // For databases predating ADR D12 (rev ≤ 2026-04-27), graft the new column
    // onto the existing table.  Idempotent — `migrateSchema` ignores errors
    // from "duplicate column name".
    return migrateSchema();
}

bool TransferHistoryDb::migrateSchema()
{
    QSqlQuery q(m_db);

    // Detect whether progress_json already exists. PRAGMA returns one row per
    // existing column; we look for the name in the result set.
    bool hasProgressJson = false;
    if (q.exec(QStringLiteral("PRAGMA table_info(transfer_history)"))) {
        while (q.next()) {
            if (q.value(1).toString() == QLatin1String("progress_json")) {
                hasProgressJson = true;
                break;
            }
        }
    }
    if (!hasProgressJson) {
        if (!q.exec(QStringLiteral(
                "ALTER TABLE transfer_history ADD COLUMN progress_json TEXT NOT NULL DEFAULT ''"))) {
            qWarning() << "[TransferHistoryDb] ALTER TABLE failed:" << q.lastError().text();
            return false;
        }
        qInfo() << "[TransferHistoryDb] Migrated schema: added progress_json column";
    }

    // Partial index to make "find anything still in flight for this user" fast.
    // States 0/1/2 = Queued/Active/Paused (matches TransferState enum).  This
    // gets hit once at startup and is kept tiny because finished rows are excluded.
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_history_inflight "
        "ON transfer_history(user_id, state) WHERE state IN (0, 1, 2)"));

    return true;
}

bool TransferHistoryDb::saveProgressSnapshot(const QString &userId,
                                              TransferType type,
                                              const QString &taskId,
                                              TransferState state,
                                              const QString &snapshotJson)
{
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE transfer_history "
        "SET progress_json = :pj, state = :s "
        "WHERE id = :id AND user_id = :uid AND type = :t"));
    q.bindValue(QStringLiteral(":pj"),  snapshotJson);
    q.bindValue(QStringLiteral(":s"),   static_cast<int>(state));
    q.bindValue(QStringLiteral(":id"),  taskId);
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":t"),   static_cast<int>(type));
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] saveProgressSnapshot failed:" << q.lastError().text();
        return false;
    }
    // numRowsAffected == 0 is normal on the first checkpoint of a brand-new
    // task — TransferService is expected to upsert the row first via the
    // existing upsertTask() path; this method only updates already-known rows.
    return true;
}

QVector<TransferTask> TransferHistoryDb::loadInFlight(const QString &userId,
                                                      QHash<QString, QString> *snapshotsOut)
{
    QVector<TransferTask> out;
    if (!m_db.isOpen()) return out;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, type, state, file_name, file_size, linkcode, local_path, "
        "       source_path, folder_id, description, secured, direct_link, "
        "       progress, error_message, group_id, folder_path, completed_at, "
        "       progress_json "
        "FROM transfer_history "
        "WHERE user_id = :uid AND state IN (0, 1, 2) "
        "ORDER BY completed_at DESC"));
    q.bindValue(QStringLiteral(":uid"), userId);
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] loadInFlight failed:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        TransferTask t;
        t.id              = q.value(0).toString();
        t.type            = static_cast<TransferType>(q.value(1).toInt());
        t.state           = static_cast<TransferState>(q.value(2).toInt());
        t.fileName        = q.value(3).toString();
        t.fileSize        = q.value(4).toLongLong();
        t.linkcode        = q.value(5).toString();
        t.localPath       = q.value(6).toString();
        t.sourcePath      = q.value(7).toString();
        t.folderId        = q.value(8).toString();
        t.description     = q.value(9).toString();
        t.secured         = q.value(10).toBool();
        t.directLink      = q.value(11).toBool();
        t.progress        = q.value(12).toDouble();
        t.errorMessage    = q.value(13).toString();
        t.groupId         = q.value(14).toString();
        t.folderPath      = q.value(15).toString();
        t.completedAt     = q.value(16).toLongLong();
        const QString pj  = q.value(17).toString();
        out.push_back(t);
        if (snapshotsOut && !pj.isEmpty()) snapshotsOut->insert(t.id, pj);
    }
    return out;
}

bool TransferHistoryDb::upsertTask(const QString &userId,
                                    TransferType type,
                                    const TransferTask &task)
{
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kInsertOrReplaceSql));
    bindTaskRow(q, userId, type, task);
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] upsertTask failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool TransferHistoryDb::upsertTasks(const QString &userId,
                                     TransferType type,
                                     const QVector<TransferTask> &tasks)
{
    if (!m_db.isOpen()) return false;
    if (tasks.isEmpty()) return true;

    // Transaction turns N INSERT OR REPLACE calls into a single WAL commit.
    // Without this, a migration of 10K JSON rows would do 10K fsync calls.
    if (!m_db.transaction()) {
        qWarning() << "[TransferHistoryDb] transaction() failed:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kInsertOrReplaceSql));
    for (const TransferTask &t : tasks) {
        bindTaskRow(q, userId, type, t);
        if (!q.exec()) {
            qWarning() << "[TransferHistoryDb] upsertTasks row failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }
    return m_db.commit();
}

QVector<TransferTask> TransferHistoryDb::loadAll(const QString &userId, TransferType type)
{
    QVector<TransferTask> out;
    if (!m_db.isOpen()) return out;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT * FROM transfer_history "
        "WHERE user_id = :uid AND type = :type "
        "ORDER BY completed_at DESC"));
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":type"), typeToInt(type));
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] loadAll failed:" << q.lastError().text();
        return out;
    }
    while (q.next()) out.append(rowToTask(q));
    return out;
}

QVector<TransferTask> TransferHistoryDb::loadRecent(const QString &userId,
                                                     TransferType type,
                                                     int limit,
                                                     int offset)
{
    QVector<TransferTask> out;
    if (!m_db.isOpen()) return out;
    if (limit <= 0) return out;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT * FROM transfer_history "
        "WHERE user_id = :uid AND type = :type "
        "ORDER BY completed_at DESC "
        "LIMIT :limit OFFSET :offset"));
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":type"), typeToInt(type));
    q.bindValue(QStringLiteral(":limit"), limit);
    q.bindValue(QStringLiteral(":offset"), std::max(0, offset));
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] loadRecent failed:" << q.lastError().text();
        return out;
    }
    out.reserve(limit);
    while (q.next()) out.append(rowToTask(q));
    return out;
}

int TransferHistoryDb::countAll(const QString &userId, TransferType type)
{
    if (!m_db.isOpen()) return 0;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM transfer_history "
        "WHERE user_id = :uid AND type = :type"));
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":type"), typeToInt(type));
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

bool TransferHistoryDb::deleteAll(const QString &userId, TransferType type)
{
    if (!m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM transfer_history "
        "WHERE user_id = :uid AND type = :type"));
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":type"), typeToInt(type));
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] deleteAll failed:" << q.lastError().text();
        return false;
    }
    return true;
}

int TransferHistoryDb::purgeOlderThan(const QString &userId,
                                       TransferType type,
                                       qint64 cutoffMs)
{
    if (!m_db.isOpen()) return 0;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM transfer_history "
        "WHERE user_id = :uid AND type = :type AND completed_at < :cutoff"));
    q.bindValue(QStringLiteral(":uid"), userId);
    q.bindValue(QStringLiteral(":type"), typeToInt(type));
    q.bindValue(QStringLiteral(":cutoff"), cutoffMs);
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] purgeOlderThan failed:" << q.lastError().text();
        return 0;
    }
    return q.numRowsAffected();
}

int TransferHistoryDb::trimToMostRecent(const QString &userId,
                                         TransferType type,
                                         int keep)
{
    if (!m_db.isOpen() || keep <= 0) return 0;

    // Delete every row outside the most-recent `keep` window. The subquery
    // uses the composite index so it doesn't scan the whole table.
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM transfer_history "
        "WHERE user_id = :uid AND type = :type AND id NOT IN ("
        "  SELECT id FROM transfer_history "
        "  WHERE user_id = :uid2 AND type = :type2 "
        "  ORDER BY completed_at DESC LIMIT :keep"
        ")"));
    q.bindValue(QStringLiteral(":uid"),   userId);
    q.bindValue(QStringLiteral(":type"),  typeToInt(type));
    q.bindValue(QStringLiteral(":uid2"),  userId);
    q.bindValue(QStringLiteral(":type2"), typeToInt(type));
    q.bindValue(QStringLiteral(":keep"),  keep);
    if (!q.exec()) {
        qWarning() << "[TransferHistoryDb] trimToMostRecent failed:" << q.lastError().text();
        return 0;
    }
    return q.numRowsAffected();
}

} // namespace fsnext
