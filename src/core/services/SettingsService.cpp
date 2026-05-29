#include "SettingsService.h"
#include "core/repositories/SettingsRepository.h"

#include <QDir>
#include <QStandardPaths>

namespace fsnext {

SettingsService::SettingsService(SettingsRepository *repo, QObject *parent)
    : QObject(parent)
    , m_repo(repo)
{
    if (m_repo) m_settings = m_repo->load();
}

AppSettings SettingsService::loadSettings()
{
    if (m_repo) m_settings = m_repo->load();
    return m_settings;
}

void SettingsService::saveSettings(const AppSettings &settings)
{
    m_settings = settings;
    if (m_repo) m_repo->save(m_settings);
    emit settingsChanged();
}

// Helper: save only the specified field by key, not the whole AppSettings struct.
// Avoids cascading writes when only one setting changes.
template<typename T>
static void saveSingle(SettingsRepository *repo, const QString &key, const T &value);

// Download
int SettingsService::downloadThreads() const       { return m_settings.downloadThreads; }
void SettingsService::setDownloadThreads(int value)
{
    if (m_settings.downloadThreads == value) return;
    m_settings.downloadThreads = value;
    if (m_repo) m_repo->setInt(QStringLiteral("Download/threads"), value);
    emit settingsChanged();
}

int SettingsService::downloadSegments() const      { return m_settings.downloadSegments; }
void SettingsService::setDownloadSegments(int value)
{
    if (m_settings.downloadSegments == value) return;
    m_settings.downloadSegments = value;
    if (m_repo) m_repo->setInt(QStringLiteral("Download/segments"), value);
    emit settingsChanged();
}

bool SettingsService::autoDownload() const         { return m_settings.autoDownload; }
void SettingsService::setAutoDownload(bool value)
{
    if (m_settings.autoDownload == value) return;
    m_settings.autoDownload = value;
    if (m_repo) m_repo->setBool(QStringLiteral("Download/autoDownload"), value);
    emit settingsChanged();
}

QString SettingsService::downloadFolder() const    { return m_settings.downloadFolder; }
void SettingsService::setDownloadFolder(const QString &path)
{
    if (m_settings.downloadFolder == path) return;
    m_settings.downloadFolder = path;
    if (m_repo) m_repo->setString(QStringLiteral("Download/folder"), path);
    emit settingsChanged();
}

QString SettingsService::effectiveDownloadFolder() const
{
    if (!m_settings.downloadFolder.isEmpty())
        return m_settings.downloadFolder;

    const QString sys = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!sys.isEmpty())
        return sys;

    return QDir::homePath();
}

// Upload
int SettingsService::uploadThreads() const         { return m_settings.uploadThreads; }
void SettingsService::setUploadThreads(int value)
{
    if (m_settings.uploadThreads == value) return;
    m_settings.uploadThreads = value;
    if (m_repo) m_repo->setInt(QStringLiteral("Upload/threads"), value);
    emit settingsChanged();
}

QString SettingsService::uploadFolder() const      { return m_settings.uploadFolder; }
void SettingsService::setUploadFolder(const QString &path)
{
    if (m_settings.uploadFolder == path) return;
    m_settings.uploadFolder = path;
    if (m_repo) m_repo->setString(QStringLiteral("Upload/folder"), path);
    emit settingsChanged();
}

// Transfer budget
int SettingsService::maxGlobalSlots() const        { return m_settings.maxGlobalSlots; }
void SettingsService::setMaxGlobalSlots(int value)
{
    // 0 = disable global cap; 1..32 = active cap.  Higher values don't buy
    // anything because the per-class sum (8+4+2 by default) is 14.
    if (value < 0) value = 0;
    if (value > 32) value = 32;
    if (m_settings.maxGlobalSlots == value) return;
    m_settings.maxGlobalSlots = value;
    if (m_repo) m_repo->setInt(QStringLiteral("Transfer/maxGlobalSlots"), value);
    emit settingsChanged();
}

// Account
bool SettingsService::rememberMe() const           { return m_settings.rememberMe; }
void SettingsService::setRememberMe(bool value)
{
    if (m_settings.rememberMe == value) return;
    m_settings.rememberMe = value;
    if (m_repo) m_repo->setBool(QStringLiteral("Account/rememberMe"), value);
    emit settingsChanged();
}

QString SettingsService::savedEmail() const        { return m_settings.savedEmail; }
void SettingsService::setSavedEmail(const QString &email)
{
    if (m_settings.savedEmail == email) return;
    m_settings.savedEmail = email;
    if (m_repo) {
        m_repo->setString(QStringLiteral("Account/email"), email);
        m_repo->setString(QStringLiteral("Account/savedEmail"), email);
    }
    emit settingsChanged();
}

bool SettingsService::autoLogin() const            { return m_settings.autoLogin; }
void SettingsService::setAutoLogin(bool value)
{
    if (m_settings.autoLogin == value) return;
    m_settings.autoLogin = value;
    if (m_repo) m_repo->setBool(QStringLiteral("General/autoLogin"), value);
    emit settingsChanged();
}

// UI
bool SettingsService::darkMode() const             { return m_settings.darkMode; }
void SettingsService::setDarkMode(bool value)
{
    if (m_settings.darkMode == value) return;
    m_settings.darkMode = value;
    if (m_repo) m_repo->setBool(QStringLiteral("UI/darkMode"), value);
    emit settingsChanged();
}

bool SettingsService::sidebarCollapsed() const     { return m_settings.sidebarCollapsed; }
void SettingsService::setSidebarCollapsed(bool value)
{
    if (m_settings.sidebarCollapsed == value) return;
    m_settings.sidebarCollapsed = value;
    if (m_repo) m_repo->setBool(QStringLiteral("UI/sidebarCollapsed"), value);
    emit settingsChanged();
}

int  SettingsService::sidebarHudVariant() const    { return m_settings.sidebarHudVariant; }
void SettingsService::setSidebarHudVariant(int value)
{
    const int clamped = qBound(0, value, 3);
    if (m_settings.sidebarHudVariant == clamped) return;
    m_settings.sidebarHudVariant = clamped;
    if (m_repo) m_repo->setInt(QStringLiteral("UI/sidebarHudVariant"), clamped);
    emit settingsChanged();
}

QString SettingsService::language() const          { return m_settings.language; }
void SettingsService::setLanguage(const QString &code)
{
    if (m_settings.language == code) return;
    m_settings.language = code;
    if (m_repo) m_repo->setString(QStringLiteral("General/language"), code);
    emit settingsChanged();
}

// General
bool SettingsService::stayOnTop() const            { return m_settings.stayOnTop; }
void SettingsService::setStayOnTop(bool value)
{
    if (m_settings.stayOnTop == value) return;
    m_settings.stayOnTop = value;
    if (m_repo) m_repo->setBool(QStringLiteral("General/stayOnTop"), value);
    emit settingsChanged();
}

bool SettingsService::autoStart() const            { return m_settings.autoStart; }
void SettingsService::setAutoStart(bool value)
{
    if (m_settings.autoStart == value) return;
    m_settings.autoStart = value;
    if (m_repo) m_repo->setBool(QStringLiteral("General/autoStart"), value);
    emit settingsChanged();
}

bool SettingsService::minimizeToTray() const       { return m_settings.minimizeToTray; }
void SettingsService::setMinimizeToTray(bool value)
{
    if (m_settings.minimizeToTray == value) return;
    m_settings.minimizeToTray = value;
    if (m_repo) m_repo->setBool(QStringLiteral("General/minimizeToTray"), value);
    emit settingsChanged();
}

bool SettingsService::confirmOnClose() const       { return m_settings.confirmOnClose; }
void SettingsService::setConfirmOnClose(bool value)
{
    if (m_settings.confirmOnClose == value) return;
    m_settings.confirmOnClose = value;
    if (m_repo) m_repo->setBool(QStringLiteral("General/confirmOnClose"), value);
    emit settingsChanged();
}

bool SettingsService::notifyOnTransferDone() const { return m_settings.notifyOnTransferDone; }
void SettingsService::setNotifyOnTransferDone(bool value)
{
    if (m_settings.notifyOnTransferDone == value) return;
    m_settings.notifyOnTransferDone = value;
    if (m_repo) m_repo->setBool(QStringLiteral("Hud/notifyOnTransferDone"), value);
    emit settingsChanged();
}

bool SettingsService::showOnHideToTray() const { return m_settings.showOnHideToTray; }
void SettingsService::setShowOnHideToTray(bool value)
{
    if (m_settings.showOnHideToTray == value) return;
    m_settings.showOnHideToTray = value;
    if (m_repo) m_repo->setBool(QStringLiteral("Hud/showOnHideToTray"), value);
    emit settingsChanged();
}

bool SettingsService::showTaskbarProgress() const { return m_settings.showTaskbarProgress; }
void SettingsService::setShowTaskbarProgress(bool value)
{
    if (m_settings.showTaskbarProgress == value) return;
    m_settings.showTaskbarProgress = value;
    if (m_repo) m_repo->setBool(QStringLiteral("Hud/showTaskbarProgress"), value);
    emit settingsChanged();
}

int     SettingsService::miniWindowX()      const { return m_settings.miniWindowX; }
int     SettingsService::miniWindowY()      const { return m_settings.miniWindowY; }
QString SettingsService::miniWindowScreen() const { return m_settings.miniWindowScreen; }

void SettingsService::setMiniWindowPosition(int x, int y, const QString &screen)
{
    // Coalesce: if every component matches the cached state, skip the
    // disk write.  The Mini window calls this every drag-release, so
    // the no-op check guards against thrashing QSettings on idle window
    // activity (focus changes can also trip onXChanged in QML).
    if (m_settings.miniWindowX == x &&
        m_settings.miniWindowY == y &&
        m_settings.miniWindowScreen == screen) return;
    m_settings.miniWindowX = x;
    m_settings.miniWindowY = y;
    m_settings.miniWindowScreen = screen;
    if (m_repo) {
        m_repo->setInt   (QStringLiteral("Hud/miniWindowX"),      x);
        m_repo->setInt   (QStringLiteral("Hud/miniWindowY"),      y);
        m_repo->setString(QStringLiteral("Hud/miniWindowScreen"), screen);
    }
    emit settingsChanged();
}

int SettingsService::fileConflictPolicy() const    { return m_settings.fileConflictPolicy; }
void SettingsService::setFileConflictPolicy(int value)
{
    // Clamp to known range so a corrupt QSettings file can't put us into an
    // unhandled fall-through path inside FileNameSanitizer::resolveConflict.
    const int clamped = qBound(0, value, 3);
    if (m_settings.fileConflictPolicy == clamped) return;
    m_settings.fileConflictPolicy = clamped;
    if (m_repo) m_repo->setInt(QStringLiteral("Download/conflictPolicy"), clamped);
    emit settingsChanged();
}

// Proxy
int SettingsService::proxyMode() const             { return m_settings.proxyMode; }
void SettingsService::setProxyMode(int mode)
{
    if (m_settings.proxyMode == mode) return;
    m_settings.proxyMode = mode;
    if (m_repo) m_repo->setInt(QStringLiteral("Connection/proxyMode"), mode);
    emit settingsChanged();
}

QString SettingsService::proxyHost() const         { return m_settings.proxyHost; }
void SettingsService::setProxyHost(const QString &host)
{
    if (m_settings.proxyHost == host) return;
    m_settings.proxyHost = host;
    if (m_repo) m_repo->setString(QStringLiteral("Connection/proxyHost"), host);
    emit settingsChanged();
}

int SettingsService::proxyPort() const             { return m_settings.proxyPort; }
void SettingsService::setProxyPort(int port)
{
    if (m_settings.proxyPort == port) return;
    m_settings.proxyPort = port;
    if (m_repo) m_repo->setInt(QStringLiteral("Connection/proxyPort"), port);
    emit settingsChanged();
}

} // namespace fsnext
