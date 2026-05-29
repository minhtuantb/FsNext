#pragma once

#include "core/models/AppSettings.h"
#include <QObject>
#include <QString>

namespace fsnext {

class SettingsRepository;

class SettingsService : public QObject
{
    Q_OBJECT

public:
    explicit SettingsService(SettingsRepository *repo, QObject *parent = nullptr);
    ~SettingsService() override = default;

    // Bulk load / save
    AppSettings loadSettings();
    void saveSettings(const AppSettings &settings);

    // --- Download ---
    int  downloadThreads() const;
    void setDownloadThreads(int value);

    int  downloadSegments() const;
    void setDownloadSegments(int value);

    bool autoDownload() const;
    void setAutoDownload(bool value);

    QString downloadFolder() const;
    void setDownloadFolder(const QString &path);

    // Returns downloadFolder() when non-empty, otherwise falls back to the
    // OS Downloads location (QStandardPaths::DownloadLocation), finally home.
    // Used by DownloadViewModel when the user leaves the save-folder field blank.
    QString effectiveDownloadFolder() const;

    // --- Upload ---
    int  uploadThreads() const;
    void setUploadThreads(int value);

    QString uploadFolder() const;
    void setUploadFolder(const QString &path);

    // --- Transfer budget ---
    // Global cap across every TransferClass (DL+UL+Metadata).  0 disables.
    // AppContext pushes this value into TransferOrchestrator on change.
    int  maxGlobalSlots() const;
    void setMaxGlobalSlots(int value);

    // --- Account ---
    bool rememberMe() const;
    void setRememberMe(bool value);

    QString savedEmail() const;
    void setSavedEmail(const QString &email);

    bool autoLogin() const;
    void setAutoLogin(bool value);

    // --- UI ---
    bool darkMode() const;
    void setDarkMode(bool value);

    // Sidebar collapsed state — see ADR / Sprint 2 item C. Persisted in
    // QSettings under "UI/sidebarCollapsed" so the user's preferred screen
    // real-estate sticks across launches.
    bool sidebarCollapsed() const;
    void setSidebarCollapsed(bool value);

    // Sidebar Mini-HUD variant (0=Speed Pulse, 1=Dual Rings, 2=File Marquee,
    // 3=Stat Grid). Persisted under "UI/sidebarHudVariant".
    int  sidebarHudVariant() const;
    void setSidebarHudVariant(int value);

    QString language() const;
    void setLanguage(const QString &code);

    // --- General ---
    bool stayOnTop() const;
    void setStayOnTop(bool value);

    bool autoStart() const;
    void setAutoStart(bool value);

    // When true, closing the main window hides it to the tray instead of
    // quitting the app.  Honoured by Main.qml's onClosing handler — see
    // ADR 003 D4.
    bool minimizeToTray() const;
    void setMinimizeToTray(bool value);

    // Pop a confirm dialog before closing the main window (default true).
    // The dialog also lets the user persist their choice — see Main.qml's
    // closeConfirmDialog wiring.
    bool confirmOnClose() const;
    void setConfirmOnClose(bool value);

    // When true, the system tray fires a balloon (showMessage) on each
    // transfer terminal event. Tray icon colour + tooltip live independently.
    bool notifyOnTransferDone() const;
    void setNotifyOnTransferDone(bool value);

    // When true, hide-to-tray (X with minimizeToTray=on) shows the Mini
    // HUD window when there's at least one active/pending transfer.
    bool showOnHideToTray() const;
    void setShowOnHideToTray(bool value);

    // Persisted Mini HUD window position. -1 means "use default".
    // miniWindowScreen stores the QScreen::name() so multi-monitor users
    // get the popup back on the same physical display.
    int     miniWindowX() const;
    int     miniWindowY() const;
    QString miniWindowScreen() const;
    void    setMiniWindowPosition(int x, int y, const QString &screen);

    // Windows taskbar progress bar toggle (no-op on other platforms).
    bool showTaskbarProgress() const;
    void setShowTaskbarProgress(bool value);

    // File-name conflict policy for downloads.  See ADR 003 D7.
    //   0 = Rename "(n)"  (default, non-destructive)
    //   1 = Overwrite
    //   2 = Skip
    //   3 = Ask each time
    int  fileConflictPolicy() const;
    void setFileConflictPolicy(int value);

    // --- Proxy ---
    int  proxyMode() const;
    void setProxyMode(int mode);

    QString proxyHost() const;
    void setProxyHost(const QString &host);

    int  proxyPort() const;
    void setProxyPort(int port);

signals:
    void settingsChanged();

private:
    SettingsRepository *m_repo = nullptr;
    AppSettings         m_settings;
};

} // namespace fsnext
