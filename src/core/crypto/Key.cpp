// SPDX-License-Identifier: Proprietary
#include "core/crypto/Key.h"

#include <sodium.h>

#include <utility>

namespace fsnext::crypto {

Key Key::random()
{
    Key k;
    k.bytes_ = SecureBytes(Size);
    randombytes_buf(k.bytes_.data(), Size);
    return k;
}

Key Key::fromBytes(const std::uint8_t *data, std::size_t n)
{
    Key k;
    if (data && n == Size)
        k.bytes_ = SecureBytes(data, n);
    return k;
}

CryptoError Key::fromPassphrase(const std::string &passphrase,
                                const std::uint8_t salt[16],
                                const KdfParams &params, Key &out)
{
    SecureBytes derived(Size);
    if (crypto_pwhash(derived.data(), Size,
                      passphrase.data(), passphrase.size(),
                      salt,
                      static_cast<unsigned long long>(params.opsLimit),
                      static_cast<std::size_t>(params.memLimitKb) * 1024u,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        // crypto_pwhash returns -1 typically when memlimit can't be allocated.
        return CryptoError::KdfFailed;
    }
    out.bytes_ = std::move(derived);
    return CryptoError::Ok;
}

std::array<std::uint8_t, 8> Key::fingerprint() const
{
    std::array<std::uint8_t, 8> fp{};
    if (valid())
        crypto_generichash(fp.data(), fp.size(), bytes_.data(), Size, nullptr, 0);
    return fp;
}

} // namespace fsnext::crypto
