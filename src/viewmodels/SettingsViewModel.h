#pragma once

#include "core/models/AppSettings.h"
#include <QObject>
#include <QString>

namespace fsnext {

class SettingsService;

class SettingsViewModel : public QObject
{
    Q_OBJECT

    // Download
    Q_PROPERTY(int downloadThreads   READ downloadThreads   WRITE setDownloadThreads   NOTIFY downloadThreadsChanged)
    Q_PROPERTY(int downloadSegments  READ downloadSegments  WRITE setDownloadSegments  NOTIFY downloadSegmentsChanged)
    Q_PROPERTY(bool autoDownload     READ autoDownload      WRITE setAutoDownload      NOTIFY autoDownloadChanged)
    Q_PROPERTY(QString downloadFolder         READ downloadFolder         WRITE setDownloadFolder NOTIFY downloadFolderChanged)
    Q_PROPERTY(QString effectiveDownloadFolder READ effectiveDownloadFolder NOTIFY downloadFolderChanged)

    // Upload
    Q_PROPERTY(int uploadThreads     READ uploadThreads     WRITE setUploadThreads     NOTIFY uploadThreadsChanged)

    // Transfer budget (global cap across all classes; 0 disables)
    Q_PROPERTY(int maxGlobalSlots    READ maxGlobalSlots    WRITE setMaxGlobalSlots    NOTIFY maxGlobalSlotsChanged)

    // Connection
    Q_PROPERTY(int proxyMode         READ proxyMode         WRITE setProxyMode         NOTIFY proxyModeChanged)
    Q_PROPERTY(QString proxyHost     READ proxyHost         WRITE setProxyHost         NOTIFY proxyHostChanged)
    Q_PROPERTY(int proxyPort         READ proxyPort         WRITE setProxyPort         NOTIFY proxyPortChanged)

    // General
    Q_PROPERTY(QString language      READ language          WRITE setLanguage          NOTIFY languageChanged)
    Q_PROPERTY(bool autoLogin        READ autoLogin         WRITE setAutoLogin         NOTIFY autoLoginChanged)
    Q_PROPERTY(bool stayOnTop        READ stayOnTop         WRITE setStayOnTop         NOTIFY stayOnTopChanged)
    Q_PROPERTY(bool darkMode         READ darkMode          WRITE setDarkMode          NOTIFY darkModeChanged)
    Q_PROPERTY(bool sidebarCollapsed READ sidebarCollapsed  WRITE setSidebarCollapsed  NOTIFY sidebarCollapsedChanged)
    Q_PROPERTY(int  sidebarHudVariant READ sidebarHudVariant WRITE setSidebarHudVariant NOTIFY sidebarHudVariantChanged)
    Q_PROPERTY(bool minimizeToTray   READ minimizeToTray    WRITE setMinimizeToTray    NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(bool confirmOnClose   READ confirmOnClose    WRITE setConfirmOnClose    NOTIFY confirmOnCloseChanged)
    Q_PROPERTY(int  fileConflictPolicy READ fileConflictPolicy WRITE setFileConflictPolicy NOTIFY fileConflictPolicyChanged)

    // HUD toggles
    Q_PROPERTY(bool notifyOnTransferDone READ notifyOnTransferDone WRITE setNotifyOnTransferDone NOTIFY notifyOnTransferDoneChanged)
    Q_PROPERTY(bool showOnHideToTray     READ showOnHideToTray     WRITE setShowOnHideToTray     NOTIFY showOnHideToTrayChanged)
    Q_PROPERTY(bool showTaskbarProgress  READ showTaskbarProgress  WRITE setShowTaskbarProgress  NOTIFY showTaskbarProgressChanged)

public:
    explicit SettingsViewModel(SettingsService *service, QObject *parent = nullptr);
    ~SettingsViewModel() override = default;

    // Getters
    int downloadThreads() const;
    int downloadSegments() const;
    bool autoDownload() const;
    QString downloadFolder() const;
    QString effectiveDownloadFolder() const;
    int uploadThreads() const;
    int maxGlobalSlots() const;
    int proxyMode() const;
    QString proxyHost() const;
    int proxyPort() const;
    QString language() const;
    bool autoLogin() const;
    bool stayOnTop() const;
    bool darkMode() const;
    bool sidebarCollapsed() const;
    int  sidebarHudVariant() const;
    bool minimizeToTray() const;
    bool confirmOnClose() const;
    int  fileConflictPolicy() const;
    bool notifyOnTransferDone() const;
    bool showOnHideToTray() const;
    bool showTaskbarProgress() const;

    // Setters
    void setDownloadThreads(int value);
    void setDownloadSegments(int value);
    void setAutoDownload(bool value);
    void setDownloadFolder(const QString &value);
    void setUploadThreads(int value);
    void setMaxGlobalSlots(int value);
    void setProxyMode(int value);
    void setProxyHost(const QString &value);
    void setProxyPort(int value);
    void setLanguage(const QString &value);
    void setAutoLogin(bool value);
    void setStayOnTop(bool value);
    void setDarkMode(bool value);
    void setSidebarCollapsed(bool value);
    void setSidebarHudVariant(int value);
    void setMinimizeToTray(bool value);
    void setConfirmOnClose(bool value);
    void setFileConflictPolicy(int value);
    void setNotifyOnTransferDone(bool value);
    void setShowOnHideToTray(bool value);
    void setShowTaskbarProgress(bool value);

    Q_INVOKABLE void applySettings();
    Q_INVOKABLE void resetSettings();

    // Persisted Mini HUD window position — read on launch, written every
    // time the user drags or snaps the floating mini window.  Returns -1
    // for x/y when no saved position exists (caller falls back to default
    // bottom-right of screen).
    Q_INVOKABLE int     savedMiniWindowX() const;
    Q_INVOKABLE int     savedMiniWindowY() const;
    Q_INVOKABLE QString savedMiniWindowScreen() const;
    Q_INVOKABLE void    saveMiniWindowPosition(int x, int y, const QString &screen);

signals:
    void downloadThreadsChanged();
    void downloadSegmentsChanged();
    void autoDownloadChanged();
    void downloadFolderChanged();
    void uploadThreadsChanged();
    void maxGlobalSlotsChanged();
    void proxyModeChanged();
    void proxyHostChanged();
    void proxyPortChanged();
    void languageChanged();
    void autoLoginChanged();
    void stayOnTopChanged();
    void darkModeChanged();
    void sidebarCollapsedChanged();
    void sidebarHudVariantChanged();
    void minimizeToTrayChanged();
    void confirmOnCloseChanged();
    void fileConflictPolicyChanged();
    void notifyOnTransferDoneChanged();
    void showOnHideToTrayChanged();
    void showTaskbarProgressChanged();

private:
    SettingsService *m_service = nullptr;
    AppSettings      m_settings;
};

} // namespace fsnext
