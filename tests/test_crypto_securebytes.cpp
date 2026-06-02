// SPDX-License-Identifier: Proprietary
//
// Unit tests for fsnext::crypto::SecureBytes (Phase C1 / E0).
// Verifies allocation, zero-init, copy-in, move-only semantics, clear(), and
// constant-time comparison. libsodium is initialized once in initTestCase().

#include <QtTest>

#include "core/crypto/Crypto.h"
#include "core/crypto/SecureBytes.h"

#include <cstdint>
#include <cstring>
#include <utility>

using fsnext::crypto::SecureBytes;

class TestCryptoSecureBytes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY2(fsnext::crypto::init(), "libsodium failed to initialize");
    }

    void defaultIsEmpty()
    {
        SecureBytes b;
        QCOMPARE(b.size(), std::size_t{0});
        QVERIFY(b.empty());
        QCOMPARE(b.data(), static_cast<std::uint8_t *>(nullptr));
    }

    void allocateIsZeroed()
    {
        SecureBytes b(32);
        QCOMPARE(b.size(), std::size_t{32});
        QVERIFY(!b.empty());
        QVERIFY(b.data() != nullptr);
        for (std::size_t i = 0; i < b.size(); ++i)
            QCOMPARE(b.data()[i], std::uint8_t{0});
    }

    void zeroLengthAllocatesNothing()
    {
        SecureBytes b(std::size_t{0});
        QVERIFY(b.empty());
        QCOMPARE(b.data(), static_cast<std::uint8_t *>(nullptr));
    }

    void writableBuffer()
    {
        SecureBytes b(8);
        for (std::size_t i = 0; i < b.size(); ++i)
            b.data()[i] = static_cast<std::uint8_t>(i + 1);
        for (std::size_t i = 0; i < b.size(); ++i)
            QCOMPARE(b.data()[i], static_cast<std::uint8_t>(i + 1));
    }

    void copyInCtor()
    {
        const std::uint8_t src[] = {0xDE, 0xAD, 0xBE, 0xEF};
        SecureBytes b(src, sizeof(src));
        QCOMPARE(b.size(), sizeof(src));
        QVERIFY(std::memcmp(b.data(), src, sizeof(src)) == 0);
    }

    void copyInNullSrcZeroFills()
    {
        SecureBytes b(nullptr, 16);
        QCOMPARE(b.size(), std::size_t{16});
        for (std::size_t i = 0; i < b.size(); ++i)
            QCOMPARE(b.data()[i], std::uint8_t{0});
    }

    void moveCtorTransfersOwnership()
    {
        const std::uint8_t src[] = {1, 2, 3, 4, 5};
        SecureBytes a(src, sizeof(src));
        const std::uint8_t *origPtr = a.data();

        SecureBytes b(std::move(a));
        QCOMPARE(b.size(), sizeof(src));
        QCOMPARE(b.data(), origPtr);          // same buffer, no realloc
        QVERIFY(std::memcmp(b.data(), src, sizeof(src)) == 0);

        // Moved-from object is empty.
        QVERIFY(a.empty());                   // NOLINT(bugprone-use-after-move)
        QCOMPARE(a.data(), static_cast<std::uint8_t *>(nullptr));
    }

    void moveAssignWipesTargetFirst()
    {
        SecureBytes a(4);
        a.data()[0] = 0xAA;

        const std::uint8_t src[] = {9, 8, 7};
        SecureBytes b(src, sizeof(src));

        a = std::move(b);
        QCOMPARE(a.size(), sizeof(src));
        QVERIFY(std::memcmp(a.data(), src, sizeof(src)) == 0);
        QVERIFY(b.empty());                   // NOLINT(bugprone-use-after-move)
    }

    void selfMoveAssignIsSafe()
    {
        SecureBytes a(4);
        a.data()[0] = 0x42;
        SecureBytes &ref = a;
        a = std::move(ref);                   // NOLINT(clang-diagnostic-self-move)
        QCOMPARE(a.size(), std::size_t{4});
        QCOMPARE(a.data()[0], std::uint8_t{0x42});
    }

    void clearResetsToEmpty()
    {
        SecureBytes b(64);
        QVERIFY(!b.empty());
        b.clear();
        QVERIFY(b.empty());
        QCOMPARE(b.data(), static_cast<std::uint8_t *>(nullptr));
        b.clear();                            // idempotent
        QVERIFY(b.empty());
    }

    void constantTimeEqualsMatches()
    {
        const std::uint8_t data[] = {10, 20, 30, 40};
        SecureBytes a(data, sizeof(data));
        SecureBytes b(data, sizeof(data));
        QVERIFY(a.constantTimeEquals(b));
        QVERIFY(b.constantTimeEquals(a));
    }

    void constantTimeEqualsDiffers()
    {
        const std::uint8_t d1[] = {10, 20, 30, 40};
        const std::uint8_t d2[] = {10, 20, 30, 41};
        SecureBytes a(d1, sizeof(d1));
        SecureBytes b(d2, sizeof(d2));
        QVERIFY(!a.constantTimeEquals(b));
    }

    void constantTimeEqualsDifferentSize()
    {
        const std::uint8_t d1[] = {1, 2, 3};
        const std::uint8_t d2[] = {1, 2, 3, 4};
        SecureBytes a(d1, sizeof(d1));
        SecureBytes b(d2, sizeof(d2));
        QVERIFY(!a.constantTimeEquals(b));
    }

    void constantTimeEqualsEmpty()
    {
        SecureBytes a;
        SecureBytes b;
        QVERIFY(a.constantTimeEquals(b));
    }
};

QTEST_APPLESS_MAIN(TestCryptoSecureBytes)
#include "test_crypto_securebytes.moc"
