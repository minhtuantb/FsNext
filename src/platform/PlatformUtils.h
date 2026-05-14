#pragma once

#include <QString>
#include <cstdint>

namespace fsnext {

class PlatformUtils
{
public:
    PlatformUtils() = delete;

    // Returns the OS default download folder (e.g. ~/Downloads on all platforms).
    static QString defaultDownloadFolder();

    // Register or unregister the app in the OS autostart mechanism.
    // Windows: HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    // macOS: LaunchAgents plist (stub for now)
    // Returns true on success.
    static bool setAutoStart(bool enable);

    // Query whether autostart is currently enabled.
    static bool isAutoStartEnabled();

    // Returns available disk space in bytes for the volume containing path.
    static int64_t freeDiskSpace(const QString &path);

    // Read the system proxy settings and return a URL string such as
    // "http://host:port" or an empty string when no system proxy is configured.
    static QString proxyFromSystem();

    // Returns true when path is (or is directly equal to) a protected system
    // folder where downloads should be blocked:
    //   • Music / Pictures / Movies user libraries (all platforms)
    //   • Windows system root, Program Files, Program Files (x86)  (Windows only)
    static bool isSystemFolder(const QString &path);

    // Open the OS file manager and highlight/select the given file.
    // Windows: explorer.exe /select,"<path>"
    // macOS:   open -R "<path>"
    static void openInExplorer(const QString &filePath);

    // Open a file with the OS default application.
    static bool openFile(const QString &filePath);

    // Hand an HTTP(S) media URL to the user's default media player.
    //
    // Qt.openUrlExternally / QDesktopServices::openUrl on an https://… URL
    // dispatch through the "https" scheme handler — which is always the web
    // browser. That's wrong for streaming: the browser either downloads the
    // file or plays it inline with its limited built-in player.
    //
    // Instead we write a tiny one-line .m3u8 playlist pointing at the URL and
    // hand that local file to the OS. Windows' registered .m3u8 handler is a
    // real media player (VLC / MPC-HC / MPV / PotPlayer / Windows Media
    // Player with codec pack), which streams the URL directly.
    //
    // suggestedName is used to make the temp playlist file human-readable in
    // the player's window title ("Playing playlist – Bach Toan Tap 01.m3u8").
    // Pass an empty string to get a generic "fshare-stream.m3u8" name.
    //
    // Returns true when the playlist file was created and handed off
    // successfully; false on I/O or launch failure.
    static bool playStreamUrl(const QString &url,
                              const QString &suggestedName = QString{});
};

} // namespace fsnext
