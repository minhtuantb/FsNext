// SPDX-License-Identifier: Proprietary
//
// Unit tests for HomeSearchViewModel — the stateful classifier behind the
// homepage search box. Covers the full QA catalog for the module (no network):
//
//   * functional.md  HOME-FN-001..027  (classify() state machine + submit()
//                    routing + debounce-scheduling + async paging / stale-
//                    response & clearResults sequence guards / soft-fail)
//   * validation.md  HOME-VAL-001..020 (length boundaries, quoted mode,
//                    bad-word n-gram / diacritic / false-positive safety,
//                    URL detection variants, giant / unicode robustness)
//
// Construction strategy (mirrors test_download_viewmodel):
//   * BadWordFilter — REAL, loaded from a throwaway JSON dictionary written in
//     initTestCase() so the bad-word matrix is deterministic and independent
//     of the shipped qrc dictionary. Entries: "sex", "phim sex" (n-gram) and
//     "tụcxấu" (diacritic-insensitive). "essex" stays clean (word-boundary).
//   * DownloadViewModel — REAL, wired to the same in-memory / null collaborator
//     graph as test_download_viewmodel. HomeSearchViewModel only ever calls its
//     pure-regex extractFshareLinks(), so no network is touched.
//   * api — the SYNC tests pass FshareApi==nullptr (search never fires). The
//     ASYNC tests (FN-019..027) build a local VM around a FakeSearchApi
//     (IFshareApi) with a controllable item count + latency; searchFiles() runs
//     on a QtConcurrent worker so they use QTRY_*/qWait to let the queued
//     callback land. (This is why searchFiles() was promoted into IFshareApi.)
//
// cleanup()/drainAndDelete() drain the global thread pool before freeing the
// graph so no stray QtConcurrent worker dereferences a dangling collaborator
// (the VM guards its own callbacks with QPointer, the graph does not).

#include <QtTest>
#include <QSignalSpy>
#include <QAtomicInteger>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QSettings>
#include <QThread>
#include <QThreadPool>

#include "viewmodels/HomeSearchViewModel.h"
#include "viewmodels/DownloadViewModel.h"
#include "core/api/IFshareApi.h"
#include "core/models/AppError.h"
#include "core/util/BadWordFilter.h"
#include "core/services/TransferService.h"
#include "core/services/SettingsService.h"
#include "core/transfer/TransferOrchestrator.h"
#include "core/repositories/SettingsRepository.h"
#include "core/repositories/HistoryRepository.h"

using fsnext::HomeSearchViewModel;
using fsnext::DownloadViewModel;
using fsnext::BadWordFilter;
using fsnext::TransferService;
using fsnext::SettingsService;
using fsnext::TransferOrchestrator;
using fsnext::SettingsRepository;
using fsnext::HistoryRepository;
using fsnext::IFshareApi;

// ── FakeSearchApi ────────────────────────────────────────────────────────────
// Controllable IFshareApi for the async keyword-search tests. searchFiles() runs
// on a QtConcurrent worker thread (HomeSearchViewModel::fetchPage), so we expose
// a latency knob to widen the race window the request-sequence guard must close.
// The auth methods are never called — they return failures.
namespace {
class FakeSearchApi : public IFshareApi
{
public:
    QAtomicInt searchCalls{0};
    int        perKeyword = 30;   // items returned per page
    bool       fail       = false;
    int        latencyMs  = 0;    // simulated server latency (on the worker thread)

    QMutex      mtx;
    QStringList keywords;         // guarded by mtx — what was requested, in order
    QList<int>  pages;

    fsnext::ApiResponse<fsnext::Session> login(const QString &, const QString &) override
    { return fsnext::ApiResponse<fsnext::Session>::failure(fsnext::AppError::auth(405, QStringLiteral("n/a"))); }
    fsnext::ApiResponse<fsnext::Session> loginOauth(const QString &, const QString &, const QString &) override
    { return fsnext::ApiResponse<fsnext::Session>::failure(fsnext::AppError::auth(400, QStringLiteral("n/a"))); }
    fsnext::ApiResponse<void> logout() override
    { return fsnext::ApiResponse<void>::success(); }
    fsnext::ApiResponse<fsnext::User> getUserInfo() override
    { return fsnext::ApiResponse<fsnext::User>::failure(fsnext::AppError::auth(401, QStringLiteral("n/a"))); }

    fsnext::ApiResponse<QVector<fsnext::FileItem>> searchFiles(const QString &kw, int page) override
    {
        searchCalls.ref();
        { QMutexLocker lk(&mtx); keywords << kw; pages << page; }
        if (latencyMs > 0) QThread::msleep(latencyMs);
        if (fail)
            return fsnext::ApiResponse<QVector<fsnext::FileItem>>::failure(
                fsnext::AppError::auth(500, QStringLiteral("boom")));
        QVector<fsnext::FileItem> items;
        items.reserve(perKeyword);
        for (int i = 0; i < perKeyword; ++i) {
            fsnext::FileItem f;
            // Unique linkcode per (keyword,page,index) so mergeItems() on
            // load-more APPENDS instead of dedup-collapsing into the page-1 rows.
            f.linkcode = kw + QStringLiteral("_p") + QString::number(page)
                         + QLatin1Char('_') + QString::number(i);
            f.name = kw + QStringLiteral(" result ") + QString::number(i);
            f.type = QStringLiteral("file");
            f.size = 1000 + i;
            items << f;
        }
        return fsnext::ApiResponse<QVector<fsnext::FileItem>>::success(items);
    }
};
} // namespace

// Short aliases for the State enum values asserted throughout.
static constexpr int kIdle        = HomeSearchViewModel::Idle;        // 0
static constexpr int kTooShort    = HomeSearchViewModel::TooShort;    // 1
static constexpr int kUrlFile     = HomeSearchViewModel::UrlFile;     // 2
static constexpr int kUrlFolder   = HomeSearchViewModel::UrlFolder;   // 3
static constexpr int kUrlMultiple = HomeSearchViewModel::UrlMultiple; // 4
static constexpr int kKeyword     = HomeSearchViewModel::Keyword;     // 5
static constexpr int kBlocked     = HomeSearchViewModel::Blocked;     // 6

class TestHomeSearchViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("HomeSearchViewModelTest"));

        // Deterministic bad-word dictionary. normalizeSpaces() turns the space
        // in "phim sex" into "phim_sex" on load; "tụcxấu" gets a stripped
        // "tucxau" companion in m_stripped.
        m_dictPath = QDir::tempPath() + QStringLiteral("/fsnext_test_badwords.json");
        QFile f(m_dictPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(R"({ "sex": 1, "phim sex": 1, "tụcxấu": 1 })");
        f.close();
    }

    void cleanupTestCase()
    {
        QFile::remove(m_dictPath);
    }

    void init()
    {
        m_filter = new BadWordFilter;
        QVERIFY2(m_filter->loadFromFile(m_dictPath),
                 "bad-word dictionary must load or the fail-open path makes "
                 "every Blocked assertion vacuous");
        QVERIFY(m_filter->entryCount() > 0);

        m_repo     = new SettingsRepository;
        m_settings = new SettingsService(m_repo);
        m_history  = new HistoryRepository;
        m_orch     = new TransferOrchestrator;
        m_service  = new TransferService(/*api=*/nullptr, m_repo, m_orch, m_history);
        m_download = new DownloadViewModel(m_service, m_settings, /*auth=*/nullptr);

        // api=nullptr → the debounced keyword search never hits the network.
        m_vm = new HomeSearchViewModel(m_filter, m_download, /*api=*/nullptr);
    }

    void cleanup()
    {
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete m_vm;       m_vm       = nullptr;
        delete m_download; m_download = nullptr;
        delete m_service;  m_service  = nullptr;
        delete m_orch;     m_orch     = nullptr;
        delete m_history;  m_history  = nullptr;
        delete m_settings; m_settings = nullptr;
        delete m_repo;     m_repo     = nullptr;
        delete m_filter;   m_filter   = nullptr;
    }

    // ═══════════════════════════════════════════════════════════════
    //  classify() — length / Idle / Keyword boundary  (FN-001..004, VAL-001..006)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-001 / HOME-VAL-001 — empty input → Idle, nothing scheduled.
    void emptyInputIsIdle()
    {
        m_vm->classify(QString());
        QCOMPARE(m_vm->state(), kIdle);
        QVERIFY(m_vm->keyword().isEmpty());
        QVERIFY(!m_vm->isSearching());
    }

    // HOME-FN-002 — whitespace-only trims to empty → Idle.
    void whitespaceOnlyIsIdle()
    {
        m_vm->classify(QStringLiteral("    "));
        QCOMPARE(m_vm->state(), kIdle);
    }

    // HOME-VAL-002 — single char → TooShort.
    void oneCharIsTooShort()
    {
        m_vm->classify(QStringLiteral("a"));
        QCOMPARE(m_vm->state(), kTooShort);
        QVERIFY(!m_vm->isSearching());
    }

    // HOME-FN-003 / HOME-VAL-003 — two chars → TooShort, keyword echoes input.
    void twoCharsIsTooShort()
    {
        m_vm->classify(QStringLiteral("ab"));
        QCOMPARE(m_vm->state(), kTooShort);
        QCOMPARE(m_vm->keyword(), QStringLiteral("ab"));
    }

    // HOME-VAL-005 — length is counted AFTER trimming surrounding whitespace.
    void trimBeforeLengthCheck()
    {
        m_vm->classify(QStringLiteral("  a  "));   // 1 real char
        QCOMPARE(m_vm->state(), kTooShort);
    }

    // HOME-FN-004 / HOME-VAL-004 — exactly kMinLength (3) clean chars → Keyword,
    // and scheduling the search flips isSearching true synchronously.
    void boundaryThreeCharsIsKeyword()
    {
        QSignalSpy progress(m_vm, &HomeSearchViewModel::searchProgressChanged);
        m_vm->classify(QStringLiteral("abc"));
        QCOMPARE(m_vm->state(), kKeyword);
        QCOMPARE(m_vm->keyword(), QStringLiteral("abc"));
        QVERIFY(m_vm->isSearching());          // scheduleKeywordSearch → setSearching(true)
        QVERIFY(progress.count() >= 1);
    }

    // HOME-VAL-006 — trimmed keyword is what gets stored / searched.
    void keywordIsTrimmed()
    {
        m_vm->classify(QStringLiteral("  abc  "));
        QCOMPARE(m_vm->state(), kKeyword);
        QCOMPARE(m_vm->keyword(), QStringLiteral("abc"));
    }

    // ═══════════════════════════════════════════════════════════════
    //  Bad-word filter — fail-closed  (FN-005, VAL-007..010)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-005 / HOME-VAL-007 — single dictionary token → Blocked + hitWord.
    void badWordSingleTokenBlocked()
    {
        m_vm->classify(QStringLiteral("sex movie"));
        QCOMPARE(m_vm->state(), kBlocked);
        QVERIFY(!m_vm->hitWord().isEmpty());
        QVERIFY(!m_vm->isSearching());          // clearResults() ran — no search
    }

    // HOME-VAL-008 — multi-word phrase caught by the 1..3 n-gram pass.
    void badWordNgramBlocked()
    {
        m_vm->classify(QStringLiteral("phim sex hd"));
        QCOMPARE(m_vm->state(), kBlocked);
    }

    // HOME-VAL-009 — diacritic-stripped match: typing without accents still hits.
    void badWordDiacriticInsensitiveBlocked()
    {
        m_vm->classify(QStringLiteral("tucxau"));   // ascii form of "tụcxấu"
        QCOMPARE(m_vm->state(), kBlocked);
    }

    // HOME-VAL-010 — word-boundary safety: "essex" contains "sex" as a substring
    // but is a distinct token → must stay clean (no over-blocking).
    void substringIsNotBlocked()
    {
        m_vm->classify(QStringLiteral("essex"));
        QCOMPARE(m_vm->state(), kKeyword);
    }

    // ═══════════════════════════════════════════════════════════════
    //  Quoted "precision" mode  (FN-015..017, VAL-011..014)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-015 / HOME-VAL-011 — quoted clean phrase → Keyword, inner phrase
    // stored WITHOUT the surrounding quotes.
    void quotedCleanPhraseIsKeyword()
    {
        m_vm->classify(QString::fromUtf8("\"Lồng tiếng\""));
        QCOMPARE(m_vm->state(), kKeyword);
        QCOMPARE(m_vm->keyword(), QString::fromUtf8("Lồng tiếng"));
    }

    // HOME-FN-016 / HOME-VAL-012 ⚠️ — a quoted phrase that EXACTLY equals a
    // dictionary entry is still rejected. Quoting is a precision flag, never a
    // bad-word bypass.
    void quotedExactBadWordStillBlocked()
    {
        m_vm->classify(QStringLiteral("\"sex\""));
        QCOMPARE(m_vm->state(), kBlocked);
        QVERIFY(!m_vm->isSearching());
    }

    // HOME-FN-017 / HOME-VAL-013 — quoted inner shorter than kMinLength → TooShort.
    void quotedShortInnerIsTooShort()
    {
        m_vm->classify(QStringLiteral("\"ab\""));
        QCOMPARE(m_vm->state(), kTooShort);
        QCOMPARE(m_vm->keyword(), QStringLiteral("ab"));
    }

    // HOME-VAL-014 — a single leading quote is NOT quoted-mode; falls through to
    // the ordinary keyword path.
    void singleQuoteIsNotQuotedMode()
    {
        m_vm->classify(QStringLiteral("\"abc"));   // opens but never closes
        QCOMPARE(m_vm->state(), kKeyword);
    }

    // ═══════════════════════════════════════════════════════════════
    //  URL detection  (FN-009..014, VAL-015..018)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-009 — one file URL → UrlFile, keyword == the URL.
    void singleFileUrl()
    {
        const QString u = QStringLiteral("https://www.fshare.vn/file/ABC123");
        m_vm->classify(u);
        QCOMPARE(m_vm->state(), kUrlFile);
        QCOMPARE(m_vm->keyword(), u);
    }

    // HOME-FN-010 — one folder URL → UrlFolder.
    void singleFolderUrl()
    {
        m_vm->classify(QStringLiteral("https://fshare.vn/folder/XYZ"));
        QCOMPARE(m_vm->state(), kUrlFolder);
    }

    // HOME-VAL-016 — /FOLDER/ matched case-insensitively.
    void folderUrlCaseInsensitive()
    {
        m_vm->classify(QStringLiteral("https://fshare.vn/FOLDER/XYZ"));
        QCOMPARE(m_vm->state(), kUrlFolder);
    }

    // HOME-FN-011 — two or more URLs → UrlMultiple.
    void multipleUrls()
    {
        m_vm->classify(QStringLiteral(
            "https://fshare.vn/file/A https://fshare.vn/file/B"));
        QCOMPARE(m_vm->state(), kUrlMultiple);
    }

    // HOME-FN-014 / HOME-VAL-017 — a URL embedded in prose is still detected;
    // URL classification wins over the keyword path.
    void urlEmbeddedInProse()
    {
        m_vm->classify(QStringLiteral("xem phim https://fshare.vn/file/ABC nhe"));
        QCOMPARE(m_vm->state(), kUrlFile);
    }

    // HOME-VAL-018 — a non-fshare URL is NOT treated as a share link; it drops
    // to the keyword path (here it is clean and long enough → Keyword).
    void nonFshareUrlIsNotDetected()
    {
        m_vm->classify(QStringLiteral("https://google.com/file/x"));
        QVERIFY(m_vm->state() != kUrlFile);
        QVERIFY(m_vm->state() != kUrlFolder);
        QVERIFY(m_vm->state() != kUrlMultiple);
    }

    // ═══════════════════════════════════════════════════════════════
    //  Robustness  (VAL-019, VAL-020)
    // ═══════════════════════════════════════════════════════════════

    // HOME-VAL-019 — a 100k-char input must classify without hanging / crashing.
    void giantInputDoesNotCrash()
    {
        const QString giant(100000, QLatin1Char('a'));
        m_vm->classify(giant);
        // Clean + long → Keyword. The point is it RETURNS (no hang/crash).
        QCOMPARE(m_vm->state(), kKeyword);
    }

    // HOME-VAL-020 — emoji / zero-width / RTL marks must not throw.
    void unicodeInputDoesNotCrash()
    {
        m_vm->classify(QString::fromUtf8("🔥​‮ابجد file"));
        // We don't pin the exact state (depends on tokenisation) — only that the
        // classifier produced a valid State and didn't crash.
        QVERIFY(m_vm->state() >= kIdle && m_vm->state() <= kBlocked);
    }

    // ═══════════════════════════════════════════════════════════════
    //  submit() routing signals  (FN-006..008, FN-012..013, FN-018)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-008 — submitting empty input is a no-op: no routing, no rejection.
    void submitIdleEmitsNothing()
    {
        QSignalSpy key(m_vm, &HomeSearchViewModel::routeKeyword);
        QSignalSpy tooShort(m_vm, &HomeSearchViewModel::rejectedTooShort);
        QSignalSpy bad(m_vm, &HomeSearchViewModel::rejectedBadWord);
        QCOMPARE(m_vm->submit(QString()), kIdle);
        QCOMPARE(key.count(), 0);
        QCOMPARE(tooShort.count(), 0);
        QCOMPARE(bad.count(), 0);
    }

    // HOME-FN-007 — TooShort submit emits rejectedTooShort, no route.
    void submitTooShortRejects()
    {
        QSignalSpy tooShort(m_vm, &HomeSearchViewModel::rejectedTooShort);
        QSignalSpy key(m_vm, &HomeSearchViewModel::routeKeyword);
        QCOMPARE(m_vm->submit(QStringLiteral("ab")), kTooShort);
        QCOMPARE(tooShort.count(), 1);
        QCOMPARE(key.count(), 0);
    }

    // HOME-FN-006 ⚠️ — Blocked submit emits rejectedBadWord and emits NO routing
    // signal. With api=nullptr no search could run regardless — the contract is
    // "blocked input never reaches the API".
    void submitBlockedRejectsAndDoesNotRoute()
    {
        QSignalSpy bad(m_vm, &HomeSearchViewModel::rejectedBadWord);
        QSignalSpy key(m_vm, &HomeSearchViewModel::routeKeyword);
        QCOMPARE(m_vm->submit(QStringLiteral("phim sex")), kBlocked);
        QCOMPARE(bad.count(), 1);
        QCOMPARE(key.count(), 0);
    }

    // HOME-FN-012 — file URL submit emits exactly routeFileUrl(url).
    void submitFileUrlRoutes()
    {
        const QString u = QStringLiteral("https://fshare.vn/file/ABC");
        QSignalSpy fileSpy(m_vm, &HomeSearchViewModel::routeFileUrl);
        QSignalSpy folderSpy(m_vm, &HomeSearchViewModel::routeFolderUrl);
        QCOMPARE(m_vm->submit(u), kUrlFile);
        QCOMPARE(fileSpy.count(), 1);
        QCOMPARE(fileSpy.first().first().toString(), u);
        QCOMPARE(folderSpy.count(), 0);
    }

    // HOME-FN-013 — folder URL and multi-URL route to their own signals.
    void submitFolderAndMultipleRoute()
    {
        QSignalSpy folderSpy(m_vm, &HomeSearchViewModel::routeFolderUrl);
        QCOMPARE(m_vm->submit(QStringLiteral("https://fshare.vn/folder/XYZ")), kUrlFolder);
        QCOMPARE(folderSpy.count(), 1);

        QSignalSpy multiSpy(m_vm, &HomeSearchViewModel::routeMultipleUrls);
        QCOMPARE(m_vm->submit(QStringLiteral(
            "https://fshare.vn/file/A https://fshare.vn/file/B")), kUrlMultiple);
        QCOMPARE(multiSpy.count(), 1);
    }

    // HOME-FN-018 — keyword submit emits routeKeyword (the overlay-search trigger).
    void submitKeywordRoutesKeyword()
    {
        QSignalSpy key(m_vm, &HomeSearchViewModel::routeKeyword);
        QCOMPARE(m_vm->submit(QStringLiteral("matrix")), kKeyword);
        QCOMPARE(key.count(), 1);
        QCOMPARE(key.first().first().toString(), QStringLiteral("matrix"));
    }

    // ═══════════════════════════════════════════════════════════════
    //  clearResults / loadMore guards  (FN-001 partial, FN-026)
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-001 (clear half) — typing a keyword then emptying the box clears
    // the searching flag and returns to Idle.
    void emptyingBoxClearsSearchingState()
    {
        m_vm->classify(QStringLiteral("abc"));
        QVERIFY(m_vm->isSearching());
        m_vm->classify(QString());
        QCOMPARE(m_vm->state(), kIdle);
        QVERIFY(!m_vm->isSearching());
        QVERIFY(!m_vm->hasResults());
    }

    // HOME-FN-026 — loadMore() is a safe no-op when there are no results / no
    // more pages (and, here, a null api).
    void loadMoreNoOpWithoutResults()
    {
        m_vm->loadMore();                       // must not crash
        QVERIFY(!m_vm->hasMorePages());
        QCOMPARE(m_vm->resultsModel()->count(), 0);
    }

    // ═══════════════════════════════════════════════════════════════
    //  Async keyword search — driven by FakeSearchApi  (FN-019..025, FN-027)
    //
    //  searchFiles() runs on a QtConcurrent worker and posts results back via
    //  invokeMethod, so these use QTRY_*/qWait to let the queued callback land.
    //  Each builds its own VM around a local fake (api≠nullptr) and drains the
    //  pool before tearing down so no worker dereferences a freed VM.
    // ═══════════════════════════════════════════════════════════════

    // HOME-FN-020 — a full page (== kPerPage) populates the model and flags
    // hasMorePages.
    void searchFullPageHasMore()
    {
        FakeSearchApi api; api.perKeyword = 30;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("matrix"));
        QTRY_COMPARE(vm->resultsModel()->count(), 30);
        QVERIFY(vm->hasResults());
        QVERIFY(vm->hasMorePages());
        QCOMPARE(vm->resultsForKeyword(), QStringLiteral("matrix"));
        QVERIFY(!vm->isSearching());
        drainAndDelete(vm);
    }

    // HOME-FN-021 — an under-filled page stops the paging chase.
    void searchPartialPageNoMore()
    {
        FakeSearchApi api; api.perKeyword = 5;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("kwd"));
        QTRY_COMPARE(vm->resultsModel()->count(), 5);
        QVERIFY(!vm->hasMorePages());
        QVERIFY(!vm->noResultsHit());
        drainAndDelete(vm);
    }

    // HOME-FN-022 — zero results sets noResultsHit and never sticks on loading.
    void searchZeroResults()
    {
        FakeSearchApi api; api.perKeyword = 0;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("zzz"));
        QTRY_VERIFY(vm->noResultsHit());
        QCOMPARE(vm->resultsModel()->count(), 0);
        QVERIFY(!vm->hasResults());
        QVERIFY(!vm->isSearching());
        drainAndDelete(vm);
    }

    // HOME-FN-019 — typing several keystrokes inside the 250ms debounce window
    // collapses to exactly ONE search, for the final keyword.
    void debounceCoalescesToSingleSearch()
    {
        FakeSearchApi api; api.perKeyword = 2;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->classify(QStringLiteral("ab"));     // < min — no schedule
        vm->classify(QStringLiteral("abc"));    // starts the debounce timer
        vm->classify(QStringLiteral("abcd"));   // resets it
        vm->classify(QStringLiteral("abcde"));  // resets it again
        QTRY_COMPARE(api.searchCalls.loadAcquire(), 1);
        QTest::qWait(120);                       // give a stray 2nd call a chance
        QCOMPARE(api.searchCalls.loadAcquire(), 1);
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        { QMutexLocker lk(&api.mtx); QCOMPARE(api.keywords.last(), QStringLiteral("abcde")); }
        delete vm;
    }

    // HOME-FN-025 — loadMore() fetches the next page and APPENDS (model grows,
    // page 2 requested).
    void loadMoreAppendsNextPage()
    {
        FakeSearchApi api; api.perKeyword = 30;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("kwd"));
        QTRY_COMPARE(vm->resultsModel()->count(), 30);
        vm->loadMore();
        QTRY_COMPARE(vm->resultsModel()->count(), 60);
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        { QMutexLocker lk(&api.mtx); QVERIFY(api.pages.contains(2)); }
        delete vm;
    }

    // HOME-FN-027 — a failed search keeps the previous results intact (soft-fail)
    // and clears the loading flag.
    void apiErrorKeepsModel()
    {
        FakeSearchApi api; api.perKeyword = 30;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("kwd"));
        QTRY_COMPARE(vm->resultsModel()->count(), 30);
        api.fail = true;
        vm->submit(QStringLiteral("other"));        // fresh search → errors
        QTRY_VERIFY(!vm->isSearching());
        QCOMPARE(vm->resultsModel()->count(), 30);  // old model preserved
        drainAndDelete(vm);
    }

    // HOME-FN-023 ⚠️ — a slow response superseded by a newer keyword is dropped:
    // the model reflects ONLY the latest query, never the stale one.
    void staleResponseIsDiscarded()
    {
        FakeSearchApi api; api.latencyMs = 150; api.perKeyword = 7;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("aaa"));   // seq 1, sleeps on the worker
        vm->submit(QStringLiteral("bbb"));   // seq 2 — bumps m_requestSeq
        QTRY_COMPARE(vm->resultsForKeyword(), QStringLiteral("bbb"));
        QCOMPARE(vm->resultsModel()->count(), 7);   // bbb only — aaa was dropped
        QTest::qWait(80);                            // let any straggler land
        QCOMPARE(vm->resultsForKeyword(), QStringLiteral("bbb"));
        QCOMPARE(vm->resultsModel()->count(), 7);
        drainAndDelete(vm);
    }

    // HOME-FN-024 ⚠️ — clearResults() bumps the sequence so a response that lands
    // after the box was emptied is discarded — no "ghost" results resurface.
    // (Pins the regression behind the clearResults race that was fixed earlier.)
    void clearResultsCancelsInflightCallback()
    {
        FakeSearchApi api; api.latencyMs = 150; api.perKeyword = 9;
        auto *vm = new HomeSearchViewModel(m_filter, m_download, &api);
        vm->submit(QStringLiteral("aaa"));   // seq 1, sleeps on the worker
        vm->clearResults();                  // seq 2 — invalidates the in-flight cb
        QTest::qWait(350);                   // > latency: response lands & is dropped
        QCoreApplication::processEvents();
        QCOMPARE(vm->resultsModel()->count(), 0);
        QVERIFY(!vm->isSearching());
        QVERIFY(vm->resultsForKeyword().isEmpty());
        drainAndDelete(vm);
    }

private:
    // Drain the pool BEFORE freeing the VM so no worker callback dereferences a
    // dangling collaborator, then delete.
    void drainAndDelete(HomeSearchViewModel *vm)
    {
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete vm;
    }

    QString             m_dictPath;
    BadWordFilter      *m_filter   = nullptr;
    SettingsRepository *m_repo     = nullptr;
    SettingsService    *m_settings = nullptr;
    HistoryRepository  *m_history  = nullptr;
    TransferOrchestrator *m_orch   = nullptr;
    TransferService    *m_service  = nullptr;
    DownloadViewModel  *m_download = nullptr;
    HomeSearchViewModel *m_vm      = nullptr;
};

QTEST_MAIN(TestHomeSearchViewModel)
#include "test_home_search_viewmodel.moc"
