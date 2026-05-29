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

#include "core/services/RefreshTokenCoordinator.h"
#include "core/repositories/SettingsRepository.h"

using fsnext::RefreshTokenCoordinator;
using fsnext::SettingsRepository;

namespace {
constexpr auto kKeyLastRefreshAt = "Account/lastRefreshAt";
constexpr qint64 kOneHourMs = 60LL * 60 * 1000;
constexpr qint64 kEightDaysMs = 8LL * 24 * 60 * 60 * 1000;
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
};

QTEST_MAIN(TestRefreshCoordinator)
#include "test_refresh_coordinator.moc"
