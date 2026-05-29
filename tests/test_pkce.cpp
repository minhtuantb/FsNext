// SPDX-License-Identifier: Proprietary
//
// Unit tests for fsnext::Pkce — PKCE (RFC 7636) material for OAuth 2.0.
// Pure crypto/string logic: QCryptographicHash + QRandomGenerator only, no
// network/DB/QObject, so this links just Pkce.cpp + Qt6::Core/Test.
//
// Covers:
//   ATM-0420  Pkce::generate()        — verifier 43 chars, challenge == BASE64URL(SHA256(verifier)) no-pad, S256
//   ATM-0421  generateOAuthState()    — 32 URL-safe chars, randomness
//
// We re-derive the expected challenge independently (QCryptographicHash on the
// verifier's Latin-1 bytes, base64url, strip '=') and assert byte equality, so
// the test validates the S256 transform rather than re-stating the impl.

#include <QtTest>
#include <QByteArray>
#include <QCryptographicHash>
#include <QString>

#include "core/util/Pkce.h"

using namespace fsnext;

namespace {

// True iff every char is in the RFC 7636 unreserved set [A-Za-z0-9-._~],
// which is also URL-safe (no padding/+//).
bool isUrlSafeUnreserved(const QString &s)
{
    for (const QChar c : s) {
        const ushort u = c.unicode();
        const bool ok =
            (u >= 'A' && u <= 'Z') ||
            (u >= 'a' && u <= 'z') ||
            (u >= '0' && u <= '9') ||
            u == '-' || u == '.' || u == '_' || u == '~';
        if (!ok)
            return false;
    }
    return true;
}

// Independent reference implementation of the S256 challenge.
QString expectedChallenge(const QString &verifier)
{
    const QByteArray hash = QCryptographicHash::hash(
        verifier.toLatin1(), QCryptographicHash::Sha256);
    QByteArray b64 = hash.toBase64(QByteArray::Base64UrlEncoding
                                 | QByteArray::OmitTrailingEquals);
    return QString::fromLatin1(b64);
}

} // namespace

class TestPkce : public QObject
{
    Q_OBJECT

private slots:
    // ATM-0420 / TC-0469
    void generate_verifierLengthAndChallengeMatchesSha256();
    // ATM-0420 / TC-0470
    void generate_distinctVerifiersAcrossCalls();
    // ATM-0421 / TC-0471
    void oauthState_urlSafe32Chars();
    // ATM-0421 / TC-0472
    void oauthState_distinctAcrossCalls();
};

void TestPkce::generate_verifierLengthAndChallengeMatchesSha256()
{
    // ATM-0420
    const Pkce pk = Pkce::generate();

    // Verifier: exactly 43 URL-safe unreserved chars.
    QCOMPARE(pk.codeVerifier.length(), 43);
    QVERIFY2(isUrlSafeUnreserved(pk.codeVerifier),
             qPrintable("verifier has non-unreserved char: " + pk.codeVerifier));

    // challengeMethod must be S256.
    QCOMPARE(QString::fromLatin1(Pkce::challengeMethod), QStringLiteral("S256"));

    // Challenge == BASE64URL(SHA256(verifier)) without padding — derived
    // independently and compared byte-for-byte.
    QCOMPARE(pk.codeChallenge, expectedChallenge(pk.codeVerifier));

    // No-pad + URL-safe invariants on the challenge string itself.
    QVERIFY2(!pk.codeChallenge.contains('='), "challenge must not contain padding '='");
    QVERIFY2(!pk.codeChallenge.contains('+') && !pk.codeChallenge.contains('/'),
             "challenge must be base64url (no '+' or '/')");
    QVERIFY(!pk.codeChallenge.isEmpty());

    // SHA256 = 32 bytes → base64url no-pad = 43 chars.
    QCOMPARE(pk.codeChallenge.length(), 43);
}

void TestPkce::generate_distinctVerifiersAcrossCalls()
{
    // ATM-0420 — randomness: two generate() calls differ.
    const Pkce a = Pkce::generate();
    const Pkce b = Pkce::generate();

    QVERIFY2(a.codeVerifier != b.codeVerifier,
             "two generate() calls produced identical verifiers");
    // Distinct verifiers must yield distinct challenges (SHA256 is injective
    // here for practical purposes).
    QVERIFY(a.codeChallenge != b.codeChallenge);

    // Each side independently satisfies the S256 contract.
    QCOMPARE(a.codeChallenge, expectedChallenge(a.codeVerifier));
    QCOMPARE(b.codeChallenge, expectedChallenge(b.codeVerifier));
}

void TestPkce::oauthState_urlSafe32Chars()
{
    // ATM-0421
    const QString state = generateOAuthState();
    QCOMPARE(state.length(), 32);
    QVERIFY2(isUrlSafeUnreserved(state),
             qPrintable("state has non-URL-safe char: " + state));
}

void TestPkce::oauthState_distinctAcrossCalls()
{
    // ATM-0421 — CSRF token must not be predictable: two calls differ.
    const QString s1 = generateOAuthState();
    const QString s2 = generateOAuthState();
    QVERIFY2(s1 != s2, "two generateOAuthState() calls produced identical strings");
}

QTEST_GUILESS_MAIN(TestPkce)
#include "test_pkce.moc"
