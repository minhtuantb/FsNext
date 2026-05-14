#pragma once

#include "core/models/FileItem.h"
#include "core/transfer/TransferPriority.h"
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QMap>

namespace fsnext {

class FshareApi;
class FileCacheDB;
class TransferOrchestrator;

// Progressively fetches every page of a folder from the API and inserts the
// results into FileCacheDB.  Runs entirely on the calling (main) thread as an
// orchestrator; actual network calls are dispatched to QtConcurrent's thread
// pool and results are posted back via QMetaObject::invokeMethod.
//
// Scheduling
// ----------
// When a TransferOrchestrator is supplied, every folder crawl is gated by the
// orchestrator's Metadata pool: concurrency, global cap (metadata is starved
// down to metadataFloorGlobal when DL/UL saturate the global pool), and
// priority ordering all live in one place.  Without an orchestrator
// (unit tests), the worker falls back to a self-limited kMaxConcurrent.
class FileSyncWorker : public QObject
{
    Q_OBJECT

public:
    explicit FileSyncWorker(FshareApi *api, FileCacheDB *db,
                            TransferOrchestrator *orchestrator = nullptr,
                            QObject *parent = nullptr);

    // Add a folder to the sync queue.
    // highPriority=true → treated as user-visible (TransferPriority::Normal),
    //                     otherwise background prefetch (Metadata priority).
    void enqueueFolder(const QString &userId, const QString &folderId, bool highPriority = false);

    // Convenience: enqueue every folder returned by loadFolderTree as background work.
    void enqueueTree(const QString &userId, const QVector<FileItem> &folders);

    // Discard pending work (e.g. on logout).  In-flight requests finish normally.
    void cancelAll();

    bool isSyncing() const;

signals:
    // Emitted on main thread after the first batch (page 1) is inserted.
    void firstBatchSynced(const QString &folderId);

    // Emitted on main thread after all pages of a folder are fetched.
    void folderSynced(const QString &folderId);

    // Emitted after a network or API error.
    void syncError(const QString &folderId, const QString &error);

private slots:
    // Orchestrator told us a metadata slot is free — start the crawl for `id`
    // if we own it.  Connected with QueuedConnection so it runs on this
    // object's thread.
    void onDispatchReady(const QString &id, fsnext::TransferClass cls,
                         fsnext::TransferPriority prio);

private:
    // Internal: actually kick a QtConcurrent crawl for the given folder.
    // Preconditions: folderId is in m_userMap, caller has incremented
    // m_inFlightCount (fallback path) or taken the orchestrator slot.
    void startCrawl(const QString &folderId);

    // Fallback path: pop next queued folder and start it (no orchestrator).
    void processNext();

    FshareApi            *m_api  = nullptr;
    FileCacheDB          *m_db   = nullptr;
    TransferOrchestrator *m_orch = nullptr; // non-owning, may be null in tests

    mutable QMutex    m_mutex;
    QStringList       m_highQueue;      // high-priority (front) — fallback only
    QStringList       m_bgQueue;        // background (back)     — fallback only
    QSet<QString>     m_inFlight;       // folder IDs currently orchestrator-held or being fetched
    QMap<QString,QString> m_userMap;    // folderId → userId

    // How many processNext invocations are currently alive (fallback path
    // only — when m_orch is non-null the orchestrator owns concurrency).
    // An "alive" invocation is one that popped an item and has not yet
    // drained the queue on re-entry.
    int               m_inFlightCount = 0;
};

} // namespace fsnext
