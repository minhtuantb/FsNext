#pragma once

// TransferBudgetViewModel — thin QML-facing binding onto the running
// BudgetManager::Usage inside TransferOrchestrator.
//
// Why a polling VM instead of signal-per-grant?
//   The orchestrator runs on its own worker thread and updates usage on
//   every tryAcquire/release.  Wiring a Qt signal across that boundary
//   for every grant would be O(transfers-per-second) of queued events
//   just to drive a status-bar indicator that the eye can't read faster
//   than ~2 Hz anyway.  A 500 ms QTimer on the main thread samples the
//   orchestrator atomically and only emits NOTIFY when values change,
//   which keeps QML bindings quiet when nothing is happening.
//
// Intended use (QML):
//   Text { text: "DL " + budgetVM.activeDownloads + "/" + budgetVM.maxDownloads }
//
// Properties are read-only from QML; the orchestrator is the source of truth.

#include <QObject>
#include <QTimer>

namespace fsnext {

class TransferOrchestrator;

class TransferBudgetViewModel : public QObject {
    Q_OBJECT
    // Currently active (slots in use).
    Q_PROPERTY(int activeDownloads READ activeDownloads NOTIFY usageChanged)
    Q_PROPERTY(int activeUploads   READ activeUploads   NOTIFY usageChanged)
    Q_PROPERTY(int activeMetadata  READ activeMetadata  NOTIFY usageChanged)
    Q_PROPERTY(int activeTotal     READ activeTotal     NOTIFY usageChanged)

    // Configured caps (refreshed on the same tick — caps change when the
    // user moves the Settings slider).
    Q_PROPERTY(int maxDownloads READ maxDownloads NOTIFY usageChanged)
    Q_PROPERTY(int maxUploads   READ maxUploads   NOTIFY usageChanged)
    Q_PROPERTY(int maxMetadata  READ maxMetadata  NOTIFY usageChanged)
    Q_PROPERTY(int maxTotal     READ maxTotal     NOTIFY usageChanged)

    // Pending (queued but not yet dispatched) per class.
    Q_PROPERTY(int pendingDownloads READ pendingDownloads NOTIFY usageChanged)
    Q_PROPERTY(int pendingUploads   READ pendingUploads   NOTIFY usageChanged)
    Q_PROPERTY(int pendingMetadata  READ pendingMetadata  NOTIFY usageChanged)

public:
    explicit TransferBudgetViewModel(TransferOrchestrator *orch,
                                     QObject *parent = nullptr);
    ~TransferBudgetViewModel() override;

    int activeDownloads() const { return m_activeDl; }
    int activeUploads()   const { return m_activeUl; }
    int activeMetadata()  const { return m_activeMeta; }
    int activeTotal()     const { return m_activeTotal; }

    int maxDownloads() const { return m_maxDl; }
    int maxUploads()   const { return m_maxUl; }
    int maxMetadata()  const { return m_maxMeta; }
    int maxTotal()     const { return m_maxTotal; }

    int pendingDownloads() const { return m_pendingDl; }
    int pendingUploads()   const { return m_pendingUl; }
    int pendingMetadata()  const { return m_pendingMeta; }

signals:
    void usageChanged();

private slots:
    // Sample the orchestrator on every tick.  Only emits usageChanged when
    // any observed value actually differs from the last snapshot, so idle
    // sessions don't thrash QML bindings.
    void poll();

private:
    TransferOrchestrator *m_orch = nullptr;
    QTimer                m_timer;

    int m_activeDl = 0, m_activeUl = 0, m_activeMeta = 0, m_activeTotal = 0;
    int m_maxDl    = 0, m_maxUl    = 0, m_maxMeta    = 0, m_maxTotal    = 0;
    int m_pendingDl = 0, m_pendingUl = 0, m_pendingMeta = 0;
};

} // namespace fsnext
