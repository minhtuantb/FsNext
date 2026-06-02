// SPDX-License-Identifier: Proprietary
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/crypto/CryptoError.h"

namespace fsnext::crypto {

/// On-disk vault descriptor (`.vault-meta.json`). Contains only WRAPPED key
/// material + KDF parameters — it is NOT sensitive (the master key cannot be
/// recovered from it without the passphrase or recovery keyfile), so it may be
/// backed up / synced freely. Pure (jsoncpp + libsodium base64), no Qt.
struct VaultMetadata {
    int           version    = 1;
    std::string   vaultId;
    std::string   name;                    // user-facing vault name
    std::int64_t  createdAt  = 0;          // unix epoch seconds
    int           autoLockMin = 15;        // inactivity auto-lock (minutes)

    // KDF (Argon2id) for the passphrase -> KEK.
    std::uint32_t kdfOps     = 3;
    std::uint32_t kdfMemKb   = 65536;
    std::uint8_t  kdfParallel = 1;
    std::uint8_t  kdfSalt[16] = {};

    // Master key sealed with the passphrase-derived KEK (secretbox: 48 / 24).
    std::vector<std::uint8_t> wrappedMasterKey;
    std::vector<std::uint8_t> wrapNonce;

    // Optional: master key sealed with a recovery keyfile key.
    bool                      hasRecoveryKey = false;
    std::vector<std::uint8_t> wrappedMasterKeyRecovery;
    std::vector<std::uint8_t> recoveryNonce;

    int storageLevel = 2;  // 1=RAM-only, 2=DPAPI, 3=plain (see VaultManager)

    /// Parse from a JSON file. Defensive against malformed input.
    static CryptoError load(const std::string &path, VaultMetadata &out);

    /// Serialize to a JSON file atomically (temp file + rename).
    CryptoError save(const std::string &path) const;
};

} // namespace fsnext::crypto
