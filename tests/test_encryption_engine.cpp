// SPDX-License-Identifier: Proprietary
//
// Unit tests for fsnext::crypto::EncryptionEngine (Phase C2 / E1).
// Covers: AES single-shot vs XChaCha streaming selection by size, round-trip
// (buffer + file), wrong key, header/body tamper, truncation, re-encrypt
// uniqueness, malformed input (no crash), Argon2id passphrase derivation.

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "core/crypto/Crypto.h"
#include "core/crypto/EncryptionEngine.h"
#include "core/crypto/FencFormat.h"
#include "core/crypto/Key.h"

#include <sodium.h>

#include <cstdint>
#include <cstring>
#include <vector>

using namespace fsnext::crypto;

class TestEncryptionEngine : public QObject
{
    Q_OBJECT

    EncryptionEngine eng;

    static std::vector<std::uint8_t> randomBuf(std::size_t n)
    {
        std::vector<std::uint8_t> v(n);
        if (n)
            randombytes_buf(v.data(), n);
        return v;
    }

private slots:
    void initTestCase() { QVERIFY2(fsnext::crypto::init(), "libsodium init"); }

    void roundTripSmall()
    {
        auto pt = randomBuf(1024);
        Key k = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "a.bin", k, ct) == CryptoError::Ok);
        FencHeader h;
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec, &h) == CryptoError::Ok);
        QVERIFY(dec == pt);
        if (aes256GcmAvailable()) {
            QCOMPARE(int(h.algoId), int(fenc::AlgoAes256Gcm));
            QVERIFY(h.flags & fenc::FlagSingleShot);
        }
        QVERIFY(h.originalSize == pt.size());
        QVERIFY(h.filename == "a.bin");
    }

    void roundTripLargeStreaming()
    {
        auto pt = randomBuf(2u * 1024 * 1024 + 123);  // > 1 MiB => XChaCha stream
        Key k = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "big.bin", k, ct) == CryptoError::Ok);
        FencHeader h;
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec, &h) == CryptoError::Ok);
        QCOMPARE(int(h.algoId), int(fenc::AlgoXChaCha20Poly1305));
        QVERIFY(dec == pt);
    }

    void forceXChaChaOnSmall()
    {
        auto pt = randomBuf(500);
        Key k = Key::random();
        EncryptOptions o;
        o.forceXChaCha = true;
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "x", k, ct, o) == CryptoError::Ok);
        FencHeader h;
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec, &h) == CryptoError::Ok);
        QCOMPARE(int(h.algoId), int(fenc::AlgoXChaCha20Poly1305));
        QVERIFY(dec == pt);
    }

    void zeroLength()
    {
        std::vector<std::uint8_t> pt;
        Key k = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), 0, "e", k, ct) == CryptoError::Ok);
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec) == CryptoError::Ok);
        QVERIFY(dec.empty());
    }

    void wrongKeyFails()
    {
        auto pt = randomBuf(2048);
        Key k1 = Key::random();
        Key k2 = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f", k1, ct) == CryptoError::Ok);
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k2, dec) == CryptoError::WrongKey);
        QVERIFY(dec.empty());  // no partial output
    }

    void tamperBodyFails()
    {
        auto pt = randomBuf(4096);
        Key k = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f", k, ct) == CryptoError::Ok);
        ct[ct.size() - 1] ^= 0x01;  // flip a ciphertext/tag byte
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec) == CryptoError::Tampered);
    }

    void tamperHeaderFails()
    {
        auto pt = randomBuf(1000);
        Key k = Key::random();
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f", k, ct) == CryptoError::Ok);
        ct[132] ^= 0x01;  // ORIGINAL_SIZE field — part of the AAD
        std::vector<std::uint8_t> dec;
        CryptoError e = eng.decryptBuffer(ct.data(), ct.size(), k, dec);
        QVERIFY(e == CryptoError::Tampered || e == CryptoError::InvalidFormat);
    }

    void reEncryptDiffers()
    {
        auto pt = randomBuf(1000);
        Key k = Key::random();
        std::vector<std::uint8_t> c1;
        std::vector<std::uint8_t> c2;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f", k, c1) == CryptoError::Ok);
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f", k, c2) == CryptoError::Ok);
        QVERIFY(c1 != c2);  // random DEK + nonce per encryption
    }

    void malformedDoesNotCrash()
    {
        Key k = Key::random();
        std::vector<std::uint8_t> dec;
        auto junk = randomBuf(50);
        QVERIFY(eng.decryptBuffer(junk.data(), junk.size(), k, dec) == CryptoError::InvalidFormat);
        QVERIFY(eng.decryptBuffer(nullptr, 0, k, dec) == CryptoError::InvalidFormat);
        // valid magic but otherwise zeroed/short — must not crash
        std::vector<std::uint8_t> bad(200, 0);
        bad[0] = 'F'; bad[1] = 'E'; bad[2] = 'N'; bad[3] = 'C';
        bad[4] = 0x01; bad[5] = fenc::AlgoXChaCha20Poly1305;
        (void)eng.decryptBuffer(bad.data(), bad.size(), k, dec);
        QVERIFY(true);
    }

    void thresholdBoundary()
    {
        if (!aes256GcmAvailable())
            QSKIP("no AES-NI on this machine");
        Key k = Key::random();
        auto a = randomBuf(1u << 20);  // exactly 1 MiB => AES
        std::vector<std::uint8_t> ca;
        QVERIFY(eng.encryptBuffer(a.data(), a.size(), "a", k, ca) == CryptoError::Ok);
        FencHeader ha;
        std::vector<std::uint8_t> da;
        QVERIFY(eng.decryptBuffer(ca.data(), ca.size(), k, da, &ha) == CryptoError::Ok);
        QCOMPARE(int(ha.algoId), int(fenc::AlgoAes256Gcm));
        QVERIFY(da == a);

        auto b = randomBuf((1u << 20) + 1);  // 1 MiB + 1 => XChaCha
        std::vector<std::uint8_t> cb;
        QVERIFY(eng.encryptBuffer(b.data(), b.size(), "b", k, cb) == CryptoError::Ok);
        FencHeader hb;
        std::vector<std::uint8_t> db;
        QVERIFY(eng.decryptBuffer(cb.data(), cb.size(), k, db, &hb) == CryptoError::Ok);
        QCOMPARE(int(hb.algoId), int(fenc::AlgoXChaCha20Poly1305));
        QVERIFY(db == b);
    }

    void kdfPassphraseRoundTrip()
    {
        std::uint8_t salt[16];
        randombytes_buf(salt, sizeof(salt));
        const KdfParams p = KdfParams::balanced();
        Key k1, k2;
        QVERIFY(Key::fromPassphrase("correct horse battery", salt, p, k1) == CryptoError::Ok);
        QVERIFY(Key::fromPassphrase("correct horse battery", salt, p, k2) == CryptoError::Ok);
        QVERIFY(k1.valid());
        QVERIFY(sodium_memcmp(k1.data(), k2.data(), 32) == 0);  // deterministic

        auto pt = randomBuf(3000);
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "p", k1, ct) == CryptoError::Ok);
        std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k2, dec) == CryptoError::Ok);
        QVERIFY(dec == pt);

        std::uint8_t salt2[16];
        randombytes_buf(salt2, sizeof(salt2));
        Key k3;
        QVERIFY(Key::fromPassphrase("correct horse battery", salt2, p, k3) == CryptoError::Ok);
        QVERIFY(sodium_memcmp(k1.data(), k3.data(), 32) != 0);  // salt changes key
    }

    void fileRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString inP  = dir.filePath("plain.bin");
        const QString outP = dir.filePath("plain.fshenc");
        const QString decP = dir.filePath("plain.out");

        auto data = randomBuf(3u * 1024 * 1024);  // streaming path
        {
            QFile f(inP);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(reinterpret_cast<const char *>(data.data()), qint64(data.size()));
        }
        Key k = Key::random();
        QVERIFY(eng.encryptFile(inP.toStdString(), outP.toStdString(), k) == CryptoError::Ok);
        QVERIFY(eng.isFencFile(outP.toStdString()));
        QVERIFY(eng.decryptFile(outP.toStdString(), decP.toStdString(), k) == CryptoError::Ok);

        QFile f(decP);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray got = f.readAll();
        QVERIFY(std::size_t(got.size()) == data.size());
        QVERIFY(std::memcmp(got.constData(), data.data(), data.size()) == 0);
    }

    void readHeaderNoKey()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString outP = dir.filePath("h.fshenc");
        auto data = randomBuf(2048);
        Key k = Key::random();
        QVERIFY(eng.encryptFile(QString(dir.filePath("in")).toStdString(),  // create input first
                                outP.toStdString(), k) == CryptoError::IoError);  // no input yet
        // Now create a real input and encrypt.
        const QString inP = dir.filePath("real.bin");
        {
            QFile f(inP);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(reinterpret_cast<const char *>(data.data()), qint64(data.size()));
        }
        QVERIFY(eng.encryptFile(inP.toStdString(), outP.toStdString(), k, [] {
            EncryptOptions o; o.storedFilename = "secret.docx"; return o; }()) == CryptoError::Ok);
        FencHeader h;
        QVERIFY(eng.readHeader(outP.toStdString(), h) == CryptoError::Ok);
        QVERIFY(h.filename == "secret.docx");
        QVERIFY(h.originalSize == data.size());
    }
};

QTEST_APPLESS_MAIN(TestEncryptionEngine)
#include "test_encryption_engine.moc"
