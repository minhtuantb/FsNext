// SPDX-License-Identifier: Proprietary
#include "SecureStore.h"

#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <wincrypt.h>   // CryptProtectData / CryptUnprotectData (Crypt32.lib)
#elif defined(Q_OS_MACOS)
#  include <Security/Security.h>   // Keychain Services (Security.framework)
#elif defined(Q_OS_LINUX) && defined(FSNEXT_HAVE_LIBSECRET)
#  include <libsecret/secret.h>
#endif

namespace fsnext::SecureStore {

#ifdef Q_OS_WIN

QByteArray encrypt(const QByteArray &plain)
{
    if (plain.isEmpty()) return {};

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.constData()));
    in.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB out{};
    // CRYPTPROTECT_UI_FORBIDDEN — don't prompt the user; fail silently instead.
    // Entropy (3rd arg) is left null: the Windows user SID is used as the
    // implicit entropy. A per-app salt could be added later if we want to
    // guarantee that another FPT app can't read our secrets.
    if (!CryptProtectData(&in,
                          L"FsNext OAuth refresh token",
                          nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN,
                          &out)) {
        qWarning() << "[SecureStore] CryptProtectData failed, GetLastError=" << GetLastError();
        return {};
    }

    QByteArray result(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
}

QByteArray decrypt(const QByteArray &encrypted)
{
    if (encrypted.isEmpty()) return {};

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(encrypted.constData()));
    in.cbData = static_cast<DWORD>(encrypted.size());

    DATA_BLOB out{};
    LPWSTR desc = nullptr;
    if (!CryptUnprotectData(&in,
                            &desc,
                            nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &out)) {
        // Common reasons: blob encrypted by a different Windows user, or
        // corrupted / truncated. Treat as "no saved secret" and fall back
        // to interactive login.
        qWarning() << "[SecureStore] CryptUnprotectData failed, GetLastError=" << GetLastError();
        return {};
    }

    QByteArray result(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    if (desc) LocalFree(desc);
    LocalFree(out.pbData);
    return result;
}

#elif defined(Q_OS_MACOS)

// macOS Keychain backend.
//
// Design: rather than encrypt in place (like DPAPI), we store the plaintext
// as a generic-password item keyed by a constant tag, and return the tag as
// the "encrypted" blob. The caller persists the tag in QSettings, and the
// actual secret lives in the login keychain — protected by the user's login
// password and access-control lists.
//
// Trade-off vs the DPAPI model: the "blob" returned is not a ciphertext but
// an opaque reference. Inverting via decrypt() fetches the real secret.
// Callers already treat empty returns as "no secret saved" (see
// AuthService::autoLogin), which matches our behavior when the user revokes
// keychain access or the item was deleted.
static const char kService[] = "com.fpt.fsnext";

static QByteArray keychainWrite(const QByteArray &plain, const QByteArray &tag)
{
    // Delete any existing item with the same tag first — SecItemAdd fails with
    // errSecDuplicateItem if we don't.
    CFStringRef svc = CFStringCreateWithCString(nullptr, kService, kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithBytes(nullptr,
        reinterpret_cast<const UInt8*>(tag.constData()), tag.size(),
        kCFStringEncodingUTF8, false);

    const void *keys[]   = { kSecClass, kSecAttrService, kSecAttrAccount };
    const void *values[] = { kSecClassGenericPassword, svc, acct };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 3,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    SecItemDelete(query);
    CFRelease(query);

    CFDataRef data = CFDataCreate(nullptr,
        reinterpret_cast<const UInt8*>(plain.constData()), plain.size());

    const void *addKeys[]   = { kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData };
    const void *addValues[] = { kSecClassGenericPassword, svc, acct, data };
    CFDictionaryRef addQuery = CFDictionaryCreate(nullptr, addKeys, addValues, 4,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    OSStatus st = SecItemAdd(addQuery, nullptr);

    CFRelease(addQuery);
    CFRelease(data);
    CFRelease(acct);
    CFRelease(svc);

    if (st != errSecSuccess) {
        qWarning() << "[SecureStore] SecItemAdd failed, OSStatus=" << st;
        return {};
    }
    return tag;  // the tag IS the blob we store in QSettings
}

static QByteArray keychainRead(const QByteArray &tag)
{
    CFStringRef svc = CFStringCreateWithCString(nullptr, kService, kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithBytes(nullptr,
        reinterpret_cast<const UInt8*>(tag.constData()), tag.size(),
        kCFStringEncodingUTF8, false);

    const void *keys[]   = { kSecClass, kSecAttrService, kSecAttrAccount,
                             kSecReturnData, kSecMatchLimit };
    const void *values[] = { kSecClassGenericPassword, svc, acct,
                             kCFBooleanTrue, kSecMatchLimitOne };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 5,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFTypeRef result = nullptr;
    OSStatus st = SecItemCopyMatching(query, &result);
    CFRelease(query);
    CFRelease(acct);
    CFRelease(svc);

    if (st != errSecSuccess || !result) {
        // errSecItemNotFound is expected on first run — don't warn.
        if (st != errSecItemNotFound)
            qWarning() << "[SecureStore] SecItemCopyMatching failed, OSStatus=" << st;
        return {};
    }

    CFDataRef data = static_cast<CFDataRef>(result);
    QByteArray plain(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),
                     static_cast<int>(CFDataGetLength(data)));
    CFRelease(result);
    return plain;
}

QByteArray encrypt(const QByteArray &plain)
{
    if (plain.isEmpty()) return {};
    // Use a stable tag so decrypt() can find it. One secret per app for now —
    // if we ever need multiple (e.g. per-user), include the account in the tag.
    return keychainWrite(plain, QByteArrayLiteral("session-secret"));
}

QByteArray decrypt(const QByteArray &encrypted)
{
    if (encrypted.isEmpty()) return {};
    return keychainRead(encrypted);
}

#elif defined(Q_OS_LINUX) && defined(FSNEXT_HAVE_LIBSECRET)

// Linux libsecret backend — talks to the Secret Service D-Bus API
// (GNOME Keyring, KWallet-secret-service, etc.). Same indirect model as
// macOS: the returned blob is an opaque lookup tag.
static const SecretSchema kSchema = {
    "com.fpt.fsnext.secret", SECRET_SCHEMA_NONE,
    {
        { "tag", SECRET_SCHEMA_ATTRIBUTE_STRING },
        { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING },
    },
    // reserved (per libsecret docs)
    0, 0, 0, 0, 0, 0, 0, 0,
};

QByteArray encrypt(const QByteArray &plain)
{
    if (plain.isEmpty()) return {};
    const QByteArray tag = "session-secret";
    GError *err = nullptr;
    gboolean ok = secret_password_store_sync(
        &kSchema,
        SECRET_COLLECTION_DEFAULT,
        "FsNext session secret",
        plain.constData(),
        nullptr,        // cancellable
        &err,
        "tag", tag.constData(),
        nullptr);
    if (!ok) {
        qWarning() << "[SecureStore] libsecret store failed:"
                   << (err ? err->message : "unknown");
        if (err) g_error_free(err);
        return {};
    }
    return tag;
}

QByteArray decrypt(const QByteArray &encrypted)
{
    if (encrypted.isEmpty()) return {};
    GError *err = nullptr;
    gchar *value = secret_password_lookup_sync(
        &kSchema,
        nullptr,
        &err,
        "tag", encrypted.constData(),
        nullptr);
    if (!value) {
        if (err) {
            qWarning() << "[SecureStore] libsecret lookup failed:" << err->message;
            g_error_free(err);
        }
        return {};
    }
    QByteArray plain(value);
    secret_password_free(value);
    return plain;
}

#else  // Unsupported platform — refuse to persist rather than silently storing plaintext.

QByteArray encrypt(const QByteArray &)
{
    qWarning() << "[SecureStore] No secure backend available on this platform. "
                  "Refusing to persist secret (would be plaintext). "
                  "Build with Q_OS_WIN, Q_OS_MACOS, or Linux + FSNEXT_HAVE_LIBSECRET.";
    return {};
}

QByteArray decrypt(const QByteArray &)
{
    return {};
}

#endif

QString encryptToBase64(const QString &plainText)
{
    if (plainText.isEmpty()) return {};
    const QByteArray blob = encrypt(plainText.toUtf8());
    if (blob.isEmpty()) return {};
    return QString::fromLatin1(blob.toBase64());
}

QString decryptFromBase64(const QString &base64Encrypted)
{
    if (base64Encrypted.isEmpty()) return {};
    const QByteArray blob = QByteArray::fromBase64(base64Encrypted.toLatin1());
    const QByteArray plain = decrypt(blob);
    if (plain.isEmpty()) return {};
    return QString::fromUtf8(plain);
}

} // namespace fsnext::SecureStore
