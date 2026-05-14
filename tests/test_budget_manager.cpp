// SPDX-License-Identifier: Proprietary
// BudgetManager unit tests — slot caps, floor quotas, global cap.
//
// Pure logic, no Qt event loop, no QObject — BudgetManager is a plain class.
// Tests focus on the priority/floor matrix because that is the trickiest
// behaviour (a wrong floor calc starves background work or, worse, lets it
// monopolise the pool).

#include <QtTest>
#include "core/transfer/BudgetManager.h"
#include "core/transfer/TransferPriority.h"

using fsnext::BudgetManager;
using fsnext::TransferClass;
using fsnext::TransferPriority;

class TestBudgetManager : public QObject
{
    Q_OBJECT
private slots:
    // Helper — empty waiting array
    static void noWaiters(int waiting[4]) { for (int i = 0; i < 4; ++i) waiting[i] = 0; }

    void perClassCapEnforced()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 2;
        cfg.maxUploadSlots   = 1;
        cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots   = 0; // disabled
        cfg.backgroundFloorPerPool = 0;
        cfg.metadataFloorGlobal    = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY(!mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));

        QVERIFY( mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Interactive, w));
        QVERIFY(!mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Interactive, w));

        QVERIFY(!mgr.tryAcquire(TransferClass::Metadata, TransferPriority::Metadata, w));
    }

    void releaseFreesSlot()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 1; cfg.maxUploadSlots = 0; cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots = 0; cfg.backgroundFloorPerPool = 0; cfg.metadataFloorGlobal = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY(!mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        mgr.release(TransferClass::Download, TransferPriority::Interactive);
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
    }

    void globalCapBlocksWhenSumExceeded()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 8; cfg.maxUploadSlots = 8; cfg.maxMetadataSlots = 8;
        cfg.maxGlobalSlots   = 3;            // hard ceiling
        cfg.backgroundFloorPerPool = 0;
        cfg.metadataFloorGlobal    = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Metadata, TransferPriority::Metadata,    w));
        // Per-class cap is 8, but global is 3 — the next acquire must be denied
        // regardless of which pool we hit.
        QVERIFY(!mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY(!mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Interactive, w));
    }

    void globalCapZeroDisablesGlobalLimit()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 4; cfg.maxUploadSlots = 4; cfg.maxMetadataSlots = 4;
        cfg.maxGlobalSlots   = 0; // disabled — only per-class enforced
        cfg.backgroundFloorPerPool = 0;
        cfg.metadataFloorGlobal    = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        // 4 + 4 + 4 = 12 active simultaneously must be possible.
        for (int i = 0; i < 4; ++i)
            QVERIFY(mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        for (int i = 0; i < 4; ++i)
            QVERIFY(mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Interactive, w));
        for (int i = 0; i < 4; ++i)
            QVERIFY(mgr.tryAcquire(TransferClass::Metadata, TransferPriority::Metadata,    w));
        const auto u = mgr.usage();
        QCOMPARE(u.totalActive,    12);
        QCOMPARE(u.activeDownloads, 4);
        QCOMPARE(u.activeUploads,   4);
        QCOMPARE(u.activeMetadata,  4);
    }

    void backgroundFloorReservesSlotForLowerPriority()
    {
        // Floor 1 means: when Background work is queued, the highest-priority
        // tasks may not occupy more than (cap - 1) of the pool.
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 4; cfg.maxUploadSlots = 0; cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots   = 0;
        cfg.backgroundFloorPerPool = 1;
        cfg.metadataFloorGlobal    = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        // Pretend one Background task is waiting in the queue.
        w[(int)TransferPriority::Background] = 1;

        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        // Cap is 4 but floor 1 is reserved for Background → 4th Interactive denied
        QVERIFY(!mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        // …but the Background task itself can take that 4th slot.
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Background,  w));
    }

    void floorIgnoredWhenNoLowerPriorityWaiting()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 4; cfg.maxUploadSlots = 0; cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots   = 0;
        cfg.backgroundFloorPerPool = 1;
        cfg.metadataFloorGlobal    = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w); // nobody waiting

        for (int i = 0; i < 4; ++i)
            QVERIFY(mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
    }

    void metadataFloorReservesGlobalSlot()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 4; cfg.maxUploadSlots = 4; cfg.maxMetadataSlots = 2;
        cfg.maxGlobalSlots   = 4;
        cfg.backgroundFloorPerPool = 0;
        cfg.metadataFloorGlobal    = 1;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        w[(int)TransferPriority::Metadata] = 1; // a metadata crawl is queued

        // Global cap is 4, metadata floor reserves 1 → DL/UL combined ≤ 3
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QVERIFY( mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Normal,      w));
        QVERIFY(!mgr.tryAcquire(TransferClass::Upload,   TransferPriority::Normal,      w));
        // Metadata still gets its reserved slot.
        QVERIFY( mgr.tryAcquire(TransferClass::Metadata, TransferPriority::Metadata,    w));
    }

    void setConfigDoesNotEvictRunningTasks()
    {
        BudgetManager::Config cfg;
        cfg.maxDownloadSlots = 4; cfg.maxUploadSlots = 0; cfg.maxMetadataSlots = 0;
        cfg.maxGlobalSlots = 0; cfg.backgroundFloorPerPool = 0; cfg.metadataFloorGlobal = 0;
        BudgetManager mgr(cfg);

        int w[4]; noWaiters(w);
        for (int i = 0; i < 4; ++i)
            QVERIFY(mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
        QCOMPARE(mgr.usage().activeDownloads, 4);

        // Lower the cap.  In-flight tasks must NOT be retroactively evicted
        // (BudgetManager doesn't kill engines).  Counter stays at 4; new
        // acquires are denied until enough release().
        cfg.maxDownloadSlots = 2;
        mgr.setConfig(cfg);
        QCOMPARE(mgr.usage().activeDownloads, 4);
        QVERIFY(!mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));

        mgr.release(TransferClass::Download, TransferPriority::Interactive);
        mgr.release(TransferClass::Download, TransferPriority::Interactive);
        mgr.release(TransferClass::Download, TransferPriority::Interactive);
        // Now at 1 active, cap is 2 → next acquire fits.
        QVERIFY( mgr.tryAcquire(TransferClass::Download, TransferPriority::Interactive, w));
    }
};

QTEST_GUILESS_MAIN(TestBudgetManager)
#include "test_budget_manager.moc"
