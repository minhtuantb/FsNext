#pragma once
#include "core/models/SyncFolder.h"
#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

namespace fsnext {

class SyncService;

// Model exposing the list of sync folders to QML (for tab bar + submenu).
// Roles match the property names QML uses directly (`model.localPath`, etc.).
class SyncFoldersModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        LocalPathRole,
        FshareFolderNameRole,
        EnabledRole,
        DeleteAfterUploadRole,
        LastScanAtRole,
        FileCountRole,           // total files tracked
        SyncedCountRole,         // files in Synced state
        FailedCountRole,
        // Per-folder settings (v6.0+) — exposed so the watch-folder settings
        // dialog can pre-populate its form from `model.<role>` bindings
        // without an extra fetch through the ViewModel.
        WatchSubfoldersRole,
        IgnorePatternsRole,
        SpeedLimitKBpsRole,      // bytes → KBps for UI consumption (0 = unlimited)
        // Aggregated visual state — drives the status pill on each folder row.
        // Derived from file-level state + folder.enabled + folder existence
        // so the UI never sees "files all Synced but disk gone".  Enum values
        // match SyncFolderUiState below (kept QML-side via stateText role).
        StatusRole,              // int enum (SyncFolderUiState)
        StatusTextRole,          // localized label
        // v6.0 Phase 3 — per-folder live upload progress.  All three are
        // updated together on every SyncService::folderProgressChanged
        // tick.  Bytes total is "sum of in-flight task sizes", NOT the
        // folder's full size — the bar fills from 0 → total of the
        // current batch, then resets when the next batch starts.
        UploadProgressRole,      // 0.0 .. 1.0  (-1 when nothing in flight)
        UploadSpeedTextRole,     // "2.4 MB/s" / "" when idle
        UploadEtaTextRole,       // "~8 phút" / "" when idle or unknown
    };

    // Visual aggregate of a folder's file states + global flags.  Pure
    // derived data — never persisted; recomputed in setFolders().  Keep the
    // numeric values stable: QML side-bar pill colour switches on them.
    enum class SyncFolderUiState : int {
        Idle      = 0,   // sync ON, all files Synced (or empty folder)
        Uploading = 1,   // ≥1 file Uploading/Pending
        Paused    = 2,   // folder.enabled = false (per-folder pause)
        Error     = 3,   // ≥1 file Failed, none Uploading
        Missing   = 4,   // localPath gone — sync can't run
    };

    explicit SyncFoldersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML-facing row accessor mirroring ListModel.get(): returns a map of
    // role-name → value so callers can read `folders.get(i).fileCount`.
    // QAbstractListModel has no built-in get(); without this QML throws
    // "get is not a function". Out-of-range rows return an empty map.
    Q_INVOKABLE QVariantMap get(int row) const;

    // `state` maps folderId → SyncFolderUiState (cast to int).  The VM
    // computes it once per refresh so the QML role read is O(1) hash lookup.
    // Empty hash falls back to Idle for every row — matches v5 behaviour
    // before the status pill existed.
    void setFolders(const QVector<SyncFolder> &folders,
                    const QHash<QString, QPair<int,int>> &counts,
                    const QHash<QString, int> &failed,
                    const QHash<QString, int> &state = {});

    // Push a single per-folder progress snapshot.  Hot-path: invoked on
    // every byte tick from SyncService.  Only fires dataChanged when the
    // ratio actually moves or the text fields shift — saves a delegate
    // re-render per micro-tick.
    void setFolderProgress(const QString &folderId, qreal ratio,
                            const QString &speedText, const QString &etaText);

signals:
    void countChanged();

private:
    QVector<SyncFolder>                 m_folders;
    QHash<QString, QPair<int,int>>      m_counts;   // id → (total, synced)
    QHash<QString, int>                 m_failed;   // id → failed count
    QHash<QString, int>                 m_state;    // id → SyncFolderUiState (int)

    // Per-folder live progress, pushed from SyncService::folderProgressChanged.
    struct FolderProgress {
        qreal   ratio    = -1.0;     // -1 => idle (no in-flight task)
        QString speedText;
        QString etaText;
    };
    QHash<QString, FolderProgress>      m_progress;
    // folder id → row index in m_folders. Used by setFolders() to emit
    // targeted dataChanged() when the folder set is unchanged but counts
    // have shifted (the hot path during active sync).
    QHash<QString, int>                 m_indexById;
};

// Newest-first feed of recent sync events (uploaded / failed / deleted-local
// / folder added or removed).  Backed by SyncRepository's rolling FIFO log —
// max kActivityCap entries.  Pure read model from the QML side; clearActivity
// goes through SyncViewModel → SyncService.
class SyncActivityModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        FolderIdRole = Qt::UserRole + 1,
        FolderLabelRole,
        RelPathRole,
        SizeBytesRole,
        KindRole,
        KindTextRole,
        MessageRole,
        AtRole,                  // localized timestamp
        AtAgoRole,               // "vừa xong", "2 phút", "1 giờ" — coarse
    };

    explicit SyncActivityModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<SyncActivityEntry> &entries);

signals:
    void countChanged();

private:
    QVector<SyncActivityEntry> m_entries;
};

// Model exposing the files of a single sync folder.
class SyncFilesModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        RelPathRole = Qt::UserRole + 1,
        SizeRole,
        MtimeRole,
        StateRole,                       // int matches SyncFileState
        StateTextRole,                   // localized
        LinkcodeRole,
        UploadedAtRole,
        ErrorMessageRole,
    };

    explicit SyncFilesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(const QVector<SyncFileEntry> &files);

signals:
    void countChanged();

private:
    QVector<SyncFileEntry>      m_files;
    // relPath → row index into m_files. Lets setFiles() run a fast diff when
    // the file-set is unchanged (common during scan: only state/mtime flip).
    QHash<QString, int>         m_indexByRelPath;
};

// QML-facing facade over SyncService.
class SyncViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(fsnext::SyncFoldersModel*  folders  READ folders  CONSTANT)
    Q_PROPERTY(fsnext::SyncFilesModel*    files    READ files    CONSTANT)
    Q_PROPERTY(fsnext::SyncActivityModel* activity READ activity CONSTANT)
    Q_PROPERTY(QString activeFolderId   READ activeFolderId  WRITE setActiveFolderId NOTIFY activeFolderIdChanged)
    Q_PROPERTY(int     maxFolders       READ maxFolders      CONSTANT)
    Q_PROPERTY(int     speedLimitKBps   READ speedLimitKBps  CONSTANT)
    Q_PROPERTY(qint64  maxFileSize      READ maxFileSize     CONSTANT)
    Q_PROPERTY(bool    canAddMore       READ canAddMore      NOTIFY foldersChanged)
    Q_PROPERTY(bool    deleteAfterUpload READ activeDeleteAfterUpload NOTIFY activeFolderChanged)
    Q_PROPERTY(bool    activeEnabled    READ activeEnabled   NOTIFY activeFolderChanged)
    Q_PROPERTY(QString activeLocalPath  READ activeLocalPath NOTIFY activeFolderChanged)
    Q_PROPERTY(QString activeFshareFolderName READ activeFshareFolderName NOTIFY activeFolderChanged)
    // v6.0 master toggle + aggregate stats (drives Sidebar badge + page header)
    Q_PROPERTY(bool    autoSyncEnabled  READ autoSyncEnabled WRITE setAutoSyncEnabled NOTIFY autoSyncEnabledChanged)
    Q_PROPERTY(int     pendingCount     READ pendingCount    NOTIFY pendingCountChanged)

public:
    explicit SyncViewModel(SyncService *service, QObject *parent = nullptr);

    SyncFoldersModel  *folders()  { return m_foldersModel; }
    SyncFilesModel    *files()    { return m_filesModel; }
    SyncActivityModel *activity() { return m_activityModel; }

    QString activeFolderId() const { return m_activeFolderId; }
    void    setActiveFolderId(const QString &id);

    int     maxFolders()     const;
    int     speedLimitKBps() const;  // for display in settings section
    qint64  maxFileSize()    const;

    bool    canAddMore() const;
    bool    activeEnabled() const;
    bool    activeDeleteAfterUpload() const;
    QString activeLocalPath() const;
    QString activeFshareFolderName() const;

    // v6.0 master toggle + aggregate stats
    bool    autoSyncEnabled() const;
    void    setAutoSyncEnabled(bool enabled);
    int     pendingCount() const { return m_pendingCount; }

    // QML API
    Q_INVOKABLE void    addFolder(const QString &localPath);
    // v6.0 add overload: AddWatchFolderDialog collects all four settings up
    // front, so wire them through in one call rather than relying on the
    // legacy add → updateFolderSettings dance (which had a window where the
    // first scan ran with default settings).  speedLimitKBps == 0 = unlimited.
    Q_INVOKABLE void    addFolderWithSettings(const QString &localPath,
                                              bool watchSubfolders,
                                              const QString &ignorePatterns,
                                              int speedLimitKBps,
                                              bool deleteAfterUpload);
    Q_INVOKABLE void    removeFolder(const QString &folderId);
    Q_INVOKABLE void    setEnabled(const QString &folderId, bool enabled);
    Q_INVOKABLE void    setDeleteAfterUpload(const QString &folderId, bool on);
    // Bulk update from WatchFolderSettingsDialog — one round trip to persist
    // every per-folder knob the dialog exposes.  speedLimitKBps == 0 = unlimited.
    Q_INVOKABLE void    updateFolderSettings(const QString &folderId,
                                              bool watchSubfolders,
                                              bool deleteAfterUpload,
                                              const QString &ignorePatterns,
                                              int speedLimitKBps);
    Q_INVOKABLE void    scanNow(const QString &folderId);
    // Dry-run preview for AddWatchFolderDialog Step 2.  Runs on a worker
    // thread so a 100k-file folder doesn't freeze the UI; result lands on
    // the main thread via QueuedConnection inside the lambda, then we emit
    // previewReady() so QML bindings update.  ignorePatterns is the same
    // comma-separated format the dialog uses.
    Q_INVOKABLE void    requestPreview(const QString &localPath,
                                        bool watchSubfolders,
                                        const QString &ignorePatterns);
    Q_INVOKABLE void    copyLinkFor(const QString &folderId, const QString &relPath);
    Q_INVOKABLE void    openFshareLink(const QString &folderId, const QString &relPath);
    Q_INVOKABLE void    openLocalFolder(const QString &folderId);
    // Clears persisted file state so the next scan re-evaluates every file.
    // Does NOT delete files on Fshare — only the local cache.
    Q_INVOKABLE void    resetFolderCache(const QString &folderId);
    Q_INVOKABLE QString normalizeLocalPath(const QString &maybeUrl) const;
    // Wipe the per-user activity log.  UI surfaces this via "Xoá lịch sử"
    // in the Hoạt động gần đây section.
    Q_INVOKABLE void    clearActivity();

    // v6.0 Phase 3 — one-shot folder actions.  Pause/Resume operate on
    // currently in-flight TransferService tasks (NOT the SyncFolder.enabled
    // flag — that one is a persistent "watch yes/no").  retryFailed flips
    // every Failed file in the folder back to Pending and kicks a scan.
    Q_INVOKABLE void    pauseFolder(const QString &folderId);
    Q_INVOKABLE void    resumeFolder(const QString &folderId);
    Q_INVOKABLE void    retryFailed(const QString &folderId);

signals:
    void foldersChanged();
    void activeFolderIdChanged();
    void activeFolderChanged();
    void addFolderError(const QString &message);
    void infoMessage(const QString &message);   // toast-worthy non-error info
    void autoSyncEnabledChanged();
    void pendingCountChanged();
    // Dry-run preview result.  fileCount/totalBytes are zero on error;
    // errorMessage non-empty when the path was missing/unreadable.
    void previewReady(int fileCount, qint64 totalBytes, const QString &errorMessage);

private slots:
    void refreshFolders();
    void refreshFiles(const QString &folderId);
    void refreshActivity();

private:
    SyncService       *m_service       = nullptr;
    SyncFoldersModel  *m_foldersModel  = nullptr;
    SyncFilesModel    *m_filesModel    = nullptr;
    SyncActivityModel *m_activityModel = nullptr;
    QString            m_activeFolderId;

    // Cached aggregate: count of files in non-Synced state across every
    // folder.  Recomputed in refreshFolders() (which already iterates) so
    // exposing it costs nothing extra.  Drives the sidebar nav badge.
    int                m_pendingCount  = 0;
};

} // namespace fsnext
