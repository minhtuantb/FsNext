// SPDX-License-Identifier: Proprietary
#pragma once

#include <QByteArray>
#include <QString>

namespace fsnext::SecureStore {

/// Encrypt `plain` using Windows DPAPI (CryptProtectData). The resulting blob
/// is tied to the current Windows user account — it can only be decrypted by
/// this app (or any app) running as the same user on the same machine.
///
/// On non-Windows platforms this returns the input unchanged (with a runtime
/// warning). FsNext is Windows-primary; macOS/Linux support can add Keychain
/// / libsecret bindings later.
///
/// Returns empty on failure. Callers should treat empty as "don't persist".
QByteArray encrypt(const QByteArray &plain);

/// Inverse of encrypt(). Returns empty on failure (e.g. blob was encrypted
/// by a different Windows user, or is corrupted).
QByteArray decrypt(const QByteArray &encrypted);

/// Convenience wrappers for QString <-> base64(encrypted) round-trip — what
/// you want when storing via QSettings (which is string-oriented).
QString  encryptToBase64(const QString &plainText);
QString  decryptFromBase64(const QString &base64Encrypted);

} // namespace fsnext::SecureStore
