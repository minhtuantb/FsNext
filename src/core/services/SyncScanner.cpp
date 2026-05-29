#include "SyncScanner.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace fsnext {

// ── Skip-list patterns ───────────────────────────────────────────────────────
// Match against the bare filename (not path). Case-insensitive.
static bool matchesAnyPattern(const QString &name, const QStringList &patterns)
{
    for (const QString &pat : patterns) {
        QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pat),
                              QRegularExpression::CaseInsensitiveOption);
        if (re.match(name).hasMatch()) return true;
    }
    return false;
}

QStringList parseIgnorePatterns(const QString &raw)
{
    if (raw.trimmed().isEmpty()) return {};
    QStringList out;
    const QStringList parts = raw.split(QLatin1Char(','), Qt::SkipEmptyParts);
    out.reserve(parts.size());
    for (const QString &p : parts) {
        const QString trimmed = p.trimmed();
        if (!trimmed.isEmpty()) out.append(trimmed);
    }
    return out;
}

// System-level skip-list — applied to every file regardless of folder.
// User-supplied patterns are MERGED on top (see overload below); we never
// let the user un-skip junk that the legacy client never uploaded.
static const QStringList kSystemFileSkip = {
    QStringLiteral(".*"),           // dotfiles
    QStringLiteral("*.tmp"),
    QStringLiteral("*.part"),        // partial browser downloads
    QStringLiteral("*.crdownload"),
    QStringLiteral("Thumbs.db"),
    QStringLiteral("desktop.ini"),
    QStringLiteral("~$*"),           // Office lock file
};

bool scanShouldSkipFile(const QString &fileName)
{
    return matchesAnyPattern(fileName, kSystemFileSkip);
}

bool scanShouldSkipFile(const QString &fileName, const QStringList &userPatterns)
{
    if (matchesAnyPattern(fileName, kSystemFileSkip)) return true;
    if (userPatterns.isEmpty()) return false;
    return matchesAnyPattern(fileName, userPatterns);
}

bool scanShouldSkipDir(const QString &dirName)
{
    // Noisy / tool-managed directories we never want to mirror.
    static const QStringList kSkip = {
        QStringLiteral(".*"),           // .git, .venv, .idea, .vs, .gradle, …
        QStringLiteral("node_modules"),
        QStringLiteral("__pycache__"),
        QStringLiteral("build"),
        QStringLiteral("dist"),
        QStringLiteral("target"),       // Rust/Java
        QStringLiteral("out"),
        QStringLiteral("bin"),
        QStringLiteral("obj"),
        QStringLiteral("$RECYCLE.BIN"),
        QStringLiteral("System Volume Information"),
    };
    return matchesAnyPattern(dirName, kSkip);
}

ScanResult scanFilesystem(const ScanSnapshot &snap, qint64 maxFileSize)
{
    ScanResult result;

    QDir rootDir(snap.localPath);
    if (snap.localPath.isEmpty() || !rootDir.exists()) {
        result.rootExists = false;
        return result;
    }

    // Parse the per-folder user ignore patterns once before the walk so the
    // hot inner loop's skip check doesn't re-tokenise on every file.  Empty
    // result = no user-level filter, only the system skip-list.
    const QStringList userIgnore = parseIgnorePatterns(snap.ignorePatterns);

    // Walk strategy: when watchSubfolders is true we run a manual BFS that
    // prunes skipped subtrees (matches what v5 did); when false we only
    // enumerate the root and never push children onto pendingDirs, so the
    // user gets a "flat sync" of just the root contents — useful for big
    // folders like Downloads where subdirs are noise.
    //
    // Visited-set uses canonicalPath to break cycles introduced by symlinks
    // or NTFS junction points — without this a junction pointing back at an
    // ancestor would make the walk run forever.
    QStringList pendingDirs;
    pendingDirs.append(snap.localPath);
    QSet<QString> visitedDirs;

    while (!pendingDirs.isEmpty()) {
        const QString dirAbs = pendingDirs.takeLast();
        QDir dir(dirAbs);
        if (!dir.exists()) continue;

        const QString canon = QFileInfo(dirAbs).canonicalFilePath();
        if (canon.isEmpty()) continue;               // symlink target gone
        if (visitedDirs.contains(canon)) continue;    // cycle guard
        visitedDirs.insert(canon);

        const QFileInfoList entries = dir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &fi : entries) {
            const QString name = fi.fileName();

            if (fi.isDir()) {
                // watchSubfolders=false → never descend.  We still process
                // sibling FILES via this same outer loop iteration; only
                // the recursive push is gated.
                if (!snap.watchSubfolders) continue;
                if (scanShouldSkipDir(name)) continue;
                pendingDirs.append(fi.absoluteFilePath());
                continue;
            }

            if (scanShouldSkipFile(name, userIgnore)) continue;
            if (fi.size() == 0) continue;

            // relPath is forward-slash, relative to the sync root.
            QString relPath = rootDir.relativeFilePath(fi.absoluteFilePath());
            relPath.replace(QLatin1Char('\\'), QLatin1Char('/'));

            // relDir = containing subfolder, "" for files at the sync root.
            QString relDir;
            const int slash = relPath.lastIndexOf(QLatin1Char('/'));
            if (slash > 0) relDir = relPath.left(slash);

            ScannedFile sf;
            sf.relPath   = relPath;
            sf.absPath   = fi.absoluteFilePath();
            sf.relDir    = relDir;
            sf.size      = fi.size();
            sf.mtime     = fi.lastModified().toSecsSinceEpoch();
            sf.oversized = sf.size > maxFileSize;

            result.files.append(sf);
            result.seenRel.insert(relPath);
        }
    }

    return result;
}

} // namespace fsnext
