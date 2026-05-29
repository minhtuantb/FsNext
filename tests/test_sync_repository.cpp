// SPDX-License-Identifier: Proprietary
//
// Unit tests for SyncRepository — the per-user QSettings store backing the
// auto-sync folder list, the master auto-sync toggle, and the rolling activity
// feed. (NOTE: despite the "SQLite/CRUD" framing in some planning docs, the
// real implementation persists to QSettings, not SQLite. These tests exercise
// the actual code as written in SyncRepository.cpp.)
//
// Behaviour under test (not trivial field round-trips):
//   - ATM-0266: per-folder settings v6 compatibility — folders written WITHOUT
//     the v6 keys (watchSubfolders/ignorePatterns/speedLimitBps) read back the
//     hardcoded defaults (true / "" / 5 MiB-per-second), while folders that DID
//     persist those keys read back the stored values unchanged.
//   - ATM-0267: userId scoping — an empty userId disables all reads/writes
//     (loadFolders -> empty, saveFolder -> no-op), and data saved under one user
//     is invisible to another (no cross-user leakage), recoverable on switch-back.
//   - ATM-0268: autoSyncEnabled master toggle — defaults to true, persists per
//     user under SyncByUser/<id>/autoSyncEnabled, and setAutoSyncEnabled is a
//     no-op when userId is empty.
//   - ATM-0269 (bonus): activity-log rolling FIFO cap — appendActivity prepends
//     newest-first and truncates to kActivityCap (50) oldest-dropped.
//
// HERMETIC ISOLATION: SyncRepository uses a default-constructed QSettings, which
// resolves against QCoreApplication::organizationName()/applicationName(). The
// real app uses NativeFormat (Windows registry); to keep this test off the
// developer's real config we force IniFormat + redirect the UserScope ini path
// to a throwaway QTemporaryDir and clear the store before each test, so every
// case starts from an empty store. Pure synchronous code — no event loop,
// no network, no QtConcurrent.

#include <QtTest>
#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDateTime>

#include "core/repositories/SyncRepository.h"
#include "core/models/SyncFolder.h"

using fsnext::SyncRepository;
using fsnext::SyncFolder;
using fsnext::SyncActivityEntry;
using fsnext::SyncActivityKind;

namespace {
constexpr qint64 kFiveMiB = 5LL * 1024 * 1024;
const QString kOrg = QStringLiteral("FsNextTest");
const QString kApp = QStringLiteral("SyncRepositoryTest");
} // namespace

class TestSyncRepository : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(kOrg);
        QCoreApplication::setApplicationName(kApp);
        // Redirect the IniFormat/UserScope store (where SyncRepository's
        // default-constructed QSettings lands) to a throwaway dir so this test
        // never reads or writes the developer's real config.
        QVERIFY(m_tmp.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tmp.path());
    }

    void init()
    {
        // Empty store before every test so each case starts from scratch.
        // Use a default-constructed QSettings to match SyncRepository's own
        // member (default ctor -> default format = IniFormat redirected above +
        // the app org/app name). QSettings(kOrg, kApp) would pick NativeFormat
        // (registry on Windows) and miss the repo's store entirely.
        QSettings().clear();
    }

    // ── ATM-0266 / TC-0275: pre-v6 folder missing keys -> hardcoded defaults ──
    void preV6Folder_missingKeys_fallbackDefaults()
    {
        // ATM-0266
        // Simulate a v5 folder: only the id + localPath keys exist on disk, none
        // of the v6 per-folder keys. We write the raw QSettings directly (rather
        // than via saveFolder, which always writes the v6 keys) to reproduce the
        // upgrade scenario faithfully.
        {
            QSettings s;
            const QString base = QStringLiteral("SyncByUser/uX");
            s.setValue(base + QStringLiteral("/folders"), QStringList{QStringLiteral("f1")});
            const QString k = base + QStringLiteral("/folder/f1");
            s.setValue(k + QStringLiteral("/localPath"), QStringLiteral("D:/Important"));
            // Deliberately NOT setting watchSubfolders/ignorePatterns/speedLimitBps.
            s.sync();
        }

        SyncRepository repo;
        repo.setUserId(QStringLiteral("uX"));
        const QVector<SyncFolder> folders = repo.loadFolders();

        QCOMPARE(folders.size(), 1);
        const SyncFolder &f = folders.first();
        QCOMPARE(f.id, QStringLiteral("f1"));
        QCOMPARE(f.localPath, QStringLiteral("D:/Important"));
        // The documented v5-equivalent fallbacks.
        QCOMPARE(f.watchSubfolders, true);
        QVERIFY(f.ignorePatterns.isEmpty());
        QCOMPARE(f.speedLimitBps, kFiveMiB);
    }

    // ── ATM-0266 / TC-0276: saved per-folder values are NOT overwritten ───────
    void savedFolder_perFolderValues_roundTrip()
    {
        // ATM-0266
        SyncRepository repo;
        repo.setUserId(QStringLiteral("uX"));

        SyncFolder f;
        f.id              = QStringLiteral("f1");
        f.localPath       = QStringLiteral("D:/Photos");
        f.watchSubfolders = false;                       // non-default
        f.ignorePatterns  = QStringLiteral("*.tmp");     // non-default
        f.speedLimitBps   = 1024 * 1024;                 // 1 MiB/s, non-default
        repo.saveFolder(f);

        const QVector<SyncFolder> folders = repo.loadFolders();
        QCOMPARE(folders.size(), 1);
        const SyncFolder &g = folders.first();
        // Stored values survive — no silent fallback to the hardcoded defaults.
        QCOMPARE(g.watchSubfolders, false);
        QCOMPARE(g.ignorePatterns, QStringLiteral("*.tmp"));
        QCOMPARE(g.speedLimitBps, static_cast<qint64>(1024 * 1024));
    }

    // ── ATM-0267 / TC-0277: empty userId disables reads/writes ────────────────
    void emptyUserId_disablesReadsAndWrites()
    {
        // ATM-0267
        SyncRepository repo;
        repo.setUserId(QString());   // empty

        SyncFolder f;
        f.id        = QStringLiteral("f1");
        f.localPath = QStringLiteral("D:/Anything");
        repo.saveFolder(f);          // must be a no-op

        // loadFolders returns empty for empty userId regardless of store state.
        QVERIFY(repo.loadFolders().isEmpty());

        // Confirm nothing was written: switching to a real user must still see
        // an empty list (the no-op saveFolder didn't leak under any key).
        repo.setUserId(QStringLiteral("uReal"));
        QVERIFY(repo.loadFolders().isEmpty());
    }

    // ── ATM-0267 / TC-0278: data scoped per user (no cross-user leakage) ──────
    void folders_scopedPerUser_noLeakage()
    {
        // ATM-0267
        SyncRepository repo;

        repo.setUserId(QStringLiteral("uA"));
        SyncFolder f1;
        f1.id        = QStringLiteral("f1");
        f1.localPath = QStringLiteral("D:/A");
        repo.saveFolder(f1);

        // A different user must not see uA's folder.
        repo.setUserId(QStringLiteral("uB"));
        QVERIFY(repo.loadFolders().isEmpty());

        // Switching back to uA recovers the folder.
        repo.setUserId(QStringLiteral("uA"));
        const QVector<SyncFolder> back = repo.loadFolders();
        QCOMPARE(back.size(), 1);
        QCOMPARE(back.first().id, QStringLiteral("f1"));
        QCOMPARE(back.first().localPath, QStringLiteral("D:/A"));
    }

    // ── ATM-0268 / TC-0279: autoSyncEnabled default true + per-user persist ───
    void autoSyncEnabled_defaultTrue_persistsPerUserKey()
    {
        // ATM-0268
        SyncRepository repo;
        repo.setUserId(QStringLiteral("u1"));

        // Default when never set.
        QCOMPARE(repo.autoSyncEnabled(), true);

        repo.setAutoSyncEnabled(false);
        QCOMPARE(repo.autoSyncEnabled(), false);

        // Stored under the documented per-user key.
        {
            QSettings s;
            QVERIFY(s.contains(QStringLiteral("SyncByUser/u1/autoSyncEnabled")));
            QCOMPARE(s.value(QStringLiteral("SyncByUser/u1/autoSyncEnabled")).toBool(), false);
        }

        // A second user keeps its own default (true) — the toggle is scoped.
        repo.setUserId(QStringLiteral("u2"));
        QCOMPARE(repo.autoSyncEnabled(), true);
    }

    // ── ATM-0268 / TC-0280: setAutoSyncEnabled no-op when userId empty ────────
    void autoSyncEnabled_emptyUserId_noOpReturnsTrue()
    {
        // ATM-0268
        SyncRepository repo;
        repo.setUserId(QString());

        repo.setAutoSyncEnabled(false);   // no-op — must not write
        // Empty-userId branch always reports true (caller treats no-user as off).
        QCOMPARE(repo.autoSyncEnabled(), true);

        // Nothing persisted: a real user still sees the default true.
        repo.setUserId(QStringLiteral("u1"));
        QCOMPARE(repo.autoSyncEnabled(), true);
    }

    // ── ATM-0269 (bonus): activity log rolling FIFO cap ───────────────────────
    void activityLog_rollingCap_newestFirst()
    {
        // ATM-0269
        SyncRepository repo;
        repo.setUserId(QStringLiteral("u1"));

        QVERIFY(repo.loadActivity().isEmpty());

        // Append more than the cap; entries carry an identifying relPath so we
        // can assert ordering and which ones survived truncation.
        const int total = SyncRepository::kActivityCap + 10;
        for (int i = 0; i < total; ++i) {
            SyncActivityEntry e;
            e.kind      = SyncActivityKind::Uploaded;
            e.folderId  = QStringLiteral("f1");
            e.relPath   = QStringLiteral("file%1.bin").arg(i);
            e.sizeBytes = i;
            e.at        = QDateTime::fromSecsSinceEpoch(1700000000 + i);
            repo.appendActivity(e);
        }

        const QVector<SyncActivityEntry> log = repo.loadActivity();
        // Capped at kActivityCap, oldest dropped.
        QCOMPARE(log.size(), SyncRepository::kActivityCap);
        // Newest-first: the last appended entry is at the front.
        QCOMPARE(log.first().relPath, QStringLiteral("file%1.bin").arg(total - 1));
        // The oldest survivor is entry index (total - kActivityCap).
        QCOMPARE(log.last().relPath,
                 QStringLiteral("file%1.bin").arg(total - SyncRepository::kActivityCap));

        // clearActivity empties the feed.
        repo.clearActivity();
        QVERIFY(repo.loadActivity().isEmpty());
    }

private:
    QTemporaryDir m_tmp;
};

QTEST_GUILESS_MAIN(TestSyncRepository)
#include "test_sync_repository.moc"
