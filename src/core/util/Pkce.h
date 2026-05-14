// SPDX-License-Identifier: Proprietary
#pragma once

#include <QString>

namespace fsnext {

/// PKCE (Proof Key for Code Exchange, RFC 7636) material for OAuth 2.0.
///
/// Use:
///     const Pkce pk = Pkce::generate();
///     // Include pk.codeChallenge + "&code_challenge_method=S256" in auth URL
///     // After receiving the code, include pk.codeVerifier in the token POST
///
/// The verifier is 43 URL-safe characters (the spec allows 43–128); the
/// challenge is BASE64URL(SHA256(verifier)) without padding.
struct Pkce {
    QString codeVerifier;   // random 43 chars [A-Z a-z 0-9 - . _ ~]
    QString codeChallenge;  // BASE64URL(SHA-256(codeVerifier)) w/o padding
    static constexpr const char *challengeMethod = "S256";

    static Pkce generate();
};

/// Short random URL-safe string used as the OAuth `state` parameter for CSRF
/// defence. Caller compares the value returned in the redirect against the
/// value it sent — a mismatch must abort the flow.
QString generateOAuthState();

} // namespace fsnext
