// SPDX-License-Identifier: Proprietary
// PriorityScheduler unit tests — per-class FIFO priority queues plus the O(1)
// side-index that backs enqueue's duplicate rejection.
//
// Pure logic, no Qt event loop, no QObject — PriorityScheduler is a plain class
// guarded by an internal mutex. Tests focus on the behaviour that the
// TransferOrchestrator relies on: dup-rejection (so a 1000-link bulk paste
// can't double-queue an id), FIFO popFront within a priority, removeById
// keeping the side-index in sync, and the pendingCounts snapshot that feeds
// BudgetManager's floor enforcement.

#include <QtTest>
#include "core/transfer/PriorityScheduler.h"
#include "core/transfer/TransferPriority.h"

using fsnext::PriorityScheduler;
using fsnext::TransferPriority;

class TestPriorityScheduler : public QObject
{
    Q_OBJECT
private slots:
    // ── ATM-0233 — enqueue: append new id, reject duplicates ──────────────
    void enqueueAddsToPriorityQueue()
    {
        // ATM-0233 (TC-0207): a fresh id lands in its priority queue, is
        // counted in totalPending and in the matching pendingCounts slot.
        PriorityScheduler s;
        QVERIFY(s.enqueue("id1", TransferPriority::Normal));
        QCOMPARE(s.totalPending(), 1);
        QVERIFY(!s.isEmpty());

        int counts[4];
        s.pendingCounts(counts);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Normal)], 1);
        // Nothing leaked into other priorities.
        QCOMPARE(counts[static_cast<int>(TransferPriority::Interactive)], 0);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Background)], 0);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Metadata)], 0);
    }

    void enqueueRejectsDuplicateIdAcrossPriorities()
    {
        // ATM-0233 (TC-0208): the side-index dup-check is across ALL
        // priorities — re-enqueuing an existing id under a *different*
        // priority must return false and must not add a second copy.
        PriorityScheduler s;
        QVERIFY(s.enqueue("id1", TransferPriority::Normal));
        QVERIFY(!s.enqueue("id1", TransferPriority::Background));

        QCOMPARE(s.totalPending(), 1);
        int counts[4];
        s.pendingCounts(counts);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Normal)], 1);
        // The rejected re-enqueue must NOT have touched the Background queue.
        QCOMPARE(counts[static_cast<int>(TransferPriority::Background)], 0);
    }

    // ── ATM-0234 — popFront: FIFO head, empty → nullopt ───────────────────
    void popFrontReturnsFifoHeadAndUnindexes()
    {
        // ATM-0234 (TC-0209): within one priority popFront is FIFO; the popped
        // id is removed from the side-index so it can be enqueued again, while
        // the still-queued sibling keeps its slot.
        PriorityScheduler s;
        QVERIFY(s.enqueue("a", TransferPriority::Normal));
        QVERIFY(s.enqueue("b", TransferPriority::Normal));

        const auto head = s.popFront(TransferPriority::Normal);
        QVERIFY(head.has_value());
        QCOMPARE(*head, QString("a"));          // FIFO: first in, first out

        int counts[4];
        s.pendingCounts(counts);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Normal)], 1); // only b left
        QCOMPARE(s.totalPending(), 1);

        // "a" left the index → it may be re-enqueued; "b" is still indexed.
        QVERIFY(s.enqueue("a", TransferPriority::Normal));
        QVERIFY(!s.enqueue("b", TransferPriority::Normal));
    }

    void popFrontOnEmptyQueueReturnsNullopt()
    {
        // ATM-0234 (TC-0210): popping a priority with nothing queued yields
        // nullopt and leaves the (other-priority) index untouched.
        PriorityScheduler s;
        QVERIFY(s.enqueue("x", TransferPriority::Normal));

        const auto miss = s.popFront(TransferPriority::Interactive);
        QVERIFY(!miss.has_value());

        // Unrelated state is unchanged.
        QCOMPARE(s.totalPending(), 1);
        QVERIFY(!s.enqueue("x", TransferPriority::Normal)); // x still indexed
    }

    // ── ATM-0235 — pendingCounts snapshot per priority ────────────────────
    void pendingCountsReflectsPerPriorityDepth()
    {
        // ATM-0235 (TC-0211): the snapshot fills out[0..3] in
        // Interactive/Normal/Background/Metadata order with the queue depths.
        PriorityScheduler s;
        QVERIFY(s.enqueue("i1", TransferPriority::Interactive));
        QVERIFY(s.enqueue("i2", TransferPriority::Interactive));
        QVERIFY(s.enqueue("n1", TransferPriority::Normal));
        QVERIFY(s.enqueue("m1", TransferPriority::Metadata)); // Background left empty

        int counts[4];
        s.pendingCounts(counts);
        QCOMPARE(counts[0], 2); // Interactive
        QCOMPARE(counts[1], 1); // Normal
        QCOMPARE(counts[2], 0); // Background
        QCOMPARE(counts[3], 1); // Metadata
        QCOMPARE(s.totalPending(), 4);
    }

    void pendingCountsAllZeroWhenEmpty()
    {
        // ATM-0235 (TC-0212): a fresh scheduler reports zero depth on every
        // priority and isEmpty()/totalPending() agree.
        PriorityScheduler s;
        int counts[4];
        s.pendingCounts(counts);
        for (int p = 0; p < 4; ++p)
            QCOMPARE(counts[p], 0);
        QVERIFY(s.isEmpty());
        QCOMPARE(s.totalPending(), 0);
    }

    // ── removeById — index/queue stay in sync (supporting behaviour) ──────
    void removeByIdDropsTaskRegardlessOfPriority()
    {
        // removeById backs cancelQueued(): it must find the id by its indexed
        // priority, drop it from both the deque and the side-index, and report
        // hit/miss correctly. Covered alongside enqueue/popFront because the
        // side-index invariant is shared across all three.
        PriorityScheduler s;
        QVERIFY(s.enqueue("a", TransferPriority::Normal));
        QVERIFY(s.enqueue("b", TransferPriority::Normal));

        QVERIFY(s.removeById("a"));
        QVERIFY(!s.removeById("a"));   // already gone → miss
        QVERIFY(!s.removeById("zzz")); // never queued → miss

        int counts[4];
        s.pendingCounts(counts);
        QCOMPARE(counts[static_cast<int>(TransferPriority::Normal)], 1);
        QCOMPARE(s.totalPending(), 1);

        // "a" left the index; "b" is still queued and pops next.
        QVERIFY(s.enqueue("a", TransferPriority::Normal));
        const auto head = s.popFront(TransferPriority::Normal);
        QVERIFY(head.has_value());
        QCOMPARE(*head, QString("b"));
    }
};

QTEST_GUILESS_MAIN(TestPriorityScheduler)
#include "test_priority_scheduler.moc"
