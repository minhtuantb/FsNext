#include "SystemTray.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QVariant>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QDebug>

namespace fsnext {

SystemTray::SystemTray(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    m_singleClickTimer.setSingleShot(true);
    m_singleClickTimer.setInterval(kDoubleClickGuardMs);
    // Timer fires only when no DoubleClick preempted it inside the guard
    // window — that's the true "single click intent" path.
    connect(&m_singleClickTimer, &QTimer::timeout, this,
            &SystemTray::togglePopupRequested);
}

SystemTray::~SystemTray()
{
    // QSystemTrayIcon::setContextMenu() does NOT take ownership and the menu
    // was created without a parent, so free it explicitly (CRASH_AUDIT M21).
    // Deleting the menu also deletes its QActions (parented via addAction).
    delete m_menu;
    m_menu = nullptr;
}

// Paint one tray icon size from scratch: a rounded-square fill in `color`
// with a white "F" centred.  Used for all three IconState families so the
// shape is consistent regardless of state.
//
// Why paint instead of tinting the app PNG?  An earlier approach used
// CompositionMode_SourceIn to recolour the existing logo, but the source
// PNG's "F" glyph is itself a fully-opaque region of the alpha mask — the
// tint flooded the entire silhouette into one solid colour, erasing the F.
// Painting the rect + text in code keeps the F visible at every state.
static QPixmap paintTrayBadge(int size, const QColor &fill)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Rounded square — radius 22% of edge matches Windows 11 taskbar
    // icon corner radius closely enough to feel native at 16-32 px.
    const qreal r = size * 0.22;
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    // 0.5-inset so the AA edge doesn't get clipped by the pixmap bounds.
    p.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), r, r);

    // White "F" centred.  Font size = 60% of edge so the glyph occupies
    // the visual centre without crowding the rounded corners.  Bold to
    // stay legible at 16 px (the smallest size Windows hands out for the
    // tray notification area).
    QFont f(QStringLiteral("Segoe UI"), -1, QFont::Bold);
    f.setPixelSize(qMax(8, int(size * 0.6)));
    p.setFont(f);
    p.setPen(QColor(Qt::white));

    // Manual baseline math — Qt::AlignCenter on QPainter::drawText positions
    // the glyph's CAP HEIGHT centre, which sits visually low because the F
    // has no descender. Compensate by shifting up by ~6% of the size so the
    // glyph reads as centred.
    QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(QStringLiteral("F"));
    const int textH = fm.capHeight();
    const QPointF baseline((size - textW) / 2.0,
                           (size + textH) / 2.0 - size * 0.04);
    p.drawText(baseline, QStringLiteral("F"));
    p.end();
    return pm;
}

// Build a QIcon family at the standard tray sizes Windows / macOS / Linux
// might request.  All sizes painted from scratch — no PNG asset dependency,
// no risk of an asset-pipeline regression hiding the F again.
static QIcon paintTrayIconFamily(const QColor &fill)
{
    QIcon icon;
    for (int s : {16, 20, 24, 32, 48, 64, 128}) {
        icon.addPixmap(paintTrayBadge(s, fill), QIcon::Normal, QIcon::Off);
    }
    return icon;
}

// Read one color property off the FsAurora.Theme/AuroraColors QML singleton.
// This is the single-source-of-truth bridge: the tray's brand colours follow
// whatever the designer sets in AuroraColors.qml, with no parallel C++ copy to
// drift. `fallback` is returned when the engine/singleton isn't reachable
// (headless CI, early teardown) so the tray still renders a sane icon.
static QColor auroraColor(QQmlApplicationEngine *engine,
                          const char *propertyName, const QColor &fallback)
{
    if (!engine) return fallback;
    static const int typeId =
        qmlTypeId("FsAurora.Theme", 1, 0, "AuroraColors");
    if (typeId < 0) return fallback;
    auto *singleton = engine->singletonInstance<QObject *>(typeId);
    if (!singleton) return fallback;
    const QVariant v = singleton->property(propertyName);
    return v.canConvert<QColor>() ? v.value<QColor>() : fallback;
}

void SystemTray::loadIconFamilies()
{
    // Active/Error pull live from AuroraColors so a palette retune in QML
    // propagates here automatically; the hex args are defensive fallbacks only.
    // Idle = neutral gray, intentionally OUTSIDE the brand palette: it must
    // read on both light and dark OS taskbars, so it isn't an Aurora token.
    m_iconIdle   = paintTrayIconFamily(QColor("#9CA3AF"));
    m_iconActive = paintTrayIconFamily(auroraColor(m_engine, "accent", QColor("#FF5B2E")));
    m_iconError  = paintTrayIconFamily(auroraColor(m_engine, "danger", QColor("#D53030")));
}

bool SystemTray::setup()
{
    if (m_tray) return true; // idempotent

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qInfo() << "[SystemTray] system tray not available on this desktop";
        return false;
    }

    loadIconFamilies();

    m_tray = new QSystemTrayIcon(this);
    applyIconState(IconState::Idle);
    m_tray->setToolTip(QStringLiteral("Fshare"));

    rebuildMenu();
    m_tray->setContextMenu(m_menu);

    // Balloon-click routing — Windows fires messageClicked on the LAST
    // balloon shown when the user clicks the toast (Action Center too).
    // We forward the cached task context so main.cpp can dispatch
    // focusTask() with no extra wiring per balloon.
    connect(m_tray, &QSystemTrayIcon::messageClicked, this, [this]() {
        const QString id = m_lastBalloonTaskId;
        m_lastBalloonTaskId.clear();
        emit balloonClicked(id);
    });

    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                switch (reason) {
                case QSystemTrayIcon::Trigger:
                    // Single click (all platforms) — defer 250 ms so a
                    // follow-up DoubleClick can preempt.  On Linux this is
                    // the only click event the tray emits; on Windows the
                    // OS fires Trigger then DoubleClick within the system
                    // double-click time (typ. 500 ms, we use 250 to keep
                    // single-click latency reasonable).
                    m_singleClickTimer.start();
                    break;
                case QSystemTrayIcon::DoubleClick:
                    // Cancel the pending single-click intent so we don't
                    // emit togglePopupRequested *and* showWindowRequested.
                    m_singleClickTimer.stop();
                    emit showWindowRequested();
                    break;
                default:
                    // MiddleClick / Context / Unknown — ignore. Context
                    // already fires QMenu via setContextMenu().
                    break;
                }
            });

    m_tray->show();
    qInfo() << "[SystemTray] visible (3-state icon family loaded)";
    return true;
}

void SystemTray::rebuildMenu()
{
    if (!m_menu) m_menu = new QMenu();
    m_menu->clear();

    m_actShow = m_menu->addAction(tr("Hiện cửa sổ"));
    connect(m_actShow, &QAction::triggered, this, &SystemTray::showWindowRequested);

    // P3: explicit "show mini HUD" entry.  Useful when the main window
    // is open and the user wants the floating mini up as a stays-on-top
    // peek over another app, without going through hide-to-tray first.
    m_actMini = m_menu->addAction(tr("Hiện mini HUD"));
    connect(m_actMini, &QAction::triggered, this, &SystemTray::showMiniRequested);

    m_actPause = m_menu->addAction(tr("Tạm dừng tất cả"));
    connect(m_actPause, &QAction::triggered, this, &SystemTray::pauseAllRequested);
    // Disabled when nothing is active — re-enabled by setHudHint when a
    // transfer starts.
    m_actPause->setEnabled(m_active > 0);

    m_menu->addSeparator();

    // P3: quick jump to Settings for the HUD/balloon toggles.  Saves the
    // user from navigating Sidebar → Cài đặt → scroll-to-section.
    m_actSettings = m_menu->addAction(tr("Cài đặt thông báo…"));
    connect(m_actSettings, &QAction::triggered, this, &SystemTray::openSettingsRequested);

    m_menu->addSeparator();

    m_actQuit = m_menu->addAction(tr("Thoát"));
    connect(m_actQuit, &QAction::triggered, this, &SystemTray::quitRequested);
}

void SystemTray::showNotification(const QString &title, const QString &message,
                                  bool isError, const QString &clickContextId)
{
    if (!m_tray) return;
    if (title.isEmpty() && message.isEmpty()) return;
    // Remember the task id so messageClicked can route back.  Each new
    // balloon overwrites the previous id — only the most recent toast is
    // routable, matching Windows' own balloon UX (older toasts in Action
    // Center don't fire messageClicked).
    m_lastBalloonTaskId = clickContextId;
    m_tray->showMessage(title, message,
                        isError ? QSystemTrayIcon::Critical
                                : QSystemTrayIcon::Information,
                        4000);
}

SystemTray::IconState SystemTray::computeState() const
{
    // Error > Active > Idle. We treat "has active transfers" as masking
    // failures because the user can already see the in-flight work; a red
    // icon while bytes are moving is misleading.
    if (m_failed > 0 && m_active == 0) return IconState::Error;
    if (m_active > 0)                  return IconState::Active;
    return IconState::Idle;
}

void SystemTray::applyIconState(IconState state)
{
    if (!m_tray) return;
    if (state == m_currentState && !m_tray->icon().isNull()) return;
    m_currentState = state;
    switch (state) {
    case IconState::Idle:   m_tray->setIcon(m_iconIdle);   break;
    case IconState::Active: m_tray->setIcon(m_iconActive); break;
    case IconState::Error:  m_tray->setIcon(m_iconError);  break;
    }
}

QRect SystemTray::trayGeometry() const
{
    // QSystemTrayIcon::geometry() returns the icon's screen rect when the
    // platform supports it (Win10+, macOS, KDE).  Falls back to an empty
    // QRect on Wayland / GNOME indicator-applet style integrations where
    // the icon is hosted in a side process the app can't query.
    return m_tray ? m_tray->geometry() : QRect{};
}

void SystemTray::setHudHint(int active, int pending, int failed)
{
    m_active  = active;
    m_pending = pending;
    m_failed  = failed;

    applyIconState(computeState());

    if (m_actPause) m_actPause->setEnabled(active > 0);

    // Tooltip — two lines: brand on top (acts as a title), human-readable
    // status below.  Windows shows multi-line tray tooltips fine and this
    // reads far more polished than the old "Fshare — đang chuyển 1".
    //   idle   →  "Fshare\nSẵn sàng"
    //   active →  "Fshare\nĐang chuyển 3 mục · 1 đang chờ"
    //   error  →  "Fshare\n2 mục lỗi"
    QString status;
    if (active > 0 || pending > 0 || failed > 0) {
        QStringList parts;
        if (active > 0)  parts << tr("Đang chuyển %n mục", nullptr, active);
        if (pending > 0) parts << tr("%n mục đang chờ", nullptr, pending);
        if (failed > 0)  parts << tr("%n mục lỗi", nullptr, failed);
        status = parts.join(QStringLiteral("  ·  "));
    } else {
        status = tr("Sẵn sàng");
    }
    if (m_tray) m_tray->setToolTip(QStringLiteral("Fshare\n%1").arg(status));
}

} // namespace fsnext
