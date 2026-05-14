#pragma once
#include <QString>

namespace fsnext {

struct AppSettings {
    // General
    QString language = QStringLiteral("en");
    bool autoLogin = false;
    bool stayOnTop = false;
    bool autoStart = false;
    // When true, closing the main window hides it to the tray instead of
    // quitting the app.  User can still quit via tray → "Thoát".  Defaults
    // to true on Windows (matches user expectation for download tools);
    // false on macOS/Linux where dock/quit semantics differ.
    bool minimizeToTray =
#ifdef Q_OS_WIN
        true;
#else
        false;
#endif

    // File-name conflict policy for downloads — see ADR 003 D7.
    //   0 = rename "(n)"  (default, non-destructive)
    //   1 = overwrite
    //   2 = skip
    //   3 = ask each time
    int fileConflictPolicy = 0;

    // Connection
    int proxyMode = 0;        // 0=none, 1=system, 2=manual
    QString proxyHost;
    int proxyPort = 0;

    // Download
    // Max concurrent files: 16. Recommended: 8 for VIP, 4 for free.
    int downloadThreads = 8;
    // Segments per file (HTTP Range parallel streams). Max: 32. Recommended: 16.
    // Requires server to support Accept-Ranges and file >= 2 MB.
    int downloadSegments = 16;
    bool autoDownload = false;
    QString downloadFolder;

    // Upload
    // Max concurrent uploads: 8. Recommended: 4.
    int uploadThreads = 4;
    QString uploadFolder;

    // Transfer budget
    // Global cap across every TransferClass (DL+UL+Metadata).  Default 10
    // matches BudgetManager::Config so a laptop doesn't get saturated when
    // the per-class caps sum to more than the machine can sustain.  Set to
    // 0 to disable the global ceiling (per-class caps still apply).  Range:
    // 1..32; clamped in SettingsService.
    int maxGlobalSlots = 10;

    // Account
    QString savedEmail;
    QString savedToken;       // Base64-encoded password (legacy compat) or session token
    bool rememberMe = false;

    // UI
    bool darkMode = false;
};

} // namespace fsnext
