#include "SystemTray.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QDebug>

namespace fsnext {

SystemTray::SystemTray(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

SystemTray::~SystemTray() = default;

bool SystemTray::setup()
{
    if (m_tray) return true; // idempotent

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qInfo() << "[SystemTray] system tray not available on this desktop";
        return false;
    }

    m_tray = new QSystemTrayIcon(this);
    // App icon is bundled at qrc:/icons/app.png (deployed copy).  Fall back to
    // a Qt-supplied generic icon so the tray is still visible during dev when
    // the resource isn't packed.
    QIcon icon(QStringLiteral(":/icons/app.png"));
    if (icon.isNull()) icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    m_tray->setIcon(icon);
    m_tray->setToolTip(QStringLiteral("Fshare Tool"));

    rebuildMenu();
    m_tray->setContextMenu(m_menu);

    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                // Single-click on Linux/macOS, double-click on Windows — both
                // map to "show me the window".  Context menu opens via right
                // click on every platform automatically.
                if (reason == QSystemTrayIcon::Trigger ||
                    reason == QSystemTrayIcon::DoubleClick) {
                    emit showWindowRequested();
                }
            });

    m_tray->show();
    qInfo() << "[SystemTray] visible";
    return true;
}

void SystemTray::rebuildMenu()
{
    if (!m_menu) m_menu = new QMenu();
    m_menu->clear();

    m_actShow = m_menu->addAction(tr("Hiện cửa sổ"));
    connect(m_actShow, &QAction::triggered, this, &SystemTray::showWindowRequested);

    m_actPause = m_menu->addAction(tr("Tạm dừng tất cả"));
    connect(m_actPause, &QAction::triggered, this, &SystemTray::pauseAllRequested);
    // Disabled when nothing is active — re-enabled by setActiveTransfersHint.
    m_actPause->setEnabled(m_activeDownloads + m_activeUploads > 0);

    m_menu->addSeparator();

    m_actQuit = m_menu->addAction(tr("Thoát"));
    connect(m_actQuit, &QAction::triggered, this, &SystemTray::quitRequested);
}

void SystemTray::showNotification(const QString &title, const QString &message)
{
    if (!m_tray) return;
    if (title.isEmpty() && message.isEmpty()) return;
    m_tray->showMessage(title, message, QSystemTrayIcon::Information, 4000);
}

void SystemTray::setActiveTransfersHint(int activeDownloads, int activeUploads)
{
    m_activeDownloads = activeDownloads;
    m_activeUploads   = activeUploads;
    if (m_actPause) m_actPause->setEnabled(activeDownloads + activeUploads > 0);
    if (m_tray) {
        const int total = activeDownloads + activeUploads;
        m_tray->setToolTip(total == 0
            ? QStringLiteral("Fshare Tool")
            : QStringLiteral("Fshare Tool — DL %1 · UL %2").arg(activeDownloads).arg(activeUploads));
    }
}

} // namespace fsnext
