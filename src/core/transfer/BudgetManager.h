#pragma once

// BudgetManager — single source of truth for "is there capacity to start a
// task of class C with priority P right now?"
//
// In Phase 1 this tracks SLOT counts only (concurrent transfers per class).
// Phase 2 will extend with bandwidth-bytes-per-second tokens and per-volume
// disk I/O tokens — keep the public tryAcquire/release shape stable so those
// additions don't churn callers.
//
// Thread-safety
// -------------
// Every public method is thread-safe (internal mutex).  BudgetManager is NOT
// a QObject — it is called directly from the TransferOrchestrator's thread
// and must work without an event loop.

#include "TransferPriority.h"
#include <mutex>

namespace fsnext {

class BudgetManager {
public:
    // Tunable caps.  Pulled from AppSettings at startup; re-applied via
    // setConfig() when the user changes Settings.
    struct Config {
        // Pool caps (hard ceiling per class).
        int maxDownloadSlots = 8;   // user + folder downloads share this
        int maxUploadSlots   = 4;   // user-upload + sync-upload share this
        int maxMetadataSlots = 2;   // FileSyncWorker / prefetch

        // Global ceiling across ALL classes. 0 = disabled (use per-class caps
        // only).  Guards against "8 DL + 4 UL + 2 meta = 14 concurrent" on a
        // laptop that can't sustain it.
        int maxGlobalSlots   = 10;

        // Floor quotas — reserve N slots WITHIN a class for lower-priority
        // work so it is never starved indefinitely.  Applied only when the
        // lower-priority queue has work pending AND the higher-priority
        // queue is hogging all slots.
        //
        //   backgroundFloorPerPool : minimum slots a Background-priority
        //                            task is guaranteed if its queue is
        //                            non-empty (default 1).  Interactive may
        //                            not occupy more than (cap - floor) of
        //                            the pool while Background is queued.
        int backgroundFloorPerPool = 1;
        int metadataFloorGlobal    = 1; // metadata is a single pool
    };

    explicit BudgetManager(Config cfg = {});

    // Try to consume one slot for the given (class, priority).
    //
    // Returns true on success — the caller MUST eventually call release(cls)
    // with the same class.  Returns false if any of these would be violated:
    //   • per-class cap
    //   • global cap
    //   • priority-aware floor (caller is "too greedy" and a lower-priority
    //     pending task would be starved)
    //
    // The `lowerPriorityWaiting` array tells BudgetManager how many tasks of
    // each priority are currently queued (needed to enforce the floor).  The
    // PriorityScheduler supplies this atomically when it calls tryAcquire.
    bool tryAcquire(TransferClass cls, TransferPriority prio,
                    const int lowerPriorityWaiting[4]);

    // Release a slot previously granted.  Idempotent only if called exactly
    // once per successful acquire.
    void release(TransferClass cls, TransferPriority prio);

    // Replace the running config (thread-safe).  Does not kill in-flight
    // tasks even if the new cap is lower; excess drains naturally as tasks
    // complete.
    void setConfig(const Config &cfg);
    Config config() const;

    // Read-only snapshot for telemetry / status bar display.
    struct Usage {
        int totalActive;
        int activeDownloads;
        int activeUploads;
        int activeMetadata;
    };
    Usage usage() const;

private:
    // Pool slot currently in use? (Phase 1 = slots only.)
    int poolActive(TransferClass cls) const;   // caller holds m_mx
    int poolCap   (TransferClass cls) const;   // caller holds m_mx

    mutable std::mutex m_mx;
    Config             m_cfg;

    int m_active[3]    = {0, 0, 0};  // indexed by TransferClass int value
    int m_activeGlobal = 0;
};

} // namespace fsnext
