// SPDX-License-Identifier: Proprietary
//
// Unit tests for FavoritesViewModel — the ViewModel behind the Favorites page.
//
// FavoritesViewModel(FshareApi*, FileCacheService*) is constructed here with a
// nullptr FshareApi and a real FileCacheService built through its test-only
// constructor (the `dbPath` overload) pointed at a SQLite file under a
// QTemporaryDir. That gives us a fully wired cache that never touches the
// network: setCurrentUser opens the per-user tables, and listFiles serves the
// (empty) local cache then enqueues a *background* FileSyncWorker job that only
// runs when the event loop pumps processNext — which these tests never do, and
// whose api is nullptr anyway.
//
// What we cover without the network:
//   * fileListModel accessor (ATM-0047)
//   * isLoading initial state + signal plumbing (ATM-0048)
//   * navigateToFolder pushes the folder stack, flips isInFolder/canGoBack and
//     populates breadcrumbs (ATM-0060)
//   * navigateBack pops one level while staying inside a folder, and is a no-op
//     at root (ATM-0061)
//
// What needs the real API (QSKIP — Lô sau):
//   * loadFavorites() / addToFavorite() / removeFromFavorite() spawn
//     QtConcurrent workers that dereference FshareApi::listFavorites /
//     changeFavorite. With api == nullptr those would crash once the pool runs,
//     so the loading round-trip and the favorite-toggle round-trip are skipped.
//   * navigateBack from the first folder level calls loadFavorites() to repaint
//     the root, which also needs the API — that sub-case is skipped too.
//
// cleanup() drains the global thread pool before deleting the fixtures so no
// stray worker can dereference freed objects.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QVariantList>
#include <QVariantMap>

#include "viewmodels/FavoritesViewModel.h"
#include "viewmodels/FileListModel.h"
#include "core/services/FileCacheService.h"
#include "core/services/FileService.h"

using fsnext::FavoritesViewModel;
using fsnext::FileCacheService;
using fsnext::FileService;
using fsnext::FileListModel;

class TestFavoritesViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // ── ATM-0047 fileListModel ─────────────────────────────
    void fileListModelIsValid();

    // ── ATM-0048 isLoading ─────────────────────────────────
    void isLoadingDefaultsFalse();
    void loadFavoritesSetsLoading_needsApi();

    // ── ATM-0060 navigateToFolder ──────────────────────────
    void navigateToFolderPushesStack();
    void navigateToFolderEmptyIdIsNoop();

    // ── ATM-0061 navigateBack ──────────────────────────────
    void navigateBackPopsOneLevel();
    void navigateBackAtRootIsNoop();

private:
    FileService      *m_svc   = nullptr;
    FileCacheService *m_cache = nullptr;
    FavoritesViewModel *m_vm  = nullptr;
    QTemporaryDir    *m_tmp   = nullptr;
};

void TestFavoritesViewModel::initTestCase()
{
    // FileCacheService/FileSyncWorker construct QObjects on the calling thread;
    // nothing here needs a GUI, but a QCoreApplication is provided by
    // QTEST_GUILESS_MAIN.
}

void TestFavoritesViewModel::init()
{
    m_tmp = new QTemporaryDir;
    QVERIFY(m_tmp->isValid());
    const QString dbPath = m_tmp->filePath(QStringLiteral("favcache.db"));

    // Real FileService with no API (write/list ops never reach the network in
    // these tests because we never pump the worker pool / event loop).
    m_svc = new FileService(/*api=*/nullptr);

    // Test-only ctor: open SQLite at our temp path instead of AppData.
    m_cache = new FileCacheService(/*api=*/nullptr, m_svc, dbPath, /*parent=*/nullptr);
    // Opens the per-user tables so listFiles takes the cache path (which only
    // schedules a *background* refresh) rather than falling through to the live
    // FileService.
    m_cache->setCurrentUser(QStringLiteral("110"));

    // FavoritesViewModel with no API — the cache-backed navigation paths used
    // below never dereference it.
    m_vm = new FavoritesViewModel(/*api=*/nullptr, m_cache, /*parent=*/nullptr);
}

void TestFavoritesViewModel::cleanup()
{
    // Drain any QtConcurrent workers BEFORE tearing down the fixtures so an
    // in-flight task can never dereference a freed cache / VM.
    QThreadPool::globalInstance()->waitForDone();

    delete m_vm;
    m_vm = nullptr;
    delete m_cache;
    m_cache = nullptr;
    delete m_svc;
    m_svc = nullptr;
    delete m_tmp;
    m_tmp = nullptr;
}

// ── ATM-0047 / TC-0031 ──────────────────────────────────────
// fileListModel() must return a valid FileListModel* (CONSTANT property used
// by the QML ListView).
void TestFavoritesViewModel::fileListModelIsValid()
{
    FileListModel *model = m_vm->fileListModel();
    QVERIFY(model != nullptr);
    // It is the FileListModel subclass, owned by the VM, and starts empty.
    QCOMPARE(model->count(), 0);
}

// ── ATM-0048 / TC-0032 (init-state half) ────────────────────
// Before any load is triggered, isLoading must be false and totalCount 0.
void TestFavoritesViewModel::isLoadingDefaultsFalse()
{
    QCOMPARE(m_vm->isLoading(), false);
    QCOMPARE(m_vm->totalCount(), 0);
    QVERIFY(!m_vm->isInFolder());
    QVERIFY(!m_vm->canGoBack());
}

// ── ATM-0048 / TC-0032 (loading round-trip) ─────────────────
// The full true→false isLoading transition is driven by loadFavorites(), which
// spawns a worker calling FshareApi::listFavorites. With api == nullptr that
// path cannot run safely — needs a real/fake API. Verify only the synchronous
// pre-conditions, then skip the async assertion.
void TestFavoritesViewModel::loadFavoritesSetsLoading_needsApi()
{
    QSignalSpy spy(m_vm, &FavoritesViewModel::isLoadingChanged);
    QVERIFY(spy.isValid());
    QSKIP("loadFavorites() dereferences FshareApi::listFavorites on a worker "
          "thread — cần FshareApi thật/fake để chạy round-trip. Lô sau.");
}

// ── ATM-0060 / TC-0035 ──────────────────────────────────────
// navigateToFolder pushes the folder onto the stack, flips isInFolder /
// canGoBack and exposes the folder in breadcrumbs; navigationChanged fires.
void TestFavoritesViewModel::navigateToFolderPushesStack()
{
    QSignalSpy nav(m_vm, &FavoritesViewModel::navigationChanged);
    QVERIFY(nav.isValid());

    m_vm->navigateToFolder(QStringLiteral("f1"), QStringLiteral("Phim"));

    QVERIFY(m_vm->isInFolder());
    QVERIFY(m_vm->canGoBack());
    QVERIFY(nav.count() >= 1);

    const QVariantList crumbs = m_vm->breadcrumbs();
    QCOMPARE(crumbs.size(), 1);
    const QVariantMap top = crumbs.at(0).toMap();
    QCOMPARE(top.value(QStringLiteral("id")).toString(),   QStringLiteral("f1"));
    QCOMPARE(top.value(QStringLiteral("name")).toString(), QStringLiteral("Phim"));
}

// ── ATM-0060 / TC-0035 (Negative) ───────────────────────────
// An empty folderId is a fail-fast no-op: stack stays empty, no navigation
// signal.
void TestFavoritesViewModel::navigateToFolderEmptyIdIsNoop()
{
    QSignalSpy nav(m_vm, &FavoritesViewModel::navigationChanged);
    QVERIFY(nav.isValid());

    m_vm->navigateToFolder(QString(), QStringLiteral("ignored"));

    QVERIFY(!m_vm->isInFolder());
    QVERIFY(!m_vm->canGoBack());
    QCOMPARE(nav.count(), 0);
    QCOMPARE(m_vm->breadcrumbs().size(), 0);
}

// ── ATM-0061 / TC-0036 ──────────────────────────────────────
// navigateBack pops one level off the stack. To stay on the cache-only path
// (and avoid loadFavorites()'s API round-trip when returning to root), descend
// two levels first, then back out one — we remain inside a folder.
void TestFavoritesViewModel::navigateBackPopsOneLevel()
{
    m_vm->navigateToFolder(QStringLiteral("f1"), QStringLiteral("Phim"));
    m_vm->navigateToFolder(QStringLiteral("f2"), QStringLiteral("2024"));
    QCOMPARE(m_vm->breadcrumbs().size(), 2);

    QSignalSpy nav(m_vm, &FavoritesViewModel::navigationChanged);
    QVERIFY(nav.isValid());

    m_vm->navigateBack();

    // Popped back to "Phim"; still inside a folder so no loadFavorites() ran.
    QVERIFY(m_vm->isInFolder());
    QVERIFY(m_vm->canGoBack());
    QVERIFY(nav.count() >= 1);

    const QVariantList crumbs = m_vm->breadcrumbs();
    QCOMPARE(crumbs.size(), 1);
    QCOMPARE(crumbs.at(0).toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("Phim"));

    // Popping the last level returns to root, which calls loadFavorites() →
    // needs the API. Covered separately when a fake API is wired (Lô sau).
}

// ── ATM-0061 / TC-0037 (Edge) ───────────────────────────────
// navigateBack at root (stack empty, canGoBack == false) must be a no-op.
void TestFavoritesViewModel::navigateBackAtRootIsNoop()
{
    QVERIFY(!m_vm->canGoBack());

    QSignalSpy nav(m_vm, &FavoritesViewModel::navigationChanged);
    QVERIFY(nav.isValid());

    m_vm->navigateBack();

    QVERIFY(!m_vm->isInFolder());
    QCOMPARE(nav.count(), 0);
}

QTEST_GUILESS_MAIN(TestFavoritesViewModel)
#include "test_favorites_viewmodel.moc"
