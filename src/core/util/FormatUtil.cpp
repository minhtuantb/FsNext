// SPDX-License-Identifier: Proprietary
#include "FormatUtil.h"

#include <QDateTime>
#include <QLocale>
#include <cmath>

namespace fsnext::FormatUtil {

QString humanBytes(qint64 bytes, bool emptyOnZero)
{
    if (bytes <= 0)
        return emptyOnZero ? QString{} : QStringLiteral("0 B");

    static const char *const units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    constexpr int maxIdx = sizeof(units) / sizeof(units[0]) - 1;

    double v = static_cast<double>(bytes);
    int i = 0;
    while (v >= 1024.0 && i < maxIdx) { v /= 1024.0; ++i; }

    // ≥100 → integer; ≥10 → 1 decimal; else 2 decimals.
    // Bytes unit never shows decimals (integer bytes).
    const int decimals = (i == 0) ? 0
                                  : (v >= 100.0 ? 0 : (v >= 10.0 ? 1 : 2));
    return QStringLiteral("%1 %2")
        .arg(QString::number(v, 'f', decimals))
        .arg(QLatin1String(units[i]));
}

QString humanSpeed(double bps)
{
    // Reject NaN / +inf BEFORE the qint64 cast — converting a non-finite
    // double to an integral type is undefined behaviour in C++ (and on MSVC
    // produces INT_MIN, which the formatter then prints as "-8 EiB/s").
    // SpeedMeter currently never produces non-finite values, but humanSpeed
    // is exposed via FormatUtil to other call-sites (QML helper, future
    // bandwidth metrics) where the invariant is harder to enforce.
    if (!std::isfinite(bps) || bps <= 0.0) return {};
    return humanBytes(static_cast<qint64>(bps)) + QStringLiteral("/s");
}

qint64 parseTimestamp(const QString &s)
{
    if (s.isEmpty()) return 0;
    bool ok = false;
    const qint64 v = s.toLongLong(&ok);
    if (ok) return v;
    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
}

static QString formatImpl(const QString &s, const QString &fallback, bool dateOnly)
{
    const qint64 secs = parseTimestamp(s);
    if (secs <= 0) return fallback;
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(secs).toLocalTime();
    const QLocale loc = QLocale::system();
    return dateOnly ? loc.toString(dt.date(), QLocale::ShortFormat)
                    : loc.toString(dt, QStringLiteral("dd/MM/yyyy HH:mm"));
}

QString formatDateTime(const QString &s, const QString &fallback)
{
    return formatImpl(s, fallback, /*dateOnly*/ false);
}

QString formatDate(const QString &s, const QString &fallback)
{
    return formatImpl(s, fallback, /*dateOnly*/ true);
}

} // namespace fsnext::FormatUtil
