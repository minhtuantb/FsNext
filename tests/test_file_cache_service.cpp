// SPDX-License-Identifier: Proprietary
//
// Phase 2 unit tests for FileCacheService — the write-op state machine that
// sits between QML and the network. These tests verify that after each write
// operation (create/rename/delete/move/copy) completes or fails on the
// underlying FileService, the cache-refresh side effect fires for the right
// folder(s).
//
// Regression focus:
//   * Root folder (empty parentId) still triggers cache refresh on success
//     — an earlier `if (!m_pendingSourceFolder.isEmpty())` guard silently
//     skipped the refresh, so newly-created root folders never appeared
//     until the user navigated away and back.
//   * Settings pass-through ops (changeSecure/setPassword/setDirectLink/
//     getFileInfo) must not consume the pending write-op state.
//
// Strategy:
//   * `FakeFileService` subclasses the real FileService with no-op overrides
//     for every write method, plus helpers that synthesise the
//     operationComplete/Failed signals the real API would fire.
//   * We pass nullptr for FshareApi. No method on FakeFileService reaches
//     m_api, and FileSyncWorker's processNext only fires when the event loop
//     pumps — which we never do. So no network calls happen.
//   * The `cacheRefreshScheduled` signal on FileCacheService is emitted at
//     every choke-point where a refresh is enqueued, making the state
//     machine observable via QSignalSpy.

#include "core/services/FileCacheService.h"
#include "core/services/FileService.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

using fsnext::FileCacheService;
using fsnext::FileService;

namespace {

// A FileService that never touches the network. All write ops are no-ops —
// tests drive the state machine by calling triggerComplete()/triggerFailed()
// to simulate the signal the real API layer would emit.
class FakeFileService : public FileService
{
public:
    FakeFileService() : FileService(/*api=*/nullptr) {}

    void createFolder(const QString &, const QString &) override { ++m_callCount; }
    void renameFile(const QString &, const QString &) override { ++m_callCount; }
    void deleteFiles(const QStringList &) override { ++m_callCount; }
    void moveFiles(const QStringList &, const QString &) override { ++m_callCount; }
    void copyFiles(const QStringList &, const QString &) override { ++m_callCount; }

    // Settings pass-through ops also need no-op overrides — the real impls
    // spawn QtConcurrent::run lambdas that would dereference the nullptr api
    // once the thread pool picks them up.
    void changeSecure(const QStringList &, bool) override { ++m_callCount; }
    void setPassword(const QStringList &, const QString &) override { ++m_callCount; }
    void setDirectLink(const QStringList &, bool) override { ++m_callCount; }
    void getFileInfo(const QString &) override { ++m_callCount; }

    void triggerComplete(const QString &msg = QStringLiteral("ok"))
    {
        emit operationComplete(msg);
    }
    void triggerFailed(const QString &err = QStringLiteral("boom"))
    {
        emit operationFailed(err);
    }

    int callCount() const { return m_callCount; }

private:
    int m_callCount = 0;
};

// Pull a (folderId, highPriority) pair out of a QSignalSpy entry.
QString spyFolder(const QList<QVariant> &call) { return call.at(0).toString(); }
bool    spyHigh  (const QList<QVariant> &call) { return call.at(1).toBool(); }

} // namespace

class TestFileCacheService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // ── create ─────────────────────────────────────────────
    void createFolderRefreshesParent();
    void createFolderAtRootRefreshesRoot_regression();

    // ── rename / delete ────────────────────────────────────
    void renameRefreshesCurrentFolder();
    void deleteRefreshesCurrentFolder();
    void renameAtRootRefreshesRoot_regression();

    // ── move ───────────────────────────────────────────────
    void moveRefreshesSourceAndTarget();
    void moveToSameFolderRefreshesOnce();

    // ── copy ───────────────────────────────────────────────
    void copyRefreshesTargetOnly();

    // ── failure path ───────────────────────────────────────
    void failedWriteRefreshesCurrentFolder();

    // ── guards ─────────────────────────────────────────────
    void settingsPassThroughDoesNotConsumeWriteOp();
    void operationCompleteWithoutPendingOpDoesNothing();

    // ── forwarding ─────────────────────────────────────────
    void completeIsForwardedToOperationComplete();
    void failedIsForwardedToOperationFailed();

private:
    FakeFileService  *m_svc   = nullptr;
    FileCacheService *m_cache = nullptr;

    // Consume the initial cacheRefreshScheduled that setCurrentUser fires on
    // first login so individual tests start from a clean slate.
    QSignalSpy *m_spy = nullptr;
};

void TestFileCacheService::init()
{
    m_svc   = new FakeFileService;
    m_cache = new FileCacheService(/*api=*/nullptr, m_svc,
                                   QStringLiteral(":memory:"),
                                   /*parent=*/nullptr);
    m_spy = new QSignalSpy(m_cache, &FileCacheService::cacheRefreshScheduled);

    m_cache->setCurrentUser(QStringLiteral("110"));
    // setCurrentUser's firstLogin path schedules an initial refresh — drop
    // those events so tests observe only the write-op refreshes.
    m_spy->clear();
}

void TestFileCacheService::cleanup()
{
    delete m_spy;
    m_spy = nullptr;
    delete m_cache;
    m_cache = nullptr;
    delete m_svc;
    m_svc = nullptr;
}

// ── create ──────────────────────────────────────────────────

void TestFileCacheService::createFolderRefreshesParent()
{
    m_cache->createFolder(QStringLiteral("NewFolder"),
                          QStringLiteral("parentXYZ"));
    QCOMPARE(m_svc->callCount(), 1);
    QCOMPARE(m_spy->count(), 0); // no refresh until server confirms

    m_svc->triggerComplete();
    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("parentXYZ"));
    QVERIFY(spyHigh(m_spy->at(0)));
}

// The canonical regression test: creating a folder at the account root
// (parentId = "") must still schedule a refresh. An earlier
// `if (!m_pendingSourceFolder.isEmpty())` guard skipped the refresh, so
// new root folders never appeared until navigation.
void TestFileCacheService::createFolderAtRootRefreshesRoot_regression()
{
    m_cache->createFolder(QStringLiteral("RootFolder"), QStringLiteral(""));
    m_svc->triggerComplete();

    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral(""));
    QVERIFY(spyHigh(m_spy->at(0)));
}

// ── rename / delete ─────────────────────────────────────────

void TestFileCacheService::renameRefreshesCurrentFolder()
{
    // listFiles() would normally set the current folder. Here the cache is
    // empty so listFiles would be a no-op; rename still tracks pendingOp.
    m_cache->listFiles(QStringLiteral("folderA"));
    m_spy->clear();

    m_cache->renameFile(QStringLiteral("lc1"), QStringLiteral("new.txt"));
    m_svc->triggerComplete();

    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("folderA"));
}

void TestFileCacheService::deleteRefreshesCurrentFolder()
{
    m_cache->listFiles(QStringLiteral("folderA"));
    m_spy->clear();

    m_cache->deleteFiles({QStringLiteral("lc1"), QStringLiteral("lc2")});
    m_svc->triggerComplete();

    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("folderA"));
}

// Same regression as create, for rename/delete: root must still refresh.
void TestFileCacheService::renameAtRootRefreshesRoot_regression()
{
    // No listFiles() → m_currentFolderId stays "".
    m_cache->renameFile(QStringLiteral("lc1"), QStringLiteral("new.txt"));
    m_svc->triggerComplete();

    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral(""));
}

// ── move ────────────────────────────────────────────────────

void TestFileCacheService::moveRefreshesSourceAndTarget()
{
    m_cache->listFiles(QStringLiteral("sourceF"));
    m_spy->clear();

    m_cache->moveFiles({QStringLiteral("lc1")}, QStringLiteral("targetF"));
    m_svc->triggerComplete();

    // Source refresh (high priority) + target refresh (background)
    QCOMPARE(m_spy->count(), 2);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("sourceF"));
    QVERIFY(spyHigh(m_spy->at(0)));
    QCOMPARE(spyFolder(m_spy->at(1)), QStringLiteral("targetF"));
    QVERIFY(!spyHigh(m_spy->at(1)));
}

void TestFileCacheService::moveToSameFolderRefreshesOnce()
{
    // No-op move from current folder to itself shouldn't schedule a
    // redundant background refresh.
    m_cache->listFiles(QStringLiteral("sameF"));
    m_spy->clear();

    m_cache->moveFiles({QStringLiteral("lc1")}, QStringLiteral("sameF"));
    m_svc->triggerComplete();

    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("sameF"));
}

// ── copy ────────────────────────────────────────────────────

void TestFileCacheService::copyRefreshesTargetOnly()
{
    m_cache->listFiles(QStringLiteral("sourceF"));
    m_spy->clear();

    m_cache->copyFiles({QStringLiteral("lc1")}, QStringLiteral("targetF"));
    m_svc->triggerComplete();

    // Source is unchanged by copy; only target gains new items.
    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("targetF"));
    QVERIFY(!spyHigh(m_spy->at(0)));
}

// ── failure path ────────────────────────────────────────────

void TestFileCacheService::failedWriteRefreshesCurrentFolder()
{
    m_cache->listFiles(QStringLiteral("folderA"));
    m_spy->clear();

    m_cache->createFolder(QStringLiteral("X"), QStringLiteral("folderA"));
    m_svc->triggerFailed(QStringLiteral("409 Conflict"));

    // On failure, the current folder is re-synced to undo any optimistic
    // cache mutations — NOT the source folder parented by the failed op.
    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("folderA"));
    QVERIFY(spyHigh(m_spy->at(0)));
}

// ── guards ──────────────────────────────────────────────────

// Settings ops (changeSecure/setPassword/setDirectLink/getFileInfo) share the
// FileService operationComplete signal. Their completion must NOT consume
// a pending write-op state — otherwise the subsequent rename/delete would
// lose its follow-up refresh.
void TestFileCacheService::settingsPassThroughDoesNotConsumeWriteOp()
{
    m_cache->listFiles(QStringLiteral("folderA"));
    m_spy->clear();

    // Start a rename (sets pendingOp = Rename).
    m_cache->renameFile(QStringLiteral("lc1"), QStringLiteral("new.txt"));

    // A settings op finishes first (increments m_settingsInFlight in the
    // service; its completion should decrement and return early).
    m_cache->changeSecure({QStringLiteral("lc1")}, /*secure=*/true);
    m_svc->triggerComplete();   // consumes the settings pass-through
    QCOMPARE(m_spy->count(), 0); // no refresh yet

    // Now the rename's own completion fires — should refresh current folder.
    m_svc->triggerComplete();
    QCOMPARE(m_spy->count(), 1);
    QCOMPARE(spyFolder(m_spy->at(0)), QStringLiteral("folderA"));
}

// If operationComplete fires with no pending write-op (stray signal), the
// state machine should quietly ignore it — no refresh, no crash.
void TestFileCacheService::operationCompleteWithoutPendingOpDoesNothing()
{
    m_svc->triggerComplete();
    QCOMPARE(m_spy->count(), 0);

    m_svc->triggerFailed();
    QCOMPARE(m_spy->count(), 0);
}

// ── forwarding ──────────────────────────────────────────────

// FileCacheService forwards FileService's operationComplete/Failed signals
// to QML. Verify the signal reaches listeners with the payload intact.
void TestFileCacheService::completeIsForwardedToOperationComplete()
{
    QSignalSpy fwd(m_cache, &FileCacheService::operationComplete);
    m_svc->triggerComplete(QStringLiteral("folder created"));
    QCOMPARE(fwd.count(), 1);
    QCOMPARE(fwd.at(0).at(0).toString(), QStringLiteral("folder created"));
}

void TestFileCacheService::failedIsForwardedToOperationFailed()
{
    QSignalSpy fwd(m_cache, &FileCacheService::operationFailed);
    m_svc->triggerFailed(QStringLiteral("server 500"));
    QCOMPARE(fwd.count(), 1);
    QCOMPARE(fwd.at(0).at(0).toString(), QStringLiteral("server 500"));
}

QTEST_MAIN(TestFileCacheService)
#include "test_file_cache_service.moc"
