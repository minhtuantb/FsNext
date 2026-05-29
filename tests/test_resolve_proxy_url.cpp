// SPDX-License-Identifier: Proprietary
// Unit tests for PlatformUtils::resolveProxyUrl — the central proxy-mode → URL
// mapping every libcurl producer (HttpClient API calls, Download/Upload engines)
// resolves through. A regression here makes uploads / downloads silently bypass
// the user's proxy, so the deterministic branches (modes 0 and 2) are covered
// exhaustively and mode 1 (system) is smoke-tested for non-crash.

#include <QtTest/QtTest>
#include "platform/PlatformUtils.h"

using fsnext::PlatformUtils;

class TestResolveProxyUrl : public QObject
{
    Q_OBJECT

private slots:
    // Mode 0 = no proxy. Must return empty regardless of host/port arguments —
    // callers MUST NOT pass the "host:port" combo to CURLOPT_PROXY in this case.
    void noneMode_returnsEmpty_data();
    void noneMode_returnsEmpty();

    // Mode 2 = manual proxy with explicit host + port.
    void manualMode_validInputs_returnsHostColonPort_data();
    void manualMode_validInputs_returnsHostColonPort();
    void manualMode_emptyHost_returnsEmpty();
    void manualMode_whitespaceHost_isTrimmedAndAccepted();
    void manualMode_invalidPort_returnsEmpty_data();
    void manualMode_invalidPort_returnsEmpty();

    // Mode 1 = system proxy. We can't assert a specific value (depends on the
    // machine's OS proxy settings), but the call must not crash and must return
    // a QString (possibly empty).
    void systemMode_doesNotCrash();

    // Unknown modes fall into the default branch (no proxy).
    void unknownMode_returnsEmpty();
};

// ── Mode 0 ───────────────────────────────────────────────────────────────

void TestResolveProxyUrl::noneMode_returnsEmpty_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::newRow("typical-host-port") << QStringLiteral("proxy.local") << 8080;
    QTest::newRow("empty-host-port")   << QString{}                     << 0;
    QTest::newRow("invalid-port")      << QStringLiteral("host")        << -1;
    // Mode 0 must NOT defer to the host/port — the user explicitly chose "no
    // proxy" and we must honour that even if junk leaks through from settings.
    QTest::newRow("ignores-host-port") << QStringLiteral("hostile.invalid") << 99999;
}

void TestResolveProxyUrl::noneMode_returnsEmpty()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QCOMPARE(PlatformUtils::resolveProxyUrl(0, host, port), QString{});
}

// ── Mode 2: valid inputs ─────────────────────────────────────────────────

void TestResolveProxyUrl::manualMode_validInputs_returnsHostColonPort_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("expected");
    QTest::newRow("hostname")    << QStringLiteral("proxy.local")  << 8080
                                 << QStringLiteral("proxy.local:8080");
    QTest::newRow("ipv4")        << QStringLiteral("192.168.1.1")  << 1080
                                 << QStringLiteral("192.168.1.1:1080");
    QTest::newRow("port-1")      << QStringLiteral("a")            << 1
                                 << QStringLiteral("a:1");
    QTest::newRow("port-65535")  << QStringLiteral("a")            << 65535
                                 << QStringLiteral("a:65535");
    QTest::newRow("scheme-host") << QStringLiteral("http://gate")  << 3128
                                 << QStringLiteral("http://gate:3128");
}

void TestResolveProxyUrl::manualMode_validInputs_returnsHostColonPort()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, expected);
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, host, port), expected);
}

// ── Mode 2: edge cases ───────────────────────────────────────────────────

void TestResolveProxyUrl::manualMode_emptyHost_returnsEmpty()
{
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QString{},                  8080), QString{});
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QStringLiteral(""),         8080), QString{});
    // Whitespace-only host trims to empty and must be rejected (otherwise the
    // bare ":port" string would tell libcurl to use the system's hostname).
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QStringLiteral("   "),      8080), QString{});
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QStringLiteral("\t\n  \t"), 8080), QString{});
}

void TestResolveProxyUrl::manualMode_whitespaceHost_isTrimmedAndAccepted()
{
    // Leading/trailing whitespace around a real host is operator-friendly —
    // SettingsService may not strip the value, so resolveProxyUrl owns it.
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QStringLiteral("  proxy.local  "), 8080),
             QStringLiteral("proxy.local:8080"));
}

void TestResolveProxyUrl::manualMode_invalidPort_returnsEmpty_data()
{
    QTest::addColumn<int>("port");
    QTest::newRow("zero")            << 0;
    QTest::newRow("negative")        << -1;
    QTest::newRow("negative-large")  << -65535;
    QTest::newRow("65536")           << 65536;
    QTest::newRow("huge")            << 1'000'000;
}

void TestResolveProxyUrl::manualMode_invalidPort_returnsEmpty()
{
    QFETCH(int, port);
    QCOMPARE(PlatformUtils::resolveProxyUrl(2, QStringLiteral("proxy.local"), port),
             QString{});
}

// ── Mode 1: system proxy (non-deterministic) ─────────────────────────────

void TestResolveProxyUrl::systemMode_doesNotCrash()
{
    // We can't assert a specific value — it depends on the OS's currently-set
    // proxy. We CAN assert: the call returns (no crash) and yields a QString.
    // The host/port arguments must be ignored in system mode (the function
    // returns proxyFromSystem() without consulting them).
    const QString r1 = PlatformUtils::resolveProxyUrl(1, QStringLiteral("ignored"), 8080);
    const QString r2 = PlatformUtils::resolveProxyUrl(1, QString{},                0);
    // Both calls must yield the same OS-derived value — the args don't matter.
    QCOMPARE(r1, r2);
}

// ── Unknown mode ─────────────────────────────────────────────────────────

void TestResolveProxyUrl::unknownMode_returnsEmpty()
{
    // The switch default is "no proxy" — defensive against a future settings
    // schema bump that adds a new mode the current binary doesn't understand.
    QCOMPARE(PlatformUtils::resolveProxyUrl(3,   QStringLiteral("host"), 8080), QString{});
    QCOMPARE(PlatformUtils::resolveProxyUrl(99,  QStringLiteral("host"), 8080), QString{});
    QCOMPARE(PlatformUtils::resolveProxyUrl(-1,  QStringLiteral("host"), 8080), QString{});
}

QTEST_APPLESS_MAIN(TestResolveProxyUrl)
#include "test_resolve_proxy_url.moc"
