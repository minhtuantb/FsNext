#pragma once
#include <QString>

namespace fsnext {

enum class ErrorCategory {
    None,
    Network,
    Auth,
    Server,
    Storage,
    Transfer,
    Validation,
    Unknown
};

struct AppError {
    ErrorCategory category = ErrorCategory::None;
    int code = 0;
    QString message;
    QString technicalDetail;
    bool retryable = false;

    bool isError() const { return category != ErrorCategory::None; }

    static AppError none() { return {}; }

    static AppError network(int code, const QString &detail) {
        return { ErrorCategory::Network, code, QStringLiteral("Network error"), detail, true };
    }

    static AppError auth(int code, const QString &msg) {
        return { ErrorCategory::Auth, code, msg, {}, false };
    }

    static AppError server(int code, const QString &msg) {
        return { ErrorCategory::Server, code, msg, {}, code == 502 };
    }

    static AppError storage(const QString &msg) {
        return { ErrorCategory::Storage, 0, msg, {}, false };
    }

    static AppError transfer(int code, const QString &msg) {
        return { ErrorCategory::Transfer, code, msg, {}, true };
    }

    static AppError validation(const QString &msg) {
        return { ErrorCategory::Validation, 0, msg, {}, false };
    }
};

} // namespace fsnext
