#pragma once

#include "core/models/User.h"
#include <QObject>
#include <QString>

namespace fsnext {

class FshareApi;
class SettingsRepository;
class OAuthService;
struct OAuthConfig;
struct OAuthResult;

class AuthService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)

public:
    explicit AuthService(FshareApi *api, SettingsRepository *settings,
                         OAuthService *oauth = nullptr, QObject *parent = nullptr);
    ~AuthService() override = default;

    bool isLoggedIn() const;
    User currentUser() const;

    QString savedEmail() const;
    QString savedPassword() const;
    bool rememberMe() const;

    void login(const QString &email, const QString &password);

    // Social login. Each routes through OAuthService to complete the RFC 8252
    // loopback flow with the given provider, then calls Fshare /api/user/oauth
    // to obtain a Fshare session. Emits loginSuccess or loginFailed as usual.
    void loginWithProvider(const OAuthConfig &cfg);

    void logout();
    void autoLogin();
    void fetchUserInfo();

    void setRememberMe(bool remember);

    // Silent re-login using the saved (DPAPI-encrypted) credentials.  Called
    // by TransferService / FshareApi when an in-flight request hits HTTP 401/
    // 403/201 — re-authenticates without bouncing the user back to the login
    // screen, then signals tryRefreshFinished so the caller can retry the
    // original request.  Returns false synchronously when there are no saved
    // credentials (the caller should fall back to handleSessionExpired).
    //
    // Single-flight: parallel callers within the same network event get the
    // same in-flight refresh.  See ADR 003 D9.
    bool tryRefreshSession();

public slots:
    // Called when another service detects the Fshare session has expired
    // (typically TransferService receiving HTTP 201 from an endpoint that
    // requires auth). Clears in-memory session state synchronously so the
    // UI switches to the login view, then emits sessionExpiredNotice so
    // QML can show a toast.
    void handleSessionExpired(const QString &message);

signals:
    void isLoggedInChanged();
    void userChanged();
    void loginSuccess();
    void loginFailed(const QString &message);

    // Relayed from TransferService / other services — carries the server's
    // error message so the UI can present a localized toast.
    void sessionExpiredNotice(const QString &message);

    // Emitted after tryRefreshSession() finishes.  `ok=true` → caller retries
    // the original request; `ok=false` → caller should bail out and emit
    // sessionExpired itself.
    void tryRefreshFinished(bool ok);

private:
    void onOAuthSucceeded(const OAuthResult &result);
    void onOAuthFailed(const QString &message);

    FshareApi          *m_api      = nullptr;
    SettingsRepository *m_settings = nullptr;
    OAuthService       *m_oauth    = nullptr;
    Session             m_session;
    bool                m_isLoggedIn = false;
    // True while tryRefreshSession() has a network call in flight; concurrent
    // callers wait for tryRefreshFinished instead of stacking redundant logins.
    bool                m_refreshInFlight = false;
};

} // namespace fsnext
