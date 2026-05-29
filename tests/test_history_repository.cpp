// SPDX-License-Identifier: Proprietary
//
// Integration tests for HistoryRepository — the per-user download/upload
// history facade that sits on top of TransferHistoryDb (SQLite) and performs a
// one-shot legacy-JSON → SQLite migration on first access.
//
// KEY="test_history_repository" — covers ATM-0261..ATM-0265:
//   • loadDownloadHistory: JSON→SQLite migration + newest-first ordering, and
//     the m_migratedKeys short-circuit (migrate-once semantics).
//   • upsertTask: single-row insert then ON CONFLICT(id) DO UPDATE (no dup row).
//   • saveProgressSnapshot: cheap UPDATE of progress_json + state, and the
//     "unknown row → true but 0 rows" edge.
//   • loadInFlight: returns only Queued/Active/Paused rows + their snapshots.
//   • trimToMostRecent: hard-cap keep-N, keep<=0 guard, and user+type scoping.
//
// Hermetic strategy:
//   HistoryRepository::dbPath() / legacyJsonPath() derive from
//   QStandardPaths::AppDataLocation (the real app stores under
//   %APPDATA%/.../FshareNext/history). We flip QStandardPaths::setTestModeEnabled
//   so AppDataLocation points into a throwaway test sandbox, then wipe the
//   history dir in init()/cleanup() so every test starts from an empty DB and
//   no legacy JSON. The repo itself is recreated per-test so m_migratedKeys
//   (the migrate-once cache) is fresh each time. Each test still uses a distinct
//   userId for defence-in-depth against any residue.
//
// Note on TC-0267 ("upsertTask false when DB not open"): HistoryRepository::
// upsertTask() always ensureOpen()s first, so it cannot observe a closed DB.
// The contract under test — an unopened store rejects the write — lives one
// layer down, so we pin it directly against a never-open()ed TransferHistoryDb.

#include "core/repositories/HistoryRepository.h"
#include "core/repositories/TransferHistoryDb.h"
#include "core/models/TransferTask.h"
#include "core/models/TransferState.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <QVector>

using fsnext::HistoryRepository;
using fsnext::HistoryType;
using fsnext::TransferHistoryDb;
using fsnext::TransferState;
using fsnext::TransferTask;
using fsnext::TransferType;

namespace {

// Minimal task factory. Default Complete so the row lands in history (not the
// in-flight set) unless a caller overrides the state.
TransferTask makeTask(const QString &id,
                      TransferState state = TransferState::Complete,
                      qint64 completedAt = 0,
                      const QString &fileName = QStringLiteral("file.bin"))
{
    TransferTask t;
    t.id          = id;
    t.type        = TransferType::Download;
    t.state       = state;
    t.fileName    = fileName;
    t.fileSize    = 1000;
    t.linkcode    = QStringLiteral("LC");
    t.completedAt = completedAt;
    return t;
}

// Build a legacy-format JSON object matching HistoryRepository's taskFromJson()
// reader: it keys off "id", "type" ("download"/"upload"), "state", and a
// numeric "completedAt" (ms since epoch).
QJsonObject legacyJson(const QString &id, int stateInt, qint64 completedAt)
{
    QJsonObject o;
    o["id"]          = id;
    o["type"]        = QStringLiteral("download");
    o["state"]       = stateInt;
    o["fileName"]    = id + QStringLiteral(".bin");
    o["fileSize"]    = 123;
    o["completedAt"] = static_cast<double>(completedAt);
    return o;
}

} // namespace

class TestHistoryRepository : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // ── ATM-0261 — loadDownloadHistory ──────────────────────────
    void loadDownloadHistory_migratesLegacyJson_newestFirst();   // TC-0263
    void loadDownloadHistory_migratesOnlyOnce();                 // TC-0264
    void loadDownloadHistory_noLegacyNoDb_returnsEmpty();        // TC-0265

    // ── ATM-0262 — upsertTask ───────────────────────────────────
    void upsertTask_insertThenUpdateKeepsSingleRow();            // TC-0266
    void upsertTask_falseWhenDbNotOpen();                        // TC-0267

    // ── ATM-0263 — saveProgressSnapshot ─────────────────────────
    void saveProgressSnapshot_updatesExistingRow();              // TC-0268
    void saveProgressSnapshot_unknownRowTrueZeroEffect();        // TC-0269

    // ── ATM-0264 — loadInFlight ─────────────────────────────────
    void loadInFlight_returnsOnlyInFlightWithSnapshots();        // TC-0270
    void loadInFlight_noInflight_returnsEmpty();                 // TC-0271

    // ── ATM-0265 — trimToMostRecent (hard cap) ──────────────────
    void trimToMostRecent_keepsNewestN();                        // TC-0272
    void trimToMostRecent_keepNonPositiveDeletesNothing();       // TC-0273
    void trimToMostRecent_scopedToUserAndType();                 // TC-0274

private:
    HistoryRepository *m_repo = nullptr;

    QString historyDir() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               + QStringLiteral("/FshareNext/history/");
    }

    void wipeHistoryDir()
    {
        QDir d(historyDir());
        if (d.exists()) d.removeRecursively();
    }

    // Read back progress_json for a task via loadInFlight's out-param (the only
    // way to observe progress_json — there is no public getter).
    QString progressJsonFor(const QString &userId, const QString &taskId)
    {
        QHash<QString, QString> snaps;
        m_repo->loadInFlight(userId, &snaps);
        return snaps.value(taskId);
    }
};

void TestHistoryRepository::initTestCase()
{
    // Redirect AppDataLocation into a per-run sandbox so we never touch the
    // real %APPDATA%/FshareNext/history tree.
    QStandardPaths::setTestModeEnabled(true);
}

void TestHistoryRepository::init()
{
    // Fresh sandbox + fresh repo per test → empty DB, no stale legacy JSON,
    // and a clean m_migratedKeys cache.
    wipeHistoryDir();
    m_repo = new HistoryRepository;
}

void TestHistoryRepository::cleanup()
{
    delete m_repo;   // closes the DB + removes the named connection
    m_repo = nullptr;
    wipeHistoryDir();
}

// ── ATM-0261 / TC-0263 ──────────────────────────────────────────
// Legacy JSON with 3 tasks (completedAt 100/300/200) must be migrated into the
// DB, the JSON renamed to *.migrated, and the returned list ordered newest-
// first by completed_at (300, 200, 100).
void TestHistoryRepository::loadDownloadHistory_migratesLegacyJson_newestFirst()
{
    const QString userId = QStringLiteral("u123");

    // Write a legacy JSON file at the exact path the repo will look for.
    QDir().mkpath(historyDir());
    const QString jsonPath = m_repo->legacyJsonPath(userId, HistoryType::Download);
    QJsonArray arr;
    arr.append(legacyJson(QStringLiteral("A"), static_cast<int>(TransferState::Complete), 100));
    arr.append(legacyJson(QStringLiteral("B"), static_cast<int>(TransferState::Complete), 300));
    arr.append(legacyJson(QStringLiteral("C"), static_cast<int>(TransferState::Complete), 200));
    QFile f(jsonPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(arr).toJson());
    f.close();

    const QVector<TransferTask> rows = m_repo->loadDownloadHistory(userId);

    // 3 rows migrated, newest-first by completedAt.
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].id, QStringLiteral("B")); // 300
    QCOMPARE(rows[1].id, QStringLiteral("C")); // 200
    QCOMPARE(rows[2].id, QStringLiteral("A")); // 100

    // The legacy file was renamed; the .migrated marker exists.
    QVERIFY(!QFile::exists(jsonPath));
    QVERIFY(QFile::exists(jsonPath + QStringLiteral(".migrated")));
}

// ── ATM-0261 / TC-0264 ──────────────────────────────────────────
// Migration runs at most once per (userId|type): after the first load the key
// is in m_migratedKeys, so a freshly-recreated legacy JSON is NOT migrated on a
// second load and stays on disk untouched.
void TestHistoryRepository::loadDownloadHistory_migratesOnlyOnce()
{
    const QString userId = QStringLiteral("u123");
    QDir().mkpath(historyDir());
    const QString jsonPath = m_repo->legacyJsonPath(userId, HistoryType::Download);

    // First legacy file → migrated on first load.
    {
        QJsonArray arr;
        arr.append(legacyJson(QStringLiteral("A"), static_cast<int>(TransferState::Complete), 100));
        QFile f(jsonPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(arr).toJson());
        f.close();
    }
    QCOMPARE(m_repo->loadDownloadHistory(userId).size(), 1);
    QVERIFY(!QFile::exists(jsonPath));

    // Re-create a NEW legacy file with extra rows.
    {
        QJsonArray arr;
        arr.append(legacyJson(QStringLiteral("X"), static_cast<int>(TransferState::Complete), 500));
        arr.append(legacyJson(QStringLiteral("Y"), static_cast<int>(TransferState::Complete), 600));
        QFile f(jsonPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(arr).toJson());
        f.close();
    }

    // Second load: key already migrated → no migration, new JSON left in place,
    // DB still holds only the original row.
    const QVector<TransferTask> rows = m_repo->loadDownloadHistory(userId);
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].id, QStringLiteral("A"));
    QVERIFY(QFile::exists(jsonPath));                                 // not renamed
    QVERIFY(!QFile::exists(jsonPath + QStringLiteral(".migrated2"))); // sanity
}

// ── ATM-0261 / TC-0265 ──────────────────────────────────────────
// No legacy JSON + empty DB → empty result, no file renamed, no error.
void TestHistoryRepository::loadDownloadHistory_noLegacyNoDb_returnsEmpty()
{
    const QString userId = QStringLiteral("u999");
    const QString jsonPath = m_repo->legacyJsonPath(userId, HistoryType::Download);
    QVERIFY(!QFile::exists(jsonPath));

    const QVector<TransferTask> rows = m_repo->loadDownloadHistory(userId);
    QVERIFY(rows.isEmpty());
    QVERIFY(!QFile::exists(jsonPath + QStringLiteral(".migrated")));
}

// ── ATM-0262 / TC-0266 ──────────────────────────────────────────
// upsertTask inserts a new row, then a second upsert with the same id updates
// it in place (ON CONFLICT DO UPDATE) — never a duplicate.
void TestHistoryRepository::upsertTask_insertThenUpdateKeepsSingleRow()
{
    const QString userId = QStringLiteral("uUp");

    TransferTask t = makeTask(QStringLiteral("T1"), TransferState::Active, 10);
    t.progress = 0.0;
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download, t));
    QCOMPARE(m_repo->countAll(userId, HistoryType::Download), 1);

    // Re-upsert same id with new state + progress.
    TransferTask t2 = makeTask(QStringLiteral("T1"), TransferState::Complete, 10);
    t2.progress = 100.0;
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download, t2));

    // Still exactly one row, with the updated fields.
    const QVector<TransferTask> rows = m_repo->loadDownloadHistory(userId);
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].id, QStringLiteral("T1"));
    QCOMPARE(rows[0].state, TransferState::Complete);
    QCOMPARE(rows[0].progress, 100.0);
}

// ── ATM-0262 / TC-0267 ──────────────────────────────────────────
// Contract: an unopened store rejects the single-row write. This lives in
// TransferHistoryDb (HistoryRepository::upsertTask always ensureOpen()s first),
// so pin it directly: a never-open()ed DB returns false and writes nothing.
void TestHistoryRepository::upsertTask_falseWhenDbNotOpen()
{
    TransferHistoryDb db;                 // never open()ed
    QVERIFY(!db.isOpen());
    const TransferTask t = makeTask(QStringLiteral("T1"));
    QVERIFY(!db.upsertTask(QStringLiteral("u"), TransferType::Download, t));
}

// ── ATM-0263 / TC-0268 ──────────────────────────────────────────
// saveProgressSnapshot updates progress_json + state on an existing row; the
// snapshot is then readable via loadInFlight (row stays in-flight as Paused).
void TestHistoryRepository::saveProgressSnapshot_updatesExistingRow()
{
    const QString userId = QStringLiteral("uSnap");

    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T1"), TransferState::Active, 5)));

    const QString snap = QStringLiteral("{\"seg\":[1,2,3]}");
    QVERIFY(m_repo->saveProgressSnapshot(userId, HistoryType::Download,
                                         QStringLiteral("T1"),
                                         TransferState::Paused, snap));

    QHash<QString, QString> snaps;
    const QVector<TransferTask> inflight = m_repo->loadInFlight(userId, &snaps);

    bool found = false;
    for (const auto &r : inflight) {
        if (r.id == QStringLiteral("T1")) {
            found = true;
            QCOMPARE(r.state, TransferState::Paused);
        }
    }
    QVERIFY(found);
    QCOMPARE(snaps.value(QStringLiteral("T1")), snap);
}

// ── ATM-0263 / TC-0269 ──────────────────────────────────────────
// saveProgressSnapshot on a non-existent row returns true (no SQL error) but
// affects 0 rows — it does NOT create a new row.
void TestHistoryRepository::saveProgressSnapshot_unknownRowTrueZeroEffect()
{
    const QString userId = QStringLiteral("uSnap2");

    QVERIFY(m_repo->saveProgressSnapshot(userId, HistoryType::Download,
                                         QStringLiteral("TX"),
                                         TransferState::Active,
                                         QStringLiteral("{}")));

    // No row was materialised.
    QCOMPARE(m_repo->countAll(userId, HistoryType::Download), 0);
    QHash<QString, QString> snaps;
    QVERIFY(m_repo->loadInFlight(userId, &snaps).isEmpty());
    QVERIFY(!snaps.contains(QStringLiteral("TX")));
}

// ── ATM-0264 / TC-0270 ──────────────────────────────────────────
// loadInFlight returns only Queued/Active/Paused rows (excludes Complete/Error)
// and fills the snapshot map only for rows that actually carry a progress_json.
void TestHistoryRepository::loadInFlight_returnsOnlyInFlightWithSnapshots()
{
    const QString userId = QStringLiteral("u1");

    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T1"), TransferState::Queued, 0)));
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T2"), TransferState::Active, 1)));
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T3"), TransferState::Paused, 2)));
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T4"), TransferState::Complete, 3)));
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("T5"), TransferState::Error, 4)));

    // Only T2 gets a checkpoint.
    QVERIFY(m_repo->saveProgressSnapshot(userId, HistoryType::Download,
                                         QStringLiteral("T2"),
                                         TransferState::Active,
                                         QStringLiteral("SNAP")));

    QHash<QString, QString> snaps;
    const QVector<TransferTask> rows = m_repo->loadInFlight(userId, &snaps);

    // Exactly the three in-flight rows, ordered completed_at DESC (T3=2, T2=1, T1=0).
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].id, QStringLiteral("T3"));
    QCOMPARE(rows[1].id, QStringLiteral("T2"));
    QCOMPARE(rows[2].id, QStringLiteral("T1"));

    // Snapshot map only carries T2 (T1/T3 have empty progress_json).
    QCOMPARE(snaps.size(), 1);
    QCOMPARE(snaps.value(QStringLiteral("T2")), QStringLiteral("SNAP"));
    QVERIFY(!snaps.contains(QStringLiteral("T1")));
    QVERIFY(!snaps.contains(QStringLiteral("T3")));
}

// ── ATM-0264 / TC-0271 ──────────────────────────────────────────
// A user with only completed rows has nothing in flight.
void TestHistoryRepository::loadInFlight_noInflight_returnsEmpty()
{
    const QString userId = QStringLiteral("u2");
    QVERIFY(m_repo->upsertTask(userId, HistoryType::Download,
                               makeTask(QStringLiteral("D1"), TransferState::Complete, 9)));

    QHash<QString, QString> snaps;
    QVERIFY(m_repo->loadInFlight(userId, &snaps).isEmpty());
    QVERIFY(snaps.isEmpty());
}

// ── ATM-0265 / TC-0272 ──────────────────────────────────────────
// trimToMostRecent keeps the N newest rows (by completed_at) and deletes the
// rest, returning the number deleted.
void TestHistoryRepository::trimToMostRecent_keepsNewestN()
{
    const QString userId = QStringLiteral("uTrim");

    for (int i = 1; i <= 10; ++i) {
        QVERIFY(m_repo->upsertTask(
            userId, HistoryType::Download,
            makeTask(QStringLiteral("R%1").arg(i), TransferState::Complete, i)));
    }
    QCOMPARE(m_repo->countAll(userId, HistoryType::Download), 10);

    const int deleted = m_repo->trimToMostRecent(userId, HistoryType::Download, 5);
    QCOMPARE(deleted, 5);
    QCOMPARE(m_repo->countAll(userId, HistoryType::Download), 5);

    // The five survivors are the newest (completed_at 10..6), newest-first.
    const QVector<TransferTask> rows = m_repo->loadDownloadHistory(userId);
    QCOMPARE(rows.size(), 5);
    QCOMPARE(rows.first().id, QStringLiteral("R10"));
    QCOMPARE(rows.last().id, QStringLiteral("R6"));
}

// ── ATM-0265 / TC-0273 ──────────────────────────────────────────
// keep <= 0 is a guard: nothing is deleted, return 0.
void TestHistoryRepository::trimToMostRecent_keepNonPositiveDeletesNothing()
{
    const QString userId = QStringLiteral("uTrim0");
    for (int i = 1; i <= 3; ++i) {
        QVERIFY(m_repo->upsertTask(
            userId, HistoryType::Download,
            makeTask(QStringLiteral("R%1").arg(i), TransferState::Complete, i)));
    }

    QCOMPARE(m_repo->trimToMostRecent(userId, HistoryType::Download, 0), 0);
    QCOMPARE(m_repo->trimToMostRecent(userId, HistoryType::Download, -1), 0);
    QCOMPARE(m_repo->countAll(userId, HistoryType::Download), 3);
}

// ── ATM-0265 / TC-0274 ──────────────────────────────────────────
// trimToMostRecent only touches the matching (user, type); other users and the
// same user's other type are untouched.
void TestHistoryRepository::trimToMostRecent_scopedToUserAndType()
{
    const QString uA = QStringLiteral("uA");
    const QString uB = QStringLiteral("uB");

    for (int i = 1; i <= 5; ++i) {
        QVERIFY(m_repo->upsertTask(uA, HistoryType::Download,
            makeTask(QStringLiteral("AD%1").arg(i), TransferState::Complete, i)));
        QVERIFY(m_repo->upsertTask(uA, HistoryType::Upload,
            makeTask(QStringLiteral("AU%1").arg(i), TransferState::Complete, i)));
        QVERIFY(m_repo->upsertTask(uB, HistoryType::Download,
            makeTask(QStringLiteral("BD%1").arg(i), TransferState::Complete, i)));
    }

    m_repo->trimToMostRecent(uA, HistoryType::Download, 2);

    QCOMPARE(m_repo->countAll(uA, HistoryType::Download), 2); // trimmed
    QCOMPARE(m_repo->countAll(uA, HistoryType::Upload), 5);   // untouched
    QCOMPARE(m_repo->countAll(uB, HistoryType::Download), 5); // untouched
}

QTEST_GUILESS_MAIN(TestHistoryRepository)
#include "test_history_repository.moc"
