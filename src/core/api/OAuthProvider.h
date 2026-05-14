// SPDX-License-Identifier: Proprietary
#pragma once

#include <QString>
// OAuth client_id + client_secret live in OAuthSecrets.h, which is
// .gitignored so credentials never reach the public repo.  Developers
// copy OAuthSecrets.h.example → OAuthSecrets.h once and fill in real
// values from the provider consoles; CI does the same from secrets store.
#include "OAuthSecrets.h"

namespace fsnext {

/// OAuth 2.0 provider configuration.
///
/// `redirectUri` holds a scheme+host template; the actual port is substituted
/// at runtime because we bind LoopbackServer to an OS-chosen free port.
/// Shape is `http://127.0.0.1:{port}/callback` — the token exchange must
/// echo back the exact redirectUri used in the authorization URL, so the
/// caller must render the same value both places.
struct OAuthConfig {
    QString name;           // Display name ("Google", "Facebook", ...)
    QString service;        // Fshare backend service id ("google" | "facebook" | "fpt-id")
    QString clientId;
    QString clientSecret;   // For desktop clients this is a non-secret;
                            // safety comes from PKCE + the exact redirect URI.
    QString authUrl;        // Authorization endpoint (browser opens this)
    QString tokenUrl;       // Token exchange endpoint (POST)
    QString userinfoUrl;    // Userinfo endpoint (GET with access_token)
    QString scope;          // Space-separated scopes

    // Host used in the auto-generated loopback redirect URI. Defaults to
    // "localhost" which Google + FPT ID accept. Ignored when
    // `fixedRedirectUri` is set (that path bypasses the auto-generated URI
    // entirely).
    QString redirectHost = QStringLiteral("localhost");

    // Fixed loopback port for the auto-generated redirect URI. 0 = let the
    // OS pick a free port (RFC 8252 recommendation). Ignored when
    // `fixedRedirectUri` is set.
    int preferredPort = 0;

    // Overrides the auto-generated loopback redirect URI with a fixed public
    // URL. Used when a provider's App Domains validator rejects every
    // loopback form (Facebook: rejects `localhost`, `127.0.0.1`, private
    // IPs). A public Fshare relay page at this URL must read the leading
    // "<port>." segment from the `state` query param and redirect the
    // browser to `http://localhost:<port>/callback` with the remainder of
    // state + the code/error params preserved. The local LoopbackServer
    // still runs to receive that second-hop redirect.
    QString fixedRedirectUri;

    // Whether the provider has been filled in with real credentials.
    // If false, AuthService refuses to start the flow and surfaces a
    // "configuration pending" message instead.
    bool configured = false;

    // ── Provider factories ──────────────────────────────────

    static OAuthConfig google()
    {
        OAuthConfig c;
        c.name         = QStringLiteral("Google");
        c.service      = QStringLiteral("google");
        c.clientId     = oauth_secrets::kGoogleClientId;
        c.clientSecret = oauth_secrets::kGoogleClientSecret;
        c.authUrl      = QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth");
        c.tokenUrl     = QStringLiteral("https://oauth2.googleapis.com/token");
        c.userinfoUrl  = QStringLiteral("https://openidconnect.googleapis.com/v1/userinfo");
        c.scope        = QStringLiteral("openid email profile");
        // Empty client_id means OAuthSecrets.h wasn't populated (e.g. fresh
        // clone before a developer ran the setup step) — AuthService keys
        // off `configured` to surface "Cấu hình đang chờ" instead of
        // launching a doomed flow.
        c.configured   = !c.clientId.isEmpty() && !c.clientSecret.isEmpty();
        return c;
    }

    static OAuthConfig facebook()
    {
        OAuthConfig c;
        c.name         = QStringLiteral("Facebook");
        c.service      = QStringLiteral("facebook");
        c.clientId     = oauth_secrets::kFacebookClientId;
        c.clientSecret = oauth_secrets::kFacebookClientSecret;
        c.authUrl      = QStringLiteral("https://www.facebook.com/v18.0/dialog/oauth");
        c.tokenUrl     = QStringLiteral("https://graph.facebook.com/v18.0/oauth/access_token");
        c.userinfoUrl  = QStringLiteral("https://graph.facebook.com/me?fields=id,name,email,picture.type(large)");
        c.scope        = QStringLiteral("email public_profile");
        // Facebook rejects every loopback form in App Domains. Redirect goes
        // to a public Fshare page which then forwards to our LoopbackServer
        // on a dynamic port — the port is encoded as the "<port>." prefix on
        // the `state` parameter (stripped by the relay before the second hop).
        c.fixedRedirectUri = QStringLiteral("https://www.fshare.vn/site/confirm-facebook");
        c.configured       = !c.clientId.isEmpty() && !c.clientSecret.isEmpty();
        return c;
    }

    // FPT ID — FTEL SSO (OpenID Connect), production domain accounts.fpt.vn.
    // Per "FTEL SSO Integration Guideline v1.5": PKCE is supported (recommended for
    // public/native clients), token endpoint is {domain}/oauth2/token, scopes include
    // "openid email profile offline" (note: "offline" not "offline_access" — FPT-specific).
    // Discovery document at https://accounts.fpt.vn/.well-known/openid-configuration.
    // Credentials shared with legacy fsharetool's `fshare-client`; registered redirect
    // URIs must include loopback `http://127.0.0.1:*/callback` for this desktop flow
    // (legacy used fshare.vn web wrapper, so loopback may still need to be added).
    static OAuthConfig fptId()
    {
        OAuthConfig c;
        c.name         = QStringLiteral("FPT ID");
        c.service      = QStringLiteral("fpt-id");
        c.clientId     = oauth_secrets::kFptIdClientId;
        c.clientSecret = oauth_secrets::kFptIdClientSecret;
        c.authUrl      = QStringLiteral("https://accounts.fpt.vn/oauth2/authorize");
        c.tokenUrl     = QStringLiteral("https://accounts.fpt.vn/oauth2/token");
        c.userinfoUrl  = QStringLiteral("https://accounts.fpt.vn/userinfo");
        c.scope        = QStringLiteral("openid email profile offline");
        c.configured   = !c.clientId.isEmpty() && !c.clientSecret.isEmpty();
        return c;
    }
};

} // namespace fsnext
