// SPDX-License-Identifier: Proprietary
#pragma once

#include <QString>

#include "core/crypto/CryptoError.h"
#include "core/crypto/KdfParams.h"
#include "core/crypto/Key.h"
#include "core/crypto/VaultMetadata.h"

namespace fsnext {

/// Owns the cryptographic lifecycle of one encryption vault: create, unlock
/// (passphrase / recovery keyfile / DPAPI auto-unlock), lock, change
/// passphrase, export recovery keyfile.
///
/// Deliberately a PLAIN class (no QObject / QTimer): the expensive Argon2id
/// step runs on a worker thread via VaultViewModel, and a QObject's timer /
/// signal machinery must not be touched off the main thread. Auto-lock and
/// state notifications therefore live in VaultViewModel. Only uses reentrant
/// QtCore value/util types (QString/QFile/QDir/QUuid/QDateTime), so a single
/// operation at a time is safe to run on any thread.
///
/// Holds the unwrapped Master Key in a SecureBytes-backed crypto::Key while
/// unlocked; wipes it on lock().
class VaultManager
{
public:
    enum class StorageLevel { HighRamOnly = 1, MediumDpapi = 2, Convenience = 3 };
    enum class State { NoVault, Locked, Unlocked };

    VaultManager() = default;

    void    setVaultDir(const QString &dir);
    QString vaultDir() const { return vaultDir_; }
    bool    vaultExists() const;
    State   state() const { return state_; }
    bool    isUnlocked() const { return state_ == State::Unlocked; }

    crypto::CryptoError createVault(const QString &name, const QString &passphrase,
                                    const crypto::KdfParams &kdf, StorageLevel level);
    crypto::CryptoError unlock(const QString &passphrase);
    crypto::CryptoError unlockWithKeyfile(const QString &keyfilePath);
    void                lock();
    crypto::CryptoError changePassphrase(const QString &oldPass, const QString &newPass,
                                         const crypto::KdfParams &kdf);
    crypto::CryptoError exportRecoveryKeyfile(const QString &path);

    /// Remove the vault configuration (meta + stored key blobs) from this
    /// machine and lock. Encrypted `.fshenc` files are NOT touched. After this
    /// the location has no vault (state == NoVault).
    crypto::CryptoError deleteVault();

    /// L2/L3: restore the Master Key without a passphrase. Returns true if the
    /// vault was unlocked. L1 (RAM-only) always returns false.
    bool tryAutoUnlock();

    /// Valid only while unlocked.
    const crypto::Key &masterKey() const { return masterKey_; }

    // ── Metadata accessors (load the meta lazily) ──
    QString vaultName();
    int     storageLevelValue();
    bool    hasRecoveryKey();
    int     autoLockMinutes();
    void    setAutoLockMinutes(int minutes);  // persists to meta

private:
    QString metaPath() const;
    QString dpapiPath() const;
    QString plainPath() const;
    bool    ensureMetaLoaded();
    void    persistForAutoUnlock();

    QString               vaultDir_;
    crypto::VaultMetadata meta_;
    bool                  metaLoaded_ = false;
    crypto::Key           masterKey_;
    State                 state_ = State::NoVault;
    StorageLevel          level_ = StorageLevel::MediumDpapi;
};

} // namespace fsnext
