#pragma once

#include "TransferListModel.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace fsnext {

class TransferService;
class SettingsService;
class AuthService;

class DownloadViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(fsnext::TransferListModel* model        READ model        CONSTANT)
    Q_PROPERTY(fsnext::TransferListModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(QString totalSpeed                       READ totalSpeed   NOTIFY totalSpeedChanged)
    Q_PROPERTY(QString defaultSaveFolder                READ defaultSaveFolder NOTIFY defaultSaveFolderChanged)
    // Whether the on-disk DB has more rows than the UI has fetched. Drives
    // the "load more" affordance on the download-history tab.
    Q_PROPERTY(bool hasMoreHistory                      READ hasMoreHistory NOTIFY hasMoreHistoryChanged)
    // "running" / "paused" / "idle" — drives the master pause/resume toggle.
    // See UploadViewModel::runState for the full rationale.
    Q_PROPERTY(QString runState                         READ runState     NOTIFY runStateChanged)

    // Folder-scan state (used by DownloadPage to show scanning indicator)
    Q_PROPERTY(bool    isScanning     READ isScanning     NOTIFY isScanningChanged)
    Q_PROPERTY(int     scanFoundFiles READ scanFoundFiles  NOTIFY scanProgressChanged)
    Q_PROPERTY(QString scanFolderName READ scanFolderName  NOTIFY isScanningChanged)
    Q_PROPERTY(QString scanGroupId    READ scanGroupId     NOTIFY isScanningChanged)

public:
    explicit DownloadViewModel(TransferService *transferService,
                               SettingsService *settingsService = nullptr,
                               AuthService     *authService     = nullptr,
                               QObject         *parent          = nullptr);
    ~DownloadViewModel() override = default;

    TransferListModel *model() const;
    TransferListModel *historyModel() const;
    QString totalSpeed() const;
    QString defaultSaveFolder() const;
    QString runState() const { return m_runState; }
    bool    hasMoreHistory() const;

    bool    isScanning()     const { return m_activeScans > 0; }
    int     scanFoundFiles() const { return m_scanFoundFiles; }
    QString scanFolderName() const { return m_scanFolderName; }
    QString scanGroupId()    const { return m_scanGroupId; }

    Q_INVOKABLE void addDownload(const QString &url,
                                 const QString &folder,
                                 const QString &password = QString{});
    Q_INVOKABLE void cancelFolderScan(const QString &groupId);

    // Returns the system clipboard's current plain-text content, so QML can
    // react to Ctrl+V / paste shortcuts that should route Fshare URLs into
    // the Download module.
    Q_INVOKABLE QString clipboardText() const;

    // Extracts Fshare file/folder URLs from a blob of text (clipboard / drop
    // payload). Returns each URL on its own line, ready to pass to
    // addDownload() or the Download add-dialog's linksText.
    Q_INVOKABLE QString extractFshareLinks(const QString &text) const;
    Q_INVOKABLE void pauseTask(const QString &id);
    Q_INVOKABLE void resumeTask(const QString &id);
    Q_INVOKABLE void cancelTask(const QString &id);
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void clearHistory();

    // Infinite-scroll: append the next page of older history rows. Calls
    // through to TransferService which hits the SQLite pager directly. No-op
    // when there are no more rows on disk.
    Q_INVOKABLE void loadMoreHistory();

    // Manually archive a just-completed item from the active list into the
    // history model. Called by the ✕ button on completed cards.
    Q_INVOKABLE void dismissCompleted(const QString &taskId);

    // ── Path / link helpers (used by the HomePage "recent files" list) ──
    // Stateless wrappers over PlatformUtils / QDesktopServices so QML
    // doesn't have to hand-build file:// URLs (which silently failed on
    // Windows paths with spaces). Each is a no-op on empty input.
    //   revealInFolder — open the OS file browser with the file selected.
    //   openLocalFile  — open the file with its default application.
    //   openShareUrl   — open the file's fshare.vn page; accepts EITHER a
    //                    bare linkcode OR an already-full http(s) URL
    //                    (download tasks store the full URL in `linkcode`),
    //                    mirroring UploadViewModel::openShareLinkInBrowser.
    Q_INVOKABLE void revealInFolder(const QString &localPath) const;
    Q_INVOKABLE void openLocalFile(const QString &localPath) const;
    Q_INVOKABLE void openShareUrl(const QString &linkcodeOrUrl) const;

    // Copy the file's fshare.vn share URL to the system clipboard and emit
    // shareLinkCopied() so the UI can confirm with a toast. Accepts a bare
    // linkcode OR a full URL (same normalisation as openShareUrl).
    Q_INVOKABLE void copyShareLink(const QString &linkcodeOrUrl);

signals:
    void totalSpeedChanged();
    void runStateChanged();
    void defaultSaveFolderChanged();
    void isScanningChanged();
    void scanProgressChanged();
    void hasMoreHistoryChanged();

    // Emitted when a download is blocked because the target folder is a
    // protected system directory (Music, Pictures, Windows, Program Files…).
    void downloadBlocked(const QString &reason);

    // Emitted after copyShareLink() puts a normalised fshare URL on the
    // clipboard — the UI shows a "đã sao chép" toast.
    void shareLinkCopied(const QString &url);

private:
    static bool isFolderUrl(const QString &url);
    // Bare linkcode | full URL → canonical https share URL.
    static QString normalizeShareUrl(const QString &linkcodeOrUrl);
    void updateTotalSpeed();

    // Soft-archive helpers — see UploadViewModel for the full rationale.
    void archiveCompleted(const QString &id);
    void sweepCompleted();
    void flushCompletedToHistory();
    // Prepend into the history model and trim back to the hard cap.
    void prependHistory(const TransferTask &task);
    // Recompute m_runState by scanning the active model; emits only on change.
    void refreshRunState();

    TransferService   *m_service         = nullptr;
    SettingsService   *m_settingsService = nullptr;
    AuthService       *m_auth            = nullptr;
    TransferListModel *m_model           = nullptr;
    TransferListModel *m_historyModel    = nullptr;
    QString            m_totalSpeed;
    QString            m_runState = QStringLiteral("idle");

    // Periodic sweep of completed items older than the TTL (see .cpp).
    QTimer m_sweepTimer;

    // ── Progress coalescing ────────────────────────────────────────────────
    // Mirrors UploadViewModel: taskProgressChanged fires 1-10 Hz per transfer,
    // and at realistic download parallelism (8+ threads) the unthrottled rate
    // dominates main-thread CPU. We buffer the latest snapshot per task id
    // and flush at ≈5 Hz — fast enough that progress bars feel live, slow
    // enough that layout work stays bounded.
    struct ProgressSnapshot {
        qint64  bytes = 0;
        qint64  total = 0;
        double  speed = 0.0;
        QString eta;
    };
    QHash<QString, ProgressSnapshot> m_pendingProgress;
    QTimer                           m_progressFlushTimer;
    void flushPendingProgress();

    // Folder scan state
    int     m_activeScans    = 0;
    int     m_scanFoundFiles = 0;
    QString m_scanFolderName;
    QString m_scanGroupId;

    // taskId → last known speed (bytes/s) for aggregate totalSpeed.
    QHash<QString, double> m_speedMap;

    // ── Infinite-scroll cursor ─────────────────────────────────────────────
    // DB-side total row count for the current user's download history.
    //   -1  → unknown (assume more; UX still shows "load more")
    //   ≥ 0 → exact; hasMoreHistory compares against m_historyModel->count()
    int m_historyTotalRows = -1;
    static constexpr int kHistoryPageSize = 50;
    void refreshHistoryCursor();
};

} // namespace fsnext
