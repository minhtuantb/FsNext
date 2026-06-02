// SPDX-License-Identifier: Proprietary
#pragma once

namespace fsnext::crypto {

/// Result codes for the encryption engine and key management. Mapped to the
/// FSE-1xx user-facing error codes in docs/specs/encryption-plan.md §D.
enum class CryptoError {
    Ok = 0,
    InvalidFormat,       // not a .fshenc / bad magic / short or malformed header
    UnsupportedVersion,  // newer file version, or unknown ALGO_ID
    WrongKey,            // DEK unwrap failed (wrong key or tampered wrapped DEK)
    Tampered,            // body authentication failed / truncated stream
    KdfFailed,           // Argon2id failed (e.g. out of memory)
    IoError,             // file read/write failure
    AlgoUnavailable,     // AES file opened on a machine without AES-NI
    AlreadyExists,       // vault already present at the target location
    NotUnlocked,         // operation requires an unlocked vault
    InternalError,       // unexpected libsodium failure / misuse
};

inline const char *toString(CryptoError e) noexcept
{
    switch (e) {
    case CryptoError::Ok:                 return "Ok";
    case CryptoError::InvalidFormat:      return "InvalidFormat";
    case CryptoError::UnsupportedVersion: return "UnsupportedVersion";
    case CryptoError::WrongKey:           return "WrongKey";
    case CryptoError::Tampered:           return "Tampered";
    case CryptoError::KdfFailed:          return "KdfFailed";
    case CryptoError::IoError:            return "IoError";
    case CryptoError::AlgoUnavailable:    return "AlgoUnavailable";
    case CryptoError::AlreadyExists:      return "AlreadyExists";
    case CryptoError::NotUnlocked:        return "NotUnlocked";
    case CryptoError::InternalError:      return "InternalError";
    }
    return "Unknown";
}

} // namespace fsnext::crypto
