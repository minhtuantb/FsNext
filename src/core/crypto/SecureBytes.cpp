// SPDX-License-Identifier: Proprietary
#include "core/crypto/SecureBytes.h"

#include <sodium.h>

#include <cstring>
#include <new>      // std::bad_alloc
#include <utility>  // std::exchange

namespace fsnext::crypto {

SecureBytes::SecureBytes(std::size_t n)
{
    if (n == 0)
        return;
    data_ = static_cast<std::uint8_t *>(sodium_malloc(n));
    if (!data_)
        throw std::bad_alloc();
    size_ = n;
    sodium_memzero(data_, size_);
}

SecureBytes::SecureBytes(const std::uint8_t *src, std::size_t n)
{
    if (n == 0)
        return;
    data_ = static_cast<std::uint8_t *>(sodium_malloc(n));
    if (!data_)
        throw std::bad_alloc();
    size_ = n;
    if (src)
        std::memcpy(data_, src, n);
    else
        sodium_memzero(data_, n);
}

SecureBytes::~SecureBytes()
{
    clear();
}

SecureBytes::SecureBytes(SecureBytes &&other) noexcept
    : data_(std::exchange(other.data_, nullptr)),
      size_(std::exchange(other.size_, std::size_t{0}))
{
}

SecureBytes &SecureBytes::operator=(SecureBytes &&other) noexcept
{
    if (this != &other) {
        clear();
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, std::size_t{0});
    }
    return *this;
}

void SecureBytes::clear() noexcept
{
    if (data_) {
        // sodium_free() already zeroes the guarded region before releasing it;
        // we wipe explicitly too so the contract holds even if that internal
        // guarantee ever changes.
        sodium_memzero(data_, size_);
        sodium_free(data_);
        data_ = nullptr;
    }
    size_ = 0;
}

bool SecureBytes::constantTimeEquals(const SecureBytes &other) const noexcept
{
    if (size_ != other.size_)
        return false;
    if (size_ == 0)
        return true;
    return sodium_memcmp(data_, other.data_, size_) == 0;
}

} // namespace fsnext::crypto
