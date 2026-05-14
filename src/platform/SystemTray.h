#pragma once

#include <QObject>
#include <QString>

class QSystemTrayIcon;
class QMenu;
class QAction;
class QQmlApplicationEngine;

namespace fsnext {

// Minimal tray integration.  Owns a QSystemTrayIcon (requires Qt6::Widgets at
// link time — see CMakeLists.txt), exposes signals for QML to react to.
//
// The QML/ApplicationWindow side is responsible for actually hiding/showing
// the window in response to `showWindowRequested` / minimize-to-tray.  Tray is
// "fire-and-forget" from the C++ perspective — we tell the GUI what the user
// clicked and let it decide what that means in app terms.
class SystemTray : public QObject
{
    Q_OBJECT

public:
    explicit SystemTray(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~SystemTray() override;

    // Idempotent: safe to call setup() multiple times.  Returns true when the
    // tray icon was constructed and is visible (i.e. the platform supports
    // a tray); false on platforms / desktops without one.
    bool setup();

    // Push a balloon notification through the tray icon.  No-op if the tray
    // didn't initialise (e.g. headless CI run).
    void showNotification(const QString &title, const QString &message);

    // Update the tooltip / icon to reflect "have active transfers" state.
    // Called from AppContext on transferBudget changes.
    void setActiveTransfersHint(int activeDownloads, int activeUploads);

signals:
    // User chose "Hiện cửa sổ" or double-clicked the tray icon.
    void showWindowRequested();
    // User chose "Tạm dừng tất cả".
    void pauseAllRequested();
    // User chose "Thoát".
    void quitRequested();

private:
    void rebuildMenu();

    QQmlApplicationEngine *m_engine = nullptr;
    QSystemTrayIcon       *m_tray   = nullptr;
    QMenu                 *m_menu   = nullptr;
    QAction               *m_actShow  = nullptr;
    QAction               *m_actPause = nullptr;
    QAction               *m_actQuit  = nullptr;
    int m_activeDownloads = 0;
    int m_activeUploads   = 0;
};

} // namespace fsnext
