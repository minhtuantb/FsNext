// SPDX-License-Identifier: Proprietary
// User model unit tests — focus on the computed helpers (isVip, trafficFree,
// webspaceFree, webspaceSecureFree) and the struct's default-constructed
// invariants. Trivial "assign-then-read" POD fields (name/id/email,
// traffic/used counters) are exercised only where they feed a computed result
// or pin a documented default; they are not given one test each.
//
// User is a header-only struct (no .cpp), so this links Qt6::Core/Test only.

#include <QtTest>
#include <cstdint>

#include "core/models/User.h"

using fsnext::User;

class TestUserModel : public QObject
{
    Q_OBJECT
private slots:
    // ── ATM-0382 / ATM-0396 / ATM-0397: default-constructed invariants ──
    // One grouped test for the trivial QString fields: they must all start
    // empty and round-trip an assigned value (incl. unicode for name).
    void defaultsAndStringFields()
    {
        // ATM-0382 / ATM-0396 / ATM-0397
        const User def{};
        QVERIFY(def.id.isEmpty());
        QVERIFY(def.email.isEmpty());
        QVERIFY(def.name.isEmpty());
        QCOMPARE(def.level, 0);

        User u;
        u.id = QStringLiteral("user-99887");
        u.email = QStringLiteral("user@fpt.com");
        u.name = QStringLiteral("Nguyễn Văn A");
        QCOMPARE(u.id, QStringLiteral("user-99887"));
        QCOMPARE(u.email, QStringLiteral("user@fpt.com"));
        QCOMPARE(u.name, QStringLiteral("Nguyễn Văn A"));
    }

    // ── ATM-0383: isVip() = (level >= 3) ──
    void isVipReflectsLevel()
    {
        // ATM-0383
        User vip;
        vip.level = 3;
        QVERIFY(vip.isVip());

        User member;
        member.level = 2;
        QVERIFY(!member.isVip());

        // Boundary: default level (0) is not VIP; anything >3 still VIP.
        User def{};
        QVERIFY(!def.isVip());
        User higher;
        higher.level = 5;
        QVERIFY(higher.isVip());
    }

    // ── ATM-0386 / ATM-0387: trafficFree() = traffic - trafficUsed ──
    void trafficFreeComputesRemaining()
    {
        // ATM-0386 / ATM-0387
        User def{};
        QCOMPARE(def.traffic, qint64(0));
        QCOMPARE(def.trafficUsed, qint64(0));
        QCOMPARE(def.trafficFree(), qint64(0));

        User u;
        u.traffic = 1000;
        u.trafficUsed = 300;
        QCOMPARE(u.trafficFree(), qint64(700));

        // Large real-world magnitude stays exact in int64.
        User big;
        big.traffic = Q_INT64_C(161061273600); // 150 GiB
        big.trafficUsed = Q_INT64_C(52428800);  // 50 MiB
        QCOMPARE(big.traffic, Q_INT64_C(161061273600));
        QCOMPARE(big.trafficFree(), Q_INT64_C(161061273600) - Q_INT64_C(52428800));
    }

    // ── ATM-0388 / ATM-0389: webspaceFree() = webspace - webspaceUsed ──
    void webspaceFreeComputesRemaining()
    {
        // ATM-0388 / ATM-0389
        User def{};
        QCOMPARE(def.webspace, qint64(0));
        QCOMPARE(def.webspaceUsed, qint64(0));
        QCOMPARE(def.webspaceFree(), qint64(0));

        User u;
        u.webspace = 1000;
        u.webspaceUsed = 400;
        QCOMPARE(u.webspaceFree(), qint64(600));

        User big;
        big.webspace = Q_INT64_C(1099511627776); // 1 TiB
        big.webspaceUsed = Q_INT64_C(104857600);  // 100 MiB
        QCOMPARE(big.webspace, Q_INT64_C(1099511627776));
        QCOMPARE(big.webspaceFree(), Q_INT64_C(1099511627776) - Q_INT64_C(104857600));
    }

    // ── ATM-0390 / ATM-0391: webspaceSecureFree() = secure - secureUsed ──
    void webspaceSecureFreeComputesRemaining()
    {
        // ATM-0390 / ATM-0391
        User def{};
        QCOMPARE(def.webspaceSecure, qint64(0));
        QCOMPARE(def.webspaceSecureUsed, qint64(0));
        QCOMPARE(def.webspaceSecureFree(), qint64(0));

        User u;
        u.webspaceSecure = 500;
        u.webspaceSecureUsed = 200;
        QCOMPARE(u.webspaceSecureFree(), qint64(300));

        User big;
        big.webspaceSecure = Q_INT64_C(53687091200); // 50 GiB
        big.webspaceSecureUsed = Q_INT64_C(10485760); // 10 MiB
        QCOMPARE(big.webspaceSecureFree(),
                 Q_INT64_C(53687091200) - Q_INT64_C(10485760));
    }
};

QTEST_GUILESS_MAIN(TestUserModel)
#include "test_user_model.moc"
