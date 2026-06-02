// SPDX-License-Identifier: Proprietary
#include "core/crypto/FencFormat.h"

#include <cstring>

namespace fsnext::crypto {

namespace {

void put16(std::vector<std::uint8_t> &v, std::uint16_t x)
{
    v.push_back(static_cast<std::uint8_t>(x));
    v.push_back(static_cast<std::uint8_t>(x >> 8));
}

void put32(std::vector<std::uint8_t> &v, std::uint32_t x)
{
    for (int i = 0; i < 4; ++i)
        v.push_back(static_cast<std::uint8_t>(x >> (8 * i)));
}

void put64(std::vector<std::uint8_t> &v, std::uint64_t x)
{
    for (int i = 0; i < 8; ++i)
        v.push_back(static_cast<std::uint8_t>(x >> (8 * i)));
}

std::uint16_t get16(const std::uint8_t *p)
{
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}

std::uint32_t get32(const std::uint8_t *p)
{
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8)
         | (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint64_t get64(const std::uint8_t *p)
{
    std::uint64_t r = 0;
    for (int i = 0; i < 8; ++i)
        r |= static_cast<std::uint64_t>(p[i]) << (8 * i);
    return r;
}

} // namespace

void serializeHeader(const FencHeader &h, std::vector<std::uint8_t> &out)
{
    out.clear();
    out.reserve(fenc::FixedHeaderSize + h.filename.size());

    const auto *magic = reinterpret_cast<const std::uint8_t *>(fenc::Magic);
    out.insert(out.end(), magic, magic + 4);
    out.push_back(h.version);
    out.push_back(h.algoId);
    put16(out, h.flags);
    out.insert(out.end(), h.kdfSalt, h.kdfSalt + 16);
    put32(out, h.kdfOps);
    put32(out, h.kdfMemKb);
    out.push_back(h.kdfParallel);
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);  // 3 reserved bytes
    out.insert(out.end(), h.wrapNonce, h.wrapNonce + fenc::WrapNonceSize);
    out.insert(out.end(), h.wrappedDek, h.wrappedDek + fenc::WrappedDekSize);
    out.insert(out.end(), h.bodyHeader, h.bodyHeader + fenc::BodyHeaderSize);
    put64(out, h.originalSize);
    put32(out, h.chunkSize);
    put16(out, static_cast<std::uint16_t>(h.filename.size()));

    const auto *fn = reinterpret_cast<const std::uint8_t *>(h.filename.data());
    out.insert(out.end(), fn, fn + h.filename.size());
}

CryptoError parseFixedHeader(const std::uint8_t *d, std::size_t len, FencHeader &h)
{
    if (!d || len < fenc::FixedHeaderSize)
        return CryptoError::InvalidFormat;
    if (std::memcmp(d, fenc::Magic, 4) != 0)
        return CryptoError::InvalidFormat;

    std::size_t o = 4;
    h.version = d[o++];
    if (h.version != fenc::Version)
        return CryptoError::UnsupportedVersion;
    h.algoId = d[o++];
    if (h.algoId != fenc::AlgoXChaCha20Poly1305 && h.algoId != fenc::AlgoAes256Gcm)
        return CryptoError::UnsupportedVersion;
    h.flags = get16(d + o); o += 2;
    std::memcpy(h.kdfSalt, d + o, 16); o += 16;
    h.kdfOps = get32(d + o); o += 4;
    h.kdfMemKb = get32(d + o); o += 4;
    h.kdfParallel = d[o++];
    o += 3;  // reserved
    std::memcpy(h.wrapNonce, d + o, fenc::WrapNonceSize); o += fenc::WrapNonceSize;
    std::memcpy(h.wrappedDek, d + o, fenc::WrappedDekSize); o += fenc::WrappedDekSize;
    std::memcpy(h.bodyHeader, d + o, fenc::BodyHeaderSize); o += fenc::BodyHeaderSize;
    h.originalSize = get64(d + o); o += 8;
    h.chunkSize = get32(d + o); o += 4;
    h.filenameLen = get16(d + o); o += 2;
    // o == FixedHeaderSize (146) here.

    if (h.filenameLen > fenc::MaxFilenameLen)
        return CryptoError::InvalidFormat;
    return CryptoError::Ok;
}

bool hasMagic(const std::uint8_t *d, std::size_t len)
{
    return d && len >= 4 && std::memcmp(d, fenc::Magic, 4) == 0;
}

} // namespace fsnext::crypto
