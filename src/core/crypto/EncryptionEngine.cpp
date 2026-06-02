// SPDX-License-Identifier: Proprietary
#include "core/crypto/EncryptionEngine.h"

#include "core/crypto/SecureBytes.h"

#include <sodium.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>

namespace fsnext::crypto {

namespace {

namespace fs = std::filesystem;

// Decrypt-side safety caps against malformed/hostile headers.
constexpr std::uint64_t kAesMaxPlain = 64ull * 1024 * 1024;  // AES single-shot read ceiling
constexpr std::uint32_t kChunkMin    = 1024;                 // 1 KiB
constexpr std::uint32_t kChunkMax    = 8u * 1024 * 1024;     // 8 MiB

bool readExact(std::istream &in, std::uint8_t *buf, std::size_t n)
{
    if (n == 0)
        return true;
    in.read(reinterpret_cast<char *>(buf), static_cast<std::streamsize>(n));
    return static_cast<std::size_t>(in.gcount()) == n;
}

std::string basenameUtf8(const std::string &path)
{
    const std::size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

CryptoError encryptStream(std::istream &in, std::uint64_t inSize, std::ostream &out,
                          const Key &wrappingKey, const EncryptOptions &opts,
                          const std::string &filename)
{
    if (!wrappingKey.valid())
        return CryptoError::InternalError;

    const bool aesAvail = (crypto_aead_aes256gcm_is_available() == 1);
    const bool useAes   = !opts.forceXChaCha && aesAvail
                          && inSize <= EncryptionEngine::SmallFileThreshold;

    // Per-file Data Encryption Key.
    SecureBytes dek(32);
    randombytes_buf(dek.data(), 32);

    FencHeader h;
    h.flags = opts.standalone ? fenc::FlagStandalone : fenc::FlagVaultMode;
    if (opts.standalone) {
        std::memcpy(h.kdfSalt, opts.kdfSalt.data(), 16);
        h.kdfOps      = opts.kdf.opsLimit;
        h.kdfMemKb    = opts.kdf.memLimitKb;
        h.kdfParallel = opts.kdf.parallelism;
    }
    randombytes_buf(h.wrapNonce, fenc::WrapNonceSize);
    if (crypto_secretbox_easy(h.wrappedDek, dek.data(), 32, h.wrapNonce, wrappingKey.data()) != 0)
        return CryptoError::InternalError;
    h.originalSize = inSize;
    // Clamp the stored display name to the same bound the parser enforces, so a
    // pathological (very long) name can't produce a .fshenc that encrypts fine
    // but fails to decrypt (parseFixedHeader rejects filenameLen > MaxFilenameLen).
    // Real names are < 255 chars, so this never affects normal use.
    h.filename = filename.size() > fenc::MaxFilenameLen
                     ? filename.substr(0, fenc::MaxFilenameLen)
                     : filename;

    crypto_secretstream_xchacha20poly1305_state st;
    if (useAes) {
        h.algoId = fenc::AlgoAes256Gcm;
        h.flags |= fenc::FlagSingleShot;
        h.chunkSize = 0;
        randombytes_buf(h.bodyHeader, crypto_aead_aes256gcm_NPUBBYTES);  // 12-byte GCM nonce
    } else {
        h.algoId    = fenc::AlgoXChaCha20Poly1305;
        h.chunkSize = (inSize > EncryptionEngine::LargeFileThreshold)
                          ? EncryptionEngine::ChunkLarge
                          : EncryptionEngine::ChunkDefault;
        if (crypto_secretstream_xchacha20poly1305_init_push(&st, h.bodyHeader, dek.data()) != 0)
            return CryptoError::InternalError;
    }

    // The serialized header doubles as the AEAD associated data (binds the
    // ciphertext to every header field — tampering any of them fails auth).
    std::vector<std::uint8_t> headerBytes;
    serializeHeader(h, headerBytes);
    out.write(reinterpret_cast<const char *>(headerBytes.data()),
              static_cast<std::streamsize>(headerBytes.size()));
    if (!out)
        return CryptoError::IoError;

    if (useAes) {
        std::vector<std::uint8_t> pt(static_cast<std::size_t>(inSize));
        if (inSize > 0 && !readExact(in, pt.data(), pt.size()))
            return CryptoError::IoError;
        std::vector<std::uint8_t> ct(pt.size() + crypto_aead_aes256gcm_ABYTES);
        unsigned long long ctlen = 0;
        if (crypto_aead_aes256gcm_encrypt(ct.data(), &ctlen, pt.data(), pt.size(),
                                          headerBytes.data(), headerBytes.size(),
                                          nullptr, h.bodyHeader, dek.data()) != 0) {
            sodium_memzero(pt.data(), pt.size());
            return CryptoError::InternalError;
        }
        sodium_memzero(pt.data(), pt.size());
        out.write(reinterpret_cast<const char *>(ct.data()), static_cast<std::streamsize>(ctlen));
        if (!out)
            return CryptoError::IoError;
    } else {
        const std::uint32_t chunk = h.chunkSize;
        std::vector<std::uint8_t> inBuf(chunk);
        std::vector<std::uint8_t> outBuf(static_cast<std::size_t>(chunk)
                                         + crypto_secretstream_xchacha20poly1305_ABYTES);
        std::uint64_t remaining = inSize;
        bool first = true;
        for (;;) {
            const std::uint32_t toRead =
                static_cast<std::uint32_t>(std::min<std::uint64_t>(remaining, chunk));
            if (toRead > 0) {
                in.read(reinterpret_cast<char *>(inBuf.data()), toRead);
                if (static_cast<std::uint32_t>(in.gcount()) != toRead)
                    return CryptoError::IoError;
            }
            remaining -= toRead;
            const bool isFinal = (remaining == 0);
            const unsigned char tag = isFinal
                ? crypto_secretstream_xchacha20poly1305_TAG_FINAL
                : crypto_secretstream_xchacha20poly1305_TAG_MESSAGE;
            const unsigned char *ad = first ? headerBytes.data() : nullptr;
            const unsigned long long adlen = first ? headerBytes.size() : 0;
            unsigned long long outlen = 0;
            if (crypto_secretstream_xchacha20poly1305_push(&st, outBuf.data(), &outlen,
                                                           inBuf.data(), toRead,
                                                           ad, adlen, tag) != 0)
                return CryptoError::InternalError;
            const std::uint8_t lenLe[4] = {
                static_cast<std::uint8_t>(outlen),
                static_cast<std::uint8_t>(outlen >> 8),
                static_cast<std::uint8_t>(outlen >> 16),
                static_cast<std::uint8_t>(outlen >> 24),
            };
            out.write(reinterpret_cast<const char *>(lenLe), 4);
            out.write(reinterpret_cast<const char *>(outBuf.data()),
                      static_cast<std::streamsize>(outlen));
            if (!out)
                return CryptoError::IoError;
            first = false;
            if (isFinal)
                break;
        }
    }
    return CryptoError::Ok;  // dek wiped by SecureBytes destructor
}

CryptoError decryptStream(std::istream &in, std::ostream &out, const Key &wrappingKey,
                          FencHeader *headerOut)
{
    if (!wrappingKey.valid())
        return CryptoError::InternalError;

    std::uint8_t fixed[fenc::FixedHeaderSize];
    if (!readExact(in, fixed, sizeof(fixed)))
        return CryptoError::InvalidFormat;
    FencHeader h;
    if (const CryptoError pe = parseFixedHeader(fixed, sizeof(fixed), h); pe != CryptoError::Ok)
        return pe;

    // Reconstruct the exact header bytes for use as AAD.
    std::vector<std::uint8_t> headerBytes(fixed, fixed + sizeof(fixed));
    if (h.filenameLen > 0) {
        std::vector<std::uint8_t> fn(h.filenameLen);
        if (!readExact(in, fn.data(), fn.size()))
            return CryptoError::InvalidFormat;
        h.filename.assign(reinterpret_cast<const char *>(fn.data()), fn.size());
        headerBytes.insert(headerBytes.end(), fn.begin(), fn.end());
    }
    if (headerOut)
        *headerOut = h;

    SecureBytes dek(32);
    if (crypto_secretbox_open_easy(dek.data(), h.wrappedDek, fenc::WrappedDekSize,
                                   h.wrapNonce, wrappingKey.data()) != 0)
        return CryptoError::WrongKey;

    if (h.algoId == fenc::AlgoAes256Gcm) {
        if (crypto_aead_aes256gcm_is_available() != 1)
            return CryptoError::AlgoUnavailable;
        if (h.originalSize > kAesMaxPlain)
            return CryptoError::InvalidFormat;
        const std::size_t ctlen =
            static_cast<std::size_t>(h.originalSize) + crypto_aead_aes256gcm_ABYTES;
        std::vector<std::uint8_t> ct(ctlen);
        if (!readExact(in, ct.data(), ct.size()))
            return CryptoError::InvalidFormat;
        std::vector<std::uint8_t> pt(static_cast<std::size_t>(h.originalSize));
        unsigned long long ptlen = 0;
        if (crypto_aead_aes256gcm_decrypt(pt.data(), &ptlen, nullptr, ct.data(), ct.size(),
                                          headerBytes.data(), headerBytes.size(),
                                          h.bodyHeader, dek.data()) != 0)
            return CryptoError::Tampered;
        if (ptlen > 0)
            out.write(reinterpret_cast<const char *>(pt.data()),
                      static_cast<std::streamsize>(ptlen));
        sodium_memzero(pt.data(), pt.size());
        if (!out)
            return CryptoError::IoError;
    } else if (h.algoId == fenc::AlgoXChaCha20Poly1305) {
        if (h.chunkSize < kChunkMin || h.chunkSize > kChunkMax)
            return CryptoError::InvalidFormat;
        crypto_secretstream_xchacha20poly1305_state st;
        if (crypto_secretstream_xchacha20poly1305_init_pull(&st, h.bodyHeader, dek.data()) != 0)
            return CryptoError::Tampered;
        const std::uint32_t maxCt = h.chunkSize + crypto_secretstream_xchacha20poly1305_ABYTES;
        std::vector<std::uint8_t> ct(maxCt);
        std::vector<std::uint8_t> pt(h.chunkSize);
        bool first = true;
        bool sawFinal = false;
        for (;;) {
            std::uint8_t lenLe[4];
            if (!readExact(in, lenLe, 4))
                break;  // clean EOF — validity checked via sawFinal below
            const std::uint32_t clen = static_cast<std::uint32_t>(lenLe[0])
                                       | (static_cast<std::uint32_t>(lenLe[1]) << 8)
                                       | (static_cast<std::uint32_t>(lenLe[2]) << 16)
                                       | (static_cast<std::uint32_t>(lenLe[3]) << 24);
            if (clen < crypto_secretstream_xchacha20poly1305_ABYTES || clen > maxCt)
                return CryptoError::InvalidFormat;
            if (!readExact(in, ct.data(), clen))
                return CryptoError::InvalidFormat;
            const unsigned char *ad = first ? headerBytes.data() : nullptr;
            const unsigned long long adlen = first ? headerBytes.size() : 0;
            unsigned long long ptlen = 0;
            unsigned char tag = 0;
            if (crypto_secretstream_xchacha20poly1305_pull(&st, pt.data(), &ptlen, &tag,
                                                           ct.data(), clen, ad, adlen) != 0) {
                sodium_memzero(pt.data(), pt.size());
                return CryptoError::Tampered;
            }
            if (ptlen > 0)
                out.write(reinterpret_cast<const char *>(pt.data()),
                          static_cast<std::streamsize>(ptlen));
            first = false;
            if (!out) {
                sodium_memzero(pt.data(), pt.size());
                return CryptoError::IoError;
            }
            if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
                sawFinal = true;
                break;
            }
        }
        sodium_memzero(pt.data(), pt.size());
        if (!sawFinal)
            return CryptoError::Tampered;  // stream truncated before TAG_FINAL
    } else {
        return CryptoError::UnsupportedVersion;
    }
    return CryptoError::Ok;
}

} // namespace

CryptoError EncryptionEngine::encryptFile(const std::string &inPath, const std::string &outPath,
                                          const Key &wrappingKey, const EncryptOptions &opts)
{
    std::error_code ec;
    const fs::path inP = fs::u8path(inPath);
    const std::uint64_t size = fs::file_size(inP, ec);
    if (ec)
        return CryptoError::IoError;
    std::ifstream in(inP, std::ios::binary);
    if (!in)
        return CryptoError::IoError;
    std::ofstream outF(fs::u8path(outPath), std::ios::binary | std::ios::trunc);
    if (!outF)
        return CryptoError::IoError;

    const std::string fname =
        opts.storedFilename.empty() ? basenameUtf8(inPath) : opts.storedFilename;
    CryptoError e = encryptStream(in, size, outF, wrappingKey, opts, fname);
    outF.flush();
    if (e == CryptoError::Ok && !outF)
        e = CryptoError::IoError;
    if (e != CryptoError::Ok) {
        // Fail-closed: never leave a partial .fshenc behind (spec §1.5).
        outF.close();
        std::error_code rmEc;
        fs::remove(fs::u8path(outPath), rmEc);
    }
    return e;
}

CryptoError EncryptionEngine::decryptFile(const std::string &inPath, const std::string &outPath,
                                          const Key &wrappingKey, FencHeader *headerOut)
{
    std::ifstream in(fs::u8path(inPath), std::ios::binary);
    if (!in)
        return CryptoError::IoError;
    std::ofstream outF(fs::u8path(outPath), std::ios::binary | std::ios::trunc);
    if (!outF)
        return CryptoError::IoError;
    CryptoError e = decryptStream(in, outF, wrappingKey, headerOut);
    outF.flush();
    if (e == CryptoError::Ok && !outF)
        e = CryptoError::IoError;
    if (e != CryptoError::Ok) {
        // Fail-closed: a wrong key / tampered / truncated input must not leave a
        // partial plaintext on disk (spec §1.5), independent of any caller cleanup.
        outF.close();
        std::error_code rmEc;
        fs::remove(fs::u8path(outPath), rmEc);
    }
    return e;
}

CryptoError EncryptionEngine::encryptBuffer(const std::uint8_t *in, std::size_t n,
                                            const std::string &name, const Key &wrappingKey,
                                            std::vector<std::uint8_t> &out,
                                            const EncryptOptions &opts)
{
    out.clear();
    std::string input;
    if (n > 0 && in)
        input.assign(reinterpret_cast<const char *>(in), n);
    std::istringstream is(input, std::ios::binary);
    std::ostringstream os(std::ios::binary);
    CryptoError e = encryptStream(is, n, os, wrappingKey, opts, name);
    if (e == CryptoError::Ok) {
        const std::string s = os.str();
        out.assign(s.begin(), s.end());
    }
    return e;
}

CryptoError EncryptionEngine::decryptBuffer(const std::uint8_t *in, std::size_t n,
                                            const Key &wrappingKey,
                                            std::vector<std::uint8_t> &out, FencHeader *headerOut)
{
    out.clear();
    std::string input;
    if (n > 0 && in)
        input.assign(reinterpret_cast<const char *>(in), n);
    std::istringstream is(input, std::ios::binary);
    std::ostringstream os(std::ios::binary);
    CryptoError e = decryptStream(is, os, wrappingKey, headerOut);
    if (e == CryptoError::Ok) {
        const std::string s = os.str();
        out.assign(s.begin(), s.end());
    }
    return e;
}

CryptoError EncryptionEngine::readHeader(const std::string &inPath, FencHeader &outH)
{
    std::ifstream in(fs::u8path(inPath), std::ios::binary);
    if (!in)
        return CryptoError::IoError;
    std::uint8_t fixed[fenc::FixedHeaderSize];
    if (!readExact(in, fixed, sizeof(fixed)))
        return CryptoError::InvalidFormat;
    if (const CryptoError e = parseFixedHeader(fixed, sizeof(fixed), outH); e != CryptoError::Ok)
        return e;
    if (outH.filenameLen > 0) {
        std::vector<std::uint8_t> fn(outH.filenameLen);
        if (!readExact(in, fn.data(), fn.size()))
            return CryptoError::InvalidFormat;
        outH.filename.assign(reinterpret_cast<const char *>(fn.data()), fn.size());
    }
    return CryptoError::Ok;
}

bool EncryptionEngine::isFencFile(const std::string &inPath)
{
    std::ifstream in(fs::u8path(inPath), std::ios::binary);
    if (!in)
        return false;
    std::uint8_t m[4];
    return readExact(in, m, 4) && hasMagic(m, 4);
}

} // namespace fsnext::crypto
