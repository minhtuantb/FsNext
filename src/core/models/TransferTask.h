#pragma once
#include "TransferState.h"
#include <QString>
#include <QMetaType>
#include <QVector>
#include <cstdint>

namespace fsnext {

struct TransferTask {
    QString id;                // Unique task ID (UUID)
    TransferType type = TransferType::Download;
    TransferState state = TransferState::Queued;

    // File info
    QString fileName;
    int64_t fileSize = 0;
    QString linkcode;          // Fshare link code

    // Download-specific
    QString localPath;         // Save destination
    QString password;          // File password (if protected)
    QString realUrl;           // Actual download URL from API
    int segments = 16;         // Parallel HTTP Range streams per file (max 32)
    // Conflict resolution policy snapshotted at enqueue time so live edits to
    // Settings don't race against an in-flight task.  See ADR 003 D7.
    //   0 = rename "(n)"  (default)
    //   1 = overwrite
    //   2 = skip
    //   3 = ask each time
    int fileConflictPolicy = 0;

    // Folder download group context (empty for single-file downloads)
    QString groupId;           // UUID shared by all files in the same folder scan batch
    QString folderPath;        // Display path within the group, e.g. "My Photos/2024"

    // Upload-specific
    QString sourcePath;        // Local file to upload
    QString folderId;          // Destination folder on Fshare
    QString description;
    bool secured = false;
    bool directLink = false;

    // Sync-specific (set by SyncService via TransferService::addSyncUpload).
    // When isSyncTask is true, UploadEngine throttles writes so the per-task
    // upload speed does not exceed speedLimitBps. syncFolderId / syncRelPath
    // are opaque routing keys the SyncService uses to bookkeep on completion.
    bool    isSyncTask     = false;
    int64_t speedLimitBps  = 0;     // 0 = unlimited
    QString syncFolderId;
    QString syncRelPath;

    // Progress
    int64_t bytesTransferred = 0;
    double progress = 0.0;     // 0.0 - 100.0
    double speed = 0.0;        // bytes/sec
    QString eta;               // Formatted time remaining
    int retryCount = 0;

    // Error
    QString errorMessage;

    // Timestamp (ms since epoch) when the task entered Complete state.
    // 0 means the task has never completed. Used by the view-models to
    // implement the "soft archive": completed items linger in the active
    // list for ~1 hour so users get a visual confirmation, then migrate
    // to the history list quietly.
    qint64 completedAt = 0;

    double progressPercent() const {
        if (fileSize <= 0) return 0.0;
        return (static_cast<double>(bytesTransferred) / fileSize) * 100.0;
    }
};

} // namespace fsnext

// Register for use in Qt queued signals (cross-thread delivery via QMetaType).
Q_DECLARE_METATYPE(fsnext::TransferTask)
Q_DECLARE_METATYPE(QVector<fsnext::TransferTask>)
