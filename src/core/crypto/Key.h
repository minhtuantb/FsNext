// SPDX-License-Identifier: Proprietary
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "core/crypto/CryptoError.h"
#include "core/crypto/KdfParams.h"
#include "core/crypto/SecureBytes.h"

namespace fsnext::crypto {

/// A 32-byte symmetric key held in a SecureBytes buffer (mlock'd, wiped on
/// destruction). Used both as the DEK-wrapping key (Master Key / passphrase
/// KEK) and, in standalone mode, derived directly from a passphrase.
///
/// Move-only — a key has exactly one owner.
class Key {
public:
    static constexpr std::size_t Size = 32;  // matches AES-256 / XChaCha20 / secretbox key sizes

    Key() = default;

    /// A fresh cryptographically-random key.
    static Key random();

    /// Wrap exactly 32 raw bytes. Returns an invalid (empty) Key if n != 32
    /// or data is null.
    static Key fromBytes(const std::uint8_t *data, std::size_t n);

    /// Derive a key from a passphrase using Argon2id with the given 16-byte
    /// salt and parameters. Writes into `out`; returns KdfFailed on error.
    static CryptoError fromPassphrase(const std::string &passphrase,
                                      const std::uint8_t salt[16],
                                      const KdfParams &params, Key &out);

    Key(Key &&) noexcept = default;
    Key &operator=(Key &&) noexcept = default;
    Key(const Key &) = delete;
    Key &operator=(const Key &) = delete;

    const std::uint8_t *data() const noexcept { return bytes_.data(); }
    std::size_t         size() const noexcept { return bytes_.size(); }
    bool                valid() const noexcept { return bytes_.size() == Size; }

    /// Short, one-way identifier (BLAKE2b-64 of the key) for display / matching
    /// in the key manager. Never the key itself. Returns zeros if invalid.
    std::array<std::uint8_t, 8> fingerprint() const;

private:
    SecureBytes bytes_;
};

} // namespace fsnext::crypto
