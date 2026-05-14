#pragma once

#include "core/models/TransferTask.h"
#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

namespace fsnext {

// SQLite-backed transfer history.
//
// Why this exists:
//   The original HistoryRepository serialized the whole download/upload
//   history as a pair of JSON files per user. For power users accumulating
//   thousands of rows (10K+), that approach had two painful properties:
//     • load-all-on-startup stalled the main thread for seconds
//     • every save rewrote the full array, turning each completed transfer
//       into a ~10 MB disk write
//   SQLite fixes both: startup loads only the window the UI actually shows
//   (via loadRecent), and completions upsert a single row instead of
//   rewriting the universe.
//
// Concurrency: Open once, call from one thread at a time. The background
// history loader owns its own connection (named via `connectionName`) so
// worker-thread reads don't fight main-thread upserts.
class TransferHistoryDb
{
public:
    TransferHistoryDb();
    ~TransferHistoryDb();

    // Open (or create) the database. Must be called once before any other
    // method. Safe to call again on a different path — the previous handle
    // is closed first.
    bool open(const QString &dbPath,
              const QString &connectionName = QStringLiteral("fs_history"));
    bool isOpen() const;
    QString connectionName() const { return m_connectionName; }

    // Bulk upsert, wrapped in a single transaction. Used by the legacy
    // save(vector) path so migrating JSON → DB stays a single commit.
    bool upsertTasks(const QString &userId,
                     TransferType type,
                     const QVector<TransferTask> &tasks);

    // Single-row upsert. Preferred for completion-time writes — O(1) disk
    // cost instead of the O(N) JSON rewrite.
    bool upsertTask(const QString &userId,
                    TransferType type,
                    const TransferTask &task);

    // Full load for a user + type, newest-first by completed_at. Backward
    // compat with the old JSON API; callers that need scale should prefer
    // loadRecent().
    QVector<TransferTask> loadAll(const QString &userId, TransferType type);

    // Paginated, newest-first. `offset` is 0-based. A QSqlDatabase connection
    // is looked up by name, so the caller can use this from a worker thread
    // by passing `open(path, workerConnection)` first.
    QVector<TransferTask> loadRecent(const QString &userId,
                                     TransferType type,
                                     int limit,
                                     int offset = 0);

    // Count rows matching the filter — for UI infinite-scroll hit counting
    // without materialising the full set.
    int countAll(const QString &userId, TransferType type);

    // Drop all rows for a user+type. Used by the "Clear history" button.
    bool deleteAll(const QString &userId, TransferType type);

    // TTL cleanup: delete anything older than `cutoffMs` (ms since epoch).
    // Returns the number of rows removed.
    int purgeOlderThan(const QString &userId, TransferType type, qint64 cutoffMs);

    // Hard cap: keep only the `keep` most recent rows per user+type and
    // drop the rest. Used by TransferService::loadHistory() to enforce the
    // display cap without ever holding the full set in memory.
    int trimToMostRecent(const QString &userId, TransferType type, int keep);

    // ── ADR D12 — progress persistence ─────────────────────────────────────
    // Cheap UPDATE that only touches the `progress_json` column (and `state`).
    // Called every ~5 s by TransferService to checkpoint in-flight transfers
    // so that an app crash mid-download resumes from the last snapshot rather
    // than byte 0. Payload is a small JSON blob holding bytesTransferred,
    // fileSize, segmentBytes[], retryCount — see TransferService for the
    // exact serialisation.
    bool saveProgressSnapshot(const QString &userId,
                              TransferType type,
                              const QString &taskId,
                              TransferState state,
                              const QString &snapshotJson);

    // Load every row whose state is in [Queued, Active, Paused] for the given
    // user. Returns each task with its progress_json copied into a separate
    // out-param map (keyed by task.id) so callers don't have to thread a
    // custom DTO type. Used at startup to resume interrupted transfers.
    QVector<TransferTask> loadInFlight(const QString &userId,
                                       QHash<QString, QString> *snapshotsOut = nullptr);

private:
    bool createSchema();
    // Adds columns introduced after the original schema (idempotent).
    // Currently only adds `progress_json`. New columns must default-initialise
    // — old rows survive without backfill.
    bool migrateSchema();

    QString      m_connectionName;
    QSqlDatabase m_db;
};

} // namespace fsnext
