#pragma once

// TransferOrchestrator — central dispatcher for every task that consumes
// network / disk / API resources.
//
// Responsibilities
// ----------------
//   • Own the BudgetManager (slot caps, floor quotas, global cap).
//   • Own one PriorityScheduler per TransferClass (DL / UL / Meta).
//   • Accept enqueue() from any producer (TransferService for user DL/UL,
//     SyncService for auto-sync UL, FileSyncWorker for metadata crawl).
//   • Emit dispatchReady(id, class, priority) ON THE PRODUCER'S THREAD when
//     a slot is available — producer then does the actual session-create +
//     engine-spawn work (that logic stays in the producer; this class only
//     decides WHEN).
//   • Accept release(id) when the task finishes / fails / is cancelled.
//   • Re-evaluate after every enqueue / release event.
//
// What it does NOT do (deliberate)
// --------------------------------
//   • No knowledge of TransferTask, DownloadEngine, UploadEngine, QThread
//     lifecycle, HTTP sessions, bytes-on-disk, etc.  Those remain in
//     TransferService / SyncService / FileSyncWorker.
//   • Phase 1 does not enforce bandwidth or I/O tokens — only slots.  The
//     acquire/release API is shaped so Phase 2 can add byte-rate leases
//     without breaking callers.
//
// Thread model
// ------------
//   TransferOrchestrator runs on its OWN QThread (created in the ctor).
//   All public methods are Q_INVOKABLE and marshal through the orchestrator's
//   event loop — callers use them directly (Qt auto-selects QueuedConnection
//   for cross-thread invocation).
//
//   dispatchReady signal is emitted FROM the orchestrator thread; producers
//   MUST connect to it with Qt::QueuedConnection so the dispatch callback
//   runs on the producer's own (typically main) thread.
//
// Lifecycle rule
// --------------
//   For every successful enqueue() the producer MUST call either:
//     • release(id) after its task finishes / fails, OR
//     • cancelQueued(id) BEFORE dispatchReady fires (then release is not
//       needed).
//   A dispatchReady signal without a matching release() leaks a slot.

#include "BudgetManager.h"
#include "TransferPriority.h"
#include <QObject>
#include <QString>
#include <memory>

namespace fsnext {

class TransferOrchestrator : public QObject {
    Q_OBJECT
public:
    explicit TransferOrchestrator(QObject *parent = nullptr);
    ~TransferOrchestrator() override;

    // Replace the running budget config (slot caps, floors).  Thread-safe.
    // Takes effect on the next scheduling tick — does not preempt anything
    // currently running.  (Preemption via "soft token expiry" is Phase 2.)
    Q_INVOKABLE void setConfig(const fsnext::BudgetManager::Config &cfg);
    BudgetManager::Config config() const;

    // Submit a task to the dispatcher.  When a slot is available (subject to
    // class cap, global cap, and floor quota), `dispatchReady(id, cls, prio)`
    // fires.
    //
    //   id   — caller-owned unique key (e.g. TransferTask::id).  Re-using a
    //          still-queued id is a no-op.
    //   cls  — pool the task will consume (Download / Upload / Metadata).
    //   prio — scheduling priority within that pool.
    //
    // Returns immediately (request is queued on the orchestrator thread).
    Q_INVOKABLE void enqueue(const QString &id, fsnext::TransferClass cls,
                             fsnext::TransferPriority prio);

    // Caller finished / failed / aborted a dispatched task — free the slot.
    // Safe to call from any thread.  No-op if id is unknown.
    Q_INVOKABLE void release(const QString &id);

    // Cancel a task that has been enqueued but has NOT yet received
    // dispatchReady.  Returns true if found-and-removed.  If the task has
    // already been dispatched, the caller must call release() instead (and
    // actually stop the engine itself — orchestrator cannot reach into the
    // engine).
    Q_INVOKABLE bool cancelQueued(const QString &id);

    // Telemetry snapshot (for status-bar / debug overlay).
    BudgetManager::Usage usage() const;
    int pendingCount(TransferClass cls) const;

signals:
    // Emitted on the orchestrator's internal thread when a task is cleared
    // to run.  Connect with Qt::QueuedConnection from the producer thread.
    //
    // Contract: by the time your slot returns, either you have STARTED the
    // actual work (engine->start, session-create kickoff, etc.) OR you call
    // release(id) back — otherwise the slot leaks.
    void dispatchReady(const QString &id, fsnext::TransferClass cls,
                       fsnext::TransferPriority prio);

    // Emitted when cancelQueued() succeeds (informational; caller already
    // knows, but this lets the TransferListModel react uniformly).
    void cancelled(const QString &id);

private slots:
    // Internal tick — runs dispatchPool across all classes.  Posted to the
    // orchestrator thread from setConfig() so a config change re-evaluates
    // the queues without blocking.
    void scheduleTick();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace fsnext
