// SPDX-License-Identifier: Proprietary
//
// Unit tests for AuthService — login / logout state transitions, driven against
// a FakeFshareApi (IFshareApi) so no network is involved.
//
// AuthService does its work on QtConcurrent worker threads and marshals the
// result back via QMetaObject::invokeMethod, so the assertions use QSignalSpy
// ::wait() / QTRY_VERIFY to pump the event loop until the async result lands.
//
// The service is created with no OAuthService and no RefreshTokenCoordinator
// (the legacy password path), which is exactly the surface these tests cover.
// cleanup() drains the global thread pool before deleting the fixtures so an
// in-flight getUserInfo() task (kicked off by a successful login) can never
// dereference a freed FakeFshareApi.

#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QSettings>
#include <QThreadPool>

#include "core/services/AuthService.h"
#include "core/api/IFshareApi.h"
#include "core/repositories/SettingsRepository.h"
#include "core/models/AppError.h"

using fsnext::AuthService;
using fsnext::IFshareApi;
using fsnext::SettingsRepository;
using fsnext::ApiResponse;
using fsnext::AppError;
using fsnext::Session;
using fsnext::User;

namespace {

// IFshareApi stub — returns canned responses, no network, counts calls.
class FakeFshareApi : public IFshareApi
{
public:
    ApiResponse<Session> login(const QString &email, const QString &) override
    {
        ++loginCalls;
        if (!succeed)
            return ApiResponse<Session>::failure(
                AppError::auth(405, QStringLiteral("Sai mật khẩu")));
        Session s;
        s.token = QStringLiteral("tok-xyz");
        s.sessionId = QStringLiteral("sid-xyz");
        s.cookie = QStringLiteral("session_id=sid-xyz; key=K");
        s.user.id = QStringLiteral("42");
        s.user.email = email;
        return ApiResponse<Session>::success(s);
    }
    ApiResponse<Session> loginOauth(const QString &, const QString &, const QString &) override
    {
        return ApiResponse<Session>::failure(AppError::auth(400, QStringLiteral("n/a")));
    }
    ApiResponse<void> logout() override { ++logoutCalls; return ApiResponse<void>::success(); }
    ApiResponse<User> getUserInfo() override
    {
        User u;
        u.id = QStringLiteral("42");
        u.name = QStringLiteral("Test User");
        return ApiResponse<User>::success(u);
    }

    bool succeed = true;
    int  loginCalls = 0;
    int  logoutCalls = 0;
};

} // namespace

class TestAuthService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("FsNextTest"));
        QCoreApplication::setApplicationName(QStringLiteral("AuthServiceTest"));
    }

    void init()
    {
        m_api  = new FakeFshareApi;
        m_repo = new SettingsRepository;
        // Clean slate — no remembered creds leaking between tests.
        m_repo->setBool(QStringLiteral("Account/rememberMe"), false);
        m_repo->setString(QStringLiteral("Account/token"), QString());
        m_repo->setString(QStringLiteral("Account/email"), QString());
        m_auth = new AuthService(m_api, m_repo);   // no oauth, no coordinator
    }

    void cleanup()
    {
        // Drain any in-flight worker (e.g. the post-login getUserInfo) BEFORE
        // the fixtures are freed, so no pool task dereferences a dangling api.
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        delete m_auth; m_auth = nullptr;
        delete m_repo; m_repo = nullptr;
        delete m_api;  m_api  = nullptr;
    }

    void loginSuccessSetsLoggedIn()
    {
        QSignalSpy success(m_auth, &AuthService::loginSuccess);
        QVERIFY(!m_auth->isLoggedIn());

        m_auth->login(QStringLiteral("user@x.com"), QStringLiteral("pw"));
        QVERIFY(success.wait(3000));

        QCOMPARE(success.count(), 1);
        QVERIFY(m_auth->isLoggedIn());
        QCOMPARE(m_api->loginCalls, 1);
        // Email is always persisted for next-login pre-fill.
        QCOMPARE(m_repo->getString(QStringLiteral("Account/email")),
                 QStringLiteral("user@x.com"));
    }

    void loginFailureEmitsFailed()
    {
        m_api->succeed = false;
        QSignalSpy failed(m_auth, &AuthService::loginFailed);

        m_auth->login(QStringLiteral("user@x.com"), QStringLiteral("bad"));
        QVERIFY(failed.wait(3000));

        QCOMPARE(failed.count(), 1);
        QVERIFY(!m_auth->isLoggedIn());
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("Sai mật khẩu"));
    }

    void logoutClearsLoggedIn()
    {
        QSignalSpy success(m_auth, &AuthService::loginSuccess);
        m_auth->login(QStringLiteral("u@x.com"), QStringLiteral("p"));
        QVERIFY(success.wait(3000));
        QVERIFY(m_auth->isLoggedIn());

        m_auth->logout();
        QTRY_VERIFY(!m_auth->isLoggedIn());   // logout is async too
        QCOMPARE(m_api->logoutCalls, 1);
    }

    // "Remember me" off → password must NOT be persisted (only the email).
    void rememberMeOffDoesNotPersistToken()
    {
        m_repo->setBool(QStringLiteral("Account/rememberMe"), false);
        QSignalSpy success(m_auth, &AuthService::loginSuccess);
        m_auth->login(QStringLiteral("u@x.com"), QStringLiteral("secret"));
        QVERIFY(success.wait(3000));

        QVERIFY(m_repo->getString(QStringLiteral("Account/token")).isEmpty());
        QVERIFY(!m_repo->getBool(QStringLiteral("General/autoLogin"), false));
    }

private:
    FakeFshareApi      *m_api  = nullptr;
    SettingsRepository *m_repo = nullptr;
    AuthService        *m_auth = nullptr;
};

QTEST_MAIN(TestAuthService)
#include "test_auth_service.moc"
