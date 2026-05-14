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
