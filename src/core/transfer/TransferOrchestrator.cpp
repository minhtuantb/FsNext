#include "TransferOrchestrator.h"
#include "PriorityScheduler.h"
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QDebug>

namespace fsnext {

// ---------------------------------------------------------------------------
// Impl — lives inside a dedicated QThread. All scheduler/budget mutation is
// marshaled through this thread via Q_INVOKABLE methods on the public facade,
// so the three PriorityScheduler instances and the BudgetManager never see
// concurrent access from QML / producer code-paths.
//
// We also keep a small "inflight" map so release(id) can figure out which
// class and priority the grant was for, without forcing callers to remember.
// ---------------------------------------------------------------------------
struct TransferOrchestrator::Impl {
    QThread           thread;
    BudgetManager     budget;
    PriorityScheduler schedulers[3];   // indexed by TransferClass

    struct Grant { TransferClass cls; TransferPriority prio; };
    QMutex             inflightMx;
    QHash<QString,Grant> inflight;     // id → (cls, prio) of active grants

    Impl() = default;
};

TransferOrchestrator::TransferOrchestrator(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    // Move ourselves to a dedicated worker thread so all Q_INVOKABLE calls
    // from producers get marshaled via QueuedConnection — no scheduler work
    // ever runs on the UI thread.
    d->thread.setObjectName(QStringLiteral("TransferOrchestrator"));
    moveToThread(&d->thread);
    d->thread.start();
}

TransferOrchestrator::~TransferOrchestrator()
{
    // Shut down the worker thread BEFORE the pimpl dies — otherwise pending
    // queued events could touch freed state.
    d->thread.quit();
    d->thread.wait();
}

void TransferOrchestrator::setConfig(const BudgetManager::Config &cfg)
{
    // BudgetManager is thread-safe internally.  After mutating the cap we
    // post a tick so queued work can pick up the new budget.
    d->budget.setConfig(cfg);
    QMetaObject::invokeMethod(this, "scheduleTick", Qt::QueuedConnection);
}

void TransferOrchestrator::scheduleTick()
{
    // Runs on the orchestrator thread (always — Qt::QueuedConnection).
    auto *impl = d.get();
    auto tick = [this, impl](TransferClass cls) {
        auto &sched = impl->schedulers[static_cast<int>(cls)];
        while (true) {
            int pending[4] = {0,0,0,0};
            sched.pendingCounts(pending);
            if (pending[0] + pending[1] + pending[2] + pending[3] == 0) return;
            bool dispatched = false;
            for (int p = 0; p < 4; ++p) {
                if (pending[p] == 0) continue;
                const auto prio = static_cast<TransferPriority>(p);
                if (!impl->budget.tryAcquire(cls, prio, pending)) continue;
                auto idOpt = sched.popFront(prio);
                if (!idOpt) { impl->budget.release(cls, prio); continue; }
                {
                    QMutexLocker lk(&impl->inflightMx);
                    impl->inflight.insert(*idOpt, { cls, prio });
                }
                emit dispatchReady(*idOpt, cls, prio);
                dispatched = true;
                break;
            }
            if (!dispatched) return;
        }
    };
    tick(TransferClass::Download);
    tick(TransferClass::Upload);
    tick(TransferClass::Metadata);
}

BudgetManager::Config TransferOrchestrator::config() const
{
    return d->budget.config();
}

// ---------------------------------------------------------------------------
// Scheduling core — see scheduleTick() below. All dispatch decisions go
// through it so there is exactly one code-path that can emit dispatchReady
// and touch BudgetManager / scheduler state concurrently.
// ---------------------------------------------------------------------------

void TransferOrchestrator::enqueue(const QString &id, TransferClass cls,
                                   TransferPriority prio)
{
    if (id.isEmpty()) return;   // empty ids reserved for internal ticks
    // Always marshal to the orchestrator thread so dispatchReady fires from
    // that thread, not from the caller's stack.  Producers who connect with
    // Qt::AutoConnection get a clean queued delivery to their own thread.
    QMetaObject::invokeMethod(this, [this, id, cls, prio]() {
        if (!d->schedulers[static_cast<int>(cls)].enqueue(id, prio)) return;
        scheduleTick();
    }, Qt::QueuedConnection);
}

void TransferOrchestrator::release(const QString &id)
{
    QMetaObject::invokeMethod(this, [this, id]() {
        Impl::Grant g{};
        bool found = false;
        {
            QMutexLocker lk(&d->inflightMx);
            auto it = d->inflight.find(id);
            if (it != d->inflight.end()) {
                g = it.value();
                d->inflight.erase(it);
                found = true;
            }
        }
        if (!found) return;   // released twice or never dispatched (no-op)
        d->budget.release(g.cls, g.prio);
        scheduleTick();
    }, Qt::QueuedConnection);
}

bool TransferOrchestrator::cancelQueued(const QString &id)
{
    // Synchronous — callers want a yes/no answer so they can decide whether
    // to fall through to release().  Schedulers are internally thread-safe.
    bool removed = false;
    for (int c = 0; c < 3; ++c) {
        if (d->schedulers[c].removeById(id)) { removed = true; break; }
    }
    if (removed) {
        // Emit on orchestrator thread to keep signal semantics uniform.
        QMetaObject::invokeMethod(this, [this, id]() { emit cancelled(id); },
                                  Qt::QueuedConnection);
    }
    return removed;
}

BudgetManager::Usage TransferOrchestrator::usage() const
{
    return d->budget.usage();
}

int TransferOrchestrator::pendingCount(TransferClass cls) const
{
    return d->schedulers[static_cast<int>(cls)].totalPending();
}

} // namespace fsnext
