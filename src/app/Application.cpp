#include "Application.h"
#include <curl/curl.h>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QNetworkProxy>
#include <QCoreApplication>
#include <QAccessible>
#include <QDir>
#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace fsnext {

void Application::initCurl()
{
    curl_global_init(CURL_GLOBAL_ALL);
    qDebug() << "[FsNext] CURL initialized:" << curl_version();
}

void Application::cleanupCurl()
{
    curl_global_cleanup();
}

void Application::initSsl()
{
    // Load CA certificates from app directory if available
    QString caPath = QCoreApplication::applicationDirPath() + QStringLiteral("/ca.crt");
    if (QFile::exists(caPath)) {
        QList<QSslCertificate> certs = QSslCertificate::fromPath(caPath);
        if (!certs.isEmpty()) {
            QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
            sslConf.addCaCertificates(certs);
            QSslConfiguration::setDefaultConfiguration(sslConf);
            qDebug() << "[FsNext] Loaded" << certs.size() << "CA certificates";
        }
    }
}

void Application::configureProxy(int mode, const QString &host, int port)
{
    switch (mode) {
    case 0: // No proxy
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;
    case 1: // System proxy
        QNetworkProxy::setApplicationProxy(QNetworkProxy::applicationProxy());
        break;
    case 2: // Manual proxy
        if (!host.isEmpty() && port > 0) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, host, static_cast<quint16>(port));
            QNetworkProxy::setApplicationProxy(proxy);
            qDebug() << "[FsNext] Proxy set:" << host << ":" << port;
        }
        break;
    }
}

QString Application::appVersion()
{
#ifdef FSNEXT_VERSION_FULL
    return QStringLiteral(FSNEXT_VERSION_FULL);
#else
    return QStringLiteral("6.0.0.0");
#endif
}

bool Application::osPrefersReducedMotion()
{
#ifdef Q_OS_WIN
    // SPI_GETCLIENTAREAANIMATION returns BOOL — TRUE when animations are
    // ENABLED. We invert: reducedMotion = !animations.  Per MSDN this is the
    // setting the OS itself uses to gate ribbon / Aero / explorer animations.
    BOOL animationsEnabled = TRUE;
    if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animationsEnabled, 0)) {
        return animationsEnabled == FALSE;
    }
    // Fall back to legacy "minimize animations" key — older Windows still set
    // this even without the modern reduce-motion preference.
    ANIMATIONINFO ai{};
    ai.cbSize = sizeof(ai);
    if (SystemParametersInfoW(SPI_GETANIMATION, sizeof(ai), &ai, 0)) {
        return ai.iMinAnimate == 0;
    }
    return false;
#else
    // Heuristic on macOS / Linux: if any accessibility tool is active, prefer
    // reduced motion.  Better than ignoring the signal.  Replace with platform-
    // specific reads (NSAccessibilityReduceMotionEnabled / GTK setting) when we
    // build on those targets.
    return QAccessible::isActive();
#endif
}

} // namespace fsnext
