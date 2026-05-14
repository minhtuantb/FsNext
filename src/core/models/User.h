#pragma once
#include <QString>
#include <cstdint>

namespace fsnext {

struct User {
    QString id;
    QString email;
    QString name;
    int level = 0;            // 2=Member, 3=VIP
    QString expireVip;
    QString joinDate;
    int64_t traffic = 0;
    int64_t trafficUsed = 0;
    int64_t webspace = 0;
    int64_t webspaceUsed = 0;
    int64_t webspaceSecure = 0;
    int64_t webspaceSecureUsed = 0;
    int totalPoints = 0;
    int dlTimeAvail = 0;
    QString accountType;
    QString avatarUrl;   // OAuth provider picture URL (empty for password-only accounts)

    bool isVip() const { return level >= 3; }
    int64_t webspaceFree() const { return webspace - webspaceUsed; }
    int64_t webspaceSecureFree() const { return webspaceSecure - webspaceSecureUsed; }
    int64_t trafficFree() const { return traffic - trafficUsed; }
};

struct Session {
    QString token;
    QString sessionId;
    QString cookie;
    User user;

    bool isValid() const { return !token.isEmpty(); }
};

} // namespace fsnext
