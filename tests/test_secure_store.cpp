// SPDX-License-Identifier: Proprietary
// SecureStore unit tests — verify the encrypt/decrypt round-trip and its
// string/base64 convenience wrappers.
//
// SecureStore is a free-function namespace (fsnext::SecureStore), not a class.
// On Windows the backend is DPAPI (CryptProtectData/CryptUnprotectData), which
// is deterministic for the current user+machine, so a round-trip performed
// within the same test process always succeeds — letting us assert that
// decrypt(encrypt(x)) == x without any external secret store. On non-Windows
// the "Unsupported platform" backend makes encrypt() return empty; the tests
// branch on Q_OS_WIN so they stay meaningful (and skip) off Windows.
//
// No QSettings, no network, no threading — pure synchronous calls.

#include <QtTest>
#include "core/util/SecureStore.h"

using fsnext::SecureStore::encrypt;
using fsnext::SecureStore::decrypt;
using fsnext::SecureStore::encryptToBase64;
using fsnext::SecureStore::decryptFromBase64;

class TestSecureStore : public QObject
{
    Q_OBJECT
private slots:

    // ATM-0422 (TC-0473) + ATM-0425 (TC-0479): encrypt() must produce a
    // non-empty blob that differs from the plaintext, and decrypt() must
    // recover the exact original bytes. (DPAPI round-trip.)
    void encryptDecryptRoundTrip()
    {
        // ATM-0422 / ATM-0425
#ifdef Q_OS_WIN
        const QByteArray plain = QByteArrayLiteral("secret-token");
        const QByteArray blob = encrypt(plain);

        QVERIFY2(!blob.isEmpty(), "DPAPI encrypt returned empty blob");
        QVERIFY2(blob != plain, "ciphertext must differ from plaintext");

        const QByteArray back = decrypt(blob);
        QCOMPARE(back, plain);
#else
        // Off-Windows the backend refuses to persist (returns empty).
        QVERIFY(encrypt(QByteArrayLiteral("secret-token")).isEmpty());
        QSKIP("DPAPI round-trip is Windows-only");
#endif
    }

    // ATM-0425 (TC-0479): decrypt() recovers binary bytes (including embedded
    // NUL and high bytes), proving the blob length is preserved end to end.
    void decryptRecoversBinaryBytes()
    {
        // ATM-0425
#ifdef Q_OS_WIN
        QByteArray plain;
        plain.append('\x00');
        plain.append("data-bytes");
        plain.append('\xFF');

        const QByteArray blob = encrypt(plain);
        QVERIFY(!blob.isEmpty());
        QCOMPARE(decrypt(blob), plain);
#else
        QSKIP("DPAPI round-trip is Windows-only");
#endif
    }

    // ATM-0422 (TC-0474): empty input short-circuits to empty on both ends —
    // encrypt(empty) is empty, decrypt(empty) is empty, no crash.
    void emptyInputIsStableEmpty()
    {
        // ATM-0422 / ATM-0425
        QVERIFY(encrypt(QByteArray()).isEmpty());
        QVERIFY(decrypt(QByteArray()).isEmpty());
    }

    // ATM-0425 (TC-0480): a corrupt / non-DPAPI blob must decrypt to empty
    // (CryptUnprotectData fails) rather than crashing or returning garbage.
    void decryptOfGarbageIsEmpty()
    {
        // ATM-0425
        QVERIFY(decrypt(QByteArrayLiteral("garbage-not-a-dpapi-blob")).isEmpty());
        QVERIFY(decrypt(QByteArray()).isEmpty());
    }

    // ATM-0423 + ATM-0424 (TC-0475 / TC-0477): the QString/base64 wrappers
    // round-trip a value unchanged. The intermediate is a non-empty base64
    // string that round-trips through QByteArray::fromBase64.
    void base64RoundTripPreservesString()
    {
        // ATM-0423 / ATM-0424
#ifdef Q_OS_WIN
        const QString plain = QStringLiteral("P@ssw0rd!");
        const QString enc = encryptToBase64(plain);

        QVERIFY2(!enc.isEmpty(), "encryptToBase64 returned empty");
        QVERIFY2(enc != plain, "base64 ciphertext must differ from plaintext");
        // Latin1 base64 alphabet only — the wrapper uses QString::fromLatin1.
        QCOMPARE(QString::fromLatin1(enc.toLatin1()), enc);

        QCOMPARE(decryptFromBase64(enc), plain);
#else
        QVERIFY(encryptToBase64(QStringLiteral("P@ssw0rd!")).isEmpty());
        QSKIP("DPAPI round-trip is Windows-only");
#endif
    }

    // ATM-0423 + ATM-0424: unicode survives the UTF-8 -> encrypt -> base64 ->
    // decrypt -> UTF-8 pipeline (the wrappers use toUtf8/fromUtf8).
    void base64RoundTripPreservesUnicode()
    {
        // ATM-0423 / ATM-0424
#ifdef Q_OS_WIN
        const QString plain = QStringLiteral(u"Tiếng Việt 🔐 — café");
        const QString enc = encryptToBase64(plain);
        QVERIFY(!enc.isEmpty());
        QCOMPARE(decryptFromBase64(enc), plain);
#else
        QSKIP("DPAPI round-trip is Windows-only");
#endif
    }

    // ATM-0423 + ATM-0424 (TC-0476): encrypting the same value twice yields
    // two ciphertexts (DPAPI salts each call) that BOTH decrypt back to the
    // original — the contract callers rely on is "decrypts to the same value",
    // not "produces the same blob".
    void base64TwoEncryptionsBothDecrypt()
    {
        // ATM-0423 / ATM-0424
#ifdef Q_OS_WIN
        const QString plain = QStringLiteral("abc");
        const QString a = encryptToBase64(plain);
        const QString b = encryptToBase64(plain);
        QVERIFY(!a.isEmpty());
        QVERIFY(!b.isEmpty());
        QCOMPARE(decryptFromBase64(a), plain);
        QCOMPARE(decryptFromBase64(b), plain);
#else
        QSKIP("DPAPI round-trip is Windows-only");
#endif
    }

    // ATM-0423 + ATM-0424 (TC-0478): empty and malformed base64 inputs to the
    // string wrappers return empty without crashing. Empty plaintext short-
    // circuits in encryptToBase64; corrupt base64 decodes to bytes that
    // CryptUnprotectData then rejects.
    void base64EdgeAndInvalidInputs()
    {
        // ATM-0423 / ATM-0424
        QVERIFY(encryptToBase64(QString()).isEmpty());
        QVERIFY(decryptFromBase64(QString()).isEmpty());
        QVERIFY(decryptFromBase64(QStringLiteral("!!!notbase64!!!")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSecureStore)
#include "test_secure_store.moc"
