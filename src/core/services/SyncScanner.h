#pragma once
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <cstdint>

// SyncScanner — the *pure* filesystem-walk half of SyncService's scan.
//
// Why a separate translation unit (M18):  the walk is the only part of the
// scan that is safe to run off the main thread, and we want to unit-test it
// in isolation.  Keeping it here — with NO dependency on FshareApi /
// TransferService / SyncRepository / QObject — lets tests/test_sync_scan link
// against Qt6::Core alone (no libcurl, no moc, no DB).  SyncService owns the
// stateful half (diff + persist + enqueue) which must stay on the main thread.
//
// Everything in this file is stateless: given the same directory tree and
// inputs it returns the same ScanResult, with no side effects.
namespace fsnext {

// One file discovered by the walk.  POD, trivially copyable across threads.
struct ScannedFile {
    QString relPath;            // forward-slash, relative to the sync root
    QString absPath;            // absolute path on disk
    QString relDir;             // containing subdir ("" = file at sync root)
    qint64  size      = 0;
    qint64  mtime     = 0;      // seconds since epoch
    bool    oversized = false;  // size > maxFileSize (still listed + seen)
};

// Result of one pure walk.  Holds NO reference to SyncService state.
struct ScanResult {
    QVector<ScannedFile> files;              // every valid file (size > 0)
    QSet<QString>        seenRel;            // relPaths seen (to compute Missing)
    bool                 rootExists = true;  // rootDir.exists() at walk time
};

// The subset of SyncFolder the walk needs — snapshotted on the main thread
// before the worker starts, so a concurrent config edit can't tear it.
struct ScanSnapshot {
    QString localPath;
    bool    watchSubfolders = true;
    QString ignorePatterns;
};

// BFS the tree rooted at snap.localPath, applying the same skip rules and
// canonical-path cycle guard the live scanner has always used.  BLOCKING on
// the filesystem (the whole point of M18 is to call this off-main).  Files
// larger than maxFileSize come back with oversized=true rather than dropped,
// so the diff can surface them as Failed.  rootExists=false short-circuits an
// empty result when the root has vanished.
ScanResult scanFilesystem(const ScanSnapshot &snap, qint64 maxFileSize);

// Skip-list predicates — pure, used by both the walk and SyncService's
// watcher rebuild.  The two-arg file overload merges the per-folder user
// patterns on top of the built-in system skip-list.
bool scanShouldSkipFile(const QString &fileName);
bool scanShouldSkipFile(const QString &fileName, const QStringList &userPatterns);
bool scanShouldSkipDir(const QString &dirName);

// Split "*.psd, *.iso" → ["*.psd", "*.iso"], trimming and dropping empties.
QStringList parseIgnorePatterns(const QString &raw);

} // namespace fsnext
