#pragma once
#include <QObject>
#include <QString>

namespace fsnext {

class Application {
public:
    static void initCurl();
    static void cleanupCurl();
    static void initSsl();
    static void configureProxy(int mode, const QString &host, int port);
    static QString appVersion();

    // Returns true when the OS is signalling "Reduce motion" / "Disable
    // animations". On Windows we read SystemParametersInfo(SPI_GETCLIENTAREAANIMATION)
    // and SPI_GETMESSAGEDURATION as a stand-in (Windows has no canonical reduce-motion
    // setting). On macOS / Linux we fall back to QAccessible::isActive() — if the
    // user has any accessibility tooling on we err on the side of fewer animations.
    static bool osPrefersReducedMotion();
};

} // namespace fsnext
