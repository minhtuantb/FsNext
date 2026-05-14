#pragma once

#include "TransferListModel.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace fsnext {

class TransferService;
class AuthService;

class UploadViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(fsnext::TransferListModel* model        READ model        CONSTANT)
    Q_PROPERTY(fsnext::TransferListModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(QString totalSpeed                       READ totalSpeed   NOTIFY totalSpeedChanged)
    // Whether there are more on-disk history rows than the UI has fetched.
    // Drives the visibility of the "Load more" trigger on the history tab.
    Q_PROPERTY(bool hasMoreHistory                      READ hasMoreHistory NOTIFY hasMoreHistoryChanged)
    // Aggregate run state used by the UI's master pause/resume toggle:
    //   "running" — at least one task is actively transferring
    //   "paused"  — nothing is active, but at least one task is Paused/Queued
    //   "idle"    — nothing is pausable or resumable (all complete/empty)
    Q_PROPERTY(QString runState                         READ runState     NOTIFY runStateChanged)

public:
    explicit UploadViewModel(TransferService *service,
                             AuthService *auth = nullptr,
                             QObject *parent = nullptr);
    ~UploadViewModel() override = default;

    TransferListModel *model() const;
    TransferListModel *historyModel() const;
    QString totalSpeed() const;
    QString runState() const { return m_runState; }
    // "Unknown" (m_historyTotalRows < 0) is treated as "assume more" so the
    // "load more" affordance is visible until the first DB round-trip
    // proves otherwise. Once we've fetched the total, the comparison is
    // exact: we hide the affordance only when the model holds every row.
    bool    hasMoreHistory() const;

    Q_INVOKABLE void addUpload(const QStringList &files,
                               const QString &folderId,
                               const QString &desc       = QString{},
                               const QString &password   = QString{},
                               bool secured              = false,
                               bool directLink           = false);
    Q_INVOKABLE void pauseTask(const QString &id);
    Q_INVOKABLE void resumeTask(const QString &id);
    Q_INVOKABLE void cancelTask(const QString &id);
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void clearHistory();

    // Infinite-scroll: append the next page of older history rows to the
    // history model. Safe to call when there's nothing more to load —
    // it's a no-op in that case. Also safe to call repeatedly (the offset
    // advances so each call pulls a fresh page).
    Q_INVOKABLE void loadMoreHistory();

    // Manually archive a completed item from the active list into the
    // history model. Called when the user clicks the ✕ on a "just-
    // completed" card in the active list.
    Q_INVOKABLE void dismissCompleted(const QString &taskId);

    // Copy the uploaded file's share link to clipboard.
    Q_INVOKABLE void copyLinkToClipboard(const QString &taskId);

    // ── Post-upload local file actions ──────────────────────────────────
    // These resolve the local source path for a completed upload by
    // searching both the active and history models (history also holds the
    // full TransferTask so disk-loaded rows work too). If the task can't
    // be found they're a silent no-op — which happens only for very old
    // history rows that were never loaded into the QML list.

    // Reveal the local source file in the OS file explorer.
    Q_INVOKABLE void revealLocalFile(const QString &taskId);
    // Open the uploaded file's share link in the default browser.
    Q_INVOKABLE void openShareLinkInBrowser(const QString &taskId);
    // Delete the on-disk source file for a completed upload. Returns true
    // if the file was removed (UI drives a confirm dialog upstream). Emits
    // localFileDeleted(taskId) on success for UI feedback; emits
    // localFileDeleteFailed(taskId, reason) otherwise.
    Q_INVOKABLE bool deleteLocalFile(const QString &taskId);
    // Returns the destination folder id on Fshare for the given task, or
    // empty if not found. Used by the "view in My Files" action.
    Q_INVOKABLE QString folderIdOf(const QString &taskId) const;

    // Queue reordering (Queued tasks only).
    Q_INVOKABLE void moveTaskFirst(const QString &id);
    Q_INVOKABLE void moveTaskUp(const QString &id);
    Q_INVOKABLE void moveTaskDown(const QString &id);
    Q_INVOKABLE void moveTaskLast(const QString &id);

signals:
    void totalSpeedChanged();
    void runStateChanged();
    void hasMoreHistoryChanged();
    // Pre-flight errors (quota exceeded, VIP-only restriction) that never reach TransferService.
    void uploadError(const QString &message);

    // Post-upload local-file action feedback. Emitted from deleteLocalFile().
    void localFileDeleted(const QString &taskId);
    void localFileDeleteFailed(const QString &taskId, const QString &reason);

private:
    void updateTotalSpeed();
    // Recompute the aggregate run state by scanning the active model. Only
    // emits runStateChanged() if the computed value differs from m_runState,
    // so it's cheap to call from every state-change handler.
    void refreshRunState();

    // Move a single completed task from the active model into the history
    // model (prepended, so newest-first). No-op if the id isn't Complete.
    void archiveCompleted(const QString &id);

    // Prepend a completed task onto the history model and enforce the cap
    // (kHistoryMaxItems) by trimming any overflow from the tail. Used by
    // both archiveCompleted() and the historical-load path so disk history
    // and live archive share the same size budget.
    void prependHistory(const TransferTask &task);

    // Scan the active model for Complete items older than the TTL and
    // archive them. Fired periodically by m_sweepTimer.
    void sweepCompleted();

    // Move ALL Complete items to history unconditionally. Fired on logout
    // so a new user doesn't inherit the previous user's completed cards.
    void flushCompletedToHistory();

    TransferService   *m_service      = nullptr;
    AuthService       *m_auth         = nullptr;
    TransferListModel *m_model        = nullptr;
    TransferListModel *m_historyModel = nullptr;
    QString            m_totalSpeed;
    QString            m_runState = QStringLiteral("idle");

    // Periodic sweep of completed items older than kCompleteTtlMs.
    QTimer m_sweepTimer;

    // ── Progress coalescing ────────────────────────────────────────────────
    // taskProgressChanged can fire 1-10 Hz per active transfer. With N active
    // uploads, 10N signals/sec × {per-row dataChanged + totalSpeed recompute}
    // saturates the main thread. We buffer the *latest* snapshot per task id
    // and flush once per m_progressCoalesceMs into the model, so render cost
    // is bounded regardless of how many tasks are active.
    struct ProgressSnapshot {
        qint64  bytes = 0;
        qint64  total = 0;
        double  speed = 0.0;
        QString eta;
    };
    QHash<QString, ProgressSnapshot> m_pendingProgress;
    QTimer                           m_progressFlushTimer;
    void flushPendingProgress();

    // taskId → last known upload speed (bytes/s) for aggregate total speed.
    QHash<QString, double>  m_speedMap;
    // taskId → linkcode, cached on taskCompleted so copyLink works after the task
    // has been moved to history (where findTask may not return the linkcode).
    QHash<QString, QString> m_linkcodeMap;

    // ── Infinite-scroll cursor ─────────────────────────────────────────────
    // DB-side total row count for the current user's upload history.
    //   -1  → unknown (assume there may be more; UX shows "load more")
    //   ≥ 0 → exact; hasMoreHistory compares against m_historyModel->count()
    int m_historyTotalRows = -1;
    // Page size used by loadMoreHistory(). Chosen to keep each scroll-triggered
    // query small enough that the DB round-trip is well under a frame even on
    // cold cache.
    static constexpr int kHistoryPageSize = 50;
    // Refresh m_historyTotalRows from the DB and emit hasMoreHistoryChanged
    // if the model's "more available?" answer would flip.
    void refreshHistoryCursor();
};

} // namespace fsnext
