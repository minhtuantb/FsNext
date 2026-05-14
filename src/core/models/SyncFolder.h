#pragma once
#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QVector>
#include <cstdint>

namespace fsnext {

// Activity log entry — captured every time the sync engine settles a file
// (success or failure) so the UI can show a "recent activity" feed without
// re-walking the per-folder file map.  Persisted via SyncRepository as a
// rolling FIFO capped at 50 entries (oldest dropped).
//
// Kind values are stable: persisted as ints, never reorder.
enum class SyncActivityKind : int {
    Uploaded        = 0,   // file pushed to Fshare successfully
    Failed          = 1,   // upload errored
    DeletedLocal    = 2,   // deleteAfterUpload removed the local copy
    FolderAdded     = 3,   // user added a new watch folder
    FolderRemoved   = 4,   // user removed a watch folder
};

struct SyncActivityEntry {
    QString          folderId;        // empty when the event isn't folder-scoped
    QString          folderLabel;     // human-readable (localPath leaf) for display
    QString          relPath;         // file relPath, empty for folder-level events
    qint64           sizeBytes = 0;
    SyncActivityKind kind = SyncActivityKind::Uploaded;
    QString          message;         // e.g. error text on Failed
    QDateTime        at;
};

// A single folder that FsNext is mirroring to Fshare.
// Persisted via SyncRepository. Phase 1: top-level files only.
//
// Per-folder settings (`watchSubfolders`, `ignorePatterns`, `speedLimitBps`)
// were added in v6.0 to back the existing watch-folder dialogs that had been
// shipping the UI for these knobs without persistence.  Older folders missing
// these keys read back the documented defaults — which match v5 behaviour —
// so an upgrade preserves what the user already has running.
struct SyncFolder {
    QString id;                    // UUID
    QString localPath;             // e.g. "D:/Important"
    QString fshareFolderName;      // Fshare folder at ROOT, named after localPath leaf
    bool    enabled            = true;
    bool    deleteAfterUpload  = false;
    QDateTime createdAt;
    QDateTime lastScanAt;

    // ── Per-folder settings ────────────────────────────────────────────────
    // When false, the scan walks ONLY the root directory — no subfolders are
    // descended into.  Default true preserves v5 recursive behaviour.
    bool    watchSubfolders    = true;

    // Comma-separated wildcard patterns matched against bare filenames.
    // Merged with (not replacing) the built-in system skip-list inside
    // SyncService::shouldSkipFile.  Empty string = no user patterns.
    // Examples: "*.psd, *.iso, *.RAW"  (case-insensitive).
    QString ignorePatterns;

    // Per-task upload rate cap, in bytes/second, passed to libcurl via
    // CURLOPT_MAX_SEND_SPEED_LARGE through TransferTask.speedLimitBps.
    // 0 = unlimited.  Default 5 MiB/s matches SyncService::kSpeedLimitBps
    // so legacy folders behave identically post-migration.
    qint64  speedLimitBps      = 5LL * 1024 * 1024;
};

// One tracked file inside a sync folder.
// state: values persisted to QSettings as ints (see SyncRepository).
enum class SyncFileState : int {
    Pending   = 0,   // detected, waiting to upload
    Uploading = 1,   // in flight
    Synced    = 2,   // upload succeeded (linkcode stored)
    Failed    = 3,   // last upload attempt failed (errorMessage set)
    Missing   = 4    // synced earlier but no longer present locally
};

struct SyncFileEntry {
    QString       folderId;        // FK → SyncFolder.id
    QString       relPath;         // filename only (phase 1); relative path (phase 2)
    int64_t       size  = 0;
    qint64        mtime = 0;       // seconds since epoch
    QString       linkcode;        // set after successful upload
    QDateTime     uploadedAt;
    SyncFileState state = SyncFileState::Pending;
    QString       errorMessage;    // populated on Failed
};

} // namespace fsnext

Q_DECLARE_METATYPE(fsnext::SyncFolder)
Q_DECLARE_METATYPE(QVector<fsnext::SyncFolder>)
Q_DECLARE_METATYPE(fsnext::SyncFileEntry)
Q_DECLARE_METATYPE(QVector<fsnext::SyncFileEntry>)
Q_DECLARE_METATYPE(fsnext::SyncActivityEntry)
Q_DECLARE_METATYPE(QVector<fsnext::SyncActivityEntry>)
