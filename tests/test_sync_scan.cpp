// SPDX-License-Identifier: Proprietary
// SyncScanner::scanFilesystem — the pure, off-main BFS half of the sync scan
// (M18).  These tests build a real tree under QTemporaryDir and assert the
// ScanResult: file count, relPath/relDir shape, skip rules, oversized flag,
// watchSubfolders pruning, and that a symlink cycle terminates instead of
// hanging.  No network / DB / QObject — links Qt6::Core + Test only.

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>

#include <filesystem>
#include <system_error>

#include "core/services/SyncScanner.h"

using namespace fsnext;

class TestSyncScan : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_tmp;

    QString root() const { return m_tmp.path(); }

    // Create a file at `relPath` (creating parent dirs) holding `bytes` bytes.
    void writeFile(const QString &relPath, qint64 bytes)
    {
        const QString full = QDir(root()).filePath(relPath);
        QDir().mkpath(QFileInfo(full).absolutePath());
        QFile f(full);
        QVERIFY(f.open(QIODevice::WriteOnly));
        if (bytes > 0) f.write(QByteArray(static_cast<int>(bytes), 'x'));
        f.close();
    }

    void mkdirp(const QString &relPath)
    {
        QDir(root()).mkpath(relPath);
    }

    // Build a snapshot rooted at the temp dir with the given knobs.
    ScanSnapshot snap(bool watchSubfolders = true,
                      const QString &ignorePatterns = QString()) const
    {
        ScanSnapshot s;
        s.localPath       = root();
        s.watchSubfolders = watchSubfolders;
        s.ignorePatterns  = ignorePatterns;
        return s;
    }

    static bool hasRel(const ScanResult &r, const QString &rel)
    {
        for (const ScannedFile &f : r.files)
            if (f.relPath == rel) return true;
        return false;
    }
    static ScannedFile fileOf(const ScanResult &r, const QString &rel)
    {
        for (const ScannedFile &f : r.files)
            if (f.relPath == rel) return f;
        return {};
    }

    static constexpr qint64 kBig = 1024LL * 1024 * 1024;   // a generous "no oversize" cap

private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }

    // Reset the tree between tests so each starts from a clean root.
    void cleanup()
    {
        QDir d(root());
        for (const QFileInfo &fi : d.entryInfoList(
                 QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
            if (fi.isDir()) QDir(fi.absoluteFilePath()).removeRecursively();
            else            QFile::remove(fi.absoluteFilePath());
        }
    }

    // ── Root existence ─────────────────────────────────────────────────────
    void missingRootReportsNotExists()
    {
        ScanSnapshot s;
        s.localPath = QDir(root()).filePath(QStringLiteral("nope-does-not-exist"));
        const ScanResult r = scanFilesystem(s, kBig);
        QVERIFY(!r.rootExists);
        QVERIFY(r.files.isEmpty());
    }

    void emptyRootScansClean()
    {
        const ScanResult r = scanFilesystem(snap(), kBig);
        QVERIFY(r.rootExists);
        QCOMPARE(r.files.size(), 0);
        QVERIFY(r.seenRel.isEmpty());
    }

    // ── Basic discovery + relPath/relDir ─────────────────────────────────────
    void findsTopLevelFilesWithEmptyRelDir()
    {
        writeFile(QStringLiteral("a.txt"), 10);
        writeFile(QStringLiteral("b.bin"), 20);

        const ScanResult r = scanFilesystem(snap(), kBig);
        QCOMPARE(r.files.size(), 2);
        QVERIFY(hasRel(r, QStringLiteral("a.txt")));
        QVERIFY(hasRel(r, QStringLiteral("b.bin")));
        QCOMPARE(fileOf(r, QStringLiteral("a.txt")).relDir, QString());
        QCOMPARE(fileOf(r, QStringLiteral("a.txt")).size, qint64(10));
        QVERIFY(r.seenRel.contains(QStringLiteral("a.txt")));
        QVERIFY(r.seenRel.contains(QStringLiteral("b.bin")));
    }

    void recursesAndComputesRelDirForwardSlash()
    {
        writeFile(QStringLiteral("sub/c.txt"), 5);
        writeFile(QStringLiteral("a/b/c/deep.dat"), 5);

        const ScanResult r = scanFilesystem(snap(), kBig);
        QVERIFY(hasRel(r, QStringLiteral("sub/c.txt")));
        QVERIFY(hasRel(r, QStringLiteral("a/b/c/deep.dat")));
        QCOMPARE(fileOf(r, QStringLiteral("sub/c.txt")).relDir, QStringLiteral("sub"));
        QCOMPARE(fileOf(r, QStringLiteral("a/b/c/deep.dat")).relDir, QStringLiteral("a/b/c"));
        // relPath is always forward-slash even on Windows.
        QVERIFY(!fileOf(r, QStringLiteral("a/b/c/deep.dat")).relPath.contains('\\'));
    }

    // ── Skip rules ───────────────────────────────────────────────────────────
    void skipsZeroByteFiles()
    {
        writeFile(QStringLiteral("zero.txt"), 0);
        writeFile(QStringLiteral("keep.txt"), 3);

        const ScanResult r = scanFilesystem(snap(), kBig);
        QCOMPARE(r.files.size(), 1);
        QVERIFY(hasRel(r, QStringLiteral("keep.txt")));
        QVERIFY(!hasRel(r, QStringLiteral("zero.txt")));
        QVERIFY(!r.seenRel.contains(QStringLiteral("zero.txt")));
    }

    void skipsSystemFilesAndDirs()
    {
        writeFile(QStringLiteral("real.txt"), 4);
        writeFile(QStringLiteral("Thumbs.db"), 4);          // system skip
        writeFile(QStringLiteral(".hidden"), 4);            // dotfile skip
        writeFile(QStringLiteral("doc.tmp"), 4);            // *.tmp skip
        writeFile(QStringLiteral(".git/config"), 4);        // skipped dir
        writeFile(QStringLiteral("node_modules/x.js"), 4);  // skipped dir

        const ScanResult r = scanFilesystem(snap(), kBig);
        QCOMPARE(r.files.size(), 1);
        QVERIFY(hasRel(r, QStringLiteral("real.txt")));
        QVERIFY(!hasRel(r, QStringLiteral("Thumbs.db")));
        QVERIFY(!hasRel(r, QStringLiteral(".hidden")));
        QVERIFY(!hasRel(r, QStringLiteral("doc.tmp")));
        QVERIFY(!hasRel(r, QStringLiteral(".git/config")));
        QVERIFY(!hasRel(r, QStringLiteral("node_modules/x.js")));
    }

    void userIgnorePatternsApply()
    {
        writeFile(QStringLiteral("photo.psd"), 6);
        writeFile(QStringLiteral("disk.iso"), 6);
        writeFile(QStringLiteral("keep.txt"), 6);

        const ScanResult r = scanFilesystem(snap(true, QStringLiteral("*.psd, *.iso")), kBig);
        QCOMPARE(r.files.size(), 1);
        QVERIFY(hasRel(r, QStringLiteral("keep.txt")));
        QVERIFY(!hasRel(r, QStringLiteral("photo.psd")));
        QVERIFY(!hasRel(r, QStringLiteral("disk.iso")));
    }

    // ── watchSubfolders=false → flat scan of root only ───────────────────────
    void watchSubfoldersFalseStaysFlat()
    {
        writeFile(QStringLiteral("rootfile.txt"), 4);
        writeFile(QStringLiteral("sub/inner.txt"), 4);

        const ScanResult r = scanFilesystem(snap(/*watchSubfolders=*/false), kBig);
        QCOMPARE(r.files.size(), 1);
        QVERIFY(hasRel(r, QStringLiteral("rootfile.txt")));
        QVERIFY(!hasRel(r, QStringLiteral("sub/inner.txt")));
    }

    // ── Oversized flag (simulated via a small cap) ──────────────────────────
    void oversizedFlaggedButStillListed()
    {
        writeFile(QStringLiteral("big.dat"), 100);
        writeFile(QStringLiteral("small.dat"), 5);

        const ScanResult r = scanFilesystem(snap(), /*maxFileSize=*/10);
        QCOMPARE(r.files.size(), 2);
        QVERIFY(fileOf(r, QStringLiteral("big.dat")).oversized);
        QVERIFY(!fileOf(r, QStringLiteral("small.dat")).oversized);
        // Oversized files must still be "seen" so the diff doesn't mark them Missing.
        QVERIFY(r.seenRel.contains(QStringLiteral("big.dat")));
    }

    // ── Cycle guard: a self-referential dir symlink must not hang ───────────
    void symlinkCycleTerminates()
    {
        writeFile(QStringLiteral("dir/file.txt"), 4);

        namespace fs = std::filesystem;
        const fs::path target = fs::path(root().toStdString()) / "dir";
        const fs::path link   = fs::path(root().toStdString()) / "dir" / "loop";
        std::error_code ec;
        fs::create_directory_symlink(target, link, ec);
        if (ec) {
            // No privilege to create symlinks (typical non-elevated Windows) —
            // the cycle guard is still exercised by the canonical-path dedupe
            // on every real run; nothing to assert here.
            QSKIP("cannot create directory symlink in this environment");
        }

        // The assertion that matters: this returns at all (no infinite loop).
        const ScanResult r = scanFilesystem(snap(), kBig);
        QVERIFY(hasRel(r, QStringLiteral("dir/file.txt")));
    }

    // ── Skip-list predicates (unit, no filesystem) ──────────────────────────
    void skipPredicatesDirect()
    {
        QVERIFY(scanShouldSkipFile(QStringLiteral(".env")));
        QVERIFY(scanShouldSkipFile(QStringLiteral("Thumbs.db")));
        QVERIFY(!scanShouldSkipFile(QStringLiteral("notes.txt")));
        QVERIFY(scanShouldSkipFile(QStringLiteral("a.iso"),
                                   {QStringLiteral("*.iso")}));
        QVERIFY(!scanShouldSkipFile(QStringLiteral("a.txt"),
                                    {QStringLiteral("*.iso")}));
        QVERIFY(scanShouldSkipDir(QStringLiteral("node_modules")));
        QVERIFY(scanShouldSkipDir(QStringLiteral(".git")));
        QVERIFY(!scanShouldSkipDir(QStringLiteral("Documents")));
    }

    void parseIgnorePatternsTrimsAndDropsEmpty()
    {
        const QStringList p = parseIgnorePatterns(QStringLiteral(" *.psd , ,*.iso "));
        QCOMPARE(p.size(), 2);
        QCOMPARE(p.at(0), QStringLiteral("*.psd"));
        QCOMPARE(p.at(1), QStringLiteral("*.iso"));
        QVERIFY(parseIgnorePatterns(QString()).isEmpty());
        QVERIFY(parseIgnorePatterns(QStringLiteral("   ")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSyncScan)
#include "test_sync_scan.moc"
