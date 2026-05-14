// SPDX-License-Identifier: Proprietary
#include "BadWordFilter.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
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

} // namespace fsnext
