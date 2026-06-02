// SPDX-License-Identifier: Proprietary
//
// Unit tests for fsnext::VaultManager (plain class — auto-lock + notifications
// now live in VaultViewModel). Covers create/unlock/lock, wrong passphrase,
// change passphrase, recovery keyfile export+import, L2 DPAPI auto-unlock,
// L1 no-auto-unlock, metadata accessors, and EncryptionEngine integration.

#include <QtTest>
#include <QTemporaryDir>

#include "core/crypto/Crypto.h"
#include "core/crypto/EncryptionEngine.h"
#include "core/crypto/Key.h"
#include "core/services/VaultManager.h"

#include <sodium.h>

#include <array>
#include <vector>

using fsnext::VaultManager;
using namespace fsnext::crypto;

namespace {
// Cheap KDF so the many derivations across tests stay fast.
KdfParams testKdf() { return KdfParams{2, 16384, 1}; }
}

class TestVaultManager : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QVERIFY2(fsnext::crypto::init(), "libsodium init"); }

    void createUnlockLock()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(!vm.vaultExists());
        QVERIFY(vm.state() == VaultManager::State::NoVault);

        QVERIFY(vm.createVault("Personal Vault", "pass-phrase-1", testKdf(),
                               VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        QVERIFY(vm.isUnlocked());
        QVERIFY(vm.vaultExists());
        QVERIFY(vm.masterKey().valid());
        QVERIFY(vm.vaultName() == "Personal Vault");
        const std::array<std::uint8_t, 8> fp = vm.masterKey().fingerprint();

        vm.lock();
        QVERIFY(!vm.isUnlocked());
        QVERIFY(vm.state() == VaultManager::State::Locked);
        QVERIFY(!vm.masterKey().valid());

        QVERIFY(vm.unlock("wrong") == CryptoError::WrongKey);
        QVERIFY(!vm.isUnlocked());

        QVERIFY(vm.unlock("pass-phrase-1") == CryptoError::Ok);
        QVERIFY(vm.isUnlocked());
        QVERIFY(vm.masterKey().fingerprint() == fp);
    }

    void createTwiceFails()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::AlreadyExists);
    }

    void changePassphrase()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "old-pass", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        QVERIFY(vm.changePassphrase("wrong-old", "new-pass", testKdf()) == CryptoError::WrongKey);
        QVERIFY(vm.changePassphrase("old-pass", "new-pass", testKdf()) == CryptoError::Ok);
        vm.lock();
        QVERIFY(vm.unlock("old-pass") == CryptoError::WrongKey);
        QVERIFY(vm.unlock("new-pass") == CryptoError::Ok);
    }

    void recoveryKeyfile()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        const std::array<std::uint8_t, 8> fp = vm.masterKey().fingerprint();
        QVERIFY(!vm.hasRecoveryKey());

        const QString keyfile = dir.filePath("recovery.key");
        QVERIFY(vm.exportRecoveryKeyfile(keyfile) == CryptoError::Ok);
        QVERIFY(vm.hasRecoveryKey());

        vm.lock();
        QVERIFY(vm.unlockWithKeyfile(keyfile) == CryptoError::Ok);
        QVERIFY(vm.isUnlocked());
        QVERIFY(vm.masterKey().fingerprint() == fp);

        vm.lock();
        QVERIFY(vm.unlock("pw") == CryptoError::Ok);  // passphrase still works
    }

    void exportRecoveryRequiresUnlocked()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        vm.lock();
        QVERIFY(vm.exportRecoveryKeyfile(dir.filePath("r.key")) == CryptoError::NotUnlocked);
    }

    void dpapiAutoUnlock()
    {
        QTemporaryDir dir;
        std::array<std::uint8_t, 8> fp{};
        {
            VaultManager vm1;
            vm1.setVaultDir(dir.path());
            QVERIFY(vm1.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::MediumDpapi)
                    == CryptoError::Ok);
            fp = vm1.masterKey().fingerprint();
        }
        VaultManager vm2;
        vm2.setVaultDir(dir.path());
        QVERIFY(!vm2.isUnlocked());
        QVERIFY(vm2.tryAutoUnlock());
        QVERIFY(vm2.isUnlocked());
        QVERIFY(vm2.masterKey().fingerprint() == fp);
    }

    void highLevelNoAutoUnlock()
    {
        QTemporaryDir dir;
        {
            VaultManager vm1;
            vm1.setVaultDir(dir.path());
            QVERIFY(vm1.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                    == CryptoError::Ok);
        }
        VaultManager vm2;
        vm2.setVaultDir(dir.path());
        QVERIFY(!vm2.tryAutoUnlock());
        QVERIFY(!vm2.isUnlocked());
        QVERIFY(vm2.unlock("pw") == CryptoError::Ok);
    }

    void autoLockMinutesPersist()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);
        QCOMPARE(vm.autoLockMinutes(), 15);
        vm.setAutoLockMinutes(30);
        VaultManager vm2;
        vm2.setVaultDir(dir.path());
        QCOMPARE(vm2.autoLockMinutes(), 30);  // persisted to meta
    }

    void deleteVaultRemovesConfig()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::MediumDpapi)
                == CryptoError::Ok);
        QVERIFY(vm.vaultExists());
        QVERIFY(vm.deleteVault() == CryptoError::Ok);
        QVERIFY(!vm.vaultExists());
        QVERIFY(!vm.isUnlocked());
        QVERIFY(vm.state() == VaultManager::State::NoVault);
        // A fresh manager on the same dir sees no vault.
        VaultManager vm2;
        vm2.setVaultDir(dir.path());
        QVERIFY(!vm2.vaultExists());
        QVERIFY(!vm2.tryAutoUnlock());
    }

    void integrationWithEngine()
    {
        QTemporaryDir dir;
        VaultManager vm;
        vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pw", testKdf(), VaultManager::StorageLevel::HighRamOnly)
                == CryptoError::Ok);

        EncryptionEngine eng;
        std::vector<std::uint8_t> pt(5000);
        randombytes_buf(pt.data(), pt.size());
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "doc", vm.masterKey(), ct)
                == CryptoError::Ok);
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), vm.masterKey(), dec) == CryptoError::Ok);
        QVERIFY(dec == pt);
    }
};

QTEST_APPLESS_MAIN(TestVaultManager)
#include "test_vault_manager.moc"
