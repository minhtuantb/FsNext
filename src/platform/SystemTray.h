#pragma once

#include <QObject>
#include <QString>
#include <QIcon>
#include <QTimer>

class QSystemTrayIcon;
class QMenu;
class QAction;
class QQmlApplicationEngine;

namespace fsnext {

// System-tray integration.  Owns a QSystemTrayIcon (Qt6::Widgets at link
// time — see CMakeLists.txt), exposes signals for QML / main.cpp to react.
//
// Tray operates in three visual states driven by setHudHint():
//   IDLE   — gray silhouette, no active transfers
//   ACTIVE — accent silhouette (Aurora orange), at least one transfer in-flight
//   ERROR  — red silhouette, recent task failed and nothing is running
//
// Icon swaps use runtime QPainter tint over resources/icons/app-*.png so we
// don't need three separate asset families. Only setIcon() when the computed
// state differs from the last applied one (paint is cheap but the QSystemTray
// icon cache thrashes on Windows if we call it every tick).
//
// Single-click on the tray (QSystemTrayIcon::Trigger) is deferred by 250 ms
// so a double-click within the window can preempt it. This is the same
// pattern Drive / Dropbox use: single → small popup, double → main window.
// At P0 there's no popup yet, so togglePopupRequested falls through to
// showWindowRequested via main.cpp wiring; P1 swaps it for the real popup.
class SystemTray : public QObject
{
    Q_OBJECT

public:
    enum class IconState { Idle, Active, Error };

    explicit SystemTray(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~SystemTray() override;

    // Idempotent: safe to call setup() multiple times.  Returns true when the
    // tray icon was constructed and is visible (i.e. the platform supports
    // a tray); false on platforms / desktops without one.
    bool setup();

    // Push a balloon notification through the tray icon.  No-op if the tray
    // didn't initialise (e.g. headless CI run).  `isError=true` switches the
    // bubble's icon family to QSystemTrayIcon::Critical so the OS theme can
    // colour the toast differently (matters on Windows 10+).
    //
    // `clickContextId` (P3) is remembered as the "last balloon shown" id;
    // when the user clicks the balloon (QSystemTrayIcon::messageClicked),
    // we emit balloonClicked(clickContextId) so main.cpp can focus that
    // task.  Pass empty when the balloon isn't tied to a specific task.
    void showNotification(const QString &title, const QString &message,
                          bool isError = false,
                          const QString &clickContextId = QString{});

    // Update icon colour + tooltip from aggregate transfer counters.  Called
    // from main.cpp on every TransferHudViewModel::countersChanged emit.
    //   active  — count of transfers currently sending/receiving bytes
    //   pending — queued / paused / waiting tasks
    //   failed  — tasks in Error state whose user hasn't acknowledged yet
    // Picks IconState by priority: Error > Active > Idle.  No-op when the
    // resulting state equals the last applied one.
    void setHudHint(int active, int pending, int failed);

    // Global screen-coordinate rect of the tray icon, if the platform
    // exposes it.  Used by main.cpp to anchor the Tray Popup window on
    // single-click.  Returns an empty QRect when the platform doesn't
    // know (most Linux DEs, some macOS bar configurations) — callers must
    // fall back to a default anchor (e.g. screen bottom-right).
    QRect trayGeometry() const;

signals:
    // Emitted on tray double-click (Windows) / right-click "Hiện cửa sổ"
    // menu / Linux single-click after the 250 ms guard expires without a
    // follow-up double-click event.  main.cpp brings the main window forward.
    void showWindowRequested();

    // Emitted on tray single-click AFTER the 250 ms double-click guard
    // expires without a preempting DoubleClick.  Routed to MiniHudWindow's
    // pinned show/hide toggle (togglePinned).
    void togglePopupRequested();

    // Emitted on tray menu → "Tạm dừng tất cả".  main.cpp dispatches to
    // TransferService::pauseAll().
    void pauseAllRequested();

    // Emitted on tray menu → "Thoát".  main.cpp calls QApplication::quit.
    void quitRequested();

    // Emitted on QSystemTrayIcon::messageClicked AFTER a balloon shown
    // with a non-empty clickContextId.  main.cpp routes this to the HUD
    // VM's focusTask(taskId) — same path as clicking a row in the popup.
    // Carries an empty string if no context id was attached.
    void balloonClicked(const QString &taskId);

    // Emitted on tray menu → "Hiện HUD mini".  main.cpp invokes a QML
    // method on the root that pops the floating mini window regardless
    // of where the main window currently sits.  P3 affordance for users
    // who want a peek without going through hide-to-tray.
    void showMiniRequested();

    // Emitted on tray menu → "Cài đặt thông báo".  main.cpp opens the
    // main window and navigates to the Settings page so the user can
    // toggle balloon / mini HUD prefs without hunting through nav.
    void openSettingsRequested();

private:
    void rebuildMenu();

    // Returns three QIcon families (one per IconState) built from the app
    // icon set by tinting silhouettes through QPainter.  Cached on first
    // call so subsequent setIcon() calls don't re-rasterize.
    void loadIconFamilies();

    // Apply the icon for `state` if it differs from m_currentState.  Cheap
    // O(1) branch + an OS-level setIcon when the cache misses.
    void applyIconState(IconState state);

    // Compute IconState from the cached (active, pending, failed) tuple.
    // Error > Active > Idle priority.
    IconState computeState() const;

    QQmlApplicationEngine *m_engine = nullptr;
    QSystemTrayIcon       *m_tray   = nullptr;
    QMenu                 *m_menu   = nullptr;
    QAction               *m_actShow      = nullptr;
    QAction               *m_actMini      = nullptr;
    QAction               *m_actPause     = nullptr;
    QAction               *m_actSettings  = nullptr;
    QAction               *m_actQuit      = nullptr;

    // Pre-rendered icon families, indexed by IconState ordinal (0..2).
    QIcon m_iconIdle, m_iconActive, m_iconError;
    IconState m_currentState = IconState::Idle;

    // Aggregate transfer counters from the HUD VM.  Cached so the right-click
    // menu's "Tạm dừng tất cả" can disable itself when there's nothing to
    // pause, without re-querying the VM on hover.
    int m_active = 0, m_pending = 0, m_failed = 0;

    // 250 ms guard: started on every QSystemTrayIcon::Trigger, fired by
    // QSystemTrayIcon::DoubleClick to cancel.  See togglePopupRequested.
    QTimer m_singleClickTimer;
    static constexpr int kDoubleClickGuardMs = 250;

    // Last shown balloon's task context.  Captured by showNotification and
    // emitted via balloonClicked when QSystemTrayIcon::messageClicked
    // fires.  Cleared after each click — only the MOST RECENT balloon is
    // routable (Windows toast UX matches: prior toasts in Action Center
    // don't route back to the app).
    QString m_lastBalloonTaskId;
};

} // namespace fsnext
