#include "PlatformUtils.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QUrl>
#include <QDebug>

#ifdef _WIN32
#  include <windows.h>
#  include <winreg.h>
#  include <shellapi.h>
#endif

namespace fsnext {

namespace {
#ifdef _WIN32
    constexpr const wchar_t *kRunKey =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr const wchar_t *kAppName = L"FshareTool";
#endif
} // anonymous namespace

// ---------------------------------------------------------------------------

QString PlatformUtils::defaultDownloadFolder()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return path.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        : path;
}

// ---------------------------------------------------------------------------

bool PlatformUtils::setAutoStart(bool enable)
{
#ifdef _WIN32
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
        return false;

    if (enable) {
        const QString exePath = QCoreApplication::applicationFilePath();
        const std::wstring exePathW = exePath.toStdWString();
        result = RegSetValueExW(hKey, kAppName, 0, REG_SZ,
                                reinterpret_cast<const BYTE *>(exePathW.c_str()),
                                static_cast<DWORD>((exePathW.size() + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(hKey, kAppName);
    }

    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
#else
    // macOS / Linux stub — implement via LaunchAgents plist or XDG autostart in a later phase.
    Q_UNUSED(enable)
    return false;
#endif
}

// ---------------------------------------------------------------------------

bool PlatformUtils::isAutoStartEnabled()
{
#ifdef _WIN32
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
        return false;

    DWORD type = 0;
    DWORD dataSize = 0;
    result = RegQueryValueExW(hKey, kAppName, nullptr, &type, nullptr, &dataSize);
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
#else
    return false;
#endif
}

// ---------------------------------------------------------------------------

int64_t PlatformUtils::freeDiskSpace(const QString &path)
{
    QStorageInfo info(path);
    if (!info.isValid())
        return -1;
    return info.bytesFree();
}

// ---------------------------------------------------------------------------

QString PlatformUtils::proxyFromSystem()
{
#ifdef _WIN32
    // Read Internet Explorer / WinHTTP proxy from the registry.
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
        0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
        return {};

    // Check if proxy is enabled.
    DWORD proxyEnable = 0;
    DWORD dataSize = sizeof(proxyEnable);
    RegQueryValueExW(hKey, L"ProxyEnable", nullptr, nullptr,
                     reinterpret_cast<BYTE *>(&proxyEnable), &dataSize);
    if (!proxyEnable) {
        RegCloseKey(hKey);
        return {};
    }

    // Read the proxy server string (e.g. "host:8080" or "http=host:8080;https=host:8443").
    wchar_t proxyServer[512] = {};
    dataSize = sizeof(proxyServer);
    result = RegQueryValueExW(hKey, L"ProxyServer", nullptr, nullptr,
                              reinterpret_cast<BYTE *>(proxyServer), &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return {};

    const QString raw = QString::fromWCharArray(proxyServer);
    // Return the first entry. If it contains '=' (protocol-specific), strip the prefix.
    const int eq = raw.indexOf(QLatin1Char('='));
    const QString server = (eq >= 0) ? raw.mid(eq + 1).section(QLatin1Char(';'), 0, 0) : raw;
    return QStringLiteral("http://") + server;
#else
    // On macOS / Linux, Qt network stack reads system proxy automatically.
    return {};
#endif
}

// ---------------------------------------------------------------------------

QString PlatformUtils::resolveProxyUrl(int mode, const QString &manualHost, int manualPort)
{
    switch (mode) {
    case 1: // System proxy — read from OS (Windows registry / "" elsewhere)
        return proxyFromSystem();
    case 2: { // Manual proxy
        const QString host = manualHost.trimmed();
        if (host.isEmpty() || manualPort < 1 || manualPort > 65535)
            return {};
        return host + QLatin1Char(':') + QString::number(manualPort);
    }
    case 0:  // No proxy
    default:
        return {};
    }
}

// ---------------------------------------------------------------------------

bool PlatformUtils::isSystemFolder(const QString &path)
{
    if (path.isEmpty())
        return false;

    // Resolve to canonical path (resolves symlinks, cleans separators).
    const QString canonical = QDir(path).canonicalPath();
    if (canonical.isEmpty())
        return false;

    // Normalise to forward-slashes + lower-case for comparison.
    auto normalise = [](const QString &p) {
        return QDir::fromNativeSeparators(p).toLower();
    };

    const QString target = normalise(canonical);

    // ── User media libraries (cross-platform) ──────────────────────────────
    static const QStandardPaths::StandardLocation kProtectedLocations[] = {
        QStandardPaths::MusicLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::MoviesLocation,
    };
    for (auto loc : kProtectedLocations) {
        const QString sysPath = QStandardPaths::writableLocation(loc);
        if (sysPath.isEmpty()) continue;
        // Block exact match only — allow subdirectories inside these libraries.
        if (target == normalise(QDir(sysPath).canonicalPath()))
            return true;
    }

#ifdef _WIN32
    // ── Windows-specific system directories ────────────────────────────────
    auto startsWithEnvPath = [&](const char *envVar) -> bool {
        const QString envRaw = QString::fromLocal8Bit(qgetenv(envVar));
        if (envRaw.isEmpty()) return false;
        const QString envCanon = normalise(QDir(envRaw).canonicalPath());
        if (envCanon.isEmpty()) return false;
        return target.startsWith(envCanon + QLatin1Char('/')) || target == envCanon;
    };

    if (startsWithEnvPath("SystemRoot"))       return true;  // C:\Windows
    if (startsWithEnvPath("ProgramFiles"))     return true;  // C:\Program Files
    if (startsWithEnvPath("ProgramFiles(x86)")) return true; // C:\Program Files (x86)
    if (startsWithEnvPath("ProgramData"))      return true;  // C:\ProgramData
#endif

    return false;
}

// ---------------------------------------------------------------------------

void PlatformUtils::openInExplorer(const QString &filePath)
{
    if (filePath.isEmpty()) return;

#ifdef _WIN32
    // explorer.exe /select,"C:\path\to\file.ext"
    const QString nativePath = QDir::toNativeSeparators(filePath);
    const QString args = QStringLiteral("/select,\"%1\"").arg(nativePath);
    // Keep the wstring alive through the ShellExecute call — passing
    // .toStdWString().c_str() inline lets the temporary be destroyed at the
    // semicolon, which is technically a dangling pointer even though the
    // Win32 call has typically already copied internally by then.
    const std::wstring argsW = args.toStdWString();
    // ShellExecuteW returns an HINSTANCE that's actually an error code when
    // <= 32 (per MSDN).  Silently failing here leaves the user wondering
    // why "Show in Explorer" did nothing; log it so it shows up in the
    // file logger we installed at startup.
    const HINSTANCE rc = ShellExecuteW(nullptr, L"open", L"explorer.exe",
                                       argsW.c_str(), nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(rc) <= 32) {
        qWarning() << "[PlatformUtils] ShellExecuteW(explorer /select) failed for"
                   << nativePath << "rc=" << reinterpret_cast<INT_PTR>(rc);
    }
#elif defined(__APPLE__)
    QProcess::startDetached(QStringLiteral("open"), { QStringLiteral("-R"), filePath });
#else
    // Linux: xdg-open the containing folder
    const QString dir = QFileInfo(filePath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
#endif
}

// ---------------------------------------------------------------------------

bool PlatformUtils::openFile(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

// ---------------------------------------------------------------------------

bool PlatformUtils::playStreamUrl(const QString &url, const QString &suggestedName)
{
    if (url.isEmpty()) return false;

    // Build a safe filename base. Windows refuses most of < > : " / \ | ? *
    // and also a handful of device names (CON, NUL, …) — strip them all to a
    // single-underscore replacement and cap at a reasonable length. Falls
    // back to "fshare-stream" when the caller didn't provide a hint.
    QString base = suggestedName;
    {
        // Drop the extension if the caller passed in a full filename
        // (e.g. "Movie.mp4") — we're writing an .m3u8 playlist, not the
        // source file, so the extension would be misleading.
        const int dot = base.lastIndexOf(QLatin1Char('.'));
        if (dot > 0) base.truncate(dot);

        static const QRegularExpression kBadChars(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]"));
        base.replace(kBadChars, QStringLiteral("_"));
        base = base.trimmed();
        if (base.length() > 64) base.truncate(64);
    }
    if (base.isEmpty())
        base = QStringLiteral("fshare-stream");

    // Put the playlist in a per-app subfolder of the OS temp dir so we don't
    // pollute the root. Create-on-demand; ignore failure (the temp dir always
    // exists as a fallback).
    QDir tmpDir(QDir::tempPath());
    tmpDir.mkpath(QStringLiteral("FsNext-streams"));
    tmpDir.cd(QStringLiteral("FsNext-streams"));

    // Add a millisecond-resolution timestamp so repeated plays of the same
    // file don't try to rewrite a file that a player has locked for reading.
    const QString fileName = QStringLiteral("%1-%2.m3u8")
        .arg(base)
        .arg(QDateTime::currentMSecsSinceEpoch());
    const QString playlistPath = tmpDir.filePath(fileName);

    QFile file(playlistPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    // Minimal but complete M3U8 playlist. EXTINF duration of -1 means
    // "unknown"; the friendly title is what most players show in their
    // window bar while playing.
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "#EXTM3U\n";
    out << "#EXTINF:-1," << base << "\n";
    out << url << "\n";
    file.close();

    return QDesktopServices::openUrl(QUrl::fromLocalFile(playlistPath));
}

} // namespace fsnext
