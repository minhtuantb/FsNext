#include "AuthService.h"
#include "core/api/FshareApi.h"
#include "core/api/OAuthProvider.h"
#include "core/services/OAuthService.h"
#include "core/services/RefreshTokenCoordinator.h"
#include "core/repositories/SettingsRepository.h"
#include "core/util/SecureStore.h"
#include <QDebug>
#include <QPointer>
#include <QtConcurrent>

namespace fsnext {

// Resolve an OAuthConfig by its service id string.
static OAuthConfig configForService(const QString &service)
{
    if (service == QStringLiteral("google"))   return OAuthConfig::google();
    if (service == QStringLiteral("facebook")) return OAuthConfig::facebook();
    if (service == QStringLiteral("fpt-id"))   return OAuthConfig::fptId();
    return {};
}

AuthService::AuthService(IFshareApi *api, SettingsRepository *settings,
                          OAuthService *oauth, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_settings(settings)
    , m_oauth(oauth)
{
    if (m_oauth) {
        connect(m_oauth, &OAuthService::succeeded, this, &AuthService::onOAuthSucceeded);
        connect(m_oauth, &OAuthService::failed,    this, &AuthService::onOAuthFailed);
    }
}

void AuthService::setRefreshCoordinator(RefreshTokenCoordinator *coord)
{
    if (m_refresh == coord) return;
    if (m_refresh) {
        disconnect(m_refresh, nullptr, this, nullptr);
    }
    m_refresh = coord;
    if (!m_refresh) return;

    // Spec §3.4 hard-fail → kick to Login.  Funnel through the existing slot
    // so the UI flow is identical (toast + state reset + isLoggedIn flip).
    connect(m_refresh, &RefreshTokenCoordinator::sessionExpired,
            this,      &AuthService::handleSessionExpired);
    // Soft-fail toast — surface as sessionExpiredNotice but DO NOT log the
    // user out (token still works for now).  QML treats this as a soft toast.
    connect(m_refresh, &RefreshTokenCoordinator::tokenRefreshFailed,
            this,      &AuthService::sessionExpiredNotice);
}

bool AuthService::isLoggedIn() const { return m_isLoggedIn; }
User AuthService::currentUser() const { return m_session.user; }

QString AuthService::savedEmail() const
{
    return m_settings->getString(QStringLiteral("Account/email"), {});
}

QString AuthService::savedPassword() const
{
    if (!m_settings->getBool(QStringLiteral("Account/rememberMe"), false))
        return {};
    // DPAPI-encrypted, tied to current Windows user (see SecureStore.cpp).
    // Returns empty on failure — including legacy base64-obfuscated tokens from
    // earlier builds. That empty falls through to interactive login, which then
    // re-saves the password through encryptToBase64() below, migrating silently.
    const QString encoded = m_settings->getString(QStringLiteral("Account/token"), {});
    return SecureStore::decryptFromBase64(encoded);
}

bool AuthService::rememberMe() const
{
    return m_settings->getBool(QStringLiteral("Account/rememberMe"), false);
}

void AuthService::setRememberMe(bool remember)
{
    m_settings->setBool(QStringLiteral("Account/rememberMe"), remember);
}

void AuthService::login(const QString &email, const QString &password)
{
    auto *api = m_api;
    QPointer<AuthService> guard(this);
    QtConcurrent::run([api, guard, email, password]() mutable {
        qDebug() << "[FsNext] Logging in:" << email;
        auto resp = api->login(email, password);
        QMetaObject::invokeMethod(guard.data(), [guard, resp, email, password]() {
            if (!guard) return;
            auto *self = guard.data();
            if (resp.isSuccess()) {
                self->m_session = resp.data();
                self->m_isLoggedIn = true;
                // Seed the refresh coordinator so the very next FshareApi call
                // observes the new (token, cookie) and can keep them fresh.
                // Must run before the post-login getUserInfo kicks off.
                if (self->m_refresh) {
                    self->m_refresh->onLoginSuccess(
                        self->m_session.token,
                        self->m_session.sessionId,
                        self->m_session.cookie);
                }
                emit self->isLoggedInChanged();

                const bool remember = self->m_settings->getBool(
                    QStringLiteral("Account/rememberMe"), false);

                // Always save email — convenient pre-fill on next login screen
                // even if the user unchecked "remember me" (password is NOT saved
                // in that case, so they still have to re-enter it).
                self->m_settings->setString(QStringLiteral("Account/email"), email);

                if (remember) {
                    // DPAPI-encrypted on Windows (tied to current user SID).
                    // On non-Windows SecureStore falls back to plaintext with a
                    // warning — replace with Keychain/libsecret before shipping
                    // to those platforms.
                    const QString encrypted = SecureStore::encryptToBase64(password);
                    if (!encrypted.isEmpty()) {
                        self->m_settings->setString(QStringLiteral("Account/token"), encrypted);
                        self->m_settings->setBool(QStringLiteral("General/autoLogin"), true);
                    } else {
                        // Encryption failed — don't persist a useless or plaintext
                        // token. User will be asked for password on next startup.
                        qWarning() << "[FsNext] Failed to encrypt password for storage; "
                                      "auto-login disabled.";
                        self->m_settings->setString(QStringLiteral("Account/token"), {});
                        self->m_settings->setBool(QStringLiteral("General/autoLogin"), false);
                    }
                } else {
                    // Wipe any previously-saved password and disable auto-login
                    // so a later "remember me" unchecked run doesn't silently
                    // log in with stale creds.
                    self->m_settings->setString(QStringLiteral("Account/token"), {});
                    self->m_settings->setBool(QStringLiteral("General/autoLogin"), false);
                }

                qDebug() << "[FsNext] Login success, token:" << self->m_session.token.left(8) + "..."
                         << "remember:" << remember;
                emit self->loginSuccess();
                self->fetchUserInfo();
            } else {
                qWarning() << "[FsNext] Login failed:" << resp.error().message
                           << "code:" << resp.error().code;
                emit self->loginFailed(resp.error().message);
            }
        });
    });
}

void AuthService::logout()
{
    auto *api = m_api;
    QPointer<AuthService> guard(this);
    QtConcurrent::run([api, guard]() mutable {
        api->logout();
        QMetaObject::invokeMethod(guard.data(), [guard]() {
            if (!guard) return;
            auto *self = guard.data();
            self->m_session = Session{};
            self->m_isLoggedIn = false;
            // Wipe the coordinator state so a stale lastRefreshAt / token
            // doesn't survive into the next login and re-trigger
            // sessionExpired spuriously.  Spec §6 edge case #8.
            if (self->m_refresh)
                self->m_refresh->onLogout();
            // Clear ALL saved credentials so next startup doesn't silently re-login.
            // Use "General/autoLogin" key to match SettingsRepository/AppSettings.
            self->m_settings->setBool  (QStringLiteral("General/autoLogin"), false);
            self->m_settings->setString(QStringLiteral("Account/token"), {});
            self->m_settings->setString(QStringLiteral("Account/email"), {});
            self->m_settings->setBool  (QStringLiteral("Account/rememberMe"), false);
            // OAuth-specific cleanup
            self->m_settings->setString(QStringLiteral("Account/oauthService"), {});
            self->m_settings->setString(QStringLiteral("Account/oauthRefreshToken"), {});
            self->m_settings->setString(QStringLiteral("Account/oauthPicture"), {});
            emit self->isLoggedInChanged();
            emit self->userChanged();
            qDebug() << "[FsNext] Logged out, credentials cleared";
        });
    });
}

bool AuthService::tryRefreshSession()
{
    // Preferred path (spec §1): the RefreshTokenCoordinator already runs
    // silent re-auth on every authenticated FshareApi call.  External
    // triggers (TransferService receiving a 201 outside the FshareApi
    // path — i.e. the standalone DownloadEngine/UploadEngine HTTP work)
    // can drive a one-shot refresh through it here.
    if (m_refresh) {
        // Spec §6 edge case #1: outside the 7-day window we KNOW a refresh
        // will fail — skip the round-trip and bounce straight to login.
        if (!m_refresh->isPersistedSessionWithinWindow()) {
            handleSessionExpired(tr("Phiên đăng nhập đã quá hạn 7 ngày. "
                                    "Vui lòng đăng nhập lại."));
            emit tryRefreshFinished(false);
            return true;
        }

        // Capture m_refresh by value — coordinator outlives AuthService (owned
        // by AppContext, destroyed AFTER us in reverse-init order).  guard
        // (QPointer) lets us bail out cleanly if AuthService disappears
        // mid-flight.
        auto *coord = m_refresh;
        QPointer<AuthService> guard(this);
        QtConcurrent::run([coord, guard]() {
            const RefreshResult r = coord->handleAuthExpired();
            const bool ok = (r.kind == RefreshResultKind::Success);
            QMetaObject::invokeMethod(guard.data(), [guard, ok]() {
                if (!guard) return;
                emit guard->tryRefreshFinished(ok);
            });
        });
        return true;
    }

    // ── Legacy fallback (no coordinator wired) ───────────────────────────
    if (m_refreshInFlight) {
        // Single-flight: caller can hook tryRefreshFinished and wait.  Return
        // true to mean "a refresh is happening" — caller should NOT bail out
        // to handleSessionExpired immediately.
        return true;
    }
    const QString email = savedEmail();
    const QString password = savedPassword();
    if (email.isEmpty() || password.isEmpty()) {
        // Nothing to refresh with.  Caller will fall back to handleSessionExpired.
        return false;
    }
    m_refreshInFlight = true;

    auto *api = m_api;
    QPointer<AuthService> guard(this);
    QtConcurrent::run([api, guard, email, password]() mutable {
        const auto resp = api->login(email, password);
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            auto *self = guard.data();
            self->m_refreshInFlight = false;
            const bool ok = resp.isSuccess();
            if (ok) {
                self->m_session = resp.data();
                self->m_isLoggedIn = true;
                emit self->isLoggedInChanged();
                qInfo() << "[FsNext] Silent session refresh succeeded";
            } else {
                qWarning() << "[FsNext] Silent session refresh FAILED:"
                           << resp.error().message;
                // Fall through to the regular expired flow.
                self->handleSessionExpired(resp.error().message);
            }
            emit self->tryRefreshFinished(ok);
        });
    });
    return true;
}

void AuthService::autoLogin()
{
    const bool autoLoginEnabled = m_settings->getBool(QStringLiteral("General/autoLogin"), false);

    // DEV builds may override via env vars — FSNEXT_DEV_EMAIL / FSNEXT_DEV_PASSWORD.
    // Kept behind both the build flag AND an explicit env-var opt-in so a leaked
    // dev binary doesn't silently log in with test credentials just by running.
    // Real passwords must NEVER be hardcoded in source.
#ifdef FSNEXT_DEV_BUILD
    const QString devEmail    = qEnvironmentVariable("FSNEXT_DEV_EMAIL");
    const QString devPassword = qEnvironmentVariable("FSNEXT_DEV_PASSWORD");
    const bool devCredsFromEnv = !devEmail.isEmpty() && !devPassword.isEmpty();
#else
    const bool devCredsFromEnv = false;
#endif

    if (!autoLoginEnabled && !devCredsFromEnv) {
        qDebug() << "[FsNext] Auto-login: disabled in settings";
        return;
    }

    // ── Priority 1: OAuth refresh_token ─────────────────────
    const QString oauthService = m_settings->getString(QStringLiteral("Account/oauthService"), {});
    const QString encRefresh   = m_settings->getString(QStringLiteral("Account/oauthRefreshToken"), {});
    if (autoLoginEnabled && !oauthService.isEmpty() && !encRefresh.isEmpty() && m_oauth) {
        const QString refreshTok = SecureStore::decryptFromBase64(encRefresh);
        if (refreshTok.isEmpty()) {
            qWarning() << "[FsNext] Auto-login: saved OAuth refresh_token failed to decrypt "
                          "(different Windows user or corrupted). Clearing.";
            m_settings->setString(QStringLiteral("Account/oauthService"), {});
            m_settings->setString(QStringLiteral("Account/oauthRefreshToken"), {});
            m_settings->setBool  (QStringLiteral("General/autoLogin"), false);
            // Fall through to password path
        } else {
            const OAuthConfig cfg = configForService(oauthService);
            if (cfg.configured) {
                qDebug() << "[FsNext] Auto-login: silent OAuth refresh for" << cfg.name;
                m_oauth->refreshTokens(cfg, refreshTok);
                return;
            }
            qWarning() << "[FsNext] Auto-login: saved OAuth service"
                       << oauthService << "no longer configured";
        }
    }

    // ── Priority 2: password-based saved creds ──────────────
    QString email    = savedEmail();
    QString password = savedPassword();
    const bool hasSavedCreds = !email.isEmpty() && !password.isEmpty();

#ifdef FSNEXT_DEV_BUILD
    if (!hasSavedCreds && devCredsFromEnv) {
        qDebug() << "[FsNext] Auto-login: using DEV credentials from environment";
        login(devEmail, devPassword);
        return;
    }
#endif

    if (!autoLoginEnabled) {
        qDebug() << "[FsNext] Auto-login: disabled";
        return;
    }
    if (!hasSavedCreds) {
        qDebug() << "[FsNext] Auto-login: no saved credentials";
        return;
    }

    qDebug() << "[FsNext] Auto-login with saved password credentials:" << email;
    login(email, password);
}

void AuthService::handleSessionExpired(const QString &message)
{
    // Already logged out — nothing to do.
    if (!m_isLoggedIn) return;

    qWarning() << "[FsNext] Session expired:" << message;

    // Clear in-memory state synchronously so QML's isLoggedIn binding flips
    // immediately and the login view appears. We deliberately do NOT hit the
    // /api/user/logout endpoint (it would 201 too) — local cleanup is enough.
    m_session = Session{};
    m_isLoggedIn = false;
    // Spec §4.5 — wipe the coordinator so the persisted lastRefreshAt is
    // cleared.  Without this, the next app launch would attempt to refresh
    // a token we know is dead and waste a round-trip / leak a soft-fail toast.
    if (m_refresh)
        m_refresh->onLogout();

    // Wipe saved credentials so the next startup shows the login form instead
    // of silently re-attempting auto-login with an expired token.
    if (m_settings) {
        m_settings->setBool  (QStringLiteral("General/autoLogin"), false);
        m_settings->setString(QStringLiteral("Account/token"),     {});
        m_settings->setBool  (QStringLiteral("Account/rememberMe"), false);
        // Keep Account/email so the login form can pre-fill it.
    }

    emit isLoggedInChanged();
    emit userChanged();
    emit sessionExpiredNotice(message);
}

void AuthService::fetchUserInfo()
{
    auto *api = m_api;
    QPointer<AuthService> guard(this);
    QtConcurrent::run([api, guard]() mutable {
        auto resp = api->getUserInfo();
        QMetaObject::invokeMethod(guard.data(), [guard, resp]() {
            if (!guard) return;
            auto *self = guard.data();
            if (resp.isSuccess()) {
                // Fshare's /api/user/get doesn't return avatarUrl (that's from
                // the OAuth provider). Preserve whatever we already have.
                const QString priorAvatar = self->m_session.user.avatarUrl;
                self->m_session.user = resp.data();
                if (self->m_session.user.avatarUrl.isEmpty())
                    self->m_session.user.avatarUrl = priorAvatar;
                emit self->userChanged();
                qDebug() << "[FsNext] User:" << self->m_session.user.name
                         << "email:" << self->m_session.user.email
                         << "level:" << self->m_session.user.level
                         << "avatar:" << (self->m_session.user.avatarUrl.isEmpty() ? "none" : "yes");
            } else {
                qWarning() << "[FsNext] User info failed:" << resp.error().message;
            }
        });
    });
}

// ── OAuth / social login ──────────────────────────────────

void AuthService::loginWithProvider(const OAuthConfig &cfg)
{
    if (!m_oauth) {
        emit loginFailed(tr("OAuth service chưa được khởi tạo."));
        return;
    }
    if (!cfg.configured) {
        emit loginFailed(tr("%1 login chưa được cấu hình. Vui lòng liên hệ admin.").arg(cfg.name));
        return;
    }
    qDebug() << "[FsNext] Starting OAuth flow:" << cfg.name;
    m_oauth->start(cfg);
}

void AuthService::onOAuthSucceeded(const OAuthResult &result)
{
    // Provider flow finished — now hand the access_token + email to Fshare's
    // /api/user/oauth endpoint to obtain a Fshare session token.
    qDebug() << "[FsNext] OAuth provider OK, exchanging for Fshare session:"
             << result.service << "email:" << result.email
             << "refresh_token:" << (result.refreshToken.isEmpty() ? "none" : "present")
             << "picture:" << (result.pictureUrl.isEmpty() ? "none" : "present");

    auto *api = m_api;
    QPointer<AuthService> guard(this);
    const QString service    = result.service;
    const QString token      = result.accessToken;
    const QString email      = result.email;
    const QString refreshTok = result.refreshToken;
    const QString pictureUrl = result.pictureUrl;
    const QString displayName = result.name;

    QtConcurrent::run([api, guard, service, token, email, refreshTok, pictureUrl, displayName]() mutable {
        auto resp = api->loginOauth(service, token, email);
        QMetaObject::invokeMethod(guard.data(),
            [guard, resp, service, email, refreshTok, pictureUrl, displayName]() {
            if (!guard) return;
            auto *self = guard.data();
            if (resp.isSuccess()) {
                self->m_session = resp.data();

                // Attach provider-supplied metadata to the session/user so the
                // UI can render avatar + name right away (before getUserInfo
                // roundtrip from Fshare).
                if (!pictureUrl.isEmpty())
                    self->m_session.user.avatarUrl = pictureUrl;
                if (!displayName.isEmpty() && self->m_session.user.name.isEmpty())
                    self->m_session.user.name = displayName;

                self->m_isLoggedIn = true;
                // Seed the refresh coordinator — same as password login.
                if (self->m_refresh) {
                    self->m_refresh->onLoginSuccess(
                        self->m_session.token,
                        self->m_session.sessionId,
                        self->m_session.cookie);
                }
                emit self->isLoggedInChanged();

                const bool remember = self->m_settings->getBool(
                    QStringLiteral("Account/rememberMe"), false);

                // Always remember the email + picture for UI pre-fill.
                self->m_settings->setString(QStringLiteral("Account/email"), email);
                self->m_settings->setString(QStringLiteral("Account/oauthPicture"), pictureUrl);

                // Clear password-based creds — this user logs in via OAuth now,
                // a stale Account/token from a previous password login would be
                // wrong. We track OAuth separately.
                self->m_settings->setString(QStringLiteral("Account/token"), {});

                if (remember && !refreshTok.isEmpty()) {
                    // DPAPI-encrypt the refresh token before storing in QSettings
                    // (registry on Windows). Ties the blob to this Windows user.
                    const QString encRefresh = SecureStore::encryptToBase64(refreshTok);
                    if (!encRefresh.isEmpty()) {
                        self->m_settings->setString(QStringLiteral("Account/oauthService"), service);
                        self->m_settings->setString(QStringLiteral("Account/oauthRefreshToken"), encRefresh);
                        self->m_settings->setBool  (QStringLiteral("General/autoLogin"), true);
                        qDebug() << "[FsNext] OAuth refresh_token saved (DPAPI encrypted)";
                    } else {
                        qWarning() << "[FsNext] OAuth refresh_token encryption failed — not saving";
                        self->m_settings->setBool(QStringLiteral("General/autoLogin"), false);
                    }
                } else {
                    // Not remembering, or provider didn't return refresh_token:
                    // clear any previous OAuth creds.
                    self->m_settings->setString(QStringLiteral("Account/oauthService"), {});
                    self->m_settings->setString(QStringLiteral("Account/oauthRefreshToken"), {});
                    self->m_settings->setBool  (QStringLiteral("General/autoLogin"), false);
                    if (remember && refreshTok.isEmpty()) {
                        qDebug() << "[FsNext] remember=true but provider didn't return refresh_token "
                                    "(Google: happens when same account re-authorises). "
                                    "Silent auto-login won't be available for this session.";
                    }
                }

                qDebug() << "[FsNext] OAuth login success, Fshare token:"
                         << self->m_session.token.left(8) + "..."
                         << "remember:" << remember;

                // Fire userChanged so the UI can pick up avatar immediately
                emit self->userChanged();
                emit self->loginSuccess();
                self->fetchUserInfo();
            } else {
                qWarning() << "[FsNext] OAuth → Fshare failed:"
                           << resp.error().message << "code:" << resp.error().code;
                emit self->loginFailed(resp.error().message);
            }
        });
    });
}

void AuthService::onOAuthFailed(const QString &message)
{
    qWarning() << "[FsNext] OAuth flow failed:" << message;
    emit loginFailed(message);
}

} // namespace fsnext
