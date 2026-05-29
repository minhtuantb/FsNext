#include "SettingsRepository.h"

namespace fsnext {

SettingsRepository::SettingsRepository()
    : m_settings(QStringLiteral("FPT"), QStringLiteral("FshareNext"))
{
}

// ---------------------------------------------------------------------------
// High-level load / save
// ---------------------------------------------------------------------------

AppSettings SettingsRepository::load()
{
    AppSettings s;

    // General
    s.language      = getString(QStringLiteral("General/language"),   QStringLiteral("en"));
    s.autoLogin     = getBool (QStringLiteral("General/autoLogin"),   false);
    s.stayOnTop     = getBool (QStringLiteral("General/stayOnTop"),   false);
    s.autoStart     = getBool (QStringLiteral("General/autoStart"),   false);
    // Default minimizeToTray is true on Windows (matches user expectation for
    // download tools), false on macOS/Linux (different dock semantics).  The
    // struct ctor in AppSettings.h already encodes the platform default; here
    // we just persist whatever the user last picked.
    s.minimizeToTray = getBool(QStringLiteral("General/minimizeToTray"), s.minimizeToTray);
    s.confirmOnClose = getBool(QStringLiteral("General/confirmOnClose"), s.confirmOnClose);

    // Connection
    s.proxyMode     = getInt  (QStringLiteral("Connection/proxyMode"), 0);
    s.proxyHost     = getString(QStringLiteral("Connection/proxyHost"), {});
    s.proxyPort     = getInt  (QStringLiteral("Connection/proxyPort"), 0);

    // Download
    s.downloadThreads     = getInt (QStringLiteral("Download/threads"),     2);
    s.downloadSegments    = getInt (QStringLiteral("Download/segments"),    4);
    s.autoDownload        = getBool(QStringLiteral("Download/autoDownload"), false);
    s.downloadFolder      = getString(QStringLiteral("Download/folder"),    {});
    // 0 = rename "(n)", 1 = overwrite, 2 = skip, 3 = ask each time.  See ADR 003 D7.
    s.fileConflictPolicy  = getInt (QStringLiteral("Download/conflictPolicy"), 0);

    // Upload
    s.uploadThreads = getInt  (QStringLiteral("Upload/threads"), 2);
    s.uploadFolder  = getString(QStringLiteral("Upload/folder"), {});

    // Transfer budget — 10 matches BudgetManager::Config default.
    s.maxGlobalSlots = getInt (QStringLiteral("Transfer/maxGlobalSlots"), 10);

    // Account
    // Note: AuthService writes/reads email under "Account/email" directly via
    // getString()/setString() for legacy compatibility — check that key first
    s.savedEmail = getString(QStringLiteral("Account/email"),
                              getString(QStringLiteral("Account/savedEmail"), {}));
    s.savedToken = getString(QStringLiteral("Account/token"), {});
    s.rememberMe = getBool  (QStringLiteral("Account/rememberMe"), false);

    // UI
    s.darkMode         = getBool(QStringLiteral("UI/darkMode"),         false);
    s.sidebarCollapsed = getBool(QStringLiteral("UI/sidebarCollapsed"), false);
    s.sidebarHudVariant = qBound(0, getInt(QStringLiteral("UI/sidebarHudVariant"), 0), 3);

    // HUD
    s.notifyOnTransferDone = getBool  (QStringLiteral("Hud/notifyOnTransferDone"), true);
    s.showOnHideToTray     = getBool  (QStringLiteral("Hud/showOnHideToTray"),     true);
    s.showTaskbarProgress  = getBool  (QStringLiteral("Hud/showTaskbarProgress"),  true);
    s.miniWindowX          = getInt   (QStringLiteral("Hud/miniWindowX"),          -1);
    s.miniWindowY          = getInt   (QStringLiteral("Hud/miniWindowY"),          -1);
    s.miniWindowScreen     = getString(QStringLiteral("Hud/miniWindowScreen"),     {});

    return s;
}

void SettingsRepository::save(const AppSettings &s)
{
    // General
    setString(QStringLiteral("General/language"),       s.language);
    setBool  (QStringLiteral("General/autoLogin"),      s.autoLogin);
    setBool  (QStringLiteral("General/stayOnTop"),      s.stayOnTop);
    setBool  (QStringLiteral("General/autoStart"),      s.autoStart);
    setBool  (QStringLiteral("General/minimizeToTray"), s.minimizeToTray);
    setBool  (QStringLiteral("General/confirmOnClose"), s.confirmOnClose);

    // Connection
    setInt   (QStringLiteral("Connection/proxyMode"), s.proxyMode);
    setString(QStringLiteral("Connection/proxyHost"), s.proxyHost);
    setInt   (QStringLiteral("Connection/proxyPort"), s.proxyPort);

    // Download
    setInt (QStringLiteral("Download/threads"),         s.downloadThreads);
    setInt (QStringLiteral("Download/segments"),        s.downloadSegments);
    setBool(QStringLiteral("Download/autoDownload"),    s.autoDownload);
    setString(QStringLiteral("Download/folder"),        s.downloadFolder);
    setInt (QStringLiteral("Download/conflictPolicy"),  s.fileConflictPolicy);

    // Upload
    setInt   (QStringLiteral("Upload/threads"), s.uploadThreads);
    setString(QStringLiteral("Upload/folder"),  s.uploadFolder);

    // Transfer budget
    setInt   (QStringLiteral("Transfer/maxGlobalSlots"), s.maxGlobalSlots);

    // Account — write to both keys for backwards compat with direct AuthService reads
    setString(QStringLiteral("Account/email"),      s.savedEmail);
    setString(QStringLiteral("Account/savedEmail"), s.savedEmail);
    setString(QStringLiteral("Account/token"),      s.savedToken);
    setBool  (QStringLiteral("Account/rememberMe"), s.rememberMe);

    // UI
    setBool(QStringLiteral("UI/darkMode"),         s.darkMode);
    setBool(QStringLiteral("UI/sidebarCollapsed"), s.sidebarCollapsed);
    setInt (QStringLiteral("UI/sidebarHudVariant"), s.sidebarHudVariant);

    // HUD
    setBool  (QStringLiteral("Hud/notifyOnTransferDone"), s.notifyOnTransferDone);
    setBool  (QStringLiteral("Hud/showOnHideToTray"),     s.showOnHideToTray);
    setBool  (QStringLiteral("Hud/showTaskbarProgress"),  s.showTaskbarProgress);
    setInt   (QStringLiteral("Hud/miniWindowX"),          s.miniWindowX);
    setInt   (QStringLiteral("Hud/miniWindowY"),          s.miniWindowY);
    setString(QStringLiteral("Hud/miniWindowScreen"),     s.miniWindowScreen);

    m_settings.sync();
}

// ---------------------------------------------------------------------------
// Low-level primitives
// ---------------------------------------------------------------------------

QString SettingsRepository::getString(const QString &key, const QString &defaultValue) const
{
    return m_settings.value(key, defaultValue).toString();
}

void SettingsRepository::setString(const QString &key, const QString &value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

int SettingsRepository::getInt(const QString &key, int defaultValue) const
{
    return m_settings.value(key, defaultValue).toInt();
}

void SettingsRepository::setInt(const QString &key, int value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

bool SettingsRepository::getBool(const QString &key, bool defaultValue) const
{
    return m_settings.value(key, defaultValue).toBool();
}

void SettingsRepository::setBool(const QString &key, bool value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

} // namespace fsnext
