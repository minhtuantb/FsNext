// SPDX-License-Identifier: Proprietary
//
// Unit tests for the TransferTask model + its TransferState/TransferType enums
// (src/core/models/TransferTask.h, src/core/models/TransferState.h).
//
// TransferTask is a plain aggregate struct — most members are trivial POD
// fields whose only "behavior" is store-then-read, which carries no logic worth
// pinning. This suite therefore concentrates on the parts that DO encode
// behaviour:
//
//   * progressPercent()  — the single computed method. It divides
//     bytesTransferred/fileSize*100 and must guard against a zero/negative
//     fileSize (avoid divide-by-zero) per the header contract.
//   * the in-struct default initialisers that the rest of the app relies on
//     (a freshly-constructed task is a Download in the Queued state, all
//     numeric progress at 0, all string fields empty, flags false). These are
//     gathered into a couple of "default invariants" tests rather than one
//     test per field.
//   * the documented lifecycle: state walks Queued → Active → Complete, and
//     the conflict-policy snapshot is just a value the task carries (no copy of
//     a live Settings object), so editing a settings value after the fact can
//     never mutate an already-stamped task.
//
// Q_ENUM_NS lives in TransferState.h, so that header is added to the test's
// source list (AUTOMOC) — the enums are used directly here, no metatype
// round-trip is exercised. Pure logic, no event loop → QTEST_GUILESS_MAIN.

#include "core/models/TransferTask.h"
#include "core/models/TransferState.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <cstdint>

using fsnext::TransferState;
using fsnext::TransferTask;
using fsnext::TransferType;

class TestTransferTask : public QObject
{
    Q_OBJECT

private slots:
    // progressPercent() — the only computed method
    void progressPercentComputesRatio();
    void progressPercentGuardsZeroAndNegativeSize();
    void progressPercentFullAndOverflow();

    // meaningful default invariants of a freshly-constructed task
    void defaultEnumsAreDownloadAndQueued();
    void defaultNumericAndStringFieldsAreZeroEmpty();
    void defaultFlagsAreFalse();

    // type / state value round-trips that carry semantic meaning
    void typeCanBeUpload();

    // documented lifecycle + snapshot-vs-live-settings invariant
    void stateWalksQueuedActiveComplete();
    void conflictPolicySnapshotIsIndependentOfLaterEdits();
};

// ── progressPercent() ───────────────────────────────────────

void TestTransferTask::progressPercentComputesRatio()
{
    // ATM-0376 / TC-0399: 500 of 1000 bytes → 50.0%
    TransferTask t;
    t.fileSize = 1000;
    t.bytesTransferred = 500;
    QCOMPARE(t.progressPercent(), 50.0);

    // A quarter done.
    t.bytesTransferred = 250;
    QCOMPARE(t.progressPercent(), 25.0);
}

void TestTransferTask::progressPercentGuardsZeroAndNegativeSize()
{
    // ATM-0357 / TC-0378: fileSize <= 0 must short-circuit to 0.0 and never
    // divide by zero, even when bytesTransferred is non-zero.
    TransferTask t;
    t.fileSize = 0;
    t.bytesTransferred = 100;
    QCOMPARE(t.progressPercent(), 0.0);

    // Defensive: a negative size (should never happen) is treated the same.
    t.fileSize = -1;
    QCOMPARE(t.progressPercent(), 0.0);
}

void TestTransferTask::progressPercentFullAndOverflow()
{
    // ATM-0376: 100% when fully transferred; large int64 sizes don't overflow
    // because the division is done in double.
    TransferTask t;
    t.fileSize = 5368709120LL;          // 5 GiB
    t.bytesTransferred = 5368709120LL;
    QCOMPARE(t.progressPercent(), 100.0);

    // Over-count (e.g. retried range overlap) yields > 100 rather than clamping;
    // pins the raw arithmetic so a future clamp is a deliberate change.
    t.bytesTransferred = t.fileSize + t.fileSize; // 200%
    QCOMPARE(t.progressPercent(), 200.0);
}

// ── default invariants ──────────────────────────────────────

void TestTransferTask::defaultEnumsAreDownloadAndQueued()
{
    // ATM-0354 / TC-0371 + ATM-0355 / TC-0373: a default-constructed task is a
    // Download sitting in the Queued state.
    TransferTask t;
    QCOMPARE(t.type, TransferType::Download);
    QCOMPARE(t.state, TransferState::Queued);
}

void TestTransferTask::defaultNumericAndStringFieldsAreZeroEmpty()
{
    // ATM-0353/0356/0357/0358/0359/0360/0366/0367/0375/0377/0378/0379/0380/0381:
    // grouped here because each is a trivial default-value check with no logic.
    TransferTask t;

    // numeric progress / size / counters all start at 0
    QCOMPARE(t.fileSize, qint64(0));
    QCOMPARE(t.bytesTransferred, qint64(0));
    QCOMPARE(t.progress, 0.0);
    QCOMPARE(t.speed, 0.0);
    QCOMPARE(t.retryCount, 0);
    QCOMPARE(t.completedAt, qint64(0));
    QCOMPARE(t.speedLimitBps, qint64(0));

    // segments default is 16 (documented "parallel HTTP Range streams per file")
    QCOMPARE(t.segments, 16);
    // fileConflictPolicy default is 0 (== rename) per ADR 003 D7
    QCOMPARE(t.fileConflictPolicy, 0);

    // string identity / path / routing fields all start empty
    QVERIFY(t.id.isEmpty());
    QVERIFY(t.fileName.isEmpty());
    QVERIFY(t.linkcode.isEmpty());
    QVERIFY(t.localPath.isEmpty());
    QVERIFY(t.password.isEmpty());
    QVERIFY(t.realUrl.isEmpty());
    QVERIFY(t.sourcePath.isEmpty());
    QVERIFY(t.folderId.isEmpty());
    QVERIFY(t.eta.isEmpty());
    QVERIFY(t.errorMessage.isEmpty());
}

void TestTransferTask::defaultFlagsAreFalse()
{
    // ATM-0369 / TC-0394, ATM-0370 / TC-0396, ATM-0371: upload/sync flags all
    // default to false (public upload, manual task, normal link).
    TransferTask t;
    QCOMPARE(t.secured, false);
    QCOMPARE(t.directLink, false);
    QCOMPARE(t.isSyncTask, false);
}

// ── semantic value round-trips ──────────────────────────────

void TestTransferTask::typeCanBeUpload()
{
    // ATM-0354 / TC-0372: the type discriminates download vs upload direction.
    TransferTask t;
    t.type = TransferType::Upload;
    QCOMPARE(t.type, TransferType::Upload);
    QVERIFY(t.type != TransferType::Download);
}

// ── lifecycle + snapshot invariant ──────────────────────────

void TestTransferTask::stateWalksQueuedActiveComplete()
{
    // ATM-0355 / TC-0374: the documented happy-path lifecycle. We drive the
    // field directly (the real driver is the engine, exercised at integration
    // level) and assert each leg of Queued → Active → Complete, plus that the
    // states are distinct enumerators.
    TransferTask t;
    QCOMPARE(t.state, TransferState::Queued);

    t.state = TransferState::Active;
    QCOMPARE(t.state, TransferState::Active);

    t.state = TransferState::Complete;
    QCOMPARE(t.state, TransferState::Complete);

    QVERIFY(TransferState::Queued != TransferState::Active);
    QVERIFY(TransferState::Active != TransferState::Complete);
}

void TestTransferTask::conflictPolicySnapshotIsIndependentOfLaterEdits()
{
    // ATM-0363 / TC-0387 + TC-0388: fileConflictPolicy is a value snapshotted
    // onto the task at enqueue time (ADR 003 D7) — it is NOT a reference to a
    // live Settings object. So once stamped, mutating an unrelated settings
    // value cannot retroactively change the in-flight task's policy.
    //
    // We model the "settings" side as a plain int (the task carries only the
    // copied value, by construction). Stamp 0, then "change settings" to 2, and
    // confirm the already-stamped task still reads 0.
    int liveSettingsPolicy = 0;

    TransferTask t;
    t.fileConflictPolicy = liveSettingsPolicy;   // snapshot at enqueue
    QCOMPARE(t.fileConflictPolicy, 0);

    liveSettingsPolicy = 2;                       // user edits Settings later
    QCOMPARE(t.fileConflictPolicy, 0);            // task is unaffected

    // And a task stamped with each of the four documented policies keeps it.
    for (int p : {0, 1, 2, 3}) {
        TransferTask u;
        u.fileConflictPolicy = p;
        QCOMPARE(u.fileConflictPolicy, p);
    }
}

QTEST_GUILESS_MAIN(TestTransferTask)
#include "test_transfer_task.moc"
