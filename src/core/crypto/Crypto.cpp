// SPDX-License-Identifier: Proprietary
#include "core/crypto/Crypto.h"

#include <sodium.h>

namespace fsnext::crypto {

bool init()
{
    // sodium_init(): returns 0 on the first successful call, 1 if the library
    // was already initialized (still safe to use), and -1 on failure.
    return sodium_init() >= 0;
}

bool aes256GcmAvailable()
{
    return crypto_aead_aes256gcm_is_available() == 1;
}

} // namespace fsnext::crypto
