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
    //
    // NOTE: from v6.0, the default UX is to ASK on close (confirmOnClose
    // below). This flag still drives behaviour but only kicks in once the
    // user has ticked "Don't ask again" in the confirm dialog AND chose
    // Minimize there.
    bool minimizeToTray =
#ifdef Q_OS_WIN
        true;
#else
        false;
#endif

    // When true (default), clicking X on the main window pops a confirm
    // dialog instead of immediately hiding / quitting. The dialog gives the
    // user three explicit choices (Quit / Minimize / Cancel) plus a
    // "Don't ask again" checkbox that flips this flag off and persists the
    // chosen action via minimizeToTray.  Without this gate, the old
    // behaviour silently left FsNext running in the tray after X — surprising
    // users who expected X = quit.
    bool confirmOnClose = true;

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
    // Sidebar collapsed state — when true, the left nav shows icons only
    // (64px). Persisted so the user's preferred screen real-estate is
    // remembered across launches.
    bool sidebarCollapsed = false;

    // Sidebar Mini-HUD variant index (Dark Aurora v2):
    //   0 = Speed Pulse (default), 1 = Dual Rings, 2 = File Marquee, 3 = Stat Grid
    // The user picks via swipe / dots / arrow buttons on the HUD card; the
    // chosen variant sticks across launches.  Clamped to 0..3 in the VM.
    int sidebarHudVariant = 0;

    // HUD — when true, show a Windows balloon notification on each transfer
    // terminal state (complete / fail) via QSystemTrayIcon::showMessage. The
    // tray icon colour swap (idle/active/error) and tooltip update are NOT
    // gated by this flag — they are always on, since they're passive
    // feedback. This flag controls only the noisy/interruptive surface.
    bool notifyOnTransferDone = true;

    // When true and the user closes the main window to the tray (X button
    // with minimizeToTray=true), the floating Mini HUD window appears with
    // current transfer state — provided there is at least one active or
    // pending transfer.  Default true: matches the spec's "user just sent
    // the app away while a download runs" use-case.  Setting OFF leaves
    // the tray icon + balloons as the only feedback channels.
    bool showOnHideToTray = true;

    // Persisted Mini HUD window position (last user drag/snap point).
    // -1 means "no saved position; use default bottom-right of screen".
    // miniWindowScreen tracks the screen name (QScreen::name) so we can
    // restore to the same physical monitor on multi-display rigs; if the
    // saved screen no longer exists, the position resets to default.
    int     miniWindowX = -1;
    int     miniWindowY = -1;
    QString miniWindowScreen;

    // When true (Windows only), draw a native progress bar on the taskbar
    // button while transfers run (via ITaskbarList3).  Zero-intrusion
    // status that complements the Mini HUD.  No-op on macOS/Linux.
    bool showTaskbarProgress = true;
};

} // namespace fsnext
