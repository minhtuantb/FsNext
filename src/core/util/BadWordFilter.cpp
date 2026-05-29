// SPDX-License-Identifier: Proprietary
#include "BadWordFilter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

namespace fsnext {

BadWordFilter::BadWordFilter(QObject *parent)
    : QObject(parent)
{
}

bool BadWordFilter::loadFromResource(const QString &resourcePath)
{
    return loadFromFile(resourcePath);
}

bool BadWordFilter::loadFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[BadWordFilter] cannot open" << filePath
                   << "err=" << f.errorString();
        return false;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[BadWordFilter] parse error:" << err.errorString()
                   << "offset=" << err.offset;
        return false;
    }

    const QJsonObject obj = doc.object();
    m_raw.clear();
    m_stripped.clear();
    m_raw.reserve(obj.size());
    m_stripped.reserve(obj.size());

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString key = normalizeSpaces(it.key().toLower().trimmed());
        if (key.isEmpty()) continue;
        m_raw.insert(key);
        const QString stripped = stripDiacritics(key);
        if (!stripped.isEmpty() && stripped != key)
            m_stripped.insert(stripped);
        else
            m_stripped.insert(stripped.isEmpty() ? key : stripped);
    }

    qInfo() << "[BadWordFilter] loaded" << obj.size() << "entries from" << filePath
            << "(raw=" << m_raw.size() << "stripped=" << m_stripped.size() << ")";
    return !m_raw.isEmpty();
}

// ---------------------------------------------------------------------------
// stripDiacritics — Unicode NFD then drop combining marks (U+0300–U+036F).
// Also folds đ/Đ to d which NFD on Qt does NOT do automatically (it is a
// precomposed letter without decomposition mapping).
// ---------------------------------------------------------------------------
QString BadWordFilter::stripDiacritics(const QString &s)
{
    QString out = s.normalized(QString::NormalizationForm_D);
    QString result;
    result.reserve(out.size());
    for (QChar c : out) {
        const ushort u = c.unicode();
        // Drop combining diacritical marks
        if (u >= 0x0300 && u <= 0x036F) continue;
        // Đ/đ — Latin Extended-A — no decomposition, fold manually
        if (u == 0x0110 || u == 0x0111) { result.append(QLatin1Char('d')); continue; }
        result.append(c);
    }
    return result.toLower();
}

QString BadWordFilter::normalizeSpaces(const QString &s)
{
    static const QRegularExpression kWs(QStringLiteral("[\\s]+"));
    QString t = s;
    t.replace(kWs, QStringLiteral("_"));
    return t;
}

// ---------------------------------------------------------------------------
// applyOverridesFromAppData — read %APPDATA%/<org>/FsNext/badwords-
// overrides.json and apply its add/remove lists on top of the bundled
// dictionary. Compliance team can ship this file independently of an
// installer update. Failures (missing file, malformed JSON) are
// non-fatal — they just leave the bundled dictionary intact.
// ---------------------------------------------------------------------------
int BadWordFilter::applyOverridesFromAppData()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) return 0;
    const QString path = dir + QStringLiteral("/badwords-overrides.json");
    QFile f(path);
    if (!f.exists()) return 0;     // override file absent — normal case
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[BadWordFilter] override file present but unreadable:" << path
                   << f.errorString();
        return 0;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[BadWordFilter] override parse error:" << err.errorString();
        return 0;
    }
    const QJsonObject obj = doc.object();

    int adds = 0, removes = 0;
    const auto applyOne = [&](const QString &word, bool add) -> bool {
        const QString key = normalizeSpaces(word.toLower().trimmed());
        if (key.isEmpty()) return false;
        const QString stripped = stripDiacritics(key);
        if (add) {
            m_raw.insert(key);
            m_stripped.insert(stripped.isEmpty() ? key : stripped);
            return true;
        } else {
            const bool r1 = m_raw.remove(key);
            const bool r2 = m_stripped.remove(stripped.isEmpty() ? key : stripped);
            return r1 || r2;
        }
    };

    if (obj.contains(QStringLiteral("add")) && obj.value(QStringLiteral("add")).isArray()) {
        for (const auto &v : obj.value(QStringLiteral("add")).toArray()) {
            if (v.isString() && applyOne(v.toString(), /*add=*/true)) ++adds;
        }
    }
    if (obj.contains(QStringLiteral("remove")) && obj.value(QStringLiteral("remove")).isArray()) {
        for (const auto &v : obj.value(QStringLiteral("remove")).toArray()) {
            if (v.isString() && applyOne(v.toString(), /*add=*/false)) ++removes;
        }
    }

    qInfo() << "[BadWordFilter] applied overrides from" << path
            << "(added=" << adds << "removed=" << removes
            << "total raw=" << m_raw.size() << "stripped=" << m_stripped.size() << ")";
    return adds + removes;
}

// ---------------------------------------------------------------------------
// check — tokenize the keyword, build 1..3-gram joins, look up in the two
// sets. We test both the raw (diacritic-preserving) and stripped form of
// the input so a user typing "phim sex" hits "phim_sex" and "địt" hits the
// raw entry without us mangling its diacritics first.
// ---------------------------------------------------------------------------
int BadWordFilter::check(const QString &keyword, QString *hitWord) const
{
    if (m_raw.isEmpty() && m_stripped.isEmpty()) return Clean;

    const QString lowered = keyword.toLower().trimmed();
    if (lowered.isEmpty()) return Clean;

    // Tokenize on any non-alphanumeric (but keep '+' for entries like "18+",
    // and treat underscore as a separator since the dictionary uses '_' to
    // join multi-word phrases).
    static const QRegularExpression kSplit(QStringLiteral("[^\\p{L}\\p{N}+]+"));
    const QStringList rawTokens = lowered.split(kSplit, Qt::SkipEmptyParts);
    if (rawTokens.isEmpty()) return Clean;

    QStringList stripTokens;
    stripTokens.reserve(rawTokens.size());
    for (const QString &t : rawTokens)
        stripTokens.append(stripDiacritics(t));

    const int n = rawTokens.size();
    const int maxN = qMin(n, kMaxNgram);
    for (int len = 1; len <= maxN; ++len) {
        for (int i = 0; i + len <= n; ++i) {
            const QString rawNgram   = QStringList(rawTokens.mid(i, len)).join(QLatin1Char('_'));
            const QString stripNgram = QStringList(stripTokens.mid(i, len)).join(QLatin1Char('_'));

            if (m_raw.contains(rawNgram)
                || m_raw.contains(stripNgram)
                || m_stripped.contains(stripNgram)) {
                if (hitWord) *hitWord = rawNgram;
                return Blocked;
            }
        }
    }
    return Clean;
}

QString BadWordFilter::checkHit(const QString &keyword) const
{
    QString hit;
    if (check(keyword, &hit) == Blocked) return hit;
    return {};
}

// ---------------------------------------------------------------------------
// checkExactPhrase — single-key membership lookup, no tokenisation.
//
// Used by the homepage "quoted-mode" search. The caller has already detected
// that the user wrapped the keyword in "…" and stripped the surrounding
// quotes; here we just normalise the inner phrase the same way the dictionary
// is normalised (lower + collapse whitespace runs to '_' + diacritic-strip
// variant) and ask: does this exact key appear in the dictionary?
//
// Contrast with check(): there we tokenise on word boundaries and try every
// 1..3-token n-gram, which makes "phim sex" hit "phim_sex" but also means
// the matcher can't distinguish "Lồng tiếng" (a legitimate VN phrase) from
// a token that happens to share a stem with a 1-token bad word in the dict.
// Quoting opts out of that splitting.
// ---------------------------------------------------------------------------
int BadWordFilter::checkExactPhrase(const QString &phrase, QString *hitWord) const
{
    if (m_raw.isEmpty() && m_stripped.isEmpty()) return Clean;

    const QString lowered = normalizeSpaces(phrase.toLower().trimmed());
    if (lowered.isEmpty()) return Clean;

    const QString stripped = stripDiacritics(lowered);

    if (m_raw.contains(lowered)
        || m_stripped.contains(stripped)
        || (!stripped.isEmpty() && m_raw.contains(stripped))) {
        if (hitWord) *hitWord = lowered;
        return Blocked;
    }
    return Clean;
}

} // namespace fsnext
