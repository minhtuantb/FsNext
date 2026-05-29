#include "HistoryRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>

namespace fsnext {

// ── JSON helpers (legacy format — retained for one-shot migration) ──────────

static QJsonObject taskToJson(const TransferTask &task)
{
    QJsonObject o;
    o["id"] = task.id;
    o["type"] = (task.type == TransferType::Download) ? "download" : "upload";
    o["state"] = static_cast<int>(task.state);
    o["fileName"] = task.fileName;
    o["fileSize"] = static_cast<qint64>(task.fileSize);
    o["linkcode"] = task.linkcode;
    o["localPath"] = task.localPath;
    o["sourcePath"] = task.sourcePath;
    o["folderId"] = task.folderId;
    o["description"] = task.description;
    o["secured"] = task.secured;
    o["directLink"] = task.directLink;
    o["progress"] = task.progress;
    o["errorMessage"] = task.errorMessage;
    o["groupId"] = task.groupId;
    o["folderPath"] = task.folderPath;
    const qint64 completedMs = task.completedAt > 0
        ? task.completedAt
        : QDateTime::currentMSecsSinceEpoch();
    o["completedAt"]   = static_cast<double>(completedMs);
    o["completedAtIso"] = QDateTime::fromMSecsSinceEpoch(completedMs).toString(Qt::ISODate);
    return o;
}

static TransferTask taskFromJson(const QJsonObject &o)
{
    TransferTask t;
    t.id = o.value("id").toString();
    t.type = (o.value("type").toString() == "upload") ? TransferType::Upload : TransferType::Download;
    t.state = static_cast<TransferState>(o.value("state").toInt(static_cast<int>(TransferState::Complete)));
    t.fileName = o.value("fileName").toString();
    t.fileSize = static_cast<int64_t>(o.value("fileSize").toVariant().toLongLong());
    t.linkcode = o.value("linkcode").toString();
    t.localPath = o.value("localPath").toString();
    t.sourcePath = o.value("sourcePath").toString();
    t.folderId = o.value("folderId").toString();
    t.description = o.value("description").toString();
    t.secured = o.value("secured").toBool();
    t.directLink = o.value("directLink").toBool();
    t.progress = o.value("progress").toDouble(100.0);
    t.errorMessage = o.value("errorMessage").toString();
    t.groupId    = o.value("groupId").toString();
    t.folderPath = o.value("folderPath").toString();

    const QJsonValue completed = o.value("completedAt");
    if (completed.isDouble()) {
        t.completedAt = static_cast<qint64>(completed.toVariant().toLongLong());
    } else if (completed.isString()) {
        const QDateTime dt = QDateTime::fromString(completed.toString(), Qt::ISODate);
        if (dt.isValid()) t.completedAt = dt.toMSecsSinceEpoch();
    }
    return t;
}

static QVector<TransferTask> loadTasksFromJson(const QString &filePath)
{
    // Hard ceiling on rows we'll ever parse from a single legacy JSON file.
    // The legacy format had no built-in cap, so a malformed (or maliciously
    // edited) history.json with millions of entries would have caused
    // QVector::reserve(arr.size()) to allocate gigabytes and either OOM the
    // process or thrash for minutes on startup before the user sees any UI.
    // The TransferHistoryDb migration target already has its own paging
    // contract, so dropping rows beyond this cap is acceptable —
    // realistically a user's *total* history is well under this number.
    static constexpr int kMaxLegacyHistoryRows = 50000;

    QVector<TransferTask> result;
    QFile f(filePath);
    if (!f.exists()) return result;
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[HistoryRepo] Cannot open" << filePath;
        return result;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[HistoryRepo] JSON parse error:" << err.errorString();
        return result;
    }
    if (!doc.isArray()) return result;
    const QJsonArray arr = doc.array();
    const int rowsToLoad = qMin(arr.size(), kMaxLegacyHistoryRows);
    if (arr.size() > kMaxLegacyHistoryRows) {
        qWarning() << "[HistoryRepo] Legacy JSON" << filePath << "has"
                   << arr.size() << "rows; capping migration at"
                   << kMaxLegacyHistoryRows;
    }
    result.reserve(rowsToLoad);
    for (int i = 0; i < rowsToLoad; ++i) {
        const auto &v = arr.at(i);
        if (v.isObject()) result.append(taskFromJson(v.toObject()));
    }
    return result;
}

// ── HistoryRepository ───────────────────────────────────────────────────────

HistoryRepository::HistoryRepository()
    : m_db(std::make_unique<TransferHistoryDb>())
{
}

HistoryRepository::~HistoryRepository() = default;

QString HistoryRepository::historyDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/FshareNext/history/");
}

QString HistoryRepository::dbPath() const
{
    return historyDir() + QStringLiteral("transfer_history.db");
}

QString HistoryRepository::legacyJsonPath(const QString &userId, HistoryType type) const
{
    const QString typeSuffix = (type == HistoryType::Download)
                               ? QStringLiteral("download")
                               : QStringLiteral("upload");
    const QString safeUser = userId.isEmpty() ? QStringLiteral("anonymous") : userId;
    return historyDir() + safeUser + QStringLiteral("_") + typeSuffix + QStringLiteral(".json");
}

bool HistoryRepository::ensureOpen()
{
    if (m_opened && m_db && m_db->isOpen()) return true;

    QDir().mkpath(historyDir());
    if (!m_db) m_db = std::make_unique<TransferHistoryDb>();
    if (!m_db->open(dbPath())) {
        qWarning() << "[HistoryRepo] Failed to open history DB at" << dbPath();
        return false;
    }
    m_opened = true;
    return true;
}

static TransferType toXferType(HistoryType t) {
    return (t == HistoryType::Upload) ? TransferType::Upload : TransferType::Download;
}

void HistoryRepository::migrateLegacyJsonIfPresent(const QString &userId, HistoryType type)
{
    const QString key = userId + QStringLiteral("|") + QString::number(static_cast<int>(type));
    if (m_migratedKeys.contains(key)) return;
    m_migratedKeys.insert(key);

    const QString jsonPath = legacyJsonPath(userId, type);
    migrateLegacyUsingDb(*m_db, jsonPath, userId, toXferType(type));
}

// Static helper — usable from any thread, as long as the TransferHistoryDb
// argument is open on the CALLING thread's connection. The background
// history loader uses this with a worker-owned connection so the on-login
// migration (parse + N × INSERT for legacy 1000-row histories) happens off
// the main thread.
void HistoryRepository::migrateLegacyUsingDb(TransferHistoryDb &db,
                                              const QString &jsonPath,
                                              const QString &userId,
                                              TransferType type)
{
    if (!QFile::exists(jsonPath)) return;

    const QVector<TransferTask> legacy = loadTasksFromJson(jsonPath);
    if (legacy.isEmpty()) {
        // Empty/corrupt JSON — still rename so we don't re-try every launch.
        QFile::rename(jsonPath, jsonPath + QStringLiteral(".migrated"));
        return;
    }

    if (db.upsertTasks(userId, type, legacy)) {
        QFile::rename(jsonPath, jsonPath + QStringLiteral(".migrated"));
        qDebug() << "[HistoryRepo] Migrated" << legacy.size()
                 << "rows from" << QFileInfo(jsonPath).fileName();
    } else {
        qWarning() << "[HistoryRepo] Migration failed for" << jsonPath
                   << "— will retry on next launch";
    }
}

QVector<TransferTask> HistoryRepository::loadDownloadHistory(const QString &userId)
{
    if (!ensureOpen()) return {};
    migrateLegacyJsonIfPresent(userId, HistoryType::Download);
    return m_db->loadAll(userId, TransferType::Download);
}

QVector<TransferTask> HistoryRepository::loadUploadHistory(const QString &userId)
{
    if (!ensureOpen()) return {};
    migrateLegacyJsonIfPresent(userId, HistoryType::Upload);
    return m_db->loadAll(userId, TransferType::Upload);
}

void HistoryRepository::saveDownloadHistory(const QString &userId,
                                             const QVector<TransferTask> &tasks)
{
    if (!ensureOpen()) return;
    // Bulk save is used by the legacy "replace whole list" flows (trim-on-
    // login, clear-all). We upsert the supplied set, then trim anything
    // the caller dropped — i.e. rows not present in `tasks` but still on
    // disk. Simpler than computing deltas and matches the old semantics.
    m_db->deleteAll(userId, TransferType::Download);
    if (!tasks.isEmpty())
        m_db->upsertTasks(userId, TransferType::Download, tasks);
}

void HistoryRepository::saveUploadHistory(const QString &userId,
                                           const QVector<TransferTask> &tasks)
{
    if (!ensureOpen()) return;
    m_db->deleteAll(userId, TransferType::Upload);
    if (!tasks.isEmpty())
        m_db->upsertTasks(userId, TransferType::Upload, tasks);
}

void HistoryRepository::clearHistory(const QString &userId, HistoryType type)
{
    if (!ensureOpen()) return;
    m_db->deleteAll(userId, toXferType(type));
}

QVector<TransferTask> HistoryRepository::loadRecent(const QString &userId,
                                                     HistoryType type,
                                                     int limit,
                                                     int offset)
{
    if (!ensureOpen()) return {};
    migrateLegacyJsonIfPresent(userId, type);
    return m_db->loadRecent(userId, toXferType(type), limit, offset);
}

int HistoryRepository::countAll(const QString &userId, HistoryType type)
{
    if (!ensureOpen()) return 0;
    migrateLegacyJsonIfPresent(userId, type);
    return m_db->countAll(userId, toXferType(type));
}

bool HistoryRepository::upsertTask(const QString &userId,
                                    HistoryType type,
                                    const TransferTask &task)
{
    if (!ensureOpen()) return false;
    return m_db->upsertTask(userId, toXferType(type), task);
}

int HistoryRepository::trimToMostRecent(const QString &userId,
                                         HistoryType type,
                                         int keep)
{
    if (!ensureOpen()) return 0;
    return m_db->trimToMostRecent(userId, toXferType(type), keep);
}

bool HistoryRepository::saveProgressSnapshot(const QString &userId,
                                              HistoryType type,
                                              const QString &taskId,
                                              TransferState state,
                                              const QString &snapshotJson)
{
    if (!ensureOpen()) return false;
    return m_db->saveProgressSnapshot(userId, toXferType(type),
                                       taskId, state, snapshotJson);
}

QVector<TransferTask> HistoryRepository::loadInFlight(const QString &userId,
                                                      QHash<QString, QString> *snapshotsOut)
{
    if (!ensureOpen()) return {};
    // The migration step lives in loadDownloadHistory / loadUploadHistory
    // because it's bound to a specific HistoryType.  loadInFlight is type-
    // agnostic so we trigger both migrations defensively — a no-op if already
    // done (m_migratedKeys short-circuits).
    migrateLegacyJsonIfPresent(userId, HistoryType::Download);
    migrateLegacyJsonIfPresent(userId, HistoryType::Upload);
    return m_db->loadInFlight(userId, snapshotsOut);
}

} // namespace fsnext
