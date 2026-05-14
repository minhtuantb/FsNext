// SPDX-License-Identifier: Proprietary
#include "Pkce.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>

namespace fsnext {

namespace {

// RFC 7636 §4.1: unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
static const char kUnreserved[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-._~";

QString randomString(int len)
{
    const int charsetSize = static_cast<int>(sizeof(kUnreserved) - 1);
    QString out;
    out.reserve(len);
    auto *rng = QRandomGenerator::system();
    for (int i = 0; i < len; ++i)
        out.append(QChar(kUnreserved[rng->bounded(charsetSize)]));
    return out;
}

// BASE64URL without padding (RFC 4648 §5).
QString base64UrlNoPad(const QByteArray &data)
{
    QByteArray b64 = data.toBase64(QByteArray::Base64UrlEncoding
                                 | QByteArray::OmitTrailingEquals);
    return QString::fromLatin1(b64);
}

} // anon

Pkce Pkce::generate()
{
    Pkce p;
    p.codeVerifier = randomString(43);

    const QByteArray hash = QCryptographicHash::hash(
        p.codeVerifier.toLatin1(), QCryptographicHash::Sha256);
    p.codeChallenge = base64UrlNoPad(hash);
    return p;
}

QString generateOAuthState()
{
    // 32 unreserved chars = ~190 bits of entropy — plenty for CSRF.
    return randomString(32);
}

} // namespace fsnext
