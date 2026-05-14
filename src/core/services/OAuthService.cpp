// SPDX-License-Identifier: Proprietary
#include "OAuthService.h"
#include "core/api/HttpClient.h"
#include "core/net/LoopbackServer.h"
#include "core/util/Pkce.h"

#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QPointer>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace fsnext {

OAuthService::OAuthService(HttpClient *http, QObject *parent)
    : QObject(parent)
    , m_http(http)
{
}

OAuthService::~OAuthService() = default;

void OAuthService::start(const OAuthConfig &cfg)
{
    if (!cfg.configured) {
        emit failed(tr("%1 login chưa được cấu hình.").arg(cfg.name));
        return;
    }
    if (m_inFlight) {
        qWarning() << "[OAuthService] another flow already in flight — ignoring start()";
        return;
    }

    m_cfg = cfg;
    m_state = generateOAuthState();
    m_refreshToken.clear();
    m_accessToken.clear();
    m_userEmail.clear();
    m_userPicture.clear();
    m_userName.clear();
    const Pkce pk = Pkce::generate();
    m_codeVerifier = pk.codeVerifier;

    // Spin up the loopback server BEFORE opening the browser so there's no
    // window where the redirect could race. The server picks its own port;
    // we plug it into the redirect_uri next.
    //
    // Timeout is 120s — long enough for a typical user to log in + approve
    // consent, short enough to fail fast when the redirect URI is misconfigured
    // (Google shows an error page and never redirects; only timeout rescues us).
    auto *srv = new LoopbackServer(this);
    if (!srv->start(/*timeoutSec=*/120, /*preferredPort=*/m_cfg.preferredPort)) {
        // Fixed-port bind failure is almost always "port already in use" —
        // either a previous flow didn't clean up or another app is holding it.
        // Surface a specific hint for that case so the user knows what to do.
        if (m_cfg.preferredPort > 0) {
            emit failed(tr("Không thể mở cổng %1 cho đăng nhập %2 — cổng đang bận.")
                           .arg(m_cfg.preferredPort).arg(m_cfg.name));
        } else {
            emit failed(tr("Không thể mở cổng localhost cho đăng nhập OAuth."));
        }
        return;
    }

    // Two modes:
    //   • Direct loopback (Google, FPT ID) — browser redirects straight to
    //     http://localhost:<port>/callback. m_redirectUri is that URL.
    //   • Relay (Facebook) — browser redirects to fixedRedirectUri on
    //     fshare.vn; that page forwards to our local port which it learns
    //     from the "<port>." prefix we prepend to `state` below. The token
    //     exchange must echo the same redirect_uri Facebook saw, so
    //     m_redirectUri holds the relay URL (not the loopback URL).
    const bool usingRelay = !m_cfg.fixedRedirectUri.isEmpty();
    if (usingRelay) {
        m_redirectUri = m_cfg.fixedRedirectUri;
    } else {
        m_redirectUri = QStringLiteral("http://%1:%2/callback")
                            .arg(m_cfg.redirectHost)
                            .arg(srv->port());
    }
    // State actually sent to the provider. In relay mode we prepend the
    // local port so the relay page knows where to forward — it strips that
    // prefix before the browser hits LoopbackServer, so state validation
    // still compares the original random value on our side.
    const QString stateForAuth = usingRelay
        ? (QString::number(srv->port()) + QLatin1Char('.') + m_state)
        : m_state;
    m_inFlight = true;

    // Capture outcome → our handlers
    QPointer<OAuthService> self(this);
    connect(srv, &LoopbackServer::codeReceived, this,
        [self, this](const QString &code, const QString &state) {
            if (!self) return;
            if (state != m_state) {
                emit failed(tr("Phản hồi OAuth không khớp (state mismatch — có thể do tấn công CSRF)."));
                m_inFlight = false;
                return;
            }
            onAuthCodeReceived(code);
        });
    connect(srv, &LoopbackServer::errorReceived, this,
        [self, this](const QString &error, const QString &desc) {
            if (!self) return;
            onAuthError(desc.isEmpty() ? error : error + QStringLiteral(": ") + desc);
        });
    connect(srv, &LoopbackServer::timedOut, this, [self, this]() {
        if (!self) return;
        // Most common cause of timeout: Google shows a redirect_uri_mismatch
        // error page and never redirects back to our loopback. Surface a
        // diagnostic hint so the developer knows where to look.
        onAuthError(tr(
            "Hết thời gian chờ đăng nhập %1 (2 phút).\n"
            "Kiểm tra Google Cloud Console: redirect_uri phải là "
            "\"http://localhost\" (không có port hoặc path)."
        ).arg(m_cfg.name));
    });

    // Build authorization URL (RFC 6749 §4.1.1 + RFC 7636 for PKCE)
    QUrl url(m_cfg.authUrl);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"),             m_cfg.clientId);
    q.addQueryItem(QStringLiteral("redirect_uri"),          m_redirectUri);
    q.addQueryItem(QStringLiteral("response_type"),         QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("scope"),                 m_cfg.scope);
    q.addQueryItem(QStringLiteral("state"),                 stateForAuth);
    q.addQueryItem(QStringLiteral("code_challenge"),        pk.codeChallenge);
    q.addQueryItem(QStringLiteral("code_challenge_method"), QString::fromLatin1(Pkce::challengeMethod));
    // Google-specific:
    //   • prompt=select_account — let user pick account
    //   • access_type=offline   — request a refresh_token so we can re-auth
    //                              silently on next app launch. The refresh
    //                              token is only returned on the FIRST
    //                              authorization with `offline` (or when
    //                              prompt=consent forces re-consent); that's
    //                              acceptable — if the user revokes and
    //                              re-authorises, they do so interactively.
    if (m_cfg.service == QStringLiteral("google")) {
        q.addQueryItem(QStringLiteral("prompt"),      QStringLiteral("select_account"));
        q.addQueryItem(QStringLiteral("access_type"), QStringLiteral("offline"));
    }
    url.setQuery(q);

    qDebug() << "[OAuthService] opening browser for" << m_cfg.name
             << "redirect_uri=" << m_redirectUri;

    if (!QDesktopServices::openUrl(url)) {
        emit failed(tr("Không thể mở trình duyệt cho %1.").arg(m_cfg.name));
        m_inFlight = false;
    }
}

void OAuthService::onAuthError(const QString &message)
{
    qWarning() << "[OAuthService] auth error:" << message;
    m_inFlight = false;
    emit failed(message);
}

void OAuthService::onAuthCodeReceived(const QString &code)
{
    qDebug() << "[OAuthService] received authorization code; exchanging for token";
    exchangeCodeForToken(code);
}

void OAuthService::exchangeCodeForToken(const QString &code)
{
    // POST application/x-www-form-urlencoded — required by OAuth 2.0 token endpoint.
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"),    QStringLiteral("authorization_code"));
    body.addQueryItem(QStringLiteral("code"),          code);
    body.addQueryItem(QStringLiteral("redirect_uri"),  m_redirectUri);
    body.addQueryItem(QStringLiteral("client_id"),     m_cfg.clientId);
    body.addQueryItem(QStringLiteral("code_verifier"), m_codeVerifier);
    // Google "installed" apps still pass client_secret alongside PKCE.
    if (!m_cfg.clientSecret.isEmpty())
        body.addQueryItem(QStringLiteral("client_secret"), m_cfg.clientSecret);

    const QByteArray postData = body.toString(QUrl::FullyEncoded).toUtf8();
    const QString tokenUrl = m_cfg.tokenUrl;

    QPointer<OAuthService> self(this);
    auto *http = m_http;
    QtConcurrent::run([self, http, tokenUrl, postData]() {
        QMap<QString, QString> headers;
        headers[QStringLiteral("Content-Type")] = QStringLiteral("application/x-www-form-urlencoded");
        const HttpResponse resp = http->post(tokenUrl, postData, headers);

        QMetaObject::invokeMethod(self.data(), [self, resp]() {
            if (!self) return;
            if (resp.statusCode != 200) {
                // Log the raw response body so the root cause is visible in
                // the log file (typical Google 400: {"error":"invalid_grant",
                // "error_description":"Bad Request"} or similar).
                qWarning().noquote() << "[OAuthService] Token exchange failed HTTP"
                                     << resp.statusCode << "body:" << resp.body;
                // Try to surface Google's structured error message if present
                QString detail;
                const auto errDoc = QJsonDocument::fromJson(resp.body);
                if (errDoc.isObject()) {
                    const auto o = errDoc.object();
                    const QString e = o.value(QStringLiteral("error")).toString();
                    const QString d = o.value(QStringLiteral("error_description")).toString();
                    if (!e.isEmpty()) detail = e + (d.isEmpty() ? QString{} : QStringLiteral(" — ") + d);
                }
                self->onAuthError(tr("Đổi mã OAuth thất bại (HTTP %1)%2.")
                                   .arg(resp.statusCode)
                                   .arg(detail.isEmpty() ? QString{} : QStringLiteral(": ") + detail));
                return;
            }
            const auto doc = QJsonDocument::fromJson(resp.body);
            if (!doc.isObject()) {
                self->onAuthError(tr("Phản hồi OAuth không phải JSON hợp lệ."));
                return;
            }
            const auto obj = doc.object();
            const QString accessToken  = obj.value(QStringLiteral("access_token")).toString();
            const QString refreshToken = obj.value(QStringLiteral("refresh_token")).toString();
            if (accessToken.isEmpty()) {
                self->onAuthError(tr("Không nhận được access_token từ %1.").arg(self->m_cfg.name));
                return;
            }
            self->m_accessToken = accessToken;
            // refresh_token is optional — Google only returns it when
            // access_type=offline was in the auth URL, and typically only on
            // the FIRST authorization. Missing refresh just means no silent
            // auto-login next time; interactive login still works.
            if (!refreshToken.isEmpty())
                self->m_refreshToken = refreshToken;
            self->fetchUserInfo(accessToken);
        });
    });
}

void OAuthService::fetchUserInfo(const QString &accessToken)
{
    const QString infoUrl = m_cfg.userinfoUrl;

    QPointer<OAuthService> self(this);
    auto *http = m_http;
    QtConcurrent::run([self, http, infoUrl, accessToken]() {
        QMap<QString, QString> headers;
        headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + accessToken;
        const HttpResponse resp = http->get(infoUrl, headers);

        QMetaObject::invokeMethod(self.data(), [self, resp, accessToken]() {
            if (!self) return;
            if (resp.statusCode != 200) {
                self->onAuthError(tr("Lấy thông tin tài khoản OAuth thất bại (HTTP %1).").arg(resp.statusCode));
                return;
            }
            const auto doc = QJsonDocument::fromJson(resp.body);
            if (!doc.isObject()) {
                self->onAuthError(tr("Thông tin tài khoản OAuth không phải JSON hợp lệ."));
                return;
            }
            const auto obj = doc.object();
            QString email = obj.value(QStringLiteral("email")).toString();
            // FPT ID fallback: if only phone is returned, synthesize "<phone>@fshare.vn".
            if (email.isEmpty() && self->m_cfg.service == QStringLiteral("fpt-id")) {
                QString phone = obj.value(QStringLiteral("phone")).toString();
                if (phone.startsWith(QLatin1Char('0')))
                    phone = QStringLiteral("84") + phone.mid(1);
                if (!phone.isEmpty())
                    email = phone + QStringLiteral("@fshare.vn");
            }
            if (email.isEmpty()) {
                self->onAuthError(tr("Không lấy được email từ %1.").arg(self->m_cfg.name));
                return;
            }

            self->m_userEmail   = email;
            self->m_userName    = obj.value(QStringLiteral("name")).toString();
            // Google returns `picture` (plain URL). Facebook nests it in
            // picture.data.url — handle both shapes.
            QString pic = obj.value(QStringLiteral("picture")).toString();
            if (pic.isEmpty()) {
                const auto picObj = obj.value(QStringLiteral("picture")).toObject();
                pic = picObj.value(QStringLiteral("data")).toObject()
                            .value(QStringLiteral("url")).toString();
            }
            self->m_userPicture = pic;

            self->emitSuccess();
        });
    });
}

void OAuthService::emitSuccess()
{
    OAuthResult r;
    r.accessToken  = m_accessToken;
    r.refreshToken = m_refreshToken;
    r.email        = m_userEmail;
    r.pictureUrl   = m_userPicture;
    r.name         = m_userName;
    r.service      = m_cfg.service;
    m_inFlight = false;
    emit succeeded(r);
}

void OAuthService::refreshTokens(const OAuthConfig &cfg, const QString &refreshToken)
{
    if (!cfg.configured) {
        emit failed(tr("%1 login chưa được cấu hình.").arg(cfg.name));
        return;
    }
    if (refreshToken.isEmpty()) {
        emit failed(tr("Không có refresh_token đã lưu cho %1.").arg(cfg.name));
        return;
    }
    if (m_inFlight) {
        qWarning() << "[OAuthService] refreshTokens: another flow in flight";
        return;
    }

    // Set minimal context so downstream steps (fetchUserInfo / emitSuccess)
    // use the right config but DON'T reset m_refreshToken — we want to
    // preserve the existing saved one in the result (refresh endpoint often
    // doesn't return a new refresh_token; callers keep using the old one).
    m_cfg = cfg;
    m_accessToken.clear();
    m_userEmail.clear();
    m_userPicture.clear();
    m_userName.clear();
    m_refreshToken = refreshToken;   // keep; will be overwritten if server rotates it
    m_inFlight = true;

    // POST to token endpoint per RFC 6749 §6
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"),    QStringLiteral("refresh_token"));
    body.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    body.addQueryItem(QStringLiteral("client_id"),     m_cfg.clientId);
    if (!m_cfg.clientSecret.isEmpty())
        body.addQueryItem(QStringLiteral("client_secret"), m_cfg.clientSecret);

    const QByteArray postData = body.toString(QUrl::FullyEncoded).toUtf8();
    const QString tokenUrl = m_cfg.tokenUrl;

    qDebug() << "[OAuthService] silent refresh for" << m_cfg.name;

    QPointer<OAuthService> self(this);
    auto *http = m_http;
    QtConcurrent::run([self, http, tokenUrl, postData]() {
        QMap<QString, QString> headers;
        headers[QStringLiteral("Content-Type")] = QStringLiteral("application/x-www-form-urlencoded");
        const HttpResponse resp = http->post(tokenUrl, postData, headers);

        QMetaObject::invokeMethod(self.data(), [self, resp]() {
            if (!self) return;
            if (resp.statusCode != 200) {
                // 400 / 401 here typically means the refresh_token has been
                // revoked (user signed out elsewhere, or consent withdrawn).
                // The caller should clear saved creds and fall back to
                // interactive login.
                self->onAuthError(tr("Refresh token đã hết hạn hoặc bị thu hồi (HTTP %1).")
                                   .arg(resp.statusCode));
                return;
            }
            const auto doc = QJsonDocument::fromJson(resp.body);
            if (!doc.isObject()) {
                self->onAuthError(tr("Phản hồi refresh không phải JSON hợp lệ."));
                return;
            }
            const auto obj = doc.object();
            const QString accessToken = obj.value(QStringLiteral("access_token")).toString();
            if (accessToken.isEmpty()) {
                self->onAuthError(tr("Refresh không trả về access_token."));
                return;
            }
            self->m_accessToken = accessToken;
            // Some providers rotate refresh_token on refresh; pick up the new
            // one if present, otherwise keep the existing.
            const QString newRefresh = obj.value(QStringLiteral("refresh_token")).toString();
            if (!newRefresh.isEmpty())
                self->m_refreshToken = newRefresh;
            self->fetchUserInfo(accessToken);
        });
    });
}

} // namespace fsnext
