#include "app/Application.h"
#include "app/AppContext.h"
#include "core/services/AuthService.h"
#include "core/services/SettingsService.h"
#include "core/services/TransferService.h"
#include "platform/SingleInstance.h"
#include "platform/SystemTray.h"
#include "platform/TaskbarProgress.h"
#include "viewmodels/LanguageViewModel.h"
#include "viewmodels/TransferHudViewModel.h"

#include <QApplication>          // QSystemTrayIcon needs QApplication, not QGuiApplication
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QIcon>
#include <QSize>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QThreadPool>
#include <cstdlib>
#include <exception>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

static QFile *g_logFile = nullptr;
static QMutex g_logMutex;
// Rotate when the active log exceeds this on startup. Keep the knob small —
// the log is opened in append mode, so 5 MiB is roughly a week of normal
// usage (several sessions per day @ ~kB-per-session). More than that and
// tail/grep starts hurting during incident triage.
static constexpr qint64 kLogRotateBytes = 5 * 1024 * 1024;
static constexpr int    kLogBackupCount = 3;

static void fileLogger(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    QMutexLocker locker(&g_logMutex);
    const char *typeStr = "INFO";
    switch (type) {
        case QtDebugMsg:    typeStr = "DEBUG"; break;
        case QtInfoMsg:     typeStr = "INFO";  break;
        case QtWarningMsg:  typeStr = "WARN";  break;
        case QtCriticalMsg: typeStr = "ERROR"; break;
        case QtFatalMsg:    typeStr = "FATAL"; break;
    }
    // Include file:line for WARN/ERROR/FATAL so crashes and non-fatal errors
    // leave a trail we can grep. DEBUG/INFO stay terse to keep volume down.
    QString location;
    if (type != QtDebugMsg && type != QtInfoMsg && ctx.file) {
        location = QStringLiteral(" (%1:%2)").arg(QString::fromUtf8(ctx.file)).arg(ctx.line);
    }
    QString line = QStringLiteral("[%1] [%2] %3%4\n")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")))
        .arg(typeStr).arg(msg).arg(location);
    if (g_logFile != nullptr && g_logFile->isOpen()) {
        g_logFile->write(line.toUtf8());
        g_logFile->flush();
    }
    fprintf(stderr, "%s", line.toLocal8Bit().constData());
    fflush(stderr);
}

// Shift fsnext.log → fsnext.log.1 → fsnext.log.2 → … dropping the oldest.
// Called once at startup before we open the active log.
static void rotateLogsIfNeeded(const QString &logPath)
{
    QFileInfo fi(logPath);
    if (!fi.exists() || fi.size() < kLogRotateBytes) return;

    // Drop the oldest backup so the rename below has somewhere to go.
    const QString oldest = logPath + QStringLiteral(".%1").arg(kLogBackupCount);
    if (QFile::exists(oldest)) QFile::remove(oldest);

    // Walk N-1..1 and shift each backup up by one.
    for (int i = kLogBackupCount - 1; i >= 1; --i) {
        const QString src = logPath + QStringLiteral(".%1").arg(i);
        const QString dst = logPath + QStringLiteral(".%1").arg(i + 1);
        if (QFile::exists(src)) QFile::rename(src, dst);
    }

    // Finally rotate the active log itself.
    QFile::rename(logPath, logPath + QStringLiteral(".1"));
}

static void setupFileLogger()
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    QString logPath = logDir + QStringLiteral("/fsnext.log");
    rotateLogsIfNeeded(logPath);
    g_logFile = new QFile(logPath);
    // Append so we preserve a bit of history within a single "chapter"
    // (same rotation bucket) across quick restarts.
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(fileLogger);
    }
}

// ── Global exception handler ──────────────────────────────
// Without these, an uncaught C++ exception or Windows SEH exception kills the
// process silently — the QFile-based logger never sees the crash. Hook both
// paths so we at least write a "FATAL ..." line with where-we-died info and
// flush the log before the kernel tears us down.
static void terminateHandler()
{
    QString what = QStringLiteral("unknown");
    if (auto ex = std::current_exception()) {
        try { std::rethrow_exception(ex); }
        catch (const std::exception &e) { what = QString::fromUtf8(e.what()); }
        catch (...) {}
    }
    // Route through fileLogger directly — qFatal would re-enter std::terminate.
    QMessageLogContext ctx;
    fileLogger(QtFatalMsg, ctx,
               QStringLiteral("std::terminate invoked, unhandled exception: %1").arg(what));
    {
        QMutexLocker locker(&g_logMutex);
        if (g_logFile && g_logFile->isOpen()) {
            g_logFile->flush();
            g_logFile->close();
        }
    }
    std::abort();
}

#ifdef Q_OS_WIN
static LONG WINAPI sehFilter(EXCEPTION_POINTERS *info)
{
    const DWORD code = info && info->ExceptionRecord
                       ? info->ExceptionRecord->ExceptionCode : 0;
    const void *addr = info && info->ExceptionRecord
                       ? info->ExceptionRecord->ExceptionAddress : nullptr;
    QMessageLogContext ctx;
    fileLogger(QtFatalMsg, ctx,
               QStringLiteral("Windows SEH exception 0x%1 at %2")
                   .arg(static_cast<quint32>(code), 8, 16, QLatin1Char('0'))
                   .arg(reinterpret_cast<quintptr>(addr), 0, 16));
    {
        QMutexLocker locker(&g_logMutex);
        if (g_logFile && g_logFile->isOpen()) {
            g_logFile->flush();
            g_logFile->close();
        }
    }
    // Let the default handler run — typically triggers WER so the user gets
    // the standard "has stopped working" dialog rather than a silent exit.
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void installCrashHandlers()
{
    std::set_terminate(terminateHandler);
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(sehFilter);
#endif
}

int main(int argc, char *argv[])
{
    // Initialize CURL before anything else
    fsnext::Application::initCurl();

    QApplication app(argc, argv);
    // NB: setQuitOnLastWindowClosed(false) is set UNCONDITIONALLY below
    // (post-engine). The earlier gated-on-tray placement caused the app to
    // silently quit when transfers finished + mini auto-hid on a machine
    // where tray.setup() raced and returned false. User has explicit quit
    // paths via close-confirm dialog + tray menu.
    app.setOrganizationName(QStringLiteral("FPT"));
    app.setOrganizationDomain(QStringLiteral("fshare.vn"));   // duplicated below for re-locking
    app.setApplicationName(QStringLiteral("FsNext"));
    // App-wide default icon: every QWindow (incl. the QML ApplicationWindow)
    // inherits this for taskbar / Alt+Tab / window header.  QIcon's resource
    // loader picks the highest-resolution match from qrc:/icons/ — we ship
    // app-16.png … app-256.png so HiDPI displays get a crisp render.
    {
        QIcon appIcon;
        for (int s : {16, 24, 32, 48, 64, 128, 256}) {
            appIcon.addFile(QStringLiteral(":/icons/app-%1.png").arg(s),
                            QSize(s, s));
        }
        // Fallback to the single-size app.png for forward-compat with builds
        // that don't have the per-size set generated yet.
        if (appIcon.isNull()) appIcon = QIcon(QStringLiteral(":/icons/app.png"));
        QApplication::setWindowIcon(appIcon);
    }

    // ── Single-instance guard ──────────────────────────────────────────────
    // QLocalServer name MUST match what nativehost / a second FsNext.exe
    // instance use to forward URLs.  Keep "fshare.singleinstance" in sync with
    // src/platform/nativehost/nativehost_main.cpp.
    fsnext::SingleInstance singleInstance;
    if (!singleInstance.tryLock(QStringLiteral("fshare.singleinstance"))) {
        // Another FsNext is already running.  Forward our argv (URLs / paths)
        // to it and exit so the user just sees the existing window pop up.
        // When invoked with no URL args (the common "user double-clicked the
        // exe again" case) we still need to wake the primary up — otherwise
        // the existing window stays hidden in the tray and the launch looks
        // like a silent no-op.  Send a sentinel "__show__" message that the
        // primary maps to show()/raise()/requestActivate() without trying to
        // open the add-download dialog with empty input.
        if (argc <= 1) {
            singleInstance.sendMessage(QStringLiteral("__show__"));
        } else {
            for (int i = 1; i < argc; ++i) {
                singleInstance.sendMessage(QString::fromLocal8Bit(argv[i]));
            }
        }
        fsnext::Application::cleanupCurl();
        return 0;
    }
    app.setApplicationVersion(fsnext::Application::appVersion());

    // Setup file logger after QApplication is created
    setupFileLogger();
    installCrashHandlers();
    qInfo() << "=== FsNext startup ===" << QDateTime::currentDateTime().toString(Qt::ISODate);

    // Set QML style
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Initialize SSL
    fsnext::Application::initSsl();

    // ── Application-context scope ────────────────────────────────────────────
    // Everything below — AppContext (and the services it owns: HttpClient,
    // FshareApi, TransferOrchestrator, etc.), the QML engine, the tray icon,
    // and all signal wiring that captures them — lives inside this block so
    // it destructs BEFORE cleanupCurl() runs at end-of-main.  Without the
    // scope, HttpClient::~HttpClient() (which calls curl_share_cleanup) ran
    // AFTER curl_global_cleanup, which libcurl explicitly documents as
    // undefined behaviour.
    int result = 0;
    {
    // Create dependency injection container
    fsnext::AppContext context;
    context.init();

    // Create QML engine
    QQmlApplicationEngine engine;
    // Add the QML resource root so module imports (Fshare.Theme, Fshare.Components, Fshare.Pages)
    // can be resolved by QML engine via qmldir lookup.
    engine.addImportPath(QStringLiteral("qrc:/qml"));

    // Register all viewmodels as QML context properties
    context.registerQml(&engine);

    // Wire language switching: when the user changes language at runtime,
    // QQmlApplicationEngine::retranslate() re-evaluates every qsTr() binding in QML.
    QObject::connect(
        context.languageViewModel(), &fsnext::LanguageViewModel::translationReloadNeeded,
        &engine, &QQmlApplicationEngine::retranslate
    );

    // Expose dev build flag to QML so dev-only UI (Showcase nav) can be conditional
#ifdef FSNEXT_DEV_BUILD
    engine.rootContext()->setContextProperty(QStringLiteral("isDevBuild"), true);
#else
    engine.rootContext()->setContextProperty(QStringLiteral("isDevBuild"), false);
#endif

    // Push the OS "Reduce motion" preference down to QML.  Main.qml mirrors it
    // into AuroraTheme.reduceMotion at Component.onCompleted, after which every
    // Behavior gated on AuroraTheme.reduceMotion respects the OS setting.
    engine.rootContext()->setContextProperty(QStringLiteral("osPrefersReducedMotion"),
                                             fsnext::Application::osPrefersReducedMotion());

    // Load main QML
    const QUrl mainUrl(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() {
            qCritical() << "[FsNext] Failed to load Main.qml";
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.load(mainUrl);

    if (engine.rootObjects().isEmpty()) {
        // Don't cleanupCurl() here — we're inside the application-context
        // scope and AppContext (owning HttpClient) hasn't destructed yet.
        // Set result and let the scope exit destruct services FIRST, then
        // the post-scope code runs curl cleanup in the correct order.
        qCritical() << "[FsNext] No root objects created";
        result = -1;
    } else {

    qDebug() << "[FsNext] Application started, version:" << fsnext::Application::appVersion();

    // Try auto-login with saved credentials
    QTimer::singleShot(100, context.authService(), &fsnext::AuthService::autoLogin);

    // Disable Qt's "quit when the last window closes" behaviour
    // UNCONDITIONALLY. Reasoning: the user has explicit quit paths in every
    // configuration — the close-confirm dialog ("Thoát hẳn"), the tray menu
    // "Thoát", and the Main.qml onClosing branch when no minimize-to-tray
    // is requested. Leaving Qt's implicit quit ON meant that as soon as
    // BOTH the main window (hidden-to-tray) and the mini HUD (auto-hidden
    // 30 s after transfers finish) were no longer visible, Qt would tear
    // down the process — the user perceived this as "the app quits when
    // upload finishes." Even on edge platforms with no system tray, the X
    // button still hits the confirm dialog whose "Thoát hẳn" calls
    // Qt.quit() explicitly, so we never leak an invisible process.
    QApplication::setQuitOnLastWindowClosed(false);

    // System tray — owns its own QSystemTrayIcon.  Wired here (not in
    // AppContext::init) because tray needs a QApplication and the QML root
    // window to already exist.
    fsnext::SystemTray tray(&engine);
    if (tray.setup()) {

        // Shared "bring main window forward" closure — reused by DoubleClick
        // (immediate) and Trigger (after the 250 ms guard; P0 fallback path
        // until the popup window ships in P1).
        auto raiseMainWindow = [&engine]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            auto *w = qobject_cast<QQuickWindow *>(roots.first());
            if (!w) return;
            w->show();
            w->raise();
            w->requestActivate();
        };

        // Tray double-click → main window.
        QObject::connect(&tray, &fsnext::SystemTray::showWindowRequested,
                         &app, raiseMainWindow);

        // Tray single-click → toggle the persistent floating HUD widget
        // (MiniHudWindow in pinned mode). The widget stays over other apps
        // until toggled off — see Main.qml toggleHudWidget(). trayRect is
        // passed for signature compatibility but unused (the widget uses its
        // own saved/default top-right position, not a tray anchor).
        QObject::connect(&tray, &fsnext::SystemTray::togglePopupRequested,
                         &app, [&engine, &tray]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            const QRect rect = tray.trayGeometry();
            QMetaObject::invokeMethod(roots.first(), "toggleHudWidget",
                                      Q_ARG(QVariant, QVariant::fromValue(rect)));
        });

        QObject::connect(&tray, &fsnext::SystemTray::pauseAllRequested,
                         &app, [&context]() {
            // TransferService::pauseAll covers both download and upload queues
            // (it iterates m_tasks regardless of TransferType).
            if (auto *svc = context.transferService()) svc->pauseAll();
        });

        // P3: tray menu → "Hiện mini HUD".  Invoke a QML method on root
        // that pops the floating mini regardless of whether the main
        // window is currently visible.
        QObject::connect(&tray, &fsnext::SystemTray::showMiniRequested,
                         &app, [&engine]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            QMetaObject::invokeMethod(roots.first(), "showMiniHud");
        });

        // P3: tray menu → "Cài đặt thông báo".  Open main + navigate to
        // Settings page.  Pages.settings == 8 (see Pages.qml) — pass via
        // setProperty so we don't depend on a QML enum import here.
        QObject::connect(&tray, &fsnext::SystemTray::openSettingsRequested,
                         &app, [&engine]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            auto *w = qobject_cast<QQuickWindow*>(roots.first());
            if (!w) return;
            w->show(); w->raise(); w->requestActivate();
            QMetaObject::invokeMethod(roots.first(), "navigateToSettings");
        });

        QObject::connect(&tray, &fsnext::SystemTray::quitRequested,
                         &app, &QApplication::quit);

        // ── HUD ↔ tray wiring ────────────────────────────────────────────
        // Tray icon colour + tooltip follow HUD VM counters.  countersChanged
        // is throttled (250 ms debounce inside the VM) so this connection
        // can't fire faster than the OS-level setIcon() can paint.
        if (auto *hudVM = context.hudViewModel()) {
            QObject::connect(hudVM, &fsnext::TransferHudViewModel::countersChanged,
                             &app, [&tray, hudVM]() {
                tray.setHudHint(hudVM->activeTotal(),
                                hudVM->pendingTotal(),
                                hudVM->failedTotal());
            });
            // Initial paint — without this the tray stays Idle until the
            // first counter change.  Matters when a resumed in-flight task
            // already shifted the VM to Active during AppContext::init.
            tray.setHudHint(hudVM->activeTotal(),
                            hudVM->pendingTotal(),
                            hudVM->failedTotal());

            // Balloon on terminal events — gated by SettingsService so
            // users who find it noisy can turn it off without losing the
            // (always-on) tray colour swap.  Setting is read at fire time
            // so toggling it in Settings takes effect immediately.
            //
            // P3: the taskId is forwarded into showNotification's
            // clickContextId so the user can click the balloon to jump
            // straight to that task in the main window (balloonClicked
            // wiring below).
            // Helper: is the user actively LOOKING at the main window right
            // now? Returns true only when the window is on-screen (not
            // hidden-to-tray, not minimised) AND has focus. Anything else
            // (minimised, hidden behind another app, tray-only) counts as
            // "user not looking" — that's when a tray balloon is useful.
            auto isUserLooking = [&engine]() -> bool {
                const auto roots = engine.rootObjects();
                if (roots.isEmpty()) return false;
                auto *w = qobject_cast<QQuickWindow*>(roots.first());
                if (!w) return false;
                const auto v = w->visibility();
                const bool onScreen =
                       v == QWindow::Windowed
                    || v == QWindow::Maximized
                    || v == QWindow::FullScreen
                    || v == QWindow::AutomaticVisibility;
                return onScreen && w->isActive();
            };

            QObject::connect(hudVM, &fsnext::TransferHudViewModel::transferDone,
                             &app, [&tray, &context, isUserLooking](
                                                      const QString &taskId,
                                                      const QString &fileName,
                                                      bool success,
                                                      const QString &errorMessage) {
                auto *settings = context.settingsService();
                if (settings && !settings->notifyOnTransferDone()) return;
                // Skip the balloon when the user is already looking at the
                // app — they'll see the in-app toast / list update without
                // a redundant OS notification. Only fire when the window is
                // hidden-to-tray, minimised, or behind another app.
                if (isUserLooking()) return;

                // Look up the task to differentiate the title by direction
                // (download vs upload) and sync-vs-manual. Falls back to a
                // generic title if findTask returns an empty record (rare —
                // history sweep timer can race with the just-completed
                // signal, especially for fast small files).
                fsnext::TransferType type    = fsnext::TransferType::Download;
                bool                 isSync  = false;
                if (auto *svc = context.transferService()) {
                    const auto t = svc->findTask(taskId);
                    type   = t.type;
                    isSync = t.isSyncTask;
                }

                if (success) {
                    QString title;
                    if (isSync)
                        title = QObject::tr("Đã đồng bộ");
                    else if (type == fsnext::TransferType::Upload)
                        title = QObject::tr("Đã tải lên xong");
                    else
                        title = QObject::tr("Đã tải xuống xong");
                    tray.showNotification(title, fileName,
                                          /*isError=*/false, taskId);
                } else {
                    QString title;
                    if (isSync)
                        title = QObject::tr("Đồng bộ thất bại: ") + fileName;
                    else if (type == fsnext::TransferType::Upload)
                        title = QObject::tr("Tải lên thất bại: ") + fileName;
                    else
                        title = QObject::tr("Tải xuống thất bại: ") + fileName;
                    tray.showNotification(title, errorMessage,
                                          /*isError=*/true, taskId);
                }
            });

            // P3: balloon click → focusTask.  Reuses the same QML path
            // popup row-clicks take (Main.qml listens to hudVM's
            // taskFocusRequested and dispatches to the destination page).
            QObject::connect(&tray, &fsnext::SystemTray::balloonClicked,
                             &app, [&engine, hudVM](const QString &taskId) {
                // Bring main window forward first so the focus dispatch
                // has somewhere to land — Loaders are inactive when the
                // window is hidden.
                const auto roots = engine.rootObjects();
                if (!roots.isEmpty()) {
                    if (auto *w = qobject_cast<QQuickWindow*>(roots.first())) {
                        w->show(); w->raise(); w->requestActivate();
                    }
                }
                // Empty id (older toasts / future non-task balloons) just
                // raises the window — already done above.
                if (!taskId.isEmpty()) hudVM->focusTask(taskId);
            });
        }
    }

    // ── Windows Taskbar Progress (spec v3) ──────────────────────────────────
    // Native progress bar on the taskbar button, independent of the tray.
    // Lives for the app lifetime (local in main()).  No-op on non-Windows /
    // when ITaskbarList3 is unavailable.  Driven by the HUD VM's aggregate
    // progress + state.
    fsnext::TaskbarProgress taskbarProgress;
    if (auto *hudVM = context.hudViewModel()) {
        // Bind the main window handle once it exists.  winId() is stable
        // after the window is created (engine.load already ran above).
        auto bindTaskbarWindow = [&engine, &taskbarProgress]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            if (auto *w = qobject_cast<QQuickWindow *>(roots.first()))
                taskbarProgress.setWindow(w->winId());
        };
        bindTaskbarWindow();

        // Push state + progress whenever counters or progress shift.  Both
        // signals are debounced inside the VM (250ms), so the COM calls
        // stay bounded.  Gated by the showTaskbarProgress setting.
        auto pushTaskbar = [hudVM, &context, &taskbarProgress]() {
            auto *settings = context.settingsService();
            if (settings && !settings->showTaskbarProgress()) {
                taskbarProgress.setState(fsnext::TaskbarProgress::NoProgress);
                return;
            }
            if (hudVM->failedTotal() > 0 && hudVM->activeTotal() == 0) {
                taskbarProgress.setState(fsnext::TaskbarProgress::Error);
            } else if (hudVM->runState() == QStringLiteral("paused")
                       && hudVM->activeTotal() == 0) {
                taskbarProgress.setState(fsnext::TaskbarProgress::Paused);
            } else if (hudVM->activeTotal() > 0) {
                taskbarProgress.setState(fsnext::TaskbarProgress::Normal);
                const double p = hudVM->aggregateProgress();
                if (p >= 0.0) taskbarProgress.setProgress(p);
            } else {
                taskbarProgress.setState(fsnext::TaskbarProgress::NoProgress);
            }
        };
        // Receiver context is &taskbarProgress (not &app) so these
        // connections auto-disconnect the instant taskbarProgress destructs
        // at shutdown — closing the narrow window where a late hudVM emit
        // (hudVM outlives taskbarProgress: it's owned by `context`, declared
        // earlier in main → destructed later) could fire a lambda against a
        // dangling &taskbarProgress reference.
        QObject::connect(hudVM, &fsnext::TransferHudViewModel::countersChanged,
                         &taskbarProgress, pushTaskbar);
        QObject::connect(hudVM, &fsnext::TransferHudViewModel::aggregateProgressChanged,
                         &taskbarProgress, pushTaskbar);
        // React live if the user toggles the setting.
        if (auto *settings = context.settingsService()) {
            QObject::connect(settings, &fsnext::SettingsService::settingsChanged,
                             &taskbarProgress, pushTaskbar);
        }
        pushTaskbar();  // initial
    }

    // ── Single-instance message routing ────────────────────────────────────
    // When a second FsNext.exe (or the Chrome native host) sends us a URL via
    // QLocalSocket, surface it through the existing window-level signal
    // `openDownloadWithLinks` that drag/drop and paste already use.  This
    // keeps the addDownloadDialog's pre-fill logic in one place.
    //
    // Connection target is `&engine` (not `&app`) on purpose: the lambda
    // captures `&engine`, and `&engine` lives inside this scope while
    // `&singleInstance` lives outside.  Once the scope exits and `engine`
    // destructs, Qt auto-removes this connection — without that, a late
    // message from a third FsNext.exe arriving during shutdown would fire
    // the lambda against a freed QQmlApplicationEngine.
    QObject::connect(&singleInstance, &fsnext::SingleInstance::messageReceived,
                     &engine, [&engine](const QString &msg) {
        const auto roots = engine.rootObjects();
        if (roots.isEmpty()) return;
        auto *w = qobject_cast<QQuickWindow *>(roots.first());
        if (!w) return;
        // Bring the window forward first so the dialog is visible.
        w->show();
        w->raise();
        w->requestActivate();
        // "__show__" is the sentinel sent by a second instance launched
        // without URLs (see the tryLock branch above).  In that case just
        // raising the window is the whole job — don't open the add-download
        // dialog with an empty link payload.
        if (msg == QStringLiteral("__show__")) return;
        // Fire the same window-level signal Main.qml's drop/paste path uses.
        // Main.qml declares: `signal openDownloadWithLinks(string links)`.
        QMetaObject::invokeMethod(w, "openDownloadWithLinks",
                                  Q_ARG(QString, msg));
    });

    result = app.exec();

    // ── Shutdown drain ───────────────────────────────────────────────────────
    // QtConcurrent::run lambdas across AuthService / FileService /
    // OAuthService / FileSyncWorker / TransferService capture raw service
    // pointers (m_api, m_http, m_refresh, db). Those services are owned by
    // AppContext and would be destructed at the closing brace of this scope;
    // any task still running at that moment would dereference a freed
    // pointer.  Block until the global thread pool drains so every captured
    // pointer is still valid when its lambda finishes.  5 s is the same
    // bound TransferService uses for individual worker threads — long
    // enough to let a typical in-flight HTTP request finish, short enough
    // that a wedged network call doesn't keep the user staring at a frozen
    // exit.
    QThreadPool::globalInstance()->waitForDone(5000);
    }   // ── end of "Main.qml loaded" else branch ─────────────────────────────
    }   // ── End of application-context scope ───────────────────────────────
    // AppContext, the QML engine, and the tray icon are now destructed.
    // HttpClient::~HttpClient ran (curl_share_cleanup) WHILE the curl global
    // state is still alive — that's the whole point of this block.  From
    // here on we only touch process-global state (logger, curl global).

    // Uninstall logger first so any lingering qDebug() during shutdown goes to default handler
    qInstallMessageHandler(nullptr);

    {
        QMutexLocker locker(&g_logMutex);
        if (g_logFile) {
            g_logFile->close();
            delete g_logFile;
            g_logFile = nullptr;
        }
    }
    fsnext::Application::cleanupCurl();
    return result;
}
