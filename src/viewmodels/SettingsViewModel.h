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
    Q_PROPERTY(bool minimizeToTray   READ minimizeToTray    WRITE setMinimizeToTray    NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(int  fileConflictPolicy READ fileConflictPolicy WRITE setFileConflictPolicy NOTIFY fileConflictPolicyChanged)

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
    bool minimizeToTray() const;
    int  fileConflictPolicy() const;

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
    void setMinimizeToTray(bool value);
    void setFileConflictPolicy(int value);

    Q_INVOKABLE void applySettings();
    Q_INVOKABLE void resetSettings();

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
    void minimizeToTrayChanged();
    void fileConflictPolicyChanged();

private:
    SettingsService *m_service = nullptr;
    AppSettings      m_settings;
};

} // namespace fsnext
