// SPDX-License-Identifier: Proprietary
// AppSettings model unit tests.
//
// AppSettings (src/core/models/AppSettings.h) is a header-only POD-ish struct
// whose only behaviour is its in-class default initialisers — including the
// platform-conditional `minimizeToTray` (#ifdef Q_OS_WIN). These defaults are
// real contract: SettingsService seeds a fresh config from them, BudgetManager
// reads maxGlobalSlots, the downloader reads downloadThreads/downloadSegments,
// etc. So the defaults and the platform invariant are worth pinning.
//
// Pure trivial "assign a value then read it back" round-trips on plain scalar
// fields (proxyHost/proxyPort/uploadThreads/uploadFolder/language/savedEmail/
// savedToken/downloadFolder...) test the compiler, not our code, so they are
// deliberately collapsed into a single sanity check rather than one test per
// field. The behaviourful parts — the documented default values and the
// 0..3 conflict-policy contract — get explicit assertions.

#include <QtTest>
#include "core/models/AppSettings.h"

using fsnext::AppSettings;

class TestAppSettings : public QObject
{
    Q_OBJECT
private slots:

    // ATM-0293 (TC-0308), ATM-0297 (TC-0310), ATM-0298 (TC-0312),
    // ATM-0303 (TC-0317), ATM-0306 (TC-0323), ATM-0311 (TC-0325),
    // ATM-0300 (TC-0315), ATM-0304 (TC-0320), ATM-0305 (TC-0322)
    // Defaults of a freshly value-initialised AppSettings. These are the
    // numbers SettingsService hands out before the user ever opens Settings,
    // so they are load-bearing.
    void defaultsMatchSpec()
    {
        const AppSettings s{};

        // Numeric tuning defaults (documented in the header comments).
        QCOMPARE(s.fileConflictPolicy, 0);   // 0 = rename, non-destructive
        QCOMPARE(s.downloadThreads,    8);   // recommended VIP default
        QCOMPARE(s.downloadSegments,   16);  // HTTP Range parallel streams
        QCOMPARE(s.uploadThreads,      4);   // recommended upload default
        QCOMPARE(s.maxGlobalSlots,     10);  // matches BudgetManager::Config
        QCOMPARE(s.proxyMode,          0);   // 0 = none
        QCOMPARE(s.proxyPort,          0);

        // Boolean defaults.
        QCOMPARE(s.autoLogin,  false);
        QCOMPARE(s.rememberMe, false);
        QCOMPARE(s.autoStart,  false);
        QCOMPARE(s.autoDownload, false);
        QCOMPARE(s.stayOnTop,  false);
        QCOMPARE(s.darkMode,   false);

        // Strings that must start empty (no leaked path / credential).
        QVERIFY(s.downloadFolder.isEmpty());
        QVERIFY(s.uploadFolder.isEmpty());
        QVERIFY(s.proxyHost.isEmpty());
        QVERIFY(s.savedEmail.isEmpty());
        QVERIFY(s.savedToken.isEmpty());

        // Language seeds to "en" (not empty) — the i18n loader relies on this.
        QCOMPARE(s.language, QStringLiteral("en"));
    }

    // ATM-0293 — minimizeToTray default is platform-conditional. This is the
    // one default with branching logic (#ifdef Q_OS_WIN), so it gets its own
    // assertion: true on Windows (download-tool UX), false elsewhere.
    void minimizeToTrayDefaultIsPlatformDependent()
    {
        const AppSettings s{};
#ifdef Q_OS_WIN
        QCOMPARE(s.minimizeToTray, true);
#else
        QCOMPARE(s.minimizeToTray, false);
#endif
    }

    // ATM-0311 / ATM-0306 — confirmOnClose gates the close-to-tray dialog and
    // defaults ON across all platforms (v6.0 "ask on close" UX). Pinning it
    // here because the X-button flow depends on this being true by default.
    void confirmOnCloseDefaultsOn()
    {
        const AppSettings s{};
        QCOMPARE(s.confirmOnClose, true);
    }

    // ATM-0293 (TC-0309) — fileConflictPolicy is a 0..3 enum-like int. Verify
    // every documented code round-trips (the struct stores them verbatim; the
    // value is the contract the download conflict resolver switches on).
    void fileConflictPolicyAcceptsFullRange()
    {
        AppSettings s{};
        for (int policy = 0; policy <= 3; ++policy) {
            s.fileConflictPolicy = policy;
            QCOMPARE(s.fileConflictPolicy, policy);  // 0=rename,1=overwrite,2=skip,3=ask
        }
    }

    // ATM-0297 (TC-0311), ATM-0298 (TC-0313), ATM-0303 (TC-0318)
    // The transfer-tuning fields must hold their documented edge values: the
    // max caps (downloadThreads=16, downloadSegments=32) and the "0 disables
    // the global ceiling" sentinel for maxGlobalSlots. Clamping itself lives
    // in SettingsService — here we only assert the struct stores the boundary
    // values it is expected to carry.
    void tuningFieldsHoldBoundaryValues()
    {
        AppSettings s{};
        s.downloadThreads  = 16;
        s.downloadSegments = 32;
        s.maxGlobalSlots   = 0;   // 0 = no global ceiling

        QCOMPARE(s.downloadThreads,  16);
        QCOMPARE(s.downloadSegments, 32);
        QCOMPARE(s.maxGlobalSlots,   0);
    }

    // ATM-0300 / ATM-0304 / ATM-0305 / ATM-0306 / ATM-0311
    // Single grouped sanity check for the plain string/bool/credential fields:
    // they round-trip user-supplied values (incl. unicode). Per-field tests
    // would just re-test member assignment, so they are collapsed here.
    void userFieldsRoundTrip()
    {
        AppSettings s{};
        s.downloadFolder = QStringLiteral("D:/Tải về/phim");  // unicode path
        s.savedEmail     = QStringLiteral("a@b.com");
        s.savedToken     = QStringLiteral("base64token==");
        s.rememberMe     = true;
        s.autoLogin      = true;
        s.proxyMode      = 2;
        s.proxyHost      = QStringLiteral("127.0.0.1");
        s.proxyPort      = 8080;

        QCOMPARE(s.downloadFolder, QStringLiteral("D:/Tải về/phim"));
        QCOMPARE(s.savedEmail,     QStringLiteral("a@b.com"));
        QCOMPARE(s.savedToken,     QStringLiteral("base64token=="));
        QVERIFY(s.rememberMe);
        QVERIFY(s.autoLogin);
        QCOMPARE(s.proxyMode, 2);
        QCOMPARE(s.proxyHost, QStringLiteral("127.0.0.1"));
        QCOMPARE(s.proxyPort, 8080);
    }
};

QTEST_GUILESS_MAIN(TestAppSettings)
#include "test_app_settings.moc"
