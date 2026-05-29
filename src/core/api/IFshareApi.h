#pragma once

// IFshareApi — the slice of FshareApi that the service layer depends on,
// extracted as a pure-virtual interface so services can be unit-tested against
// a fake (no HttpClient / network). Currently scoped to AuthService's needs;
// widen it as other services gain test coverage (see docs/BACKLOG.md).
//
// FshareApi derives from this and overrides the four methods below. Production
// callers keep holding FshareApi* and use its full concrete surface — only
// AuthService takes an IFshareApi* so a FakeFshareApi can be injected in tests.

#include "core/models/ApiResponse.h"
#include "core/models/User.h"

#include <QString>

namespace fsnext {

class IFshareApi {
public:
    virtual ~IFshareApi() = default;

    virtual ApiResponse<Session> login(const QString &email, const QString &password) = 0;
    virtual ApiResponse<Session> loginOauth(const QString &service,
                                            const QString &accessToken,
                                            const QString &email) = 0;
    virtual ApiResponse<void>    logout() = 0;
    virtual ApiResponse<User>    getUserInfo() = 0;
};

} // namespace fsnext
