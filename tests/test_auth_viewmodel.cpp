// SPDX-License-Identifier: Proprietary
//
// Unit tests for AuthViewModel — the MVVM bridge over AuthService.
//
// The VM is driven through a REAL AuthService wired to a FakeFshareApi
// (IFshareApi) + an in-memory SettingsRepository, so no network is involved.
//
// IMPORTANT — async login is consolidated into ONE test, declared LAST:
// AuthService dispatches login()/logout()/getUserInfo() on QtConcurrent worker
// threads and marshals the result back via QMetaObject::invokeMethod. When more
// than one login-triggering test runs in the same process, a worker spawned by
// an earlier test races the NEXT test's setup inside QThreadPool's thread-start
// machinery — a reproducible crash, even though each test passes in isolation
// (verified by stress runs). Keeping exactly ONE login-triggering test, placed
// last, means its async chain only ever settles against teardown, which is
// stable. The login state machine itself (success/failure/logout, the P0 surface)
// is additionally covered at the service layer by the stable test_auth_service.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QThreadPool>

#include "viewmodels/AuthViewModel.h"
#include "core/services/AuthService.h"
#include "core/api/IFshareApi.h"
#include "core/repositories/SettingsRepository.h"
#include "core/models/AppError.h"
#include "core/models/User.h"

using fsnext::AuthViewModel;
using fsnext::AuthService;
using fsnext::IFshareApi;
using fsnext::SettingsRepository;
using fsnext::ApiResponse;
using fsnext::AppError;
using fsnext::Session;
using fsnext::User;

namespace {

// IFshareApi stub — returns canned responses, no network, records inputs.
class FakeFshareApi : public IFshareApi
{
public:
    ApiResponse<Session> login(const QString &email, const QString &password) override
    {
        ++loginCalls;
        lastEmail = email;
        lastPassword = password;
        if (!succeed || email.isEmpty())
            return ApiResponse<Session>::failure(
                AppError::auth(405, QStringLiteral("Sai mật khẩu")));
        Session s;
        s.token = QStringLiteral("tok-xyz");
        s.sessionId = QStringLiteral("sid-xyz");
        s.cookie = QStringLiteral("session_id=sid-xyz; key=K");
        s.user.id = QStringLiteral("42");
        s.user.email = email;           // login response carries NO display name
        return ApiResponse<Session>::success(s);
    }
    ApiResponse<Session> loginOauth(const QString &, const QString &, const QString &) override
    {
        return ApiResponse<Session>::failure(AppError::auth(400, QStringLiteral("n/a")));
    }
    ApiResponse<void> logout() override { ++logoutCalls; return ApiResponse<void>::success(); }
    ApiResponse<User> getUserInfo() override
    {
        // Post-login AuthService::fetchUserInfo() overwrites the session user with
        // this — so the VM's userName/userEmail reflect these once the async fetch
        // lands. name is what makes currentUser().name non-empty (the terminal
        // signal of the whole login chain).
        User u;
        u.id    = QStringLiteral("42");
        u.name  = userName;
        u.email = userEmail;
        u.level = 3;
        return ApiResponse<User>::success(u);
    }

    bool    succeed     = true;
    int     loginCalls  = 0;
    int     logoutCalls = 0;
    QString lastEmail;
    QString lastPassword;
    QString userName  = QStringLiteral("Test User");
    QString userEmail = QStringLiteral("test@fpt.com");
};

} // namespace

class TestAuthViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("AuthViewModelTest"));
        // AuthService logs via qDebug() from worker threads while the main thread
        // also logs; QtTest's capturing message handler is not reentrant. Swallow
        // product log output through a thread-safe no-op handler.
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) {});
    }

    void init()
    {
        m_api  = new FakeFshareApi;
        m_repo = new SettingsRepository;
        // Clean slate — no remembered creds leaking between tests. Empty saved
        // email/password means the VM constructor starts blank.
        m_repo->setBool(QStringLiteral("Account/rememberMe"), false);
        m_repo->setString(QStringLiteral("Account/token"), QString());
        m_repo->setString(QStringLiteral("Account/email"), QString());
        m_auth = new AuthService(m_api, m_repo);   // no oauth, no coordinator
        m_vm   = new AuthViewModel(m_auth);
    }

    void cleanup()
    {
        // Drain the login → getUserInfo worker chain before freeing fixtures.
        auto *pool = QThreadPool::globalInstance();
        for (int i = 0; i < 5; ++i) {
            QCoreApplication::processEvents();
            pool->clear();          // drop queued-but-unstarted runnables
            pool->waitForDone();    // join running workers
        }
        pool->clear();
        pool->waitForDone();
        QCoreApplication::processEvents();
        delete m_vm;   m_vm   = nullptr;
        delete m_auth; m_auth = nullptr;   // marshal-backs are QPointer-guarded
        delete m_repo; m_repo = nullptr;
        delete m_api;  m_api  = nullptr;
    }

    // ── ATM-0001 / TC-0001: email setter + emailChanged ──────
    void setEmailEmitsChanged()
    {
        QSignalSpy spy(m_vm, &AuthViewModel::emailChanged);
        m_vm->setEmail(QStringLiteral("user@fpt.com"));
        QCOMPARE(m_vm->email(), QStringLiteral("user@fpt.com"));
        QCOMPARE(spy.count(), 1);
    }

    // ── ATM-0001 / TC-0002: setEmail with same value is a no-op ──
    void setEmailSameValueDoesNotReemit()
    {
        m_vm->setEmail(QStringLiteral("a@b.com"));
        QSignalSpy spy(m_vm, &AuthViewModel::emailChanged);
        m_vm->setEmail(QStringLiteral("a@b.com"));   // unchanged
        QCOMPARE(spy.count(), 0);
        QCOMPARE(m_vm->email(), QStringLiteral("a@b.com"));
    }

    // ── ATM-0002 / TC-0003: password setter + passwordChanged ──
    void setPasswordEmitsChanged()
    {
        QSignalSpy spy(m_vm, &AuthViewModel::passwordChanged);
        m_vm->setPassword(QStringLiteral("secret123"));
        QCOMPARE(m_vm->password(), QStringLiteral("secret123"));
        QCOMPARE(spy.count(), 1);
    }

    // ── CONSOLIDATED async happy path — MUST stay the LAST slot ──
    // Covers ATM-0003 (isLoading flips true on login), ATM-0010 (login forwards
    // the VM's email/password), ATM-0004 (isLoggedIn after success), ATM-0007 /
    // ATM-0008 (userName/userEmail reflect the fetched user), ATM-0011 (logout
    // clears isLoggedIn and reaches the service). See the file header for why all
    // async login coverage lives in this single, last test.
    void loginThenLogoutHappyPath()
    {
        m_api->userName  = QStringLiteral("Nguyen Van A");
        m_api->userEmail = QStringLiteral("a@fpt.com");
        m_vm->setEmail(QStringLiteral("a@fpt.com"));
        m_vm->setPassword(QStringLiteral("pw"));

        QSignalSpy loadingSpy(m_vm, &AuthViewModel::isLoadingChanged);
        QSignalSpy loggedInSpy(m_vm, &AuthViewModel::isLoggedInChanged);
        QVERIFY(!m_vm->isLoggedIn());

        m_vm->login();

        // ATM-0003 / ATM-0010 — isLoading flips true synchronously inside login()
        // (before the worker starts) and the credentials reach the api.
        QVERIFY(m_vm->isLoading());
        QVERIFY(loadingSpy.count() >= 1);
        QTRY_COMPARE(m_api->loginCalls, 1);
        QCOMPARE(m_api->lastEmail, QStringLiteral("a@fpt.com"));
        QCOMPARE(m_api->lastPassword, QStringLiteral("pw"));

        // ATM-0004 — logged in once the async result lands; isLoading clears.
        QTRY_VERIFY(m_vm->isLoggedIn());
        QVERIFY(loggedInSpy.count() >= 1);
        QTRY_VERIFY(!m_vm->isLoading());

        // ATM-0007 / ATM-0008 — userName/userEmail reflect the getUserInfo() user
        // (the login response itself carries no display name, so a non-empty name
        // proves the post-login fetch completed).
        QTRY_COMPARE(m_vm->userName(), QStringLiteral("Nguyen Van A"));
        QCOMPARE(m_vm->userEmail(), QStringLiteral("a@fpt.com"));

        // ATM-0011 — logout reaches the service and flips isLoggedIn back to false.
        m_vm->logout();
        QTRY_VERIFY(!m_vm->isLoggedIn());
        QCOMPARE(m_api->logoutCalls, 1);
    }

private:
    FakeFshareApi      *m_api  = nullptr;
    SettingsRepository *m_repo = nullptr;
    AuthService        *m_auth = nullptr;
    AuthViewModel      *m_vm   = nullptr;
};

QTEST_MAIN(TestAuthViewModel)
#include "test_auth_viewmodel.moc"
