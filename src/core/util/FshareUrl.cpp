// SPDX-License-Identifier: Proprietary
#include "FshareUrl.h"

#include <QRegularExpression>

namespace fsnext::FshareUrl {

// Matches, case-insensitively:
//   [(http(s):)?//][www.]fshare.vn/(file|folder)/<linkcode>[/][?query][#frag]
//
// Scheme prefix is intentionally flexible: bare host ("fshare.vn/..."),
// protocol-relative ("//fshare.vn/...") and explicit http/https are all
// accepted. Users paste from many sources (browser bar, share dialogs, HTML)
// and rejecting any of these would surface as "không hợp lệ" for input the
// user thinks is obviously a Fshare link.
//
// linkcode must be [A-Za-z0-9] — Fshare link codes are alphanumeric.
static const QRegularExpression &pattern()
{
    static const QRegularExpression kRe(
        QStringLiteral(R"(^\s*(?:(?:https?:)?//)?(?:www\.)?fshare\.vn/(file|folder)/([A-Za-z0-9]+)/?(?:[?#].*)?\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    return kRe;
}

ParsedLink parse(const QString &rawUrl)
{
    ParsedLink out;
    if (rawUrl.isEmpty()) return out;

    const auto m = pattern().match(rawUrl);
    if (!m.hasMatch()) return out;

    const QString kind = m.captured(1).toLower();
    out.linkcode = m.captured(2);
    out.kind = (kind == QStringLiteral("folder")) ? Kind::Folder : Kind::File;
    return out;
}

QString linkcodeOf(const QString &rawUrl)
{
    return parse(rawUrl).linkcode;
}

// Extract the share-access token from the query string, e.g. "?token=123&x=y"
// → "123". Returns empty if no token= param is present.
static QString extractShareToken(const QString &rawUrl)
{
    const int q = rawUrl.indexOf(QLatin1Char('?'));
    if (q < 0) return {};

    QString qs = rawUrl.mid(q + 1);
    const int hash = qs.indexOf(QLatin1Char('#'));
    if (hash >= 0) qs = qs.left(hash);

    for (const QString &kv : qs.split(QLatin1Char('&'), Qt::SkipEmptyParts)) {
        if (kv.startsWith(QStringLiteral("token="), Qt::CaseInsensitive))
            return kv.mid(6);
    }
    return {};
}

QString canonicalUrl(const QString &rawUrl)
{
    const ParsedLink p = parse(rawUrl);
    if (p.kind == Kind::Invalid) return {};

    const QString path = (p.kind == Kind::Folder)
        ? QStringLiteral("folder") : QStringLiteral("file");
    QString out = QStringLiteral("https://www.fshare.vn/%1/%2").arg(path, p.linkcode);

    // Preserve the share-access token — required by the Fshare API for
    // folder listing / file session creation on token-gated shares.
    const QString token = extractShareToken(rawUrl);
    if (!token.isEmpty())
        out += QStringLiteral("?token=") + token;

    return out;
}

} // namespace fsnext::FshareUrl
