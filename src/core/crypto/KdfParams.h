// SPDX-License-Identifier: Proprietary
#pragma once

#include <cstdint>

namespace fsnext::crypto {

/// Argon2id key-derivation parameters (libsodium crypto_pwhash). libsodium
/// fixes internal parallelism for Argon2id, so `parallelism` is stored for
/// the on-disk header only (informational) and is not passed to the KDF.
struct KdfParams {
    std::uint32_t opsLimit    = 3;       // time cost (iterations)
    std::uint32_t memLimitKb  = 65536;   // memory cost, in KiB (64 MiB)
    std::uint8_t  parallelism = 1;       // header metadata only

    // The three user-facing strength presets (UI: Cân bằng / Mạnh / Tối đa).
    static KdfParams balanced() { return {3,   65536, 1}; }   // 64 MiB
    static KdfParams strong()   { return {4,  262144, 1}; }   // 256 MiB
    static KdfParams maximum()  { return {4, 1048576, 1}; }   // 1 GiB
};

} // namespace fsnext::crypto
