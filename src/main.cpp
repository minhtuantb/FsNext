#include "app/Application.h"
#include "app/AppContext.h"
#include "core/services/AuthService.h"
#include "core/services/TransferService.h"
#include "platform/SingleInstance.h"
#include "platform/SystemTray.h"
#include "viewmodels/LanguageViewModel.h"

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
    // NB: setQuitOnLastWindowClosed(false) is deferred until AFTER tray.setup()
    // succeeds — without a tray, the only way to "quit" must remain closing the
    // last window (otherwise headless CI / desktops without tray would leak the
    // process).  See the post-engine block below.
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
        qCritical() << "[FsNext] No root objects created";
        fsnext::Application::cleanupCurl();
        return -1;
    }

    qDebug() << "[FsNext] Application started, version:" << fsnext::Application::appVersion();

    // Try auto-login with saved credentials
    QTimer::singleShot(100, context.authService(), &fsnext::AuthService::autoLogin);

    // System tray — owns its own QSystemTrayIcon.  Wired here (not in
    // AppContext::init) because tray needs a QApplication and the QML root
    // window to already exist.
    fsnext::SystemTray tray(&engine);
    if (tray.setup()) {
        // Only safe to enable minimize-to-tray once we KNOW there's a tray to
        // restore from.  Without this guard, closing the window on a desktop
        // without tray would leave the app running invisibly forever.
        QApplication::setQuitOnLastWindowClosed(false);
        // Show / hide the root window when user clicks the tray.
        QObject::connect(&tray, &fsnext::SystemTray::showWindowRequested,
                         &app, [&engine]() {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            auto *w = qobject_cast<QQuickWindow *>(roots.first());
            if (!w) return;
            w->show();
            w->raise();
            w->requestActivate();
        });
        QObject::connect(&tray, &fsnext::SystemTray::pauseAllRequested,
                         &app, [&context]() {
            // TransferService::pauseAll covers both download and upload queues
            // (it iterates m_tasks regardless of TransferType).
            if (auto *svc = context.transferService()) svc->pauseAll();
        });
        QObject::connect(&tray, &fsnext::SystemTray::quitRequested,
                         &app, &QApplication::quit);
    }

    // ── Single-instance message routing ────────────────────────────────────
    // When a second FsNext.exe (or the Chrome native host) sends us a URL via
    // QLocalSocket, surface it through the existing window-level signal
    // `openDownloadWithLinks` that drag/drop and paste already use.  This
    // keeps the addDownloadDialog's pre-fill logic in one place.
    QObject::connect(&singleInstance, &fsnext::SingleInstance::messageReceived,
                     &app, [&engine](const QString &msg) {
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

    int result = app.exec();

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
