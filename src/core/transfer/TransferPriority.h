#pragma once

// TransferPriority / TransferClass — two orthogonal axes used by the
// TransferOrchestrator to decide WHICH pool a task consumes and WHEN it runs.
//
//   • TransferClass  = physical resource pool the task competes in
//                      (Download / Upload / Metadata). User-upload and
//                      sync-upload SHARE the Upload pool — the distinction
//                      is priority, not a separate pool.
//   • TransferPriority = scheduling order within its class.
//
// Design intent
// -------------
// Keep these tiny and stable. They flow through Qt queued signals and
// across thread boundaries, so any change ripples.  Do NOT put behaviour
// flags here — put them in BudgetManager::Config.

#include <QMetaType>

namespace fsnext {

// Scheduling priority — lower numeric value = higher priority.
// Interactive >> Normal >> Background >> Metadata.
enum class TransferPriority : int {
    Interactive = 0,   // User just clicked Download / Upload / "Sync now".
                       //   Weight hint: 70%. Never starved by anything.
    Normal      = 1,   // User just pasted a file into a synced folder and the
                       // FileSystemWatcher triggered an upload. Weight hint 20%.
    Background  = 2,   // Auto-sync timer tick, full-tree rescan.  Weight 8%.
    Metadata    = 3,   // API-only: FileSyncWorker folder crawl, prefetch.
                       // Weight 2%. Runs only on leftover capacity.
};

// Physical resource pool. Pools are independent; per-pool slot caps are
// enforced by BudgetManager.
enum class TransferClass : int {
    Download = 0,      // HTTP GET stream (multi-segment capable)
    Upload   = 1,      // HTTP PUT stream (user + sync share this pool)
    Metadata = 2,      // No byte transfer, just an API round-trip
};

// Stringify for logs/telemetry (defined in BudgetManager.cpp to keep the
// header free of Qt dependencies other than QMetaType).
const char *toString(TransferPriority p);
const char *toString(TransferClass c);

} // namespace fsnext

Q_DECLARE_METATYPE(fsnext::TransferPriority)
Q_DECLARE_METATYPE(fsnext::TransferClass)
