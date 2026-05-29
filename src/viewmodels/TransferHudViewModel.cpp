#include "TransferHudViewModel.h"

#include "DownloadViewModel.h"
#include "UploadViewModel.h"
#include "SyncViewModel.h"
#include "TransferBudgetViewModel.h"
#include "TransferListModel.h"
#include "core/services/TransferService.h"
#include "core/util/FormatUtil.h"

#include <QDebug>
#include <algorithm>

namespace fsnext {

// ─────────────────────────────────────────────────────────────────────────────
//  TransferHudTopModel
// ─────────────────────────────────────────────────────────────────────────────

TransferHudTopModel::TransferHudTopModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TransferHudTopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant TransferHudTopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    switch (role) {
    case TaskIdRole:        return r.id;
    case DirectionRole:     return r.direction;
    case FileNameRole:      return r.fileName;
    case ProgressRole:      return r.progress;
    case SpeedTextRole:     return r.speedText;
    case EtaTextRole:       return r.etaText;
    case StatusRole:        return r.status;
    case ErrorMessageRole:  return r.errorMessage;
    case PageRole:          return r.page;
    case BytesDoneRole:     return r.bytesDone;
    case BytesTotalRole:    return r.bytesTotal;
    case BytesTextRole:     return r.bytesText;
    default:                return {};
    }
}

QHash<int, QByteArray> TransferHudTopModel::roleNames() const
{
    return {
        { TaskIdRole,        QByteArrayLiteral("taskId") },
        { DirectionRole,     QByteArrayLiteral("direction") },
        { FileNameRole,      QByteArrayLiteral("fileName") },
        { ProgressRole,      QByteArrayLiteral("progress") },
        { SpeedTextRole,     QByteArrayLiteral("speedText") },
        { EtaTextRole,       QByteArrayLiteral("etaText") },
        { StatusRole,        QByteArrayLiteral("status") },
        { ErrorMessageRole,  QByteArrayLiteral("errorMessage") },
        { PageRole,          QByteArrayLiteral("page") },
        { BytesDoneRole,     QByteArrayLiteral("bytesDone") },
        { BytesTotalRole,    QByteArrayLiteral("bytesTotal") },
        { BytesTextRole,     QByteArrayLiteral("bytesText") },
    };
}

// Equality check — used by setRows() to skip the dataChanged emit when a
// row is identical byte-for-byte to its previous snapshot.  Done by field
// rather than memcmp because QString has implicit-share semantics.
static bool rowEquals(const TransferHudTopModel::Row &a, const TransferHudTopModel::Row &b)
{
    return a.id == b.id
        && a.fileName == b.fileName
        && a.speedText == b.speedText
        && a.etaText == b.etaText
        && a.errorMessage == b.errorMessage
        && a.direction == b.direction
        && a.page == b.page
        && a.status == b.status
        && a.bytesDone  == b.bytesDone
        && a.bytesTotal == b.bytesTotal
        && a.bytesText  == b.bytesText
        && qFuzzyCompare(1.0 + a.progress, 1.0 + b.progress);
}

void TransferHudTopModel::setRows(QVector<Row> rows)
{
    const int oldCount = m_rows.size();
    const int newCount = rows.size();

    // Common case during steady-state progress: same length, same IDs in
    // same order, just bytes ticked.  Emit dataChanged only on rows that
    // actually shifted — saves a delegate re-render storm.
    if (oldCount == newCount) {
        bool anyDiff = false;
        int firstDiff = -1, lastDiff = -1;
        for (int i = 0; i < newCount; ++i) {
            if (m_rows[i].id != rows[i].id || !rowEquals(m_rows[i], rows[i])) {
                if (firstDiff < 0) firstDiff = i;
                lastDiff = i;
                anyDiff = true;
            }
        }
        if (!anyDiff) return;
        m_rows = std::move(rows);
        m_indexById.clear();
        for (int i = 0; i < m_rows.size(); ++i) m_indexById.insert(m_rows[i].id, i);
        emit dataChanged(index(firstDiff), index(lastDiff));
        return;
    }

    // Length changed — do a full reset.  We don't bother with finer
    // beginInsertRows/beginRemoveRows because the HUD list caps at 5
    // entries; the overhead of a reset on a 5-row model is negligible.
    beginResetModel();
    m_rows = std::move(rows);
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) m_indexById.insert(m_rows[i].id, i);
    endResetModel();
}

// ─────────────────────────────────────────────────────────────────────────────
//  TransferHudViewModel
// ─────────────────────────────────────────────────────────────────────────────

TransferHudViewModel::TransferHudViewModel(UploadViewModel        *upload,
                                           DownloadViewModel      *download,
                                           SyncViewModel          *sync,
                                           TransferBudgetViewModel *budget,
                                           TransferService        *service,
                                           QObject *parent)
    : QObject(parent)
    , m_upload(upload)
    , m_download(download)
    , m_sync(sync)
    , m_budget(budget)
    , m_service(service)
    , m_topModel(new TransferHudTopModel(this))
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(kRefreshDebounceMs);
    connect(&m_refreshTimer, &QTimer::timeout, this, &TransferHudViewModel::refreshAll);

    // Speed history ring buffer — start full of zeros so the QVariantList
    // returned by speedHistory() always has the canonical kSpeedSamples
    // length.  QML sparkline expects a fixed-length series; pre-filling
    // avoids a separate "empty" rendering path.
    m_speedHistory.fill(0.0, kSpeedSamples);
    m_speedSampleTimer.setInterval(kSpeedSampleMs);
    connect(&m_speedSampleTimer, &QTimer::timeout,
            this, &TransferHudViewModel::sampleSpeedTick);
    m_speedSampleTimer.start();

    connectInputs();

    // Initial snapshot — no debounce, populate immediately so QML bindings
    // get real values on first paint instead of empty defaults.
    refreshAll();
}

TransferHudViewModel::~TransferHudViewModel() = default;

void TransferHudViewModel::connectInputs()
{
    // Listen to every signal that could shift the aggregate state.  All
    // funnel into scheduleRefresh() which kicks the debounce timer.
    if (m_upload) {
        connect(m_upload, &UploadViewModel::totalSpeedChanged,
                this, &TransferHudViewModel::scheduleRefresh);
        connect(m_upload, &UploadViewModel::runStateChanged,
                this, &TransferHudViewModel::scheduleRefresh);
        if (auto *m = m_upload->model()) {
            connect(m, &QAbstractItemModel::rowsInserted,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::rowsRemoved,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::dataChanged,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::modelReset,
                    this, &TransferHudViewModel::scheduleRefresh);
        }
    }
    if (m_download) {
        connect(m_download, &DownloadViewModel::totalSpeedChanged,
                this, &TransferHudViewModel::scheduleRefresh);
        connect(m_download, &DownloadViewModel::runStateChanged,
                this, &TransferHudViewModel::scheduleRefresh);
        if (auto *m = m_download->model()) {
            connect(m, &QAbstractItemModel::rowsInserted,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::rowsRemoved,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::dataChanged,
                    this, &TransferHudViewModel::scheduleRefresh);
            connect(m, &QAbstractItemModel::modelReset,
                    this, &TransferHudViewModel::scheduleRefresh);
        }
    }
    if (m_sync) {
        connect(m_sync, &SyncViewModel::pendingCountChanged,
                this, &TransferHudViewModel::scheduleRefresh);
    }
    if (m_budget) {
        // Budget VM polls the orchestrator every 500 ms.  Its usageChanged
        // signal is a cheap heartbeat that also catches state we don't see
        // through the model signals (e.g. queued tasks waiting for a slot).
        connect(m_budget, &TransferBudgetViewModel::usageChanged,
                this, &TransferHudViewModel::scheduleRefresh);
    }
    if (m_service) {
        connect(m_service, &TransferService::taskCompleted,
                this, &TransferHudViewModel::onTaskCompleted);
        connect(m_service, &TransferService::taskFailed,
                this, &TransferHudViewModel::onTaskFailed);
    }
}

void TransferHudViewModel::scheduleRefresh()
{
    if (!m_refreshTimer.isActive()) m_refreshTimer.start();
}

QString TransferHudViewModel::formatSpeed(double bytesPerSec) const
{
    // FormatUtil::humanSpeed returns "" for ≤0 — which is exactly what HUD
    // wants ("nothing to show").
    return FormatUtil::humanSpeed(bytesPerSec);
}

// Aurora widget hero — aggregate ETA formatter.
// Buckets: > 1 hour → "Xh YYm", > 1 minute → "MM:SS", else "X giây".
// Hours uses a literal "h" + zero-padded minutes (matches the design's
// "1h 32m" style). Sub-minute durations get the Vietnamese plural-safe
// "X giây" form rather than "0:05" which reads as an MM:SS where MM is
// meaningless. Returns empty for negative / overflow seconds.
QString TransferHudViewModel::formatEtaSeconds(qint64 secs) const
{
    if (secs <= 0 || secs > qint64(99) * 3600) return {};
    if (secs >= 3600) {
        const qint64 h = secs / 3600;
        const qint64 m = (secs % 3600) / 60;
        return QStringLiteral("%1h %2m").arg(h)
                                         .arg(m, 2, 10, QLatin1Char('0'));
    }
    if (secs >= 60) {
        const qint64 m = secs / 60;
        const qint64 s = secs % 60;
        return QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0'))
                                       .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1 giây").arg(secs);
}

int TransferHudViewModel::priorityOf(int state, bool /*hasError*/)
{
    // Lower = higher priority.  Order from spec section "HUD content rules":
    //   Failed (Error) > Active > Paused > Queued > Complete (recent)
    using S = TransferState;
    switch (static_cast<S>(state)) {
    case S::Error:     return 0;
    case S::Active:    return 1;
    case S::Paused:    return 2;
    case S::Queued:    return 3;
    case S::Complete:  return 4;
    case S::Cancelled: return 5;
    }
    return 6;
}

void TransferHudViewModel::refreshAll()
{
    recomputeFromLists();
}

void TransferHudViewModel::recomputeFromLists()
{
    const QString prevAggEta = m_aggEtaText;
    int prevActive = m_activeTotal, prevPending = m_pendingTotal, prevFailed = m_failedTotal;
    int prevSync = m_syncPending, prevOverflow = m_overflowCount;
    QString prevRunState = m_runState;
    QString prevDl = m_dlSpeedText, prevUl = m_ulSpeedText;
    bool prevShould = m_shouldShowMini;
    double prevProgress = m_aggregateProgress;

    double totalDl = 0.0, totalUl = 0.0;
    int active = 0, pending = 0, failed = 0;
    int totalIncludingHidden = 0;          // for overflow chip count
    // Accumulators for aggregate taskbar progress (Active tasks only).
    qint64 sumBytes = 0, sumTotal = 0;

    QVector<TransferHudTopModel::Row> candidates;
    candidates.reserve(16);

    auto ingest = [&](TransferListModel *m, int direction, int page) {
        if (!m) return;
        const auto &tasks = m->tasks();
        for (const auto &t : tasks) {
            const int st = static_cast<int>(t.state);
            const bool isDl = (direction == 0);
            const bool isUl = (direction == 1 || direction == 2);

            if (t.state == TransferState::Active) {
                ++active;
                if (isDl) totalDl += t.speed;
                if (isUl) totalUl += t.speed;
                // Aggregate progress: only count Active tasks with a known
                // size (fileSize > 0).  Zero-size / unknown tasks are
                // skipped so they don't drag the ratio toward 0.
                if (t.fileSize > 0) {
                    sumBytes += t.bytesTransferred;
                    sumTotal += t.fileSize;
                }
            } else if (t.state == TransferState::Queued ||
                       t.state == TransferState::Paused) {
                ++pending;
            } else if (t.state == TransferState::Error) {
                ++failed;
            }

            // Skip terminal states with no recent-complete grace window:
            // Cancelled / Complete are noise unless their completedAt is
            // recent enough that the parent VM kept them in the active
            // model (soft-archive TTL).  We just trust the VM here.
            const bool show = (t.state == TransferState::Active ||
                               t.state == TransferState::Paused ||
                               t.state == TransferState::Queued ||
                               t.state == TransferState::Error  ||
                               (t.state == TransferState::Complete && t.completedAt > 0));
            if (!show) continue;

            ++totalIncludingHidden;

            TransferHudTopModel::Row r;
            r.id        = t.id;
            r.fileName  = t.fileName;
            r.direction = direction;
            r.page      = page;
            r.status    = st;
            r.progress  = (t.fileSize > 0)
                              ? std::clamp(double(t.bytesTransferred) / double(t.fileSize), 0.0, 1.0)
                              : (t.progress / 100.0);
            r.speedText = (t.state == TransferState::Active)
                              ? FormatUtil::humanSpeed(t.speed)
                              : QString{};
            r.etaText   = (t.state == TransferState::Active) ? t.eta : QString{};
            r.errorMessage = (t.state == TransferState::Error) ? t.errorMessage : QString{};

            // Bytes meta — Aurora widget meta line "384 MB / 6.4 GB".
            // Skip when fileSize is unknown (folder scans / pre-resolve)
            // since "0 B / 0 B" reads as broken. Active row gets bytes
            // already; queued/paused/done keep them for at-rest context.
            r.bytesDone  = t.bytesTransferred;
            r.bytesTotal = t.fileSize;
            r.bytesText  = (t.fileSize > 0)
                              ? (FormatUtil::humanBytes(t.bytesTransferred)
                                 + QStringLiteral(" / ")
                                 + FormatUtil::humanBytes(t.fileSize))
                              : QString{};

            candidates.push_back(std::move(r));
        }
    };

    // Pages enum (qml/FsAurora/Theme/Pages.qml):
    //   0 = download, 1 = upload.  Sync rows route to the upload page because
    //   sync uses the upload pipeline (they live in the upload list).
    // NOTE: these MUST match Pages.qml — it was reordered (download→0) and an
    // earlier 1/2 numbering here sent focusTask to the wrong page.
    ingest(m_download ? m_download->model() : nullptr, 0, 0);
    ingest(m_upload   ? m_upload->model()   : nullptr, 1, 1);
    // Sync uploads bubble through UploadViewModel already (TransferService
    // emits the same taskAdded signal for isSyncTask=true).  We mark them
    // direction=2 by overriding after ingest: look for tasks whose id we
    // know came from a sync upload.  For P0 we keep it simple and skip
    // re-tagging — visually sync rows appear as uploads, which is fine
    // until SyncViewModel exposes a separate file model we can ingest.

    // Sort: priority bucket, then progress descending (more-done first
    // within a bucket so user sees "almost done" at top).
    std::sort(candidates.begin(), candidates.end(),
              [](const TransferHudTopModel::Row &a, const TransferHudTopModel::Row &b) {
        const int pa = priorityOf(a.status, !a.errorMessage.isEmpty());
        const int pb = priorityOf(b.status, !b.errorMessage.isEmpty());
        if (pa != pb) return pa < pb;
        return a.progress > b.progress;
    });

    QVector<TransferHudTopModel::Row> top = candidates.mid(0, kTopItemsCap);
    const int overflow = std::max(0, totalIncludingHidden - kTopItemsCap);

    // Compose run state.  Mirror Upload/Download VM semantics:
    //   "running" — at least one Active
    //   "paused"  — none Active but ≥1 Paused/Queued
    //   "idle"    — empty (or only Complete/Cancelled)
    QString runState;
    if (active > 0)       runState = QStringLiteral("running");
    else if (pending > 0) runState = QStringLiteral("paused");
    else                  runState = QStringLiteral("idle");

    const int syncPending = m_sync ? m_sync->pendingCount() : 0;

    // shouldShowMini: any activity AND user hasn't manually dismissed.
    // A new transfer after dismiss is the only thing that resets the
    // dismiss flag (handled in onTaskCompleted/recomputeFromLists when
    // a fresh active count rises from 0 → >0).
    const int totalForShow = active + pending + failed + syncPending;
    if (totalForShow > 0 && prevActive + prevPending + prevFailed + prevSync == 0) {
        // Fresh activity — re-arm the mini if it was dismissed.
        m_userDismissed = false;
    }
    const bool shouldShow = totalForShow > 0 && !m_userDismissed;

    // Commit cached state — all-or-nothing so getters see a consistent snapshot.
    m_dlSpeedText   = formatSpeed(totalDl);
    m_ulSpeedText   = formatSpeed(totalUl);
    m_runState      = runState;
    m_activeTotal   = active;
    m_pendingTotal  = pending;
    m_failedTotal   = failed;
    m_syncPending   = syncPending;
    m_overflowCount = overflow;
    m_shouldShowMini = shouldShow;
    // Aggregate progress: -1 when no Active task carries a known size
    // (idle, or only size-unknown tasks). Else Σbytes/Σtotal clamped 0..1.
    m_aggregateProgress = (sumTotal > 0)
                              ? std::clamp(double(sumBytes) / double(sumTotal), 0.0, 1.0)
                              : -1.0;

    // Aggregate ETA — the Aurora widget hero shows "ETA tổng X". Compute
    // from Σ(remaining bytes) / Σ(speed) of Active tasks: the slowest
    // bottleneck dominates because its remaining bytes are the largest
    // share of the sum. Returns empty string when there's no speed (idle)
    // or no size info; QML hero falls back to "—" in that case.
    const double aggSpeed = totalDl + totalUl;
    QString aggEta;
    if (aggSpeed > 0.0 && sumTotal > sumBytes) {
        const qint64 remaining = sumTotal - sumBytes;
        const qint64 secs = static_cast<qint64>(remaining / aggSpeed);
        aggEta = formatEtaSeconds(secs);
    }
    m_aggEtaText = aggEta;

    m_topModel->setRows(top);

    // Emit change notifications narrowly — bindings only re-evaluate when
    // their backing value actually moved.
    if (m_dlSpeedText != prevDl || m_ulSpeedText != prevUl)
        emit speedChanged();
    if (m_runState != prevRunState)
        emit runStateChanged();
    if (m_activeTotal != prevActive || m_pendingTotal != prevPending ||
        m_failedTotal != prevFailed || m_syncPending != prevSync)
        emit countersChanged();
    if (m_overflowCount != prevOverflow)
        emit topItemsChanged();
    if (m_shouldShowMini != prevShould)
        emit shouldShowMiniChanged();
    // Emit progress when it crosses the idle/active boundary or moves
    // > 0.5% — avoids spamming the taskbar COM call every 250ms tick on
    // sub-pixel changes.
    const bool idleFlip = (prevProgress < 0) != (m_aggregateProgress < 0);
    if (idleFlip || std::abs(m_aggregateProgress - prevProgress) > 0.005)
        emit aggregateProgressChanged();
    if (m_aggEtaText != prevAggEta)
        emit aggEtaTextChanged();
}

// ─── Service-level terminal hooks ───────────────────────────────────────────

void TransferHudViewModel::onTaskCompleted(const QString &id)
{
    // Idempotent: each task id → at most one "done" notification.  Keyed
    // by id alone (not (id, success)) because once a task succeeds it
    // can't subsequently fail — TransferService guarantees a terminal
    // state is final.
    if (m_notifiedDone.contains(id)) return;
    m_notifiedDone.insert(id);

    QString name = id;
    if (m_service) {
        const auto t = m_service->findTask(id);
        if (!t.fileName.isEmpty()) name = t.fileName;
    }
    emit transferDone(id, name, /*success=*/true, /*err=*/QString{});

    scheduleRefresh();  // counters will shift on next debounce tick
}

void TransferHudViewModel::onTaskFailed(const QString &id, const QString &error)
{
    if (m_notifiedDone.contains(id)) return;
    m_notifiedDone.insert(id);

    QString name = id;
    if (m_service) {
        const auto t = m_service->findTask(id);
        if (!t.fileName.isEmpty()) name = t.fileName;
    }
    emit transferDone(id, name, /*success=*/false, error);

    scheduleRefresh();
}

// ─── Q_INVOKABLE actions ────────────────────────────────────────────────────

void TransferHudViewModel::pauseAll()
{
    if (m_upload)   m_upload->pauseAll();
    if (m_download) m_download->pauseAll();
}

void TransferHudViewModel::resumeAll()
{
    if (m_upload)   m_upload->resumeAll();
    if (m_download) m_download->resumeAll();
}

void TransferHudViewModel::pauseTask(const QString &id)
{
    // Both VMs forward to TransferService which de-dupes by id internally —
    // safe to fan out without knowing which VM owns the task.
    if (m_upload)   m_upload->pauseTask(id);
    if (m_download) m_download->pauseTask(id);
}

void TransferHudViewModel::resumeTask(const QString &id)
{
    if (m_upload)   m_upload->resumeTask(id);
    if (m_download) m_download->resumeTask(id);
}

void TransferHudViewModel::cancelTask(const QString &id)
{
    if (m_upload)   m_upload->cancelTask(id);
    if (m_download) m_download->cancelTask(id);
}

void TransferHudViewModel::focusTask(const QString &id)
{
    // Find the task in our top-N snapshot to determine which page.  If
    // it's not in top-N (overflow) we fall back to download page since
    // that's the most likely surface user wants.
    int page = 0;  // Pages.download (see Pages.qml — download is 0, not 1)
    auto *m = m_topModel;
    if (m) {
        for (int i = 0; i < m->rowCount(); ++i) {
            const auto idx = m->index(i);
            if (m->data(idx, TransferHudTopModel::TaskIdRole).toString() == id) {
                page = m->data(idx, TransferHudTopModel::PageRole).toInt();
                break;
            }
        }
    }
    emit taskFocusRequested(page, id);
}

QVariantList TransferHudViewModel::speedHistory() const
{
    QVariantList out;
    out.reserve(m_speedHistory.size());
    for (double v : m_speedHistory) out.append(v);
    return out;
}

void TransferHudViewModel::sampleSpeedTick()
{
    // Sum every Active task's current bytes/sec across both lists.  This
    // matches what totalDownloadSpeedText + totalUploadSpeedText would
    // show summed — but as a raw number so the sparkline can scale.
    double total = 0.0;
    auto sumActive = [&](TransferListModel *m) {
        if (!m) return;
        const auto &tasks = m->tasks();
        for (const auto &t : tasks) {
            if (t.state == TransferState::Active) total += t.speed;
        }
    };
    sumActive(m_upload   ? m_upload->model()   : nullptr);
    sumActive(m_download ? m_download->model() : nullptr);

    // Append the new sample to the tail, drop the oldest from the head.
    // Done in place — QVector handles the shift; 60 doubles is trivial.
    m_speedHistory.removeFirst();
    m_speedHistory.append(total);

    // Suppress emit if the buffer just stayed "all zero".  This is the
    // steady-state idle path (no transfers) and Hot-emitting a 60-zero
    // list every second wastes QML bindings + Sparkline redraws.
    const bool nowAllZero = (total == 0.0 && m_speedAllZero);
    if (!nowAllZero) {
        m_speedAllZero = (total == 0.0)
                            ? std::all_of(m_speedHistory.cbegin(),
                                          m_speedHistory.cend(),
                                          [](double v) { return v == 0.0; })
                            : false;
        emit speedHistoryChanged();
    }
}

void TransferHudViewModel::dismissMini()
{
    if (m_userDismissed) return;
    m_userDismissed = true;
    if (m_shouldShowMini) {
        m_shouldShowMini = false;
        emit shouldShowMiniChanged();
    }
}

void TransferHudViewModel::acknowledgeMini()
{
    if (!m_userDismissed) return;
    m_userDismissed = false;
    // Re-evaluate visibility on next debounce tick.
    scheduleRefresh();
}

} // namespace fsnext
