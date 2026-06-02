// SPDX-License-Identifier: Proprietary
//
// Unit tests for fsnext::VaultViewModel (Phase U1). Verifies the async,
// non-blocking lifecycle (create/unlock run off-main via QtConcurrent and
// report via operationFinished), state properties, and busy re-entrancy
// gating that prevents concurrent VaultManager access.

#include <QtTest>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariantMap>

#include "core/crypto/Crypto.h"
#include "viewmodels/VaultViewModel.h"

using fsnext::VaultViewModel;

class TestVaultViewModel : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QVERIFY2(fsnext::crypto::init(), "libsodium init"); }

    void aesAvailableProperty()
    {
        VaultViewModel vm;
        QCOMPARE(vm.aesAvailable(), fsnext::crypto::aes256GcmAvailable());
    }

    void createUnlockAsync()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        VaultViewModel vm;
        vm.setVaultDir(dir.path());
        QVERIFY(!vm.vaultExists());
        QCOMPARE(vm.lockState(), 0);  // NoVault

        QSignalSpy spy(&vm, &VaultViewModel::operationFinished);
        vm.createVault(QStringLiteral("My Vault"), QStringLiteral("pw"), 0 /*balanced*/,
                       1 /*HighRamOnly*/);
        QVERIFY(vm.busy());  // set synchronously, before the worker runs
        QVERIFY(spy.wait(10000));
        {
            const QList<QVariant> a = spy.takeFirst();
            QCOMPARE(a.at(0).toString(), QStringLiteral("create"));
            QVERIFY(a.at(1).toBool());
        }
        QVERIFY(!vm.busy());
        QVERIFY(vm.isUnlocked());
        QCOMPARE(vm.lockState(), 2);
        QCOMPARE(vm.vaultName(), QStringLiteral("My Vault"));

        vm.lock();
        QVERIFY(!vm.isUnlocked());
        QCOMPARE(vm.lockState(), 1);  // Locked

        QSignalSpy spyWrong(&vm, &VaultViewModel::operationFinished);
        vm.unlock(QStringLiteral("nope"));
        QVERIFY(spyWrong.wait(10000));
        QVERIFY(!spyWrong.takeFirst().at(1).toBool());
        QVERIFY(!vm.isUnlocked());

        QSignalSpy spyOk(&vm, &VaultViewModel::operationFinished);
        vm.unlock(QStringLiteral("pw"));
        QVERIFY(spyOk.wait(10000));
        QVERIFY(spyOk.takeFirst().at(1).toBool());
        QVERIFY(vm.isUnlocked());
    }

    void fileEncryptList()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        VaultViewModel vm;
        vm.setVaultDir(dir.path());

        QSignalSpy spyCreate(&vm, &VaultViewModel::operationFinished);
        vm.createVault(QStringLiteral("V"), QStringLiteral("pw"), 0, 1 /*HighRamOnly*/);
        QVERIFY(spyCreate.wait(10000));
        QVERIFY(vm.isUnlocked());

        const QString src = dir.filePath("doc.txt");
        {
            QFile f(src);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("hello vault — xin chào");
        }

        QSignalSpy spyFile(&vm, &VaultViewModel::fileOpFinished);
        vm.addFiles(QStringList{src});
        QVERIFY(vm.fileBusy());
        QVERIFY(spyFile.wait(10000));
        {
            const QList<QVariant> a = spyFile.takeFirst();
            QCOMPARE(a.at(0).toString(), QStringLiteral("encrypt"));
            QVERIFY(a.at(1).toBool());
        }
        QVERIFY(!vm.fileBusy());
        QCOMPARE(vm.vaultFiles().size(), 1);
        const QVariantMap m = vm.vaultFiles().at(0).toMap();
        QCOMPARE(m.value(QStringLiteral("name")).toString(), QStringLiteral("doc.txt"));
        QVERIFY(QFile::exists(dir.path() + "/doc.txt.fshenc"));
    }

    void keyManagerList()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        VaultViewModel vm;
        vm.setVaultDir(dir.path());

        QSignalSpy spyC(&vm, &VaultViewModel::operationFinished);
        vm.createVault(QStringLiteral("V"), QStringLiteral("pw"), 0, 1);
        QVERIFY(spyC.wait(10000));
        QVERIFY(vm.isUnlocked());

        QVERIFY(vm.keys().size() >= 1);
        const QVariantMap v = vm.keys().at(0).toMap();
        QCOMPARE(v.value(QStringLiteral("type")).toString(), QStringLiteral("vault"));
        QVERIFY(!v.value(QStringLiteral("fingerprint")).toString().isEmpty());

        // Import a 32-byte keyfile → registers + appears in keys.
        const QString kf = dir.filePath("my.key");
        {
            QFile f(kf);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QByteArray(32, 'A'));
        }
        QSignalSpy spyI(&vm, &VaultViewModel::operationFinished);
        vm.importKeyfile(kf);
        QCOMPARE(spyI.count(), 1);   // sync op
        QCOMPARE(spyI.at(0).at(0).toString(), QStringLiteral("importKey"));
        QVERIFY(spyI.at(0).at(1).toBool());

        bool found = false;
        const QVariantList ks = vm.keys();
        for (const QVariant &kv : ks)
            if (kv.toMap().value(QStringLiteral("type")).toString() == QStringLiteral("keyfile"))
                found = true;
        QVERIFY(found);
    }

    void busyGating()
    {
        QTemporaryDir dir;
        VaultViewModel vm;
        vm.setVaultDir(dir.path());

        QSignalSpy spy(&vm, &VaultViewModel::operationFinished);
        vm.createVault(QStringLiteral("V"), QStringLiteral("pw"), 0, 1);
        QVERIFY(vm.busy());

        // A second op while busy must fail immediately (synchronously) with "Busy",
        // never touching VaultManager concurrently.
        vm.unlock(QStringLiteral("pw"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("unlock"));
        QVERIFY(!spy.at(0).at(1).toBool());
        QCOMPARE(spy.at(0).at(2).toString(), QStringLiteral("Busy"));

        // The original create still completes.
        QVERIFY(spy.wait(10000));
        QVERIFY(vm.isUnlocked());
    }
};

QTEST_GUILESS_MAIN(TestVaultViewModel)
#include "test_vault_viewmodel.moc"
