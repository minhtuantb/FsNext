// SPDX-License-Identifier: Proprietary
//
// Robustness / edge-case / crash-safety tests for the Vault feature.
// Complements test_encryption_engine (happy-path) and test_vault_manager
// (lifecycle) with the adversarial surface: malformed/fuzzed .fshenc headers
// (untrusted input), truncation, fail-closed output removal, partial-write /
// kill-mid-op simulation, edge filenames/sizes, and VaultManager state
// corruption.
//
// See docs/specs/vault-test-cases.md (VLT-### ids referenced in comments).

#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "core/crypto/Crypto.h"
#include "core/crypto/EncryptionEngine.h"
#include "core/crypto/FencFormat.h"
#include "core/crypto/Key.h"
#include "core/crypto/KdfParams.h"
#include "core/services/VaultManager.h"

#include <sodium.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace fsnext::crypto;
using fsnext::VaultManager;

namespace {

std::vector<std::uint8_t> randomBuf(std::size_t n)
{
    std::vector<std::uint8_t> v(n);
    if (n)
        randombytes_buf(v.data(), n);
    return v;
}

// Build a valid .fshenc buffer for `n` random bytes with display name `name`.
std::vector<std::uint8_t> makeFenc(EncryptionEngine &eng, const Key &k, std::size_t n,
                                   const std::string &name, bool forceXChaCha = false)
{
    auto pt = randomBuf(n);
    EncryptOptions o;
    o.forceXChaCha = forceXChaCha;
    std::vector<std::uint8_t> ct;
    eng.encryptBuffer(pt.data(), pt.size(), name, k, ct, o);
    return ct;
}

void putU16(std::vector<std::uint8_t> &b, std::size_t off, std::uint16_t v)
{
    b[off] = std::uint8_t(v); b[off + 1] = std::uint8_t(v >> 8);
}
void putU32(std::vector<std::uint8_t> &b, std::size_t off, std::uint32_t v)
{
    for (int i = 0; i < 4; ++i) b[off + i] = std::uint8_t(v >> (8 * i));
}
void putU64(std::vector<std::uint8_t> &b, std::size_t off, std::uint64_t v)
{
    for (int i = 0; i < 8; ++i) b[off + i] = std::uint8_t(v >> (8 * i));
}

// .fshenc fixed-header field offsets (encryption-plan.md §7).
constexpr std::size_t kOffVersion  = 4;
constexpr std::size_t kOffAlgo     = 5;
constexpr std::size_t kOffOrigSize = 132;
constexpr std::size_t kOffChunk    = 140;
constexpr std::size_t kOffFnameLen = 144;

QByteArray readAll(const QString &p)
{
    QFile f(p);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
bool writeAll(const QString &p, const QByteArray &d)
{
    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(d) == d.size();
}
bool writeFile(const QString &p, const std::vector<std::uint8_t> &d)
{
    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(reinterpret_cast<const char *>(d.data()), qint64(d.size())) == qint64(d.size());
}

} // namespace

class TestVaultRobustness : public QObject
{
    Q_OBJECT

    EncryptionEngine eng;

private slots:
    void initTestCase() { QVERIFY2(fsnext::crypto::init(), "libsodium init"); }

    // ── Parser / untrusted-input validation (VLT-PARSE-*) ──────────────────

    void parseShortHeaderRejected()
    {
        // Any buffer shorter than the 146-byte fixed header must be rejected.
        Key k = Key::random();
        std::vector<std::uint8_t> dec;
        for (std::size_t n : {std::size_t(0), std::size_t(1), std::size_t(4),
                              std::size_t(145), fenc::FixedHeaderSize - 1}) {
            auto buf = randomBuf(n);
            const CryptoError e = eng.decryptBuffer(buf.empty() ? nullptr : buf.data(), n, k, dec);
            QVERIFY2(e == CryptoError::InvalidFormat,
                     qPrintable(QString("len=%1 -> %2").arg(n).arg(toString(e))));
            QVERIFY(dec.empty());
        }
    }

    void parseBadMagic()
    {
        Key k = Key::random();
        auto ct = makeFenc(eng, k, 1024, "x");
        ct[0] = 'X';
        std::vector<std::uint8_t> dec;
        QCOMPARE(int(eng.decryptBuffer(ct.data(), ct.size(), k, dec)), int(CryptoError::InvalidFormat));
        QVERIFY(dec.empty());
    }

    void parseAllZeros()
    {
        Key k = Key::random();
        std::vector<std::uint8_t> z(300, 0), dec;
        QCOMPARE(int(eng.decryptBuffer(z.data(), z.size(), k, dec)), int(CryptoError::InvalidFormat));
        QVERIFY(dec.empty());
    }

    void parseBadVersion()
    {
        Key k = Key::random();
        auto ct = makeFenc(eng, k, 512, "x");
        ct[kOffVersion] = 0x02;  // unknown future version
        std::vector<std::uint8_t> dec;
        const CryptoError e = eng.decryptBuffer(ct.data(), ct.size(), k, dec);
        QVERIFY2(e != CryptoError::Ok, qPrintable(QString("version -> %1").arg(toString(e))));
        QVERIFY(dec.empty());
    }

    void parseBadAlgo()
    {
        Key k = Key::random();
        auto ct = makeFenc(eng, k, 512, "x");
        for (std::uint8_t algo : {std::uint8_t(0x00), std::uint8_t(0x03), std::uint8_t(0xFF)}) {
            auto c = ct;
            c[kOffAlgo] = algo;
            std::vector<std::uint8_t> dec;
            const CryptoError e = eng.decryptBuffer(c.data(), c.size(), k, dec);
            QVERIFY2(e != CryptoError::Ok, qPrintable(QString("algo=%1 -> %2").arg(algo).arg(toString(e))));
            QVERIFY(dec.empty());
        }
    }

    void parseHugeFilenameLenNoOverread()
    {
        // FILENAME_LEN claims a giant filename but the buffer is short. Must not
        // over-read / crash; must return an error.
        Key k = Key::random();
        auto ct = makeFenc(eng, k, 256, "x");
        putU16(ct, kOffFnameLen, 0xFFFF);
        std::vector<std::uint8_t> dec;
        const CryptoError e = eng.decryptBuffer(ct.data(), ct.size(), k, dec);
        QVERIFY2(e != CryptoError::Ok, qPrintable(QString("hugeFnameLen -> %1").arg(toString(e))));
        QVERIFY(dec.empty());
    }

    void parseHugeOriginalSizeAesRejected()
    {
        if (!aes256GcmAvailable())
            QSKIP("AES single-shot path unavailable on this CPU");
        // Small file => AES single-shot. A hostile huge ORIGINAL_SIZE must be
        // rejected by the read cap (no multi-GB allocation), not honoured.
        Key k = Key::random();
        auto ct = makeFenc(eng, k, 1024, "x");  // AES path
        QCOMPARE(int(ct[kOffAlgo]), int(fenc::AlgoAes256Gcm));
        putU64(ct, kOffOrigSize, 0xFFFFFFFFFFFFULL);
        std::vector<std::uint8_t> dec;
        const CryptoError e = eng.decryptBuffer(ct.data(), ct.size(), k, dec);
        QVERIFY2(e == CryptoError::InvalidFormat || e == CryptoError::Tampered,
                 qPrintable(QString("hugeOrigSize -> %1").arg(toString(e))));
        QVERIFY(dec.empty());
    }

    void parseBadChunkSizeXChaChaRejected()
    {
        Key k = Key::random();
        auto base = makeFenc(eng, k, 4096, "x", /*forceXChaCha*/ true);
        QCOMPARE(int(base[kOffAlgo]), int(fenc::AlgoXChaCha20Poly1305));
        for (std::uint32_t cs : {std::uint32_t(0), std::uint32_t(1), std::uint32_t(0xFFFFFFFF)}) {
            auto c = base;
            putU32(c, kOffChunk, cs);
            std::vector<std::uint8_t> dec;
            const CryptoError e = eng.decryptBuffer(c.data(), c.size(), k, dec);
            QVERIFY2(e != CryptoError::Ok, qPrintable(QString("chunk=%1 -> %2").arg(cs).arg(toString(e))));
            QVERIFY(dec.empty());
        }
    }

    void fuzzSingleByteFlipNeverDecrypts()
    {
        // Flip one byte anywhere in a valid .fshenc; it must NEVER silently
        // decrypt to the original (every header byte is AAD, every body byte is
        // authenticated). No crash, no wrong-plaintext.
        Key k = Key::random();
        auto pt = randomBuf(2000);
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "f.bin", k, ct) == CryptoError::Ok);
        for (int iter = 0; iter < 400; ++iter) {
            auto c = ct;
            const std::size_t pos = randombytes_uniform(std::uint32_t(c.size()));
            c[pos] ^= std::uint8_t(1 + randombytes_uniform(255));
            std::vector<std::uint8_t> dec;
            const CryptoError e = eng.decryptBuffer(c.data(), c.size(), k, dec);
            if (e == CryptoError::Ok) {
                QVERIFY2(false, qPrintable(QString("byte-flip at %1 decrypted OK (must not)").arg(pos)));
            }
        }
    }

    void fuzzRandomGarbageNoCrash()
    {
        Key k = Key::random();
        for (int iter = 0; iter < 300; ++iter) {
            const std::size_t n = randombytes_uniform(400);
            auto buf = randomBuf(n);
            // Half the time, stamp a valid magic so we exercise deeper paths.
            if (n >= 6 && (iter & 1)) {
                buf[0] = 'F'; buf[1] = 'E'; buf[2] = 'N'; buf[3] = 'C';
                buf[kOffVersion] = fenc::Version;
                buf[kOffAlgo] = (iter & 2) ? fenc::AlgoAes256Gcm : fenc::AlgoXChaCha20Poly1305;
            }
            std::vector<std::uint8_t> dec;
            const CryptoError e = eng.decryptBuffer(buf.empty() ? nullptr : buf.data(), n, k, dec);
            QVERIFY(e != CryptoError::Ok);  // random data must never decrypt
        }
        QVERIFY(true);  // reached here => no crash
    }

    // ── Truncation / kill-mid-write simulation (VLT-CRASH-*) ───────────────

    void truncatedStreamRejected()
    {
        // Encrypt a streaming (XChaCha) buffer, then cut it at various points to
        // mimic the app being killed mid-write. Decrypt must fail (truncation
        // before TAG_FINAL), never return Ok, never crash.
        Key k = Key::random();
        auto pt = randomBuf(3u * 1024 * 1024);  // > 1 MiB => streaming, many chunks
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), "big", k, ct) == CryptoError::Ok);
        for (double frac : {0.0, 0.25, 0.5, 0.9, 0.999}) {
            const std::size_t cut = fenc::FixedHeaderSize + 3
                                    + std::size_t((ct.size() - fenc::FixedHeaderSize - 3) * frac);
            std::vector<std::uint8_t> c(ct.begin(), ct.begin() + std::min(cut, ct.size()));
            std::vector<std::uint8_t> dec;
            const CryptoError e = eng.decryptBuffer(c.data(), c.size(), k, dec);
            QVERIFY2(e != CryptoError::Ok, qPrintable(QString("frac=%1 -> %2").arg(frac).arg(toString(e))));
        }
    }

    // ── Fail-closed: decrypt to FILE must leave no partial output ──────────

    void decryptFileWrongKeyLeavesNoOutput()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString inP = dir.filePath("a.bin"), encP = dir.filePath("a.fshenc"),
                      outP = dir.filePath("a.out");
        QVERIFY(writeFile(inP, randomBuf(4096)));
        Key k1 = Key::random(), k2 = Key::random();
        QVERIFY(eng.encryptFile(inP.toStdString(), encP.toStdString(), k1) == CryptoError::Ok);
        const CryptoError e = eng.decryptFile(encP.toStdString(), outP.toStdString(), k2);
        QCOMPARE(int(e), int(CryptoError::WrongKey));
        QVERIFY2(!QFileInfo::exists(outP), "wrong-key decrypt must not leave a partial plaintext");
    }

    void decryptFileTamperedLeavesNoOutput()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString inP = dir.filePath("a.bin"), encP = dir.filePath("a.fshenc"),
                      outP = dir.filePath("a.out");
        QVERIFY(writeFile(inP, randomBuf(3u * 1024 * 1024)));  // streaming
        Key k = Key::random();
        QVERIFY(eng.encryptFile(inP.toStdString(), encP.toStdString(), k) == CryptoError::Ok);
        // Flip a byte deep in the body.
        QByteArray raw = readAll(encP);
        QVERIFY(raw.size() > 1000);
        raw[raw.size() / 2] = char(raw[raw.size() / 2] ^ 0x40);
        QVERIFY(writeAll(encP, raw));
        const CryptoError e = eng.decryptFile(encP.toStdString(), outP.toStdString(), k);
        QVERIFY(e == CryptoError::Tampered || e == CryptoError::InvalidFormat);
        QVERIFY2(!QFileInfo::exists(outP), "tampered decrypt must not leave a partial plaintext");
    }

    void decryptFileTruncatedLeavesNoOutput()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString inP = dir.filePath("a.bin"), encP = dir.filePath("a.fshenc"),
                      outP = dir.filePath("a.out");
        QVERIFY(writeFile(inP, randomBuf(3u * 1024 * 1024)));
        Key k = Key::random();
        QVERIFY(eng.encryptFile(inP.toStdString(), encP.toStdString(), k) == CryptoError::Ok);
        QByteArray raw = readAll(encP);
        QVERIFY(writeAll(encP, raw.left(raw.size() / 2)));  // simulate kill mid-write
        const CryptoError e = eng.decryptFile(encP.toStdString(), outP.toStdString(), k);
        QVERIFY(e != CryptoError::Ok);
        QVERIFY2(!QFileInfo::exists(outP), "truncated decrypt must not leave a partial plaintext");
    }

    void encryptFileBadOutputPathNoPartial()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString inP = dir.filePath("a.bin");
        QVERIFY(writeFile(inP, randomBuf(2048)));
        Key k = Key::random();
        const QString badOut = dir.filePath("no_such_dir/sub/a.fshenc");  // parent doesn't exist
        const CryptoError e = eng.encryptFile(inP.toStdString(), badOut.toStdString(), k);
        QCOMPARE(int(e), int(CryptoError::IoError));
        QVERIFY(!QFileInfo::exists(badOut));
    }

    // ── Edge cases: file sizes & filenames (VLT-EDGE-*) ────────────────────

    void zeroByteFileRoundTrip()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString inP = dir.filePath("empty.bin"), encP = dir.filePath("empty.fshenc"),
                      outP = dir.filePath("empty.out");
        QVERIFY(writeFile(inP, {}));  // 0-byte file
        QVERIFY(QFileInfo(inP).size() == 0);
        Key k = Key::random();
        QVERIFY(eng.encryptFile(inP.toStdString(), encP.toStdString(), k) == CryptoError::Ok);
        QVERIFY(eng.decryptFile(encP.toStdString(), outP.toStdString(), k) == CryptoError::Ok);
        QVERIFY(QFileInfo::exists(outP));
        QCOMPARE(QFileInfo(outP).size(), qint64(0));
    }

    void fileSizeBoundaryRoundTrip()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        Key k = Key::random();
        for (std::size_t n : {std::size_t(EncryptionEngine::SmallFileThreshold),
                              std::size_t(EncryptionEngine::SmallFileThreshold + 1)}) {
            const QString inP = dir.filePath(QString("b%1.bin").arg(n));
            const QString encP = inP + ".fshenc", outP = inP + ".out";
            auto data = randomBuf(n);
            QVERIFY(writeFile(inP, data));
            QVERIFY(eng.encryptFile(inP.toStdString(), encP.toStdString(), k) == CryptoError::Ok);
            FencHeader h;
            QVERIFY(eng.decryptFile(encP.toStdString(), outP.toStdString(), k, &h) == CryptoError::Ok);
            QByteArray got = readAll(outP);
            QCOMPARE(std::size_t(got.size()), n);
            QVERIFY(std::memcmp(got.constData(), data.data(), n) == 0);
        }
    }

    void filenameEdgeCasesRoundTrip()
    {
        Key k = Key::random();
        const std::vector<std::string> names = {
            "",                                   // empty
            "tài liệu mật.pdf",                   // unicode VN
            "no_extension",                       // no ext
            "a b & c (1) [2] @#$.txt",            // special chars
            std::string(300, 'x') + ".dat",       // long-ish (<4096)
        };
        for (const auto &name : names) {
            std::vector<std::uint8_t> ct;
            auto pt = randomBuf(777);
            QVERIFY2(eng.encryptBuffer(pt.data(), pt.size(), name, k, ct) == CryptoError::Ok,
                     qPrintable(QString("encrypt name='%1'").arg(QString::fromStdString(name))));
            FencHeader h; std::vector<std::uint8_t> dec;
            QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec, &h) == CryptoError::Ok);
            QVERIFY(dec == pt);
            QCOMPARE(QString::fromStdString(h.filename), QString::fromStdString(name));
        }
    }

    // VLT-EDGE: filename longer than MaxFilenameLen — documents the
    // encrypt/decrypt asymmetry (encryptable, then NOT decryptable).
    void filenameOverMaxLenAsymmetry()
    {
        Key k = Key::random();
        const std::string huge(fenc::MaxFilenameLen + 100, 'n');  // 4196 chars
        auto pt = randomBuf(64);
        std::vector<std::uint8_t> ct;
        const CryptoError enc = eng.encryptBuffer(pt.data(), pt.size(), huge, k, ct);
        std::vector<std::uint8_t> dec;
        const CryptoError dq = (enc == CryptoError::Ok)
            ? eng.decryptBuffer(ct.data(), ct.size(), k, dec) : enc;
        // EXPECTED (robust) behaviour: either reject at encrypt, OR accept and
        // round-trip. The failure mode we guard against is "encrypt OK but
        // decrypt InvalidFormat" — a file you can create but never open.
        const bool createdButUnreadable = (enc == CryptoError::Ok && dq != CryptoError::Ok);
        QVERIFY2(!createdButUnreadable,
                 qPrintable(QString("over-max filename: enc=%1 dec=%2 (created-but-unreadable)")
                                .arg(toString(enc)).arg(toString(dq))));
    }

    // VLT-SEC: a hostile .fshenc whose stored filename is a path-traversal /
    // absolute path. The engine round-trips it verbatim (by design); this test
    // pins that downstream consumers MUST sanitize before using it as an output
    // path. Documents the contract; flags if the header silently drops it.
    void filenameTraversalRoundTripsVerbatim()
    {
        Key k = Key::random();
        const std::string evil = "../../../../Windows/Temp/evil.exe";
        auto pt = randomBuf(32);
        std::vector<std::uint8_t> ct;
        QVERIFY(eng.encryptBuffer(pt.data(), pt.size(), evil, k, ct) == CryptoError::Ok);
        FencHeader h; std::vector<std::uint8_t> dec;
        QVERIFY(eng.decryptBuffer(ct.data(), ct.size(), k, dec, &h) == CryptoError::Ok);
        // The engine preserves the name as-is — proving the VM/UI layer is
        // responsible for stripping path components (see bug report VLT-SEC-01).
        QCOMPARE(QString::fromStdString(h.filename), QString::fromStdString(evil));
    }

    // ── VaultManager state corruption / crash-safety (VLT-VM-*) ────────────

    void vaultAlreadyExists()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        VaultManager vm; vm.setVaultDir(dir.path());
        QCOMPARE(int(vm.createVault("V", "passphrase-one", KdfParams::balanced(),
                                    VaultManager::StorageLevel::Convenience)),
                 int(CryptoError::Ok));
        VaultManager vm2; vm2.setVaultDir(dir.path());
        QCOMPARE(int(vm2.createVault("V2", "passphrase-two", KdfParams::balanced(),
                                     VaultManager::StorageLevel::Convenience)),
                 int(CryptoError::AlreadyExists));
    }

    void wrongPassphraseDoesNotCorrupt()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        VaultManager vm; vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "right-pass-123", KdfParams::balanced(),
                               VaultManager::StorageLevel::MediumDpapi) == CryptoError::Ok);
        vm.lock();
        const CryptoError bad = vm.unlock("wrong-pass-xyz");
        QVERIFY(bad == CryptoError::WrongKey || bad == CryptoError::Tampered);
        QVERIFY(!vm.isUnlocked());
        // Vault must still be openable with the correct passphrase.
        QCOMPARE(int(vm.unlock("right-pass-123")), int(CryptoError::Ok));
        QVERIFY(vm.isUnlocked());
    }

    void changePassphraseWrongOldKeepsVault()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        VaultManager vm; vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "old-pass-123", KdfParams::balanced(),
                               VaultManager::StorageLevel::MediumDpapi) == CryptoError::Ok);
        const CryptoError e = vm.changePassphrase("not-the-old", "new-pass-456",
                                                  KdfParams::balanced());
        QVERIFY(e != CryptoError::Ok);
        vm.lock();
        // Old passphrase must still work (change was rejected, not half-applied).
        QCOMPARE(int(vm.unlock("old-pass-123")), int(CryptoError::Ok));
    }

    void unlockWithBadKeyfileSize()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        VaultManager vm; vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                               VaultManager::StorageLevel::MediumDpapi) == CryptoError::Ok);
        // Export a recovery keyfile, then corrupt its size.
        const QString kf = dir.filePath("rec.key");
        QVERIFY(vm.exportRecoveryKeyfile(kf) == CryptoError::Ok);
        vm.lock();
        QVERIFY(writeAll(kf, QByteArray(10, '\0')));  // wrong size (not 64)
        const CryptoError e = vm.unlockWithKeyfile(kf);
        QVERIFY(e == CryptoError::InvalidFormat || e == CryptoError::WrongKey);
        QVERIFY(!vm.isUnlocked());
    }

    void corruptMetaLoadsGracefully()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        {
            VaultManager vm; vm.setVaultDir(dir.path());
            QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                                   VaultManager::StorageLevel::MediumDpapi) == CryptoError::Ok);
        }
        // Find and corrupt vault-meta.json.
        const QString meta = dir.path() + "/.vault-meta.json";
        QVERIFY2(QFileInfo::exists(meta), "vault-meta.json should exist after create");
        QVERIFY(writeAll(meta, QByteArray("{ this is not valid json ::::")));
        // A fresh manager must not crash; unlock should fail gracefully.
        VaultManager vm2; vm2.setVaultDir(dir.path());
        const CryptoError e = vm2.unlock("pass-123-456");
        QVERIFY(e != CryptoError::Ok);
        QVERIFY(!vm2.isUnlocked());
    }

    void truncatedMetaLoadsGracefully()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        {
            VaultManager vm; vm.setVaultDir(dir.path());
            QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                                   VaultManager::StorageLevel::MediumDpapi) == CryptoError::Ok);
        }
        const QString meta = dir.path() + "/.vault-meta.json";
        QByteArray raw = readAll(meta);
        QVERIFY(raw.size() > 4);
        QVERIFY(writeAll(meta, raw.left(raw.size() / 2)));  // kill mid-write
        VaultManager vm2; vm2.setVaultDir(dir.path());
        const CryptoError e = vm2.unlock("pass-123-456");
        QVERIFY(e != CryptoError::Ok);
        QVERIFY(!vm2.isUnlocked());
    }

    void autoUnlockMissingKeyBlob()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        {
            VaultManager vm; vm.setVaultDir(dir.path());
            QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                                   VaultManager::StorageLevel::Convenience) == CryptoError::Ok);
        }
        // Delete the stored plaintext key blob, keep meta → inconsistent state.
        QFile::remove(dir.path() + "/.vault-key.plain");
        VaultManager vm2; vm2.setVaultDir(dir.path());
        QVERIFY(!vm2.tryAutoUnlock());  // no blob → false, must not crash
        QVERIFY(!vm2.isUnlocked());
    }

    void autoUnlockCorruptKeyBlob()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        {
            VaultManager vm; vm.setVaultDir(dir.path());
            QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                                   VaultManager::StorageLevel::Convenience) == CryptoError::Ok);
        }
        const QString blob = dir.path() + "/.vault-key.plain";
        QVERIFY(writeAll(blob, QByteArray(7, 'Z')));  // wrong-size key
        VaultManager vm2; vm2.setVaultDir(dir.path());
        QVERIFY(!vm2.tryAutoUnlock());
        QVERIFY(!vm2.isUnlocked());
    }

    void deleteVaultKeepsEncryptedFiles()
    {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        VaultManager vm; vm.setVaultDir(dir.path());
        QVERIFY(vm.createVault("V", "pass-123-456", KdfParams::balanced(),
                               VaultManager::StorageLevel::Convenience) == CryptoError::Ok);
        // Drop a .fshenc into the vault dir; deleteVault must not remove it.
        const QString enc = dir.path() + "/keepme.fshenc";
        QVERIFY(writeFile(enc, makeFenc(eng, Key::random(), 128, "keepme")));
        QVERIFY(vm.deleteVault() == CryptoError::Ok);
        QCOMPARE(int(vm.state()), int(VaultManager::State::NoVault));
        QVERIFY2(QFileInfo::exists(enc), "deleteVault must NOT delete encrypted .fshenc files");
        QVERIFY2(!QFileInfo::exists(dir.path() + "/.vault-meta.json"), "meta should be gone");
    }
};

QTEST_APPLESS_MAIN(TestVaultRobustness)
#include "test_vault_robustness.moc"
