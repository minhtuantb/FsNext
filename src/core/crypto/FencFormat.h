// SPDX-License-Identifier: Proprietary
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/crypto/CryptoError.h"

namespace fsnext::crypto {

// ── .fshenc on-disk format constants ──────────────────────────────────────
// Full byte layout in docs/specs/encryption-plan.md §7.
namespace fenc {

inline constexpr char    Magic[4] = {'F', 'E', 'N', 'C'};
inline constexpr uint8_t Version  = 0x01;

inline constexpr uint8_t AlgoXChaCha20Poly1305 = 0x01;  // secretstream (large / streaming)
inline constexpr uint8_t AlgoAes256Gcm         = 0x02;  // single-shot (small files, AES-NI)

inline constexpr uint16_t FlagFilenameEncrypted = 1u << 0;
inline constexpr uint16_t FlagVaultMode         = 1u << 1;
inline constexpr uint16_t FlagSingleShot        = 1u << 2;
inline constexpr uint16_t FlagStandalone        = 1u << 3;

inline constexpr std::size_t FixedHeaderSize = 146;  // bytes before FILENAME
inline constexpr std::size_t WrapNonceSize   = 24;   // crypto_secretbox_NONCEBYTES
inline constexpr std::size_t WrappedDekSize  = 48;   // 32-byte DEK + 16-byte secretbox MAC
inline constexpr std::size_t BodyHeaderSize  = 24;   // secretstream header OR 12-byte AES nonce
inline constexpr std::size_t MaxFilenameLen  = 4096; // defensive cap on untrusted input

} // namespace fenc

/// Parsed / to-be-serialized .fshenc header. Fixed-size fields plus a
/// variable-length filename.
struct FencHeader {
    std::uint8_t  version  = fenc::Version;
    std::uint8_t  algoId   = fenc::AlgoAes256Gcm;
    std::uint16_t flags    = 0;
    std::uint8_t  kdfSalt[16] = {};
    std::uint32_t kdfOps   = 0;
    std::uint32_t kdfMemKb = 0;
    std::uint8_t  kdfParallel = 1;
    std::uint8_t  wrapNonce[fenc::WrapNonceSize]   = {};  // secretbox nonce for the wrapped DEK
    std::uint8_t  wrappedDek[fenc::WrappedDekSize] = {};  // DEK sealed with the wrapping key
    std::uint8_t  bodyHeader[fenc::BodyHeaderSize] = {};  // XChaCha stream header, or AES 12B nonce
    std::uint64_t originalSize = 0;
    std::uint32_t chunkSize    = 0;   // XChaCha plaintext chunk size; 0 for AES single-shot
    std::uint16_t filenameLen  = 0;   // length as read from disk (parse only)
    std::string   filename;           // UTF-8; serialize uses filename.size()
};

/// Serialize the fixed header + filename into `out` (cleared first).
void serializeHeader(const FencHeader &h, std::vector<std::uint8_t> &out);

/// Validate and parse the 146-byte fixed header from `d` (len must be at least
/// FixedHeaderSize). Fills the fixed fields and `filenameLen`; leaves
/// `filename` empty — the caller reads `filenameLen` bytes next. Defensive
/// against untrusted/malformed input.
CryptoError parseFixedHeader(const std::uint8_t *d, std::size_t len, FencHeader &out);

/// True if the first bytes carry the .fshenc magic.
bool hasMagic(const std::uint8_t *d, std::size_t len);

} // namespace fsnext::crypto
