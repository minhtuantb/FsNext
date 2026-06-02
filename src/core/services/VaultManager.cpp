// SPDX-License-Identifier: Proprietary
#include "core/services/VaultManager.h"

#include "core/crypto/SecureBytes.h"
#include "core/util/SecureStore.h"

#include <sodium.h>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>

#include <cstring>
#include <vector>

namespace fsnext {

using crypto::CryptoError;
using crypto::Key;
using crypto::KdfParams;
using crypto::SecureBytes;

namespace {

constexpr int kMasterKeyBytes = 32;

CryptoError wrapMaster(const Key &kek, const std::uint8_t *master32,
                       std::vector<std::uint8_t> &wrapped, std::vector<std::uint8_t> &nonce)
{
    nonce.assign(crypto_secretbox_NONCEBYTES, 0);
    randombytes_buf(nonce.data(), nonce.size());
    wrapped.assign(kMasterKeyBytes + crypto_secretbox_MACBYTES, 0);
    if (crypto_secretbox_easy(wrapped.data(), master32, kMasterKeyBytes, nonce.data(), kek.data())
        != 0)
        return CryptoError::InternalError;
    return CryptoError::Ok;
}

CryptoError unwrapMaster(const Key &kek, const std::vector<std::uint8_t> &wrapped,
                         const std::vector<std::uint8_t> &nonce, SecureBytes &out32)
{
    if (wrapped.size() != std::size_t(kMasterKeyBytes + crypto_secretbox_MACBYTES)
        || nonce.size() != crypto_secretbox_NONCEBYTES)
        return CryptoError::InvalidFormat;
    out32 = SecureBytes(kMasterKeyBytes);
    if (crypto_secretbox_open_easy(out32.data(), wrapped.data(), wrapped.size(), nonce.data(),
                                   kek.data())
        != 0)
        return CryptoError::WrongKey;
    return CryptoError::Ok;
}

} // namespace

void VaultManager::setVaultDir(const QString &dir)
{
    vaultDir_   = dir;
    meta_       = crypto::VaultMetadata{};
    metaLoaded_ = false;
    masterKey_  = Key{};
    state_      = vaultExists() ? State::Locked : State::NoVault;
}

QString VaultManager::metaPath() const { return vaultDir_ + QStringLiteral("/.vault-meta.json"); }
QString VaultManager::dpapiPath() const { return vaultDir_ + QStringLiteral("/.vault-key.dpapi"); }
QString VaultManager::plainPath() const { return vaultDir_ + QStringLiteral("/.vault-key.plain"); }

bool VaultManager::vaultExists() const
{
    return !vaultDir_.isEmpty() && QFile::exists(metaPath());
}

bool VaultManager::ensureMetaLoaded()
{
    if (metaLoaded_)
        return true;
    if (!vaultExists())
        return false;
    if (crypto::VaultMetadata::load(metaPath().toStdString(), meta_) != CryptoError::Ok)
        return false;
    metaLoaded_ = true;
    level_      = static_cast<StorageLevel>(meta_.storageLevel);
    return true;
}

CryptoError VaultManager::createVault(const QString &name, const QString &passphrase,
                                      const KdfParams &kdf, StorageLevel level)
{
    if (vaultDir_.isEmpty())
        return CryptoError::InternalError;
    QDir().mkpath(vaultDir_);
    if (vaultExists())
        return CryptoError::AlreadyExists;

    SecureBytes master(kMasterKeyBytes);
    randombytes_buf(master.data(), kMasterKeyBytes);

    std::uint8_t salt[16];
    randombytes_buf(salt, sizeof(salt));
    Key kek;
    if (Key::fromPassphrase(passphrase.toStdString(), salt, kdf, kek) != CryptoError::Ok)
        return CryptoError::KdfFailed;

    std::vector<std::uint8_t> wrapped, nonce;
    if (CryptoError e = wrapMaster(kek, master.data(), wrapped, nonce); e != CryptoError::Ok)
        return e;

    meta_              = crypto::VaultMetadata{};
    meta_.version      = 1;
    meta_.vaultId      = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    meta_.name         = name.toStdString();
    meta_.createdAt    = QDateTime::currentSecsSinceEpoch();
    meta_.autoLockMin  = 15;
    meta_.kdfOps       = kdf.opsLimit;
    meta_.kdfMemKb     = kdf.memLimitKb;
    meta_.kdfParallel  = kdf.parallelism;
    std::memcpy(meta_.kdfSalt, salt, 16);
    meta_.wrappedMasterKey = wrapped;
    meta_.wrapNonce        = nonce;
    meta_.storageLevel     = static_cast<int>(level);
    if (meta_.save(metaPath().toStdString()) != CryptoError::Ok)
        return CryptoError::IoError;

    metaLoaded_ = true;
    level_      = level;
    masterKey_  = Key::fromBytes(master.data(), kMasterKeyBytes);
    state_      = State::Unlocked;
    persistForAutoUnlock();
    return CryptoError::Ok;
}

CryptoError VaultManager::unlock(const QString &passphrase)
{
    if (!ensureMetaLoaded())
        return CryptoError::InvalidFormat;
    KdfParams p{meta_.kdfOps, meta_.kdfMemKb, meta_.kdfParallel};
    Key kek;
    if (Key::fromPassphrase(passphrase.toStdString(), meta_.kdfSalt, p, kek) != CryptoError::Ok)
        return CryptoError::KdfFailed;
    SecureBytes master;
    if (CryptoError e = unwrapMaster(kek, meta_.wrappedMasterKey, meta_.wrapNonce, master);
        e != CryptoError::Ok)
        return e;
    masterKey_ = Key::fromBytes(master.data(), kMasterKeyBytes);
    state_     = State::Unlocked;
    persistForAutoUnlock();
    return CryptoError::Ok;
}

CryptoError VaultManager::unlockWithKeyfile(const QString &keyfilePath)
{
    if (!ensureMetaLoaded())
        return CryptoError::InvalidFormat;
    if (!meta_.hasRecoveryKey)
        return CryptoError::InvalidFormat;
    QFile f(keyfilePath);
    if (!f.open(QIODevice::ReadOnly))
        return CryptoError::IoError;
    QByteArray raw = f.read(64);
    f.close();
    if (raw.size() != kMasterKeyBytes) {
        sodium_memzero(raw.data(), static_cast<size_t>(raw.size()));
        return CryptoError::InvalidFormat;
    }
    Key rkey = Key::fromBytes(reinterpret_cast<const std::uint8_t *>(raw.constData()),
                              kMasterKeyBytes);
    sodium_memzero(raw.data(), static_cast<size_t>(raw.size()));  // key now lives in rkey (SecureBytes)
    SecureBytes master;
    if (CryptoError e = unwrapMaster(rkey, meta_.wrappedMasterKeyRecovery, meta_.recoveryNonce,
                                     master);
        e != CryptoError::Ok)
        return e;
    masterKey_ = Key::fromBytes(master.data(), kMasterKeyBytes);
    state_     = State::Unlocked;
    persistForAutoUnlock();
    return CryptoError::Ok;
}

void VaultManager::lock()
{
    masterKey_ = Key{};  // wipes the previous key via SecureBytes
    state_     = vaultExists() ? State::Locked : State::NoVault;
}

CryptoError VaultManager::changePassphrase(const QString &oldPass, const QString &newPass,
                                           const KdfParams &kdf)
{
    if (!ensureMetaLoaded())
        return CryptoError::InvalidFormat;
    KdfParams oldP{meta_.kdfOps, meta_.kdfMemKb, meta_.kdfParallel};
    Key oldKek;
    if (Key::fromPassphrase(oldPass.toStdString(), meta_.kdfSalt, oldP, oldKek) != CryptoError::Ok)
        return CryptoError::KdfFailed;
    SecureBytes master;
    if (CryptoError e = unwrapMaster(oldKek, meta_.wrappedMasterKey, meta_.wrapNonce, master);
        e != CryptoError::Ok)
        return e;

    std::uint8_t newSalt[16];
    randombytes_buf(newSalt, sizeof(newSalt));
    Key newKek;
    if (Key::fromPassphrase(newPass.toStdString(), newSalt, kdf, newKek) != CryptoError::Ok)
        return CryptoError::KdfFailed;
    std::vector<std::uint8_t> wrapped, nonce;
    if (CryptoError e = wrapMaster(newKek, master.data(), wrapped, nonce); e != CryptoError::Ok)
        return e;

    meta_.kdfOps      = kdf.opsLimit;
    meta_.kdfMemKb    = kdf.memLimitKb;
    meta_.kdfParallel = kdf.parallelism;
    std::memcpy(meta_.kdfSalt, newSalt, 16);
    meta_.wrappedMasterKey = wrapped;
    meta_.wrapNonce        = nonce;
    if (meta_.save(metaPath().toStdString()) != CryptoError::Ok)
        return CryptoError::IoError;
    return CryptoError::Ok;
}

CryptoError VaultManager::exportRecoveryKeyfile(const QString &path)
{
    if (!isUnlocked() || !masterKey_.valid())
        return CryptoError::NotUnlocked;
    SecureBytes recovery(kMasterKeyBytes);
    randombytes_buf(recovery.data(), kMasterKeyBytes);
    Key rkey = Key::fromBytes(recovery.data(), kMasterKeyBytes);

    std::vector<std::uint8_t> wrapped, nonce;
    if (CryptoError e = wrapMaster(rkey, masterKey_.data(), wrapped, nonce); e != CryptoError::Ok)
        return e;
    meta_.hasRecoveryKey           = true;
    meta_.wrappedMasterKeyRecovery = wrapped;
    meta_.recoveryNonce            = nonce;
    if (meta_.save(metaPath().toStdString()) != CryptoError::Ok)
        return CryptoError::IoError;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return CryptoError::IoError;
    const qint64 written =
        f.write(reinterpret_cast<const char *>(recovery.data()), kMasterKeyBytes);
    f.close();
    if (written != kMasterKeyBytes)
        return CryptoError::IoError;
    return CryptoError::Ok;
}

CryptoError VaultManager::deleteVault()
{
    masterKey_ = Key{};  // wipe key from RAM
    QFile::remove(metaPath());
    QFile::remove(dpapiPath());
    QFile::remove(plainPath());
    meta_       = crypto::VaultMetadata{};
    metaLoaded_ = false;
    state_      = State::NoVault;
    return CryptoError::Ok;
}

void VaultManager::persistForAutoUnlock()
{
    if (!masterKey_.valid())
        return;
    if (level_ == StorageLevel::MediumDpapi) {
        QByteArray plain(reinterpret_cast<const char *>(masterKey_.data()), kMasterKeyBytes);
        const QByteArray blob = SecureStore::encrypt(plain);
        sodium_memzero(plain.data(), static_cast<size_t>(plain.size()));
        if (!blob.isEmpty()) {
            QFile f(dpapiPath());
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                f.write(blob);
                f.close();
            }
        }
    } else if (level_ == StorageLevel::Convenience) {
        QFile f(plainPath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(reinterpret_cast<const char *>(masterKey_.data()), kMasterKeyBytes);
            f.close();
        }
    }
}

bool VaultManager::tryAutoUnlock()
{
    if (!ensureMetaLoaded())
        return false;
    if (level_ == StorageLevel::MediumDpapi) {
        QFile f(dpapiPath());
        if (!f.open(QIODevice::ReadOnly))
            return false;
        const QByteArray blob = f.readAll();
        f.close();
        QByteArray master = SecureStore::decrypt(blob);
        if (master.size() != kMasterKeyBytes) {
            sodium_memzero(master.data(), static_cast<size_t>(master.size()));
            return false;
        }
        masterKey_ = Key::fromBytes(reinterpret_cast<const std::uint8_t *>(master.constData()),
                                    kMasterKeyBytes);
        sodium_memzero(master.data(), static_cast<size_t>(master.size()));
        state_ = State::Unlocked;
        return true;
    }
    if (level_ == StorageLevel::Convenience) {
        QFile f(plainPath());
        if (!f.open(QIODevice::ReadOnly))
            return false;
        QByteArray raw = f.read(64);
        f.close();
        if (raw.size() != kMasterKeyBytes) {
            sodium_memzero(raw.data(), static_cast<size_t>(raw.size()));
            return false;
        }
        masterKey_ = Key::fromBytes(reinterpret_cast<const std::uint8_t *>(raw.constData()),
                                    kMasterKeyBytes);
        sodium_memzero(raw.data(), static_cast<size_t>(raw.size()));
        state_ = State::Unlocked;
        return true;
    }
    return false;  // HighRamOnly never auto-unlocks
}

QString VaultManager::vaultName()
{
    return ensureMetaLoaded() ? QString::fromStdString(meta_.name) : QString();
}

int VaultManager::storageLevelValue()
{
    return ensureMetaLoaded() ? meta_.storageLevel : static_cast<int>(StorageLevel::MediumDpapi);
}

bool VaultManager::hasRecoveryKey()
{
    return ensureMetaLoaded() && meta_.hasRecoveryKey;
}

int VaultManager::autoLockMinutes()
{
    return ensureMetaLoaded() ? meta_.autoLockMin : 15;
}

void VaultManager::setAutoLockMinutes(int minutes)
{
    if (!ensureMetaLoaded())
        return;
    meta_.autoLockMin = minutes < 0 ? 0 : minutes;
    meta_.save(metaPath().toStdString());
}

} // namespace fsnext
