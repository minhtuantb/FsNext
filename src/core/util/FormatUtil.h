// SPDX-License-Identifier: Proprietary
#pragma once

#include <QString>
#include <QtGlobal>

class QDateTime;

namespace fsnext::FormatUtil {

// Human-readable bytes: "1.5 MB", "820 KB", "0 B".
// `bytes` ≤ 0 yields "0 B" (except when `emptyOnZero` is true — returns "").
QString humanBytes(qint64 bytes, bool emptyOnZero = false);

// Human-readable bytes/sec: "1.5 MB/s". `bps` ≤ 0 yields "" (empty).
QString humanSpeed(double bps);

// Set the locale used by formatDate/formatDateTime, driven by the app's
// language toggle ("vi" | "en"; anything else → system locale). Call from
// LanguageViewModel on startup and on every language change so localized
// dates follow the in-app language instead of the OS locale. Thread-safe.
void setLocaleLanguage(const QString &code);

// Parse Fshare API timestamp (either UNIX epoch seconds as string,
// or ISO-8601). Returns 0 when unparseable/empty.
qint64 parseTimestamp(const QString &s);

// Format a raw API timestamp (epoch string or ISO) as a localized date-time.
// `s` empty or invalid → returns `fallback` (default "—").
QString formatDateTime(const QString &s, const QString &fallback = QStringLiteral("—"));

// Date-only variant (drops time component).
QString formatDate(const QString &s, const QString &fallback = QStringLiteral("—"));

} // namespace fsnext::FormatUtil
