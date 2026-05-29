// SPDX-License-Identifier: Proprietary
// AppError model unit tests — verify each factory (none/network/auth/server/
// storage/transfer/validation) populates category/code/message/technicalDetail/
// retryable correctly, plus the server-502 retryable rule and isError().
//
// AppError is a header-only struct (no .cpp), so the only translation unit is
// this test. Pure value logic — no event loop, no I/O.

#include <QtTest>
#include "core/models/AppError.h"

using fsnext::AppError;
using fsnext::ErrorCategory;

class TestAppError : public QObject
{
    Q_OBJECT
private slots:
    // ATM-0278 (TC-0297) — default-constructed AppError has category None.
    void defaultCategoryIsNone()
    {
        AppError e{};
        QCOMPARE(e.category, ErrorCategory::None);
        // Defaults of the rest of the struct, while we're here.
        QCOMPARE(e.code, 0);
        QVERIFY(e.message.isEmpty());
        QVERIFY(e.technicalDetail.isEmpty());
        QCOMPARE(e.retryable, false);
    }

    // ATM-0278 (TC-0298) — every factory stamps its own category.
    void eachFactorySetsCategory()
    {
        QCOMPARE(AppError::network(0, QString()).category, ErrorCategory::Network);
        QCOMPARE(AppError::auth(0, QString()).category, ErrorCategory::Auth);
        QCOMPARE(AppError::server(0, QString()).category, ErrorCategory::Server);
        QCOMPARE(AppError::storage(QString()).category, ErrorCategory::Storage);
        QCOMPARE(AppError::transfer(0, QString()).category, ErrorCategory::Transfer);
        QCOMPARE(AppError::validation(QString()).category, ErrorCategory::Validation);
        QCOMPARE(AppError::none().category, ErrorCategory::None);
    }

    // ATM-0279 (TC-0299) — factories carrying a code store it verbatim.
    void codeStoredFromFactory()
    {
        QCOMPARE(AppError::network(28, QStringLiteral("timeout")).code, 28);
        QCOMPARE(AppError::auth(401, QStringLiteral("x")).code, 401);
        QCOMPARE(AppError::server(500, QStringLiteral("x")).code, 500);
        QCOMPARE(AppError::transfer(7, QStringLiteral("x")).code, 7);
    }

    // ATM-0279 (TC-0300) — codeless factories default to 0.
    void codeDefaultsZeroForCodelessFactories()
    {
        QCOMPARE(AppError::storage(QStringLiteral("đầy")).code, 0);
        QCOMPARE(AppError::validation(QStringLiteral("invalid")).code, 0);
    }

    // ATM-0280 (TC-0301) — auth carries the caller's message through to message.
    void messageStoredForUserFacingFactories()
    {
        const QString msg = QStringLiteral("Đăng nhập thất bại");
        const AppError e = AppError::auth(401, msg);
        QCOMPARE(e.message, msg);
        // auth leaves technicalDetail empty.
        QVERIFY(e.technicalDetail.isEmpty());
    }

    // ATM-0280 / ATM-0281 (TC-0302) — network uses a fixed display message and
    // routes the caller's string into technicalDetail instead.
    void networkUsesFixedMessageAndTechnicalDetail()
    {
        const AppError e = AppError::network(0, QStringLiteral("detail"));
        QCOMPARE(e.message, QStringLiteral("Network error"));
        QCOMPARE(e.technicalDetail, QStringLiteral("detail"));
    }

    // ATM-0282 (TC-0303) — network and transfer are retryable.
    void retryableTrueForNetworkAndTransfer()
    {
        QVERIFY(AppError::network(0, QStringLiteral("x")).retryable);
        QVERIFY(AppError::transfer(1, QStringLiteral("y")).retryable);
    }

    // ATM-0282 (TC-0304) — auth/validation/storage are fatal (not retryable).
    void retryableFalseForFatalCategories()
    {
        QVERIFY(!AppError::auth(401, QStringLiteral("x")).retryable);
        QVERIFY(!AppError::validation(QStringLiteral("x")).retryable);
        QVERIFY(!AppError::storage(QStringLiteral("x")).retryable);
    }

    // ATM-0282 (TC-0305) — server is retryable ONLY for a 502 (bad gateway).
    void serverRetryableOnlyFor502()
    {
        QVERIFY(AppError::server(502, QStringLiteral("x")).retryable);
        QVERIFY(!AppError::server(500, QStringLiteral("y")).retryable);
        QVERIFY(!AppError::server(503, QStringLiteral("z")).retryable);
    }

    // ATM-0283 (TC-0306) — none() reports no error.
    void isErrorFalseWhenNone()
    {
        QVERIFY(!AppError::none().isError());
        QVERIFY(!AppError{}.isError());
    }

    // ATM-0283 (TC-0307) — any non-None category reports an error.
    void isErrorTrueWhenCategorySet()
    {
        QVERIFY(AppError::server(500, QStringLiteral("x")).isError());
        QVERIFY(AppError::auth(401, QStringLiteral("x")).isError());
        QVERIFY(AppError::validation(QStringLiteral("x")).isError());
    }
};

QTEST_GUILESS_MAIN(TestAppError)
#include "test_app_error.moc"
