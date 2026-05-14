#pragma once
#include "core/models/TransferTask.h"
#include "core/repositories/TransferHistoryDb.h"
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>
#include <memory>

namespace fsnext {

enum class HistoryType {
    Download,
    Upload
};

// Persists per-user download/upload history.
//
// Storage: SQLite (TransferHistoryDb), with a one-shot import from the legacy
// per-user JSON files on first access. The JSON format is kept readable by
// the save* methods so downgrades work during early rollouts — but the new
// write path is single-row UPSERT via upsertTask(), not full-array rewrite.
class HistoryRepository {
public:
    HistoryRepository();
    ~HistoryRepository();

    // Download history
    QVector<TransferTask> loadDownloadHistory(const QString &userId);
    void saveDownloadHistory(const QString &userId, const QVector<TransferTask> &tasks);

    // Upload history
    QVector<TransferTask> loadUploadHistory(const QString &userId);
    void saveUploadHistory(const QString &userId, const QVector<TransferTask> &tasks);

    // Clear
    void clearHistory(const QString &userId, HistoryType type);

    // Paginated load — preferred for scale. `limit`+`offset` are 0-based.
    QVector<TransferTask> loadRecent(const QString &userId, HistoryType type,
                                     int limit, int offset = 0);

    // Total row count for a user+type. Used to drive infinite-scroll bounds.
    int countAll(const QString &userId, HistoryType type);

    // Single-row upsert. The hot path for completion-time writes — avoids
    // the N² cost of rewriting the full JSON array per finished transfer.
    bool upsertTask(const QString &userId, HistoryType type, const TransferTask &task);

    // ── ADR D12 — progress checkpoint pass-through ─────────────────────────
    // Forward to TransferHistoryDb::saveProgressSnapshot. Cheap UPDATE that
    // only touches progress_json + state, called on a 5-second cadence by
    // TransferService while a transfer is active.
    bool saveProgressSnapshot(const QString &userId, HistoryType type,
                              const QString &taskId, TransferState state,
                              const QString &snapshotJson);

    // Load every Queued/Active/Paused row for a user.  Returns the rows AND
    // their progress_json blobs (keyed by task.id) so TransferService can
    // resume where the previous session left off.
    QVector<TransferTask> loadInFlight(const QString &userId,
                                       QHash<QString, QString> *snapshotsOut = nullptr);

    // Trim a user's history to the most recent `keep` rows. Used on login
    // to enforce the display cap without materialising the full set.
    int trimToMostRecent(const QString &userId, HistoryType type, int keep);

    // Access to the underlying DB — lets TransferService spin up a worker-
    // thread connection for background loading (see BackgroundHistoryLoader).
    TransferHistoryDb *db() { return m_db.get(); }

    // Absolute filesystem path to the SQLite DB file. Exposed so the
    // background history loader can open a second connection on its own
    // thread (see TransferService::loadHistory).
    QString dbPath() const;

    // Path to the legacy per-user JSON file for (userId, type). Exposed so
    // the background loader can migrate it into the DB without having to
    // reach through this class — the loader only sees the free-standing
    // migrateLegacyUsingDb() static below.
    QString legacyJsonPath(const QString &userId, HistoryType type) const;

    // One-shot migration helper usable from any thread, as long as the
    // caller passes a TransferHistoryDb instance that's open on *that*
    // thread's connection. If `jsonPath` doesn't exist it's a no-op; on
    // success the file is renamed with a `.migrated` suffix.
    static void migrateLegacyUsingDb(TransferHistoryDb &db,
                                     const QString &jsonPath,
                                     const QString &userId,
                                     TransferType type);

private:
    // Ensure the DB is open. Runs the JSON→SQLite migration on first call.
    bool ensureOpen();

    // Migrate a single legacy JSON file (if present) into the DB and mark
    // it as migrated by renaming to .migrated so we never re-import it.
    void migrateLegacyJsonIfPresent(const QString &userId, HistoryType type);

    QString historyDir() const;

    std::unique_ptr<TransferHistoryDb> m_db;
    bool                               m_opened = false;
    QSet<QString>                      m_migratedKeys;  // userId|type keys already imported
};

} // namespace fsnext
