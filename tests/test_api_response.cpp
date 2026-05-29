// SPDX-License-Identifier: Proprietary
// ApiResponse<T> unit tests — covers the success/failure factories and the
// accessor contract (isSuccess/isError/data/takeData/error/httpCode).
// Header-only template over AppError; pure value semantics, no event loop.

#include <QtTest>
#include <optional>
#include <string>

#include "core/models/ApiResponse.h"
#include "core/models/AppError.h"

using fsnext::ApiResponse;
using fsnext::AppError;
using fsnext::ErrorCategory;

class TestApiResponse : public QObject
{
    Q_OBJECT
private slots:
    // ATM-0272 — isSuccess(): true from success(), false from failure().
    // ATM-0273 — isError() is the exact inverse on the same instances.
    void successAndFailureFlags()
    {
        const auto ok = ApiResponse<int>::success(42, 200);
        QVERIFY(ok.isSuccess());
        QVERIFY(!ok.isError());

        const auto bad = ApiResponse<int>::failure(AppError::auth(401, QStringLiteral("x")));
        QVERIFY(!bad.isSuccess());
        QVERIFY(bad.isError());
    }

    // ATM-0274 — data() returns the value stored by success().
    void dataReturnsSuccessValue()
    {
        const auto r = ApiResponse<QString>::success(QStringLiteral("abc"));
        QCOMPARE(r.data(), QStringLiteral("abc"));
    }

    // ATM-0274 — data() on a failure() has no stored optional → bad_optional_access.
    void dataOnFailureThrows()
    {
        const auto r = ApiResponse<int>::failure(AppError::server(500, QStringLiteral("x")));
        bool threw = false;
        try {
            (void)r.data();
        } catch (const std::bad_optional_access &) {
            threw = true;
        }
        QVERIFY(threw);
    }

    // ATM-0275 — takeData() moves the stored value out.
    void takeDataMovesValueOut()
    {
        auto r = ApiResponse<std::string>::success(std::string("big"));
        QCOMPARE(QString::fromStdString(r.takeData()), QStringLiteral("big"));
    }

    // ATM-0275 — takeData() on a failure() also throws bad_optional_access.
    void takeDataOnFailureThrows()
    {
        auto r = ApiResponse<int>::failure(AppError::validation(QStringLiteral("x")));
        bool threw = false;
        try {
            (void)r.takeData();
        } catch (const std::bad_optional_access &) {
            threw = true;
        }
        QVERIFY(threw);
    }

    // ATM-0276 — error() echoes the AppError passed to failure().
    void errorReturnsStoredError()
    {
        const auto r = ApiResponse<int>::failure(
            AppError::auth(401, QStringLiteral("sai mật khẩu")));
        QCOMPARE(r.error().category, ErrorCategory::Auth);
        QCOMPARE(r.error().code, 401);
        QCOMPARE(r.error().message, QStringLiteral("sai mật khẩu"));
    }

    // ATM-0276 — error() on a success() is a default (None) AppError.
    void errorOnSuccessIsNone()
    {
        const auto r = ApiResponse<int>::success(1);
        QCOMPARE(r.error().category, ErrorCategory::None);
        QVERIFY(!r.error().isError());
    }

    // ATM-0277 — httpCode() preserves the code given to success().
    void httpCodeFromSuccess()
    {
        const auto r = ApiResponse<int>::success(1, 200);
        QCOMPARE(r.httpCode(), 200);
    }

    // ATM-0277 — httpCode() defaults to 0 for failure() when no code passed.
    void httpCodeDefaultsToZeroOnFailure()
    {
        const auto r = ApiResponse<int>::failure(AppError::network(0, QStringLiteral("x")));
        QCOMPARE(r.httpCode(), 0);
    }
};

QTEST_GUILESS_MAIN(TestApiResponse)
#include "test_api_response.moc"
