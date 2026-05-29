#pragma once

// TransferHudViewModel — aggregate VM that feeds the floating Mini Window
// (P2) and the Tray Popup (P1).  Both surfaces bind to the SAME instance
// so they show identical state.
//
// Why an aggregate VM instead of binding each surface directly to Upload /
// Download / Sync VMs?
//   • Top-N sort is a cross-VM problem (a failed sync upload should outrank
//     an active download in the HUD).  Doing it at the QML layer means
//     duplicating the logic in two .qml files.
//   • Tray icon colour + balloon notifications need ONE owner so we don't
//     fire two balloons for one taskCompleted event.
//   • Speed history (ring buffer for P2 sparkline) is a single time-series.
//
// Refresh strategy:
//   Every model-mutating signal from Upload/Download/Sync triggers a
//   debounced single-shot timer (250 ms).  When it fires, we walk both
//   active lists once and recompute everything.  Worst-case at 20 active
//   tasks the walk is < 1 ms.  No polling.
//
// Lifetimes:
//   The VM does not own any of its inputs — Upload/Download/Sync/Budget
//   VMs all outlive it (AppContext owns everything and destructs in
//   reverse declaration order).

#include "core/models/TransferState.h"
#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

namespace fsnext {

class UploadViewModel;
class DownloadViewModel;
class SyncViewModel;
class TransferBudgetViewModel;
class TransferService;
class TransferListModel;

// Snapshot row in the HUD's top-N model.  Pure value type — populated by
// TransferHudViewModel::computeTopItems() from the active Upload + Download
// list models, sorted by priority (Failed > Active > Paused > Queued >
// Recently complete).  Diff-based updates emit narrow dataChanged() so QML
// delegates rebind only what actually moved.
class TransferHudTopModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TaskIdRole         = Qt::UserRole + 1,
        DirectionRole,                          // 0=download, 1=upload, 2=sync
        FileNameRole,
        ProgressRole,                           // 0.0 .. 1.0
        SpeedTextRole,                          // "2.1 MB/s" / ""
        EtaTextRole,                            // "Còn 4 phút" / ""
        StatusRole,                             // int matches TransferState
        ErrorMessageRole,                       // non-empty only when status==Error
        // Page hint for focusTask routing.  Matches the Pages enum in
        // qml/FsAurora/Theme/Pages.qml — 1=download, 2=upload.  Sync rows
        // route to page=upload because Sync uses the upload pipeline.
        PageRole,
        // Aurora widget row meta — raw bytes for math, formatted text for
        // display. BytesText is the prebuilt "384 MB / 6.4 GB" string used
        // by the file row meta line so the QML side doesn't need to
        // re-format on every dataChanged.
        BytesDoneRole,                          // qint64 — bytes transferred
        BytesTotalRole,                         // qint64 — file size
        BytesTextRole,                          // "384 MB / 6.4 GB" or ""
    };

    struct Row {
        QString id, fileName, speedText, etaText, errorMessage;
        int direction = 0;       // 0=download, 1=upload, 2=sync
        int page      = 1;       // Pages enum value
        double progress = 0.0;
        int status      = 0;     // TransferState as int
        qint64 bytesDone  = 0;   // bytesTransferred from TransferTask
        qint64 bytesTotal = 0;   // fileSize from TransferTask
        QString bytesText;       // "X MB / Y GB" prebuilt by ingest()
    };

    explicit TransferHudTopModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Diff-based replace.  Compares by id+status; emits dataChanged on
    // touched rows, beginInsertRows / beginRemoveRows on additions /
    // deletions.  No-op when the new vector matches the cached one byte
    // for byte (cheap idle path).
    void setRows(QVector<Row> rows);

private:
    QVector<Row>            m_rows;
    QHash<QString, int>     m_indexById;  // rebuilt by setRows
};

class TransferHudViewModel : public QObject
{
    Q_OBJECT

    // Aggregated speed text — uses FsFormat conventions ("12.4 MB/s").
    // Empty string when nothing is moving in that direction.
    Q_PROPERTY(QString totalDownloadSpeedText READ totalDownloadSpeedText NOTIFY speedChanged)
    Q_PROPERTY(QString totalUploadSpeedText   READ totalUploadSpeedText   NOTIFY speedChanged)

    // Master pause/resume hint — "running" / "paused" / "idle".  Mirrors the
    // existing UploadVM/DownloadVM runState semantics so HUD UI bindings can
    // share code.
    Q_PROPERTY(QString runState               READ runState               NOTIFY runStateChanged)

    // Live counters — drive tray icon colour + tooltip + footer chips.
    // active  : Active|Paused-with-progress count across all transfer types
    // pending : Queued count + tasks awaiting a slot
    // failed  : Error count (recent, not yet acknowledged)
    // syncPending : SyncViewModel.pendingCount mirror — separate so footer
    //               can show "⟲ Đồng bộ: N chờ" without summing into active.
    Q_PROPERTY(int activeTotal  READ activeTotal  NOTIFY countersChanged)
    Q_PROPERTY(int pendingTotal READ pendingTotal NOTIFY countersChanged)
    Q_PROPERTY(int failedTotal  READ failedTotal  NOTIFY countersChanged)
    Q_PROPERTY(int syncPending  READ syncPending  NOTIFY countersChanged)

    // Top-N model (currently capped at kTopItemsCap = 5).  Sorted by
    // priority: Error > Active > Paused > Queued > recently Complete.
    // overflowCount = (totalActive + Paused + Queued) - kTopItemsCap, used
    // for the "+ N mục khác" chip in the HUD.
    Q_PROPERTY(fsnext::TransferHudTopModel* topItems READ topItems CONSTANT)
    Q_PROPERTY(int overflowCount READ overflowCount NOTIFY topItemsChanged)

    // Whether the HUD should currently be visible.  Aggregate of:
    //   (activeTotal + pendingTotal > 0) AND user hasn't dismissed manually
    // Main.qml binds MiniHudWindow.visible to this in P2.  Stays here so
    // both surfaces (mini window + tray popup) share dismissal logic.
    Q_PROPERTY(bool shouldShowMini READ shouldShowMini NOTIFY shouldShowMiniChanged)

    // Rolling 60-sample × 1Hz speed history feeding the Mini Window's
    // sparkline.  Index 0 is the OLDEST sample; index 59 is the most
    // recent (just appended).  Values are bytes-per-second (double).
    // Emitted at most once per second, and only when something would
    // visually change (sample > 5% delta vs. previous, or buffer just
    // refilled with zeros after activity stopped).
    Q_PROPERTY(QVariantList speedHistory READ speedHistory NOTIFY speedHistoryChanged)

    // Aggregate transfer progress (0.0 .. 1.0) = Σ bytesTransferred /
    // Σ fileSize across all Active tasks.  Returns -1.0 when nothing is
    // active (idle) so the Windows taskbar progress can switch to
    // NoProgress.  Drives src/platform/TaskbarProgress.
    Q_PROPERTY(double aggregateProgress READ aggregateProgress NOTIFY aggregateProgressChanged)

    // Aggregate ETA text — Aurora widget hero shows "ETA tổng X". Computed
    // as Σ(remaining bytes) / Σ(speed) of Active tasks, formatted to a
    // human string ("1h 32m" / "47 phút" / "25 giây"). Empty when idle or
    // when no size info is available; QML hero falls back to "—" then.
    Q_PROPERTY(QString aggEtaText READ aggEtaText NOTIFY aggEtaTextChanged)

public:
    explicit TransferHudViewModel(UploadViewModel        *upload,
                                  DownloadViewModel      *download,
                                  SyncViewModel          *sync,
                                  TransferBudgetViewModel *budget,
                                  TransferService        *service,
                                  QObject *parent = nullptr);
    ~TransferHudViewModel() override;

    QString totalDownloadSpeedText() const { return m_dlSpeedText; }
    QString totalUploadSpeedText()   const { return m_ulSpeedText; }
    QString runState()               const { return m_runState; }
    int     activeTotal()            const { return m_activeTotal; }
    int     pendingTotal()           const { return m_pendingTotal; }
    int     failedTotal()            const { return m_failedTotal; }
    int     syncPending()            const { return m_syncPending; }
    int     overflowCount()          const { return m_overflowCount; }
    bool    shouldShowMini()         const { return m_shouldShowMini; }
    TransferHudTopModel *topItems()  const { return m_topModel; }
    QVariantList speedHistory()      const;
    double  aggregateProgress()      const { return m_aggregateProgress; }
    QString aggEtaText()             const { return m_aggEtaText; }

    // QML actions — proxy through to the service layer.  Kept short so the
    // HUD QML doesn't have to know which underlying VM owns the task.
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void pauseTask(const QString &id);
    Q_INVOKABLE void resumeTask(const QString &id);
    Q_INVOKABLE void cancelTask(const QString &id);
    // Emits taskFocusRequested(page, id) — Main.qml listens, switches page,
    // and post-frame calls ListView.positionViewAtIndex on the destination
    // page's transfer list.
    Q_INVOKABLE void focusTask(const QString &id);

    // User explicitly closed the HUD (mini window ✕).  Suppresses shouldShowMini
    // until a new transfer starts AFTER the dismiss.  Reset by acknowledgeMini().
    Q_INVOKABLE void dismissMini();
    // Re-arm shouldShowMini after dismiss.  Called by Main.qml when user
    // brings the main window forward — implies they're "back in the app"
    // and can opt back in to the mini.
    Q_INVOKABLE void acknowledgeMini();

signals:
    void speedChanged();
    void runStateChanged();
    void countersChanged();
    void topItemsChanged();
    void shouldShowMiniChanged();
    void speedHistoryChanged();
    void aggregateProgressChanged();
    void aggEtaTextChanged();

    // Main.qml routes this: switches currentPage and asks the destination
    // page to focus the row.  See HUD spec section "Scroll-to-task".
    //   page: 1=download, 2=upload (sync routes to upload page)
    void taskFocusRequested(int page, const QString &taskId);

    // Forwarded for tray balloon — main.cpp listens and calls
    // SystemTray::showNotification gated by SettingsService::notifyOnTransferDone.
    // Each (taskId, terminal-state) pair fires AT MOST once, even if the
    // service re-emits taskCompleted (which it shouldn't, but be defensive).
    //
    // P3: taskId is carried so the tray can remember the most-recently
    // notified task and route balloon clicks back to focusTask().  This
    // is what makes the "click toast to jump to that file" UX work.
    void transferDone(const QString &taskId, const QString &fileName,
                      bool success, const QString &errorMessage);

private slots:
    // Coalescing entry point — every input signal lands here, which only
    // starts the debounce timer if not already running.
    void scheduleRefresh();
    // Single source of truth: walks both active lists once, recomputes
    // counters / runState / top items, emits the appropriate change signals.
    void refreshAll();

    // Service-level terminal-event hooks.  Look up the file name via
    // TransferService::findTask (works for both active and recently-
    // completed tasks).  Idempotent per (id, state) pair.
    void onTaskCompleted(const QString &id);
    void onTaskFailed(const QString &id, const QString &error);

    // 1Hz tick: read current aggregate speed (sum of all active task
    // bytes/sec across DL + UL), append to ring buffer, emit if changed.
    // Self-quiesces: when 60 samples of zero in a row are present the
    // timer stays running but speedHistoryChanged is suppressed so QML
    // sparkline doesn't redraw a flat line every second.
    void sampleSpeedTick();

private:
    static constexpr int kTopItemsCap       = 5;
    static constexpr int kRefreshDebounceMs = 250;
    static constexpr int kSpeedSamples      = 60;
    static constexpr int kSpeedSampleMs     = 1000;

    UploadViewModel         *m_upload   = nullptr;
    DownloadViewModel       *m_download = nullptr;
    SyncViewModel           *m_sync     = nullptr;
    TransferBudgetViewModel *m_budget   = nullptr;
    TransferService         *m_service  = nullptr;

    TransferHudTopModel     *m_topModel = nullptr;
    QTimer                   m_refreshTimer;
    QTimer                   m_speedSampleTimer;

    // Ring buffer of 60 doubles (bytes/sec).  Logical view: index 0 is
    // OLDEST, index 59 is NEWEST.  Stored as a rolling QVector with a
    // physical write head — we append() at the tail and pop_front() to
    // keep size fixed at kSpeedSamples.
    QVector<double>          m_speedHistory;
    // Tracks whether the buffer is "all zero" — when this is true the
    // sampleSpeedTick suppresses speedHistoryChanged emissions because a
    // 60-zero buffer renders identically across ticks.
    bool                     m_speedAllZero = true;

    // Cached state — getters return these in O(1).  Only updated inside
    // refreshAll() so external callers always see consistent snapshots.
    QString m_dlSpeedText, m_ulSpeedText;
    QString m_runState = QStringLiteral("idle");
    int     m_activeTotal = 0, m_pendingTotal = 0, m_failedTotal = 0;
    int     m_syncPending = 0, m_overflowCount = 0;
    double  m_aggregateProgress = -1.0;  // -1 = idle; 0..1 = Σbytes/Σtotal
    QString m_aggEtaText;                // formatted hero ETA — "" when idle
    bool    m_shouldShowMini = false;
    bool    m_userDismissed  = false;   // see dismissMini / acknowledgeMini

    // (taskId, success-bool) pairs already notified.  Prevents double-fire
    // if a service flake re-emits taskCompleted.  Bounded growth — entries
    // are pruned when the task disappears from active+history (TODO P3).
    QSet<QString> m_notifiedDone;

    // Aggregate helpers — split out for testability.
    void connectInputs();
    void recomputeFromLists();
    QString formatSpeed(double bytesPerSec) const;
    QString formatEtaSeconds(qint64 secs)   const;
    // Map TransferState int → priority bucket for top-N sort.
    // Lower number = higher priority.
    static int priorityOf(int state, bool hasError);
};

} // namespace fsnext
