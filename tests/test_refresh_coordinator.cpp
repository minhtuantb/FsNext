// SPDX-License-Identifier: Proprietary
//
// Unit tests for RefreshTokenCoordinator — the silent re-auth gate that owns
// the canonical (token, sessionId, cookie) tuple and the 7-day refresh window.
//
// Scope: the NON-NETWORK paths, which is where the highest-cost bug class lives
// (a wrong window check or a botched logout silently drops the user's session).
// We construct the coordinator with http=nullptr and never trigger an actual
// refresh, so no HTTP call is made:
//   • appKey() returns the decoded platform key
//   • session ownership: onLoginSuccess seeds, onLogout wipes
//   • ensureFreshToken() is a no-op (returns true) when not logged in or when
//     the token is still fresh — neither path touches HttpClient
//   • isPersistedSessionWithinWindow() honours the 7-day hard window
//
// The single-flight concurrency + hard/soft classification paths require a
// mockable HttpClient (see docs/BACKLOG.md "IFshareApi interface") and are not
// covered here.

#include <QtTest>
#include <QSignalSpy>
#include <QDateTime>
#include <QCoreApplication>
#include <QSettings>
#include <QSemaphore>
#include <QThread>
#include <QAtomicInt>

#include <thread>
#include <vector>

#include "core/services/RefreshTokenCoordinator.h"
#include "core/repositories/SettingsRepository.h"
#include "core/api/HttpClient.h"

using fsnext::RefreshTokenCoordinator;
using fsnext::RefreshResultKind;
using fsnext::SettingsRepository;
using fsnext::HttpClient;
using fsnext::HttpResponse;

namespace {
constexpr auto kKeyLastRefreshAt = "Account/lastRefreshAt";
constexpr qint64 kOneHourMs = 60LL * 60 * 1000;
constexpr qint64 kEightDaysMs = 8LL * 24 * 60 * 60 * 1000;

HttpResponse jsonResp(int status, const QByteArray &body)
{
    HttpResponse r;
    r.statusCode = status;
    r.body = body;
    return r;
}

// HttpClient subclass that returns a canned response instead of hitting the
// network. Optionally blocks inside post() on a semaphore so the single-flight
// test can hold the leader "in flight" while followers pile up behind the gate.
class FakeHttpClient : public HttpClient
{
public:
    HttpResponse post(const QString &, const QByteArray &,
                      const QMap<QString, QString> & = {}) override
    {
        m_postCount.fetchAndAddOrdered(1);
        if (m_blockGate)
            m_gate.acquire();          // released by the test once followers parked
        return m_resp;
    }
    void setCookie(const QString &c) override { m_lastCookie = c; }

    void setResponse(const HttpResponse &r) { m_resp = r; }
    int  postCount() const { return m_postCount.loadAcquire(); }

    QAtomicInt   m_postCount{0};
    bool         m_blockGate = false;
    QSemaphore   m_gate{0};
    HttpResponse m_resp;
    QString      m_lastCookie;
};
}

class TestRefreshCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Isolate QSettings into a throwaway org/app so the test never touches
        // the real FsNext registry/INI.
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("RefreshCoordTest"));
    }

    void init()
    {
        // Each test starts from a clean persisted state.
        SettingsRepository repo;
        repo.setString(QLatin1String(kKeyLastRefreshAt), QString());
    }

    void appKeyIsNonEmpty()
    {
        // XOR-decoded platform key — value is a secret, but it must decode to a
        // non-empty string or every authed request would ship a blank key.
        QVERIFY(!RefreshTokenCoordinator::appKey().isEmpty());
    }

    void onLoginSuccessSeedsSession()
    {
        SettingsRepository repo;
        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);

        coord.onLoginSuccess(QStringLiteral("tok-123"),
                             QStringLiteral("sess-abc"),
                             QStringLiteral("session_id=sess-abc; key=APPKEY"));

        QCOMPARE(coord.token(), QStringLiteral("tok-123"));
        QCOMPARE(coord.sessionId(), QStringLiteral("sess-abc"));
        QCOMPARE(coord.cookie(), QStringLiteral("session_id=sess-abc; key=APPKEY"));
        // Freshly seeded → inside the 7-day window.
        QVERIFY(coord.isPersistedSessionWithinWindow());
    }

    void onLogoutWipesSession()
    {
        SettingsRepository repo;
        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        coord.onLoginSuccess(QStringLiteral("tok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));

        coord.onLogout();

        QVERIFY(coord.token().isEmpty());
        QVERIFY(coord.sessionId().isEmpty());
        QVERIFY(coord.cookie().isEmpty());
        // Persisted timestamp cleared → window check is false.
        QVERIFY(!coord.isPersistedSessionWithinWindow());
    }

    // ensureFreshToken() must NOT attempt a refresh (which would dereference the
    // null HttpClient) when there is no session — there's nothing to refresh.
    void ensureFreshTokenNoopWhenLoggedOut()
    {
        SettingsRepository repo;
        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        QVERIFY(coord.ensureFreshToken());   // returns true, touches no HTTP
    }

    // A freshly-seeded token is younger than kTokenSoftMaxAgeMs, so the gate
    // returns true immediately without a (null) HTTP round-trip.
    void ensureFreshTokenNoopWhenTokenFresh()
    {
        SettingsRepository repo;
        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        coord.onLoginSuccess(QStringLiteral("tok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));
        QVERIFY(coord.ensureFreshToken());
    }

    // ── 7-day persisted window ──────────────────────────────
    void windowTrueForRecentRefresh()
    {
        SettingsRepository repo;
        const qint64 recent = QDateTime::currentMSecsSinceEpoch() - kOneHourMs;
        repo.setString(QLatin1String(kKeyLastRefreshAt), QString::number(recent));

        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        QVERIFY(coord.isPersistedSessionWithinWindow());
    }

    void windowFalseForStaleRefresh()
    {
        SettingsRepository repo;
        const qint64 stale = QDateTime::currentMSecsSinceEpoch() - kEightDaysMs;
        repo.setString(QLatin1String(kKeyLastRefreshAt), QString::number(stale));

        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        QVERIFY(!coord.isPersistedSessionWithinWindow());  // > 7 days → drop token
    }

    void windowFalseWhenNeverRefreshed()
    {
        SettingsRepository repo;
        repo.setString(QLatin1String(kKeyLastRefreshAt), QString()); // cleared
        RefreshTokenCoordinator coord(/*http=*/nullptr, &repo);
        QVERIFY(!coord.isPersistedSessionWithinWindow());
    }

    // ── handleAuthExpired classification (via FakeHttpClient) ──
    void successRotatesToken()
    {
        SettingsRepository repo;
        FakeHttpClient http;
        http.setResponse(jsonResp(200,
            R"({"code":200,"token":"newtok","session_id":"newsid","msg":"ok"})"));
        RefreshTokenCoordinator coord(&http, &repo);
        coord.onLoginSuccess(QStringLiteral("oldtok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));

        const auto r = coord.handleAuthExpired();
        QVERIFY(r.kind == RefreshResultKind::Success);
        QCOMPARE(r.newToken, QStringLiteral("newtok"));
        QCOMPARE(coord.token(), QStringLiteral("newtok"));   // rotated in place
        QCOMPARE(http.postCount(), 1);
    }

    void hardFailWipesSession()
    {
        SettingsRepository repo;
        FakeHttpClient http;
        http.setResponse(jsonResp(403, R"({"code":403,"msg":"expired"})"));
        RefreshTokenCoordinator coord(&http, &repo);
        coord.onLoginSuccess(QStringLiteral("tok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));

        const auto r = coord.handleAuthExpired();
        QVERIFY(r.kind == RefreshResultKind::HardFail);
        QVERIFY(coord.token().isEmpty());   // token wiped → caller must re-login
    }

    void softFailKeepsToken()
    {
        SettingsRepository repo;
        FakeHttpClient http;
        http.setResponse(jsonResp(500, R"({"code":500})"));   // 5xx, not a hard code
        RefreshTokenCoordinator coord(&http, &repo);
        coord.onLoginSuccess(QStringLiteral("tok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));

        const auto r = coord.handleAuthExpired();
        QVERIFY(r.kind == RefreshResultKind::SoftFail);
        QCOMPARE(coord.token(), QStringLiteral("tok"));   // kept — retry next gesture
    }

    // Concurrent callers must collapse to a SINGLE network refresh (spec §2.1
    // #2). The fake blocks the leader inside post() until every follower has
    // parked on the in-flight wait, so the post-count is a deterministic 1.
    void singleFlightCollapsesConcurrentRefresh()
    {
        SettingsRepository repo;
        FakeHttpClient http;
        http.setResponse(jsonResp(200, R"({"code":200,"token":"newtok","msg":"ok"})"));
        http.m_blockGate = true;
        RefreshTokenCoordinator coord(&http, &repo);
        coord.onLoginSuccess(QStringLiteral("oldtok"), QStringLiteral("sid"),
                             QStringLiteral("ck"));

        constexpr int kFollowers = 6;
        std::vector<RefreshResultKind> kinds(kFollowers + 1, RefreshResultKind::SoftFail);

        // Leader enters post() and blocks on the gate → state stays InFlight.
        std::thread leader([&] { kinds[0] = coord.handleAuthExpired().kind; });
        QTRY_COMPARE(http.postCount(), 1);

        // Followers all observe InFlight and wait on the shared result.
        std::vector<std::thread> followers;
        for (int i = 1; i <= kFollowers; ++i)
            followers.emplace_back([&, i] { kinds[i] = coord.handleAuthExpired().kind; });
        QThread::msleep(120);          // let followers reach the in-flight wait

        http.m_gate.release();         // unblock the leader's post()
        leader.join();
        for (auto &t : followers) t.join();

        QCOMPARE(http.postCount(), 1); // exactly one network round-trip
        for (auto k : kinds)
            QVERIFY(k == RefreshResultKind::Success);   // all share the leader's result
        QCOMPARE(coord.token(), QStringLiteral("newtok"));
    }
};

QTEST_MAIN(TestRefreshCoordinator)
#include "test_refresh_coordinator.moc"
