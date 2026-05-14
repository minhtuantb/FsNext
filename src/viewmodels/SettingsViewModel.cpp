#include "SettingsViewModel.h"
#include "core/services/SettingsService.h"

namespace fsnext {

SettingsViewModel::SettingsViewModel(SettingsService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    // Load persisted settings on startup so QML bindings initialize with
    // whatever the user saved last session.
    if (m_service) {
        m_settings = m_service->loadSettings();

        // If the service's state changes from outside this ViewModel (e.g. a
        // direct call on the service from another component), reflect it in QML.
        connect(m_service, &SettingsService::settingsChanged, this, [this]() {
            if (!m_service) return;
            const AppSettings fresh = m_service->loadSettings();
            if (fresh.downloadFolder != m_settings.downloadFolder) {
                m_settings.downloadFolder = fresh.downloadFolder;
                emit downloadFolderChanged();
            }
        });
    }
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

int SettingsViewModel::downloadThreads() const  { return m_settings.downloadThreads; }
int SettingsViewModel::downloadSegments() const { return m_settings.downloadSegments; }
bool SettingsViewModel::autoDownload() const    { return m_settings.autoDownload; }
QString SettingsViewModel::downloadFolder() const { return m_settings.downloadFolder; }

QString SettingsViewModel::effectiveDownloadFolder() const
{
    return m_service ? m_service->effectiveDownloadFolder() : m_settings.downloadFolder;
}

int SettingsViewModel::uploadThreads() const    { return m_settings.uploadThreads; }
int SettingsViewModel::maxGlobalSlots() const   { return m_settings.maxGlobalSlots; }
int SettingsViewModel::proxyMode() const        { return m_settings.proxyMode; }
QString SettingsViewModel::proxyHost() const    { return m_settings.proxyHost; }
int SettingsViewModel::proxyPort() const        { return m_settings.proxyPort; }
QString SettingsViewModel::language() const     { return m_settings.language; }
bool SettingsViewModel::autoLogin() const       { return m_settings.autoLogin; }
bool SettingsViewModel::stayOnTop() const       { return m_settings.stayOnTop; }
bool SettingsViewModel::darkMode() const        { return m_settings.darkMode; }
bool SettingsViewModel::minimizeToTray() const  { return m_settings.minimizeToTray; }
int  SettingsViewModel::fileConflictPolicy() const { return m_settings.fileConflictPolicy; }

// ---------------------------------------------------------------------------
// Setters — each one forwards to the service so the change is persisted
// immediately (no separate "Apply" button required).
// ---------------------------------------------------------------------------

void SettingsViewModel::setDownloadThreads(int value)
{
    if (m_settings.downloadThreads == value) return;
    m_settings.downloadThreads = value;
    if (m_service) m_service->setDownloadThreads(value);
    emit downloadThreadsChanged();
}

void SettingsViewModel::setDownloadSegments(int value)
{
    if (m_settings.downloadSegments == value) return;
    m_settings.downloadSegments = value;
    if (m_service) m_service->setDownloadSegments(value);
    emit downloadSegmentsChanged();
}

void SettingsViewModel::setAutoDownload(bool value)
{
    if (m_settings.autoDownload == value) return;
    m_settings.autoDownload = value;
    if (m_service) m_service->setAutoDownload(value);
    emit autoDownloadChanged();
}

void SettingsViewModel::setDownloadFolder(const QString &value)
{
    if (m_settings.downloadFolder == value) return;
    m_settings.downloadFolder = value;
    if (m_service) m_service->setDownloadFolder(value);
    emit downloadFolderChanged();
}

void SettingsViewModel::setUploadThreads(int value)
{
    if (m_settings.uploadThreads == value) return;
    m_settings.uploadThreads = value;
    if (m_service) m_service->setUploadThreads(value);
    emit uploadThreadsChanged();
}

void SettingsViewModel::setMaxGlobalSlots(int value)
{
    if (m_settings.maxGlobalSlots == value) return;
    m_settings.maxGlobalSlots = value;
    if (m_service) m_service->setMaxGlobalSlots(value);
    emit maxGlobalSlotsChanged();
}

void SettingsViewModel::setProxyMode(int value)
{
    if (m_settings.proxyMode == value) return;
    m_settings.proxyMode = value;
    if (m_service) m_service->setProxyMode(value);
    emit proxyModeChanged();
}

void SettingsViewModel::setProxyHost(const QString &value)
{
    if (m_settings.proxyHost == value) return;
    m_settings.proxyHost = value;
    if (m_service) m_service->setProxyHost(value);
    emit proxyHostChanged();
}

void SettingsViewModel::setProxyPort(int value)
{
    if (m_settings.proxyPort == value) return;
    m_settings.proxyPort = value;
    if (m_service) m_service->setProxyPort(value);
    emit proxyPortChanged();
}

void SettingsViewModel::setLanguage(const QString &value)
{
    if (m_settings.language == value) return;
    m_settings.language = value;
    if (m_service) m_service->setLanguage(value);
    emit languageChanged();
}

void SettingsViewModel::setAutoLogin(bool value)
{
    if (m_settings.autoLogin == value) return;
    m_settings.autoLogin = value;
    if (m_service) m_service->setAutoLogin(value);
    emit autoLoginChanged();
}

void SettingsViewModel::setStayOnTop(bool value)
{
    if (m_settings.stayOnTop == value) return;
    m_settings.stayOnTop = value;
    if (m_service) m_service->setStayOnTop(value);
    emit stayOnTopChanged();
}

void SettingsViewModel::setDarkMode(bool value)
{
    if (m_settings.darkMode == value) return;
    m_settings.darkMode = value;
    if (m_service) m_service->setDarkMode(value);
    emit darkModeChanged();
}

void SettingsViewModel::setMinimizeToTray(bool value)
{
    if (m_settings.minimizeToTray == value) return;
    m_settings.minimizeToTray = value;
    if (m_service) m_service->setMinimizeToTray(value);
    emit minimizeToTrayChanged();
}

void SettingsViewModel::setFileConflictPolicy(int value)
{
    // Mirror the SettingsService clamp (0..3).  Doing it here too lets QML
    // pass arbitrary values without thinking about validation.
    const int clamped = qBound(0, value, 3);
    if (m_settings.fileConflictPolicy == clamped) return;
    m_settings.fileConflictPolicy = clamped;
    if (m_service) m_service->setFileConflictPolicy(clamped);
    emit fileConflictPolicyChanged();
}

// ---------------------------------------------------------------------------
// Invokables
// ---------------------------------------------------------------------------

void SettingsViewModel::applySettings()
{
    // Individual setters already persist immediately, so applySettings()
    // is a no-op for now. Kept as a QML-visible hook for future bulk commits.
    if (m_service) m_service->saveSettings(m_settings);
}

void SettingsViewModel::resetSettings()
{
    // Reset to default-constructed AppSettings values.
    AppSettings defaults;
    setDownloadThreads(defaults.downloadThreads);
    setDownloadSegments(defaults.downloadSegments);
    setAutoDownload(defaults.autoDownload);
    setDownloadFolder(defaults.downloadFolder);
    setUploadThreads(defaults.uploadThreads);
    setMaxGlobalSlots(defaults.maxGlobalSlots);
    setProxyMode(defaults.proxyMode);
    setProxyHost(defaults.proxyHost);
    setProxyPort(defaults.proxyPort);
    setLanguage(defaults.language);
    setAutoLogin(defaults.autoLogin);
    setStayOnTop(defaults.stayOnTop);
    setDarkMode(defaults.darkMode);
    setMinimizeToTray(defaults.minimizeToTray);
    setFileConflictPolicy(defaults.fileConflictPolicy);
}

} // namespace fsnext
