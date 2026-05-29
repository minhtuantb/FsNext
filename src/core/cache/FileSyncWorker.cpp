#include "FileSyncWorker.h"

#include "FileCacheDB.h"
#include "core/api/FshareApi.h"
#include "core/transfer/TransferOrchestrator.h"

#include <QDebug>
#include <QPointer>
#include <QtConcurrent>

namespace fsnext {

// Fetch at most this many pages per folder to prevent runaway on huge accounts.
static constexpr int kMaxPages    = 400;   // 400 × 50 = 20 000 files
static constexpr int kPageSize    = 50;

// Fallback concurrency cap (only when no orchestrator is supplied — unit
// tests).  When the orchestrator is present it owns concurrency via
// BudgetManager::maxMetadataSlots, so this constant is unused in production.
static constexpr int kMaxConcurrent = 2;

FileSyncWorker::FileSyncWorker(FshareApi *api, FileCacheDB *db,
                               TransferOrchestrator *orchestrator,
                               QObject *parent)
    : QObject(parent), m_api(api), m_db(db), m_orch(orchestrator)
{
    if (m_orch) {
        // Qt auto-selects QueuedConnection across the orchestrator's thread.
        connect(m_orch, &TransferOrchestrator::dispatchReady,
                this,   &FileSyncWorker::onDispatchReady);
    }
}

// ── public API ────────────────────────────────────────────────────────────────

void FileSyncWorker::enqueueFolder(const QString &userId,
                                   const QString &folderId,
                                   bool           highPriority)
{
    if (m_orch) {
        // Orchestrator-gated path.  Dedup on m_inFlight (covers both "queued
        // in orchestrator" and "currently crawling").  Priority mapping:
        //   highPriority  → Normal  (user-visible folder, user is waiting)
        //   background    → Metadata (lowest, runs on leftover capacity)
        {
            QMutexLocker lock(&m_mutex);
            if (m_inFlight.contains(folderId)) return;  // already known
            m_userMap[folderId] = userId;
            m_inFlight.insert(folderId);
        }
        const TransferPriority prio = highPriority
            ? TransferPriority::Normal
            : TransferPriority::Metadata;
        m_orch->enqueue(folderId, TransferClass::Metadata, prio);
        return;
    }

    // ── Fallback path (no orchestrator, e.g. unit tests) ─────────────────────
    QMutexLocker lock(&m_mutex);

    if (m_inFlight.contains(folderId)) return; // already fetching

    m_userMap[folderId] = userId;

    if (highPriority) {
        if (!m_highQueue.contains(folderId))
            m_highQueue.prepend(folderId);
        // Remove from background queue if it was queued there earlier
        m_bgQueue.removeOne(folderId);
    } else {
        if (!m_highQueue.contains(folderId) && !m_bgQueue.contains(folderId))
            m_bgQueue.append(folderId);
    }

    // Kick as many queued processNext invocations as there are free metadata
    // slots.  Each in-flight crawl already self-reenters via QMetaObject on
    // completion, so these kickoffs only matter on the transition from
    // "idle / under-utilised" to "work pending".
    while (m_inFlightCount < kMaxConcurrent
           && (!m_highQueue.isEmpty() || !m_bgQueue.isEmpty())) {
        ++m_inFlightCount;
        QMetaObject::invokeMethod(this, &FileSyncWorker::processNext,
                                  Qt::QueuedConnection);
    }
}

void FileSyncWorker::enqueueTree(const QString &userId, const QVector<FileItem> &folders)
{
    for (const FileItem &f : folders) {
        if (f.isFolder())
            enqueueFolder(userId, f.linkcode, false);
    }
}

void FileSyncWorker::cancelAll()
{
    QStringList toCancel;
    {
        QMutexLocker lock(&m_mutex);
        m_highQueue.clear();
        m_bgQueue.clear();
        // Items already in-flight will finish but their results are still inserted —
        // that is fine; the cache stays consistent.

        if (m_orch) {
            // Ask the orchestrator to drop any queued-but-not-yet-dispatched
            // metadata items we own.  Items already dispatched will complete
            // and release normally.
            toCancel.reserve(m_inFlight.size());
            for (const QString &id : m_inFlight)
                toCancel.push_back(id);
        }
    }
    for (const QString &id : toCancel) {
        if (m_orch->cancelQueued(id)) {
            // Successfully removed before dispatch — clear our bookkeeping.
            QMutexLocker lock(&m_mutex);
            m_inFlight.remove(id);
            m_userMap.remove(id);
        }
    }
}

bool FileSyncWorker::isSyncing() const
{
    QMutexLocker lock(&m_mutex);
    // Orchestrator path: isSyncing means "we have work either queued or in
    // flight" — m_inFlight covers both.  Fallback path: m_inFlightCount > 0.
    if (m_orch) return !m_inFlight.isEmpty();
    return m_inFlightCount > 0;
}

// ── internal ──────────────────────────────────────────────────────────────────

void FileSyncWorker::onDispatchReady(const QString &id, TransferClass cls,
                                     TransferPriority /*prio*/)
{
    // dispatchReady is broadcast — filter to metadata items we own.
    if (cls != TransferClass::Metadata) return;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_userMap.contains(id)) return;  // not one of ours
    }
    startCrawl(id);
}

void FileSyncWorker::processNext()
{
    QString folderId;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_highQueue.isEmpty())
            folderId = m_highQueue.takeFirst();
        else if (!m_bgQueue.isEmpty())
            folderId = m_bgQueue.takeFirst();
        else {
            // No work left for THIS invocation.  Decrement so the next
            // enqueueFolder() correctly sees a free slot.
            --m_inFlightCount;
            return;
        }
        m_inFlight.insert(folderId);
    }
    startCrawl(folderId);
}

void FileSyncWorker::startCrawl(const QString &folderId)
{
    QString userId;
    {
        QMutexLocker lock(&m_mutex);
        userId = m_userMap.value(folderId);
    }

    FshareApi  *api = m_api;
    FileCacheDB *db = m_db;
    QPointer<FileSyncWorker> guard(this);

    // Guard against null api (unit tests construct the worker with no HTTP
    // layer — startCrawl would otherwise crash when a queued invocation
    // fires after the owning service is torn down).
    if (!api) {
        QMutexLocker lock(&m_mutex);
        m_inFlight.remove(folderId);
        m_userMap.remove(folderId);
        if (m_orch) m_orch->release(folderId);
        else       --m_inFlightCount;
        return;
    }

    // Run on the thread pool.  The lambda must not touch Qt objects of other
    // threads except through QMetaObject::invokeMethod (Qt::QueuedConnection).
    QtConcurrent::run([api, db, guard, folderId, userId]() mutable {

        int  page        = 0;           // Fshare API uses 0-indexed pages
        bool firstBatch  = true;
        int  totalItems  = 0;

        while (page < kMaxPages) {

            // ── network call (blocking on this worker thread) ─────────────
            auto resp = api->listFiles(folderId, page, kPageSize);

            // ── route result back to main thread ──────────────────────────
            if (!resp.isSuccess()) {
                const QString errMsg = resp.error().message;
                QMetaObject::invokeMethod(guard.data(),
                    [guard, folderId, errMsg]() {
                        if (!guard) return;
                        auto *self = guard.data();
                        {
                            QMutexLocker lk(&self->m_mutex);
                            self->m_inFlight.remove(folderId);
                            self->m_userMap.remove(folderId);
                        }
                        emit self->syncError(folderId, errMsg);
                        if (self->m_orch) {
                            self->m_orch->release(folderId);
                            // No explicit re-entry — orchestrator fires
                            // dispatchReady again once a slot opens.
                        } else {
                            QMetaObject::invokeMethod(self,
                                &FileSyncWorker::processNext,
                                Qt::QueuedConnection);
                        }
                    });
                return;
            }

            QVector<FileItem> batch = resp.data();
            // Fshare's API returns "pid" as the parent folder's NUMERIC id
            // (e.g. "77023976") or null, but our cache is keyed on LINKCODE
            // (e.g. "DZO3551H1GUO") — the same id the UI passes to navigateTo.
            // Without this override the cache would store children under the
            // numeric pid, and subsequent queries-by-linkcode would return
            // nothing. Since we requested exactly this folder, every item in
            // the batch is by definition a direct child of `folderId`.
            for (auto &item : batch)
                item.parentId = folderId;
            qInfo().noquote() << "[FileSyncWorker] batch folderId=" << folderId
                              << "page=" << page << "items=" << batch.size();
            totalItems += batch.size();

            QMetaObject::invokeMethod(guard.data(),
                [guard, db, folderId, userId, batch, firstBatch]() {
                    if (!guard || !db) return;
                    db->upsertFiles(userId, batch);
                    if (firstBatch)
                        emit guard->firstBatchSynced(folderId);
                });

            firstBatch = false;

            // Last page reached when the batch is smaller than a full page.
            if (batch.size() < kPageSize) break;

            ++page;
        }

        // All pages done — mark sync state and kick the next item.
        // If the folder came back completely empty, we don't know whether it's
        // truly empty or a transient API hiccup. Mark it partial so the next
        // visit re-queries via the "!state.isComplete" branch in
        // FileCacheService::listFiles — a small price to avoid getting stuck
        // showing an empty pane until kStaleSecs (60 s) expires.
        QMetaObject::invokeMethod(guard.data(),
            [guard, db, folderId, userId, totalItems]() {
                if (!guard || !db) return;
                auto *self = guard.data();
                if (totalItems > 0)
                    db->setFolderSyncComplete(userId, folderId);
                else
                    db->setFolderSyncPartial(userId, folderId);
                {
                    QMutexLocker lk(&self->m_mutex);
                    self->m_inFlight.remove(folderId);
                    self->m_userMap.remove(folderId);
                }
                emit self->folderSynced(folderId);
                if (self->m_orch) {
                    self->m_orch->release(folderId);
                } else {
                    QMetaObject::invokeMethod(self,
                        &FileSyncWorker::processNext,
                        Qt::QueuedConnection);
                }
            });
    });
}

} // namespace fsnext
