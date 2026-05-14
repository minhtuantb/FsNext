#include "TransferBudgetViewModel.h"

#include "core/transfer/TransferOrchestrator.h"
#include "core/transfer/BudgetManager.h"
#include "core/transfer/TransferPriority.h"

namespace fsnext {

// Sampling cadence.  500 ms is fast enough that users perceive the status
// bar as "live" while keeping the orchestrator's mutex pressure negligible
// (BudgetManager::usage() is a single short lock + three reads).
static constexpr int kPollIntervalMs = 500;

TransferBudgetViewModel::TransferBudgetViewModel(TransferOrchestrator *orch,
                                                 QObject *parent)
    : QObject(parent), m_orch(orch)
{
    m_timer.setInterval(kPollIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &TransferBudgetViewModel::poll);
    m_timer.start();

    // Prime the values so QML initial bindings see the real caps immediately
    // rather than 0 for the first 500 ms.
    poll();
}

TransferBudgetViewModel::~TransferBudgetViewModel() = default;

void TransferBudgetViewModel::poll()
{
    if (!m_orch) return;

    const auto u   = m_orch->usage();
    const auto cfg = m_orch->config();
    const int pDl   = m_orch->pendingCount(TransferClass::Download);
    const int pUl   = m_orch->pendingCount(TransferClass::Upload);
    const int pMeta = m_orch->pendingCount(TransferClass::Metadata);

    // Compare-then-assign so we only emit usageChanged on actual drift.
    // QML ListView/Text bindings that are wired to these properties would
    // otherwise re-render twice per second even on a completely idle app.
    bool changed = false;
    auto upd = [&](int &dst, int src) {
        if (dst != src) { dst = src; changed = true; }
    };

    upd(m_activeDl,    u.activeDownloads);
    upd(m_activeUl,    u.activeUploads);
    upd(m_activeMeta,  u.activeMetadata);
    upd(m_activeTotal, u.totalActive);

    upd(m_maxDl,    cfg.maxDownloadSlots);
    upd(m_maxUl,    cfg.maxUploadSlots);
    upd(m_maxMeta,  cfg.maxMetadataSlots);
    upd(m_maxTotal, cfg.maxGlobalSlots);

    upd(m_pendingDl,   pDl);
    upd(m_pendingUl,   pUl);
    upd(m_pendingMeta, pMeta);

    if (changed) emit usageChanged();
}

} // namespace fsnext
