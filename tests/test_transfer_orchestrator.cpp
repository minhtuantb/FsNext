// SPDX-License-Identifier: Proprietary
//
// Unit tests for TransferOrchestrator — the central dispatcher that gates every
// download / upload / metadata task behind BudgetManager slot caps and
// PriorityScheduler ordering.
//
// BudgetManager's pure slot math is covered by test_budget_manager; here we
// exercise the *orchestration* layer: the async enqueue → dispatchReady →
// release loop across the orchestrator's own thread, plus cancel-before-dispatch
// and priority ordering when a freed slot is contended.
//
// dispatchReady is emitted on the orchestrator's internal thread and delivered
// to the test thread via a queued connection, so every assertion uses
// QTRY_COMPARE to pump the event loop until the async dispatch settles.

#include <QtTest>
#include <QSignalSpy>

#include "core/transfer/TransferOrchestrator.h"
#include "core/transfer/TransferPriority.h"

using fsnext::TransferOrchestrator;
using fsnext::TransferClass;
using fsnext::TransferPriority;
using fsnext::BudgetManager;

class TestTransferOrchestrator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // dispatchReady carries these across a thread boundary — register so
        // the queued connection (and QSignalSpy) can marshal them.
        qRegisterMetaType<fsnext::TransferClass>("fsnext::TransferClass");
        qRegisterMetaType<fsnext::TransferPriority>("fsnext::TransferPriority");
    }

    // Build a config with a single download slot and nothing else, so slot
    // contention is easy to provoke deterministically.
    static BudgetManager::Config oneDownloadSlot()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 1;
        cfg.maxUploadSlots   = 0;
        cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots   = 0;
        cfg.backgroundFloorPerPool = 0;
        cfg.metadataFloorGlobal    = 0;
        return cfg;
    }

    // Up to the per-class cap dispatches immediately; the rest stay pending.
    void dispatchesUpToSlotCap()
    {
        TransferOrchestrator orch;
        BudgetManager::Config cfg = oneDownloadSlot();
        cfg.maxDownloadSlots = 2;
        orch.setConfig(cfg);

        QSignalSpy spy(&orch, &TransferOrchestrator::dispatchReady);
        orch.enqueue("a", TransferClass::Download, TransferPriority::Interactive);
        orch.enqueue("b", TransferClass::Download, TransferPriority::Interactive);
        orch.enqueue("c", TransferClass::Download, TransferPriority::Interactive);

        QTRY_COMPARE(spy.count(), 2);          // exactly the 2 slots fill
        QTRY_COMPARE(orch.pendingCount(TransferClass::Download), 1); // c waits
    }

    // release() of a running task frees its slot and the next queued task
    // dispatches.
    void releaseDispatchesNextQueued()
    {
        TransferOrchestrator orch;
        orch.setConfig(oneDownloadSlot());

        QSignalSpy spy(&orch, &TransferOrchestrator::dispatchReady);
        orch.enqueue("first",  TransferClass::Download, TransferPriority::Interactive);
        orch.enqueue("second", TransferClass::Download, TransferPriority::Interactive);

        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("first"));

        orch.release("first");
        QTRY_COMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toString(), QStringLiteral("second"));
    }

    // cancelQueued() drops a task that has not been dispatched yet, so freeing
    // the occupied slot must NOT dispatch the cancelled id.
    void cancelQueuedRemovesBeforeDispatch()
    {
        TransferOrchestrator orch;
        orch.setConfig(oneDownloadSlot());

        QSignalSpy spy(&orch, &TransferOrchestrator::dispatchReady);
        QSignalSpy cancelSpy(&orch, &TransferOrchestrator::cancelled);

        orch.enqueue("running",   TransferClass::Download, TransferPriority::Interactive);
        orch.enqueue("cancelled", TransferClass::Download, TransferPriority::Interactive);

        QTRY_COMPARE(spy.count(), 1);                  // "running" took the slot
        QVERIFY(orch.cancelQueued("cancelled"));       // still queued → removable
        QTRY_COMPARE(cancelSpy.count(), 1);

        orch.release("running");
        // Give the scheduler a chance to (wrongly) dispatch the cancelled id.
        QTest::qWait(150);
        QCOMPARE(spy.count(), 1);                      // nothing new dispatched
        QCOMPARE(orch.pendingCount(TransferClass::Download), 0);
    }

    // When a single slot frees with two tasks waiting, the higher-priority one
    // (Interactive) wins over the lower (Background) regardless of enqueue order.
    void higherPriorityDispatchesFirst()
    {
        TransferOrchestrator orch;
        orch.setConfig(oneDownloadSlot());

        QSignalSpy spy(&orch, &TransferOrchestrator::dispatchReady);

        // Fill the only slot first so the next two genuinely queue.
        orch.enqueue("occupy", TransferClass::Download, TransferPriority::Interactive);
        QTRY_COMPARE(spy.count(), 1);

        // Enqueue low priority BEFORE high priority — ordering must come from
        // priority, not insertion order.
        orch.enqueue("bg", TransferClass::Download, TransferPriority::Background);
        orch.enqueue("hi", TransferClass::Download, TransferPriority::Interactive);

        orch.release("occupy");
        QTRY_COMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toString(), QStringLiteral("hi"));  // priority wins

        orch.release("hi");
        QTRY_COMPARE(spy.count(), 3);
        QCOMPARE(spy.at(2).at(0).toString(), QStringLiteral("bg"));  // then the leftover
    }

    // Re-enqueuing a still-queued id is a no-op (header contract) — it must not
    // double-dispatch or double-count.
    void duplicateEnqueueIsNoOp()
    {
        TransferOrchestrator orch;
        orch.setConfig(oneDownloadSlot());

        QSignalSpy spy(&orch, &TransferOrchestrator::dispatchReady);
        orch.enqueue("dup", TransferClass::Download, TransferPriority::Interactive);
        orch.enqueue("dup", TransferClass::Download, TransferPriority::Interactive);

        QTRY_COMPARE(spy.count(), 1);
        QTest::qWait(100);
        QCOMPARE(spy.count(), 1);   // the second enqueue produced nothing
    }
};

QTEST_MAIN(TestTransferOrchestrator)
#include "test_transfer_orchestrator.moc"
