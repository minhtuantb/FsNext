// SPDX-License-Identifier: Proprietary
#pragma once

namespace fsnext::crypto {

/// Initialize libsodium. MUST be called once at startup, on the main thread,
/// before any other crypto operation (SecureBytes allocation, KDF,
/// encryption). Safe to call more than once. Returns true on success.
///
/// libsodium's RNG and guarded heap allocator (sodium_malloc) rely on this
/// having run, so call it as early as practical in main().
bool init();

/// True when the CPU exposes hardware AES (AES-NI) and libsodium's
/// AES-256-GCM implementation is usable on this machine. This decides whether
/// small files are encrypted with AES-256-GCM (ALGO 0x02) or fall back to
/// XChaCha20-Poly1305 (0x01). See docs/specs/encryption-plan.md §2.
bool aes256GcmAvailable();

} // namespace fsnext::crypto
