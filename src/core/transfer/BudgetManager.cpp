#include "BudgetManager.h"

namespace fsnext {

const char *toString(TransferPriority p)
{
    switch (p) {
        case TransferPriority::Interactive: return "Interactive";
        case TransferPriority::Normal:      return "Normal";
        case TransferPriority::Background:  return "Background";
        case TransferPriority::Metadata:    return "Metadata";
    }
    return "?";
}

const char *toString(TransferClass c)
{
    switch (c) {
        case TransferClass::Download: return "Download";
        case TransferClass::Upload:   return "Upload";
        case TransferClass::Metadata: return "Metadata";
    }
    return "?";
}

BudgetManager::BudgetManager(Config cfg)
    : m_cfg(cfg)
{}

int BudgetManager::poolActive(TransferClass cls) const
{
    return m_active[static_cast<int>(cls)];
}

int BudgetManager::poolCap(TransferClass cls) const
{
    switch (cls) {
        case TransferClass::Download: return m_cfg.maxDownloadSlots;
        case TransferClass::Upload:   return m_cfg.maxUploadSlots;
        case TransferClass::Metadata: return m_cfg.maxMetadataSlots;
    }
    return 0;
}

bool BudgetManager::tryAcquire(TransferClass cls, TransferPriority prio,
                               const int lowerPriorityWaiting[4])
{
    std::lock_guard<std::mutex> lk(m_mx);

    // (1) Per-class cap
    const int cap = poolCap(cls);
    if (cap <= 0 || poolActive(cls) >= cap) return false;

    // (2) Global cap (0 = disabled)
    if (m_cfg.maxGlobalSlots > 0 && m_activeGlobal >= m_cfg.maxGlobalSlots)
        return false;

    // (3) Floor quota — prevent high-priority from eating the last `floor`
    //     slots of a pool while a lower-priority task is queued.
    const int prioInt = static_cast<int>(prio);
    const int bgInt   = static_cast<int>(TransferPriority::Background);
    const int metaInt = static_cast<int>(TransferPriority::Metadata);

    if (cls == TransferClass::Download || cls == TransferClass::Upload) {
        // Background floor only matters to Interactive/Normal callers.
        if (prioInt < bgInt && lowerPriorityWaiting[bgInt] > 0) {
            const int floor = m_cfg.backgroundFloorPerPool;
            if (poolActive(cls) >= cap - floor) return false;
        }
    }
    if (cls == TransferClass::Metadata) {
        // Metadata-priority floor for metadata pool.
        if (prioInt < metaInt && lowerPriorityWaiting[metaInt] > 0) {
            const int floor = m_cfg.metadataFloorGlobal;
            if (poolActive(cls) >= cap - floor) return false;
        }
    }

    // Grant
    m_active[static_cast<int>(cls)]++;
    m_activeGlobal++;
    return true;
}

void BudgetManager::release(TransferClass cls, TransferPriority /*prio*/)
{
    std::lock_guard<std::mutex> lk(m_mx);
    if (m_active[static_cast<int>(cls)] > 0) {
        m_active[static_cast<int>(cls)]--;
    }
    if (m_activeGlobal > 0) m_activeGlobal--;
}

void BudgetManager::setConfig(const Config &cfg)
{
    std::lock_guard<std::mutex> lk(m_mx);
    m_cfg = cfg;
}

BudgetManager::Config BudgetManager::config() const
{
    std::lock_guard<std::mutex> lk(m_mx);
    return m_cfg;
}

BudgetManager::Usage BudgetManager::usage() const
{
    std::lock_guard<std::mutex> lk(m_mx);
    Usage u{};
    u.totalActive     = m_activeGlobal;
    u.activeDownloads = m_active[0];
    u.activeUploads   = m_active[1];
    u.activeMetadata  = m_active[2];
    return u;
}

} // namespace fsnext
