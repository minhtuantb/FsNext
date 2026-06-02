// SPDX-License-Identifier: Proprietary
#pragma once

#include <cstddef>
#include <cstdint>

namespace fsnext::crypto {

/// Owns a buffer of sensitive bytes (passphrase-derived keys, master keys,
/// per-file DEKs) backed by libsodium's guarded heap (`sodium_malloc`):
///   * the region is `mlock()`'d so it never gets paged out to swap,
///   * it is fenced by guard pages + a canary to catch over/under-runs,
///   * it is wiped with `sodium_memzero` and released on destruction.
///
/// Move-only by design — a secret must have exactly one owner; copying a key
/// around in plain heap defeats the purpose. Use std::move to transfer.
///
/// Requires fsnext::crypto::init() to have run first (sodium_malloc relies on
/// libsodium being initialized).
class SecureBytes {
public:
    SecureBytes() noexcept = default;

    /// Allocate `n` locked, zero-filled bytes. `n == 0` yields an empty object
    /// (no allocation). Throws std::bad_alloc if the guarded allocation fails.
    explicit SecureBytes(std::size_t n);

    /// Allocate `n` bytes and copy `n` bytes from `src` into them. If `src` is
    /// null the buffer is zero-filled instead. `n == 0` yields an empty object.
    SecureBytes(const std::uint8_t *src, std::size_t n);

    ~SecureBytes();

    SecureBytes(SecureBytes &&other) noexcept;
    SecureBytes &operator=(SecureBytes &&other) noexcept;

    SecureBytes(const SecureBytes &) = delete;
    SecureBytes &operator=(const SecureBytes &) = delete;

    std::uint8_t       *data() noexcept { return data_; }
    const std::uint8_t *data() const noexcept { return data_; }
    std::size_t         size() const noexcept { return size_; }
    bool                empty() const noexcept { return size_ == 0; }

    /// Wipe and release the buffer; the object becomes empty. Idempotent.
    void clear() noexcept;

    /// Constant-time equality — does not short-circuit on the first differing
    /// byte, so it leaks no timing information about the contents. Buffers of
    /// different size always compare unequal.
    bool constantTimeEquals(const SecureBytes &other) const noexcept;

private:
    std::uint8_t *data_ = nullptr;
    std::size_t   size_ = 0;
};

} // namespace fsnext::crypto
