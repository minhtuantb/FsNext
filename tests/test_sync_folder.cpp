// SPDX-License-Identifier: Proprietary
//
// SyncFolder model unit tests (KEY=test_sync_folder).
//
// SyncFolder.h is a header-only POD: SyncFolder / SyncFileEntry structs plus the
// SyncFileState + SyncActivityKind enums.  There is no behaviour to drive, so
// these tests deliberately focus on the *invariants that the rest of the codebase
// relies on* rather than trivial assign-then-read of every QString field:
//
//   * the documented member defaults — they are the v5/v6 migration contract
//     ("older folders missing these keys read back the documented defaults"), so
//     a silent default change is a real regression;
//   * the persisted enum integer values — both enums carry a "never reorder /
//     values persisted as ints" comment in the header (SyncRepository writes them
//     raw to QSettings), so the numeric layout is load-bearing;
//   * the SyncFileEntry.folderId ⇄ SyncFolder.id foreign-key round-trip.
//
// Trivial PODs that are pure QString/qint64 storage with no default or invariant
// (localPath / fshareFolderName arbitrary values, relPath sub-paths, createdAt /
// lastScanAt / uploadedAt timestamps, ignorePatterns) are intentionally NOT given
// one-test-per-field — that would only re-test QString's assignment operator.
// Their defaults are still covered inside the grouped "defaults" tests below.

#include <QtTest>
#include "core/models/SyncFolder.h"

using fsnext::SyncFolder;
using fsnext::SyncFileEntry;
using fsnext::SyncFileState;
using fsnext::SyncActivityKind;
using fsnext::SyncActivityEntry;

class TestSyncFolder : public QObject
{
    Q_OBJECT
private slots:

    // ATM-0335 / ATM-0337 / ATM-0338 / ATM-0339 (+ ATM-0342/0344 P2 defaults).
    // The documented default set is the migration contract for folders persisted
    // before per-folder settings existed — assert all of it in one place.
    void syncFolderDefaults()
    {
        // ATM-0338 / ATM-0339 / ATM-0335 / ATM-0337 / ATM-0342 / ATM-0344
        const SyncFolder f{};
        QVERIFY(f.id.isEmpty());                 // ATM-0335 TC-0346
        QVERIFY(f.localPath.isEmpty());
        QVERIFY(f.fshareFolderName.isEmpty());   // ATM-0337 TC-0350
        QVERIFY(f.ignorePatterns.isEmpty());     // ATM-0343
        QCOMPARE(f.enabled, true);               // ATM-0338 TC-0351
        QCOMPARE(f.deleteAfterUpload, false);    // ATM-0339 TC-0353
        QCOMPARE(f.watchSubfolders, true);       // ATM-0342: v5 recursive default
        // ATM-0344: default cap is 5 MiB/s and MUST equal the literal the header
        // documents (SyncService::kSpeedLimitBps) so legacy folders are identical.
        QCOMPARE(f.speedLimitBps, qint64(5LL * 1024 * 1024));
        QCOMPARE(f.speedLimitBps, qint64(5242880));
        // createdAt / lastScanAt default-construct to an invalid QDateTime.
        QVERIFY(!f.createdAt.isValid());
        QVERIFY(!f.lastScanAt.isValid());
    }

    // ATM-0335 / ATM-0338 / ATM-0339: the flags/id are plain mutable members.
    // One grouped round-trip stands in for the per-field "gán rồi đọc lại" cases
    // (TC-0345 id, TC-0352 enabled=false, TC-0354 deleteAfterUpload=true).
    void syncFolderMutationRoundTrip()
    {
        // ATM-0335 TC-0345 / ATM-0338 TC-0352 / ATM-0339 TC-0354
        SyncFolder f{};
        f.id               = QStringLiteral("550e8400-e29b-41d4-a716-446655440000");
        f.enabled          = false;
        f.deleteAfterUpload = true;
        f.watchSubfolders  = false;
        f.speedLimitBps    = 0; // 0 = unlimited per header

        QCOMPARE(f.id, QStringLiteral("550e8400-e29b-41d4-a716-446655440000"));
        QCOMPARE(f.enabled, false);
        QCOMPARE(f.deleteAfterUpload, true);
        QCOMPARE(f.watchSubfolders, false);
        QCOMPARE(f.speedLimitBps, qint64(0));
    }

    // ATM-0345 / ATM-0347 / ATM-0348 / ATM-0349 / ATM-0351 / ATM-0352:
    // SyncFileEntry default state — the "freshly detected, not yet uploaded" shape.
    void syncFileEntryDefaults()
    {
        // ATM-0345 TC-0356 / ATM-0347 TC-0360 / ATM-0348 TC-0362 /
        // ATM-0349 TC-0364 / ATM-0351 / ATM-0352 TC-0368
        const SyncFileEntry e{};
        QVERIFY(e.folderId.isEmpty());            // ATM-0345 TC-0356
        QVERIFY(e.relPath.isEmpty());
        QCOMPARE(e.size, qint64(0));              // ATM-0347 TC-0360
        QCOMPARE(e.mtime, qint64(0));             // ATM-0348 TC-0362
        QVERIFY(e.linkcode.isEmpty());            // ATM-0349 TC-0364: empty until uploaded
        QVERIFY(!e.uploadedAt.isValid());         // ATM-0350
        QCOMPARE(e.state, SyncFileState::Pending);// ATM-0351: detected → Pending
        QVERIFY(e.errorMessage.isEmpty());        // ATM-0352 TC-0368: empty unless Failed
    }

    // ATM-0347 / ATM-0348: size is a 64-bit field — confirm it carries values
    // beyond 32 bits without truncation (real sync files can exceed 2 GiB).
    void syncFileEntrySizeIs64Bit()
    {
        // ATM-0347 TC-0359 / ATM-0348 TC-0361
        SyncFileEntry e{};
        e.size  = 1099511627776LL; // 1 TiB
        e.mtime = 1716960000;      // seconds since epoch
        QCOMPARE(e.size, qint64(1099511627776LL));
        QCOMPARE(e.mtime, qint64(1716960000));
    }

    // ATM-0345 / ATM-0346: SyncFileEntry.folderId is the foreign key back to
    // SyncFolder.id — assert the round-trip keeps them equal (the only relational
    // "logic" the model expresses), plus relPath stores a relative path verbatim.
    void syncFileEntryForeignKeyLink()
    {
        // ATM-0345 TC-0355 / ATM-0346 TC-0357 / ATM-0346 TC-0358
        SyncFolder folder{};
        folder.id = QStringLiteral("uuid-A");

        SyncFileEntry entry{};
        entry.folderId = folder.id;
        entry.relPath  = QStringLiteral("2024/anh01.jpg"); // phase-2 sub-path

        QCOMPARE(entry.folderId, folder.id);
        QCOMPARE(entry.folderId, QStringLiteral("uuid-A"));
        QCOMPARE(entry.relPath, QStringLiteral("2024/anh01.jpg"));
    }

    // ATM-0351: SyncFileState is persisted to QSettings as a raw int (header:
    // "values persisted to QSettings as ints"). The numeric layout is therefore a
    // wire/storage contract — pin it so a reorder can't silently corrupt history.
    void syncFileStateEnumValuesAreStable()
    {
        // ATM-0351 TC-0365 / ATM-0351 TC-0366
        QCOMPARE(static_cast<int>(SyncFileState::Pending),   0);
        QCOMPARE(static_cast<int>(SyncFileState::Uploading), 1);
        QCOMPARE(static_cast<int>(SyncFileState::Synced),    2);
        QCOMPARE(static_cast<int>(SyncFileState::Failed),    3);
        QCOMPARE(static_cast<int>(SyncFileState::Missing),   4);
    }

    // ATM-0351 / ATM-0352: the lifecycle transition expressed as the entry's state
    // field. With no engine in a pure-model test, the meaningful assertion is that
    // each state is assignable and the Failed state co-exists with an errorMessage
    // while Synced carries a linkcode and no error — the documented coupling.
    void syncFileEntryStateTransitions()
    {
        // ATM-0351 TC-0365 / ATM-0352 TC-0367
        SyncFileEntry e{};
        QCOMPARE(e.state, SyncFileState::Pending);

        e.state = SyncFileState::Uploading;
        QCOMPARE(e.state, SyncFileState::Uploading);

        // Synced: linkcode set, no error.
        e.state    = SyncFileState::Synced;
        e.linkcode = QStringLiteral("ABC123");
        e.errorMessage.clear();
        QCOMPARE(e.state, SyncFileState::Synced);
        QVERIFY(!e.linkcode.isEmpty());
        QVERIFY(e.errorMessage.isEmpty());

        // Failed: errorMessage populated.
        e.state        = SyncFileState::Failed;
        e.errorMessage = QStringLiteral("HTTP 500");
        QCOMPARE(e.state, SyncFileState::Failed);
        QVERIFY(!e.errorMessage.isEmpty());

        // Missing: was synced, file gone locally.
        e.state = SyncFileState::Missing;
        QCOMPARE(e.state, SyncFileState::Missing);
    }

    // SyncActivityKind is also persisted as raw ints (header: "Kind values are
    // stable: persisted as ints, never reorder"). Pin the layout and the default.
    void syncActivityKindEnumStableAndDefault()
    {
        // (covered alongside ATM-0351 — same persisted-int invariant class)
        QCOMPARE(static_cast<int>(SyncActivityKind::Uploaded),      0);
        QCOMPARE(static_cast<int>(SyncActivityKind::Failed),        1);
        QCOMPARE(static_cast<int>(SyncActivityKind::DeletedLocal),  2);
        QCOMPARE(static_cast<int>(SyncActivityKind::FolderAdded),   3);
        QCOMPARE(static_cast<int>(SyncActivityKind::FolderRemoved), 4);

        const SyncActivityEntry a{};
        QCOMPARE(a.kind, SyncActivityKind::Uploaded); // documented default
        QCOMPARE(a.sizeBytes, qint64(0));
        QVERIFY(a.folderId.isEmpty());
        QVERIFY(!a.at.isValid());
    }
};

QTEST_GUILESS_MAIN(TestSyncFolder)
#include "test_sync_folder.moc"
