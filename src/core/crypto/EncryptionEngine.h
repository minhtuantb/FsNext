// SPDX-License-Identifier: Proprietary
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/crypto/CryptoError.h"
#include "core/crypto/FencFormat.h"
#include "core/crypto/KdfParams.h"
#include "core/crypto/Key.h"

namespace fsnext::crypto {

/// Per-operation knobs for encryption. Defaults = vault mode, auto algorithm.
struct EncryptOptions {
    bool                        forceXChaCha = false;  // never use AES (portability mode)
    bool                        standalone   = false;  // embed KDF salt + params in header (CLI)
    KdfParams                   kdf{};                 // used only when standalone
    std::array<std::uint8_t, 16> kdfSalt{};            // used only when standalone
    std::string                 storedFilename;        // name to embed; empty => basename(inPath)
};

/// Encrypts/decrypts files and buffers in the .fshenc format. Stateless —
/// safe to call from any thread once fsnext::crypto::init() has run.
///
/// Algorithm/mode is chosen automatically by size (see encryption-plan.md §3):
///   * file <= 1 MiB and AES-NI present  -> AES-256-GCM single-shot
///   * otherwise                         -> XChaCha20-Poly1305 secretstream
class EncryptionEngine {
public:
    static constexpr std::uint64_t SmallFileThreshold = 1ull << 20;   // 1 MiB
    static constexpr std::uint64_t LargeFileThreshold = 1ull << 30;   // 1 GiB
    static constexpr std::uint32_t ChunkDefault       = 256u * 1024;  // 256 KiB
    static constexpr std::uint32_t ChunkLarge         = 1024u * 1024; // 1 MiB

    CryptoError encryptFile(const std::string &inPath, const std::string &outPath,
                            const Key &wrappingKey, const EncryptOptions &opts = {});
    CryptoError decryptFile(const std::string &inPath, const std::string &outPath,
                            const Key &wrappingKey, FencHeader *headerOut = nullptr);

    CryptoError encryptBuffer(const std::uint8_t *in, std::size_t n, const std::string &name,
                              const Key &wrappingKey, std::vector<std::uint8_t> &out,
                              const EncryptOptions &opts = {});
    CryptoError decryptBuffer(const std::uint8_t *in, std::size_t n, const Key &wrappingKey,
                              std::vector<std::uint8_t> &out, FencHeader *headerOut = nullptr);

    /// Read just the header (no body decryption, no key needed).
    CryptoError readHeader(const std::string &inPath, FencHeader &out);

    /// Cheap magic-bytes check.
    bool isFencFile(const std::string &inPath);
};

} // namespace fsnext::crypto
