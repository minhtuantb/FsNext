// SPDX-License-Identifier: Proprietary
#include "FileNameSanitizer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringLiteral>

namespace fsnext::FileNameSanitizer {

// Reserved DOS device names on Windows — even with an extension, "CON.txt"
// cannot be created. Match case-insensitively on the basename.
static const QSet<QString> &reservedDeviceNames()
{
    static const QSet<QString> kNames = {
        QStringLiteral("CON"), QStringLiteral("PRN"),
        QStringLiteral("AUX"), QStringLiteral("NUL"),
        QStringLiteral("COM1"), QStringLiteral("COM2"), QStringLiteral("COM3"),
        QStringLiteral("COM4"), QStringLiteral("COM5"), QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),
        QStringLiteral("LPT1"), QStringLiteral("LPT2"), QStringLiteral("LPT3"),
        QStringLiteral("LPT4"), QStringLiteral("LPT5"), QStringLiteral("LPT6"),
        QStringLiteral("LPT7"), QStringLiteral("LPT8"), QStringLiteral("LPT9"),
    };
    return kNames;
}

QString sanitize(const QString &name)
{
    QString s = name;

    // Drop any path components — a server-supplied "../../etc/passwd" or
    // "C:\\Windows\\System32\\foo" must not escape the save folder.
    s.replace(QLatin1Char('/'),  QLatin1Char('_'));
    s.replace(QLatin1Char('\\'), QLatin1Char('_'));

    // Substitute Windows-reserved characters.
    static const QString kReserved = QStringLiteral("<>:\"|?*");
    for (QChar c : kReserved) s.replace(c, QLatin1Char('_'));

    // Strip control characters (0x00–0x1F, 0x7F). These are legal on POSIX
    // but break Windows and most archive tools; blanket-strip for portability.
    QString clean;
    clean.reserve(s.size());
    for (QChar c : std::as_const(s)) {
        const ushort u = c.unicode();
        if (u < 0x20 || u == 0x7F) continue;
        clean.append(c);
    }
    s = std::move(clean);

    // Windows silently strips trailing dots and spaces — do it ourselves so
    // the path we return actually matches the file that ends up on disk.
    while (!s.isEmpty() && (s.endsWith(QLatin1Char('.')) || s.endsWith(QLatin1Char(' ')))) {
        s.chop(1);
    }
    while (!s.isEmpty() && s.startsWith(QLatin1Char(' '))) {
        s.remove(0, 1);
    }

    // Escape DOS device names (CON, COM1, etc.) — match against the basename
    // (pre-extension) case-insensitively. Prepend "_" to neutralise.
    {
        const QFileInfo fi(s);
        const QString base = fi.completeBaseName();
        if (!base.isEmpty() && reservedDeviceNames().contains(base.toUpper())) {
            s = QLatin1Char('_') + s;
        }
    }

    if (s.isEmpty()) s = QStringLiteral("download");

    // Length clamp. Preserve the last extension if it fits so the file still
    // opens in the right application.
    constexpr int kMaxLen = 200;
    if (s.size() > kMaxLen) {
        const int dot = s.lastIndexOf(QLatin1Char('.'));
        if (dot > 0 && (s.size() - dot) < 16) {
            const QString ext = s.mid(dot);
            s = s.left(kMaxLen - ext.size()) + ext;
        } else {
            s = s.left(kMaxLen);
        }
    }

    return s;
}

QString resolveConflict(const QString &dir, const QString &name, ConflictPolicy policy)
{
    const QString safe = sanitize(name);
    const QString joined = QDir(dir).filePath(safe);

    if (policy == ConflictPolicy::Overwrite)        return joined;
    if (!QFile::exists(joined))                     return joined;
    if (policy == ConflictPolicy::Skip)             return QString{};

    // Rename / Ask both produce the same candidate.  Split into base + ext
    // first so " (1)" lands BEFORE the dot.
    const QFileInfo fi(safe);
    const QString base = fi.completeBaseName();
    const QString ext  = fi.suffix();
    const QString dot  = ext.isEmpty() ? QString{} : QStringLiteral(".") + ext;

    // Cap at 9999 to avoid infinite loop on a buggy filesystem; in practice
    // any directory with thousands of clashes signals a deeper problem.
    for (int i = 1; i < 10000; ++i) {
        const QString candidate = QStringLiteral("%1 (%2)%3").arg(base).arg(i).arg(dot);
        const QString full = QDir(dir).filePath(candidate);
        if (!QFile::exists(full)) return full;
    }
    // Fall back to a UUID-suffixed name as a last resort — should be unique
    // even on a filesystem that we somehow can't read accurately.
    return QDir(dir).filePath(base + QStringLiteral("_dup") + dot);
}

} // namespace fsnext::FileNameSanitizer
