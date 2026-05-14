// SPDX-License-Identifier: Proprietary
#pragma once

#include "core/api/OAuthProvider.h"
#include <QObject>
#include <QString>

namespace fsnext {

class HttpClient;

/// Result of a successful OAuth flow: the provider's access_token plus the
/// user's email (extracted from the userinfo endpoint). The email is what
/// Fshare's /api/user/oauth endpoint needs to link the Fshare account.
struct OAuthResult {
    QString accessToken;
    QString refreshToken;   // may be empty (provider didn't return one — no auto-login)
    QString email;
    QString pictureUrl;     // avatar URL from userinfo (Google: `picture`)
    QString name;           // user's display name from userinfo
    QString service;        // "google" | "facebook" | "fpt-id" — mirrors OAuthConfig::service
};

/// Orchestrates the RFC 8252 loopback OAuth 2.0 + PKCE flow end-to-end.
///
///   1. Start LoopbackServer on a random port
///   2. Render redirect_uri with the actual port
///   3. Open the authorization URL in the user's default browser
///   4. Wait for the browser to redirect with ?code=...
///   5. Validate the `state` parameter (CSRF)
///   6. POST to the token endpoint to exchange code → access_token (PKCE)
///   7. GET the userinfo endpoint to fetch the user's email
///   8. Emit succeeded(result) or failed(message)
///
/// Exactly ONE signal fires per start() invocation. Safe to reuse across flows
/// (previous in-flight state is cancelled when start() is called again).
class OAuthService : public QObject
{
    Q_OBJECT

public:
    explicit OAuthService(HttpClient *http, QObject *parent = nullptr);
    ~OAuthService() override;

    /// Begin an interactive flow (opens browser). Returns immediately;
    /// result comes via signals. If `cfg.configured == false`, emits failed()
    /// synchronously.
    void start(const OAuthConfig &cfg);

    /// Non-interactive refresh using a previously-obtained refresh_token.
    /// No browser is opened. Emits succeeded() with a fresh access_token
    /// (and re-fetched userinfo) on success, failed() if the refresh_token
    /// has been revoked or network fails. Used by AuthService::autoLogin()
    /// to re-establish a session silently at app startup.
    void refreshTokens(const OAuthConfig &cfg, const QString &refreshToken);

signals:
    void succeeded(const fsnext::OAuthResult &result);
    void failed(const QString &message);

private:
    // Step handlers (invoked on Qt main thread; network I/O runs on QtConcurrent)
    void onAuthCodeReceived(const QString &code);
    void onAuthError(const QString &message);
    void exchangeCodeForToken(const QString &code);
    void fetchUserInfo(const QString &accessToken);
    void emitSuccess();   // combines access+refresh+userinfo into OAuthResult

    HttpClient   *m_http = nullptr;
    OAuthConfig   m_cfg;
    QString       m_state;           // CSRF token
    QString       m_codeVerifier;    // PKCE
    QString       m_redirectUri;     // http://127.0.0.1:<port>/callback (port chosen per flow)
    QString       m_accessToken;     // filled by exchangeCodeForToken / refreshTokens
    QString       m_refreshToken;    // filled by exchangeCodeForToken (may be empty)
    QString       m_userEmail;
    QString       m_userPicture;
    QString       m_userName;
    bool          m_inFlight = false;
};

} // namespace fsnext
