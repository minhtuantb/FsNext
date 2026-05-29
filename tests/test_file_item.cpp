// SPDX-License-Identifier: Proprietary
// FileItem model unit tests.
//
// FileItem is a header-only POD struct (src/core/models/FileItem.h). The only
// real behaviour worth testing is the type-dispatch helper pair
// isFolder()/isFile() and the documented default-ctor invariants (ids/sizes
// zero, all flags false, all QString fields empty). Pure assign-then-read of
// the remaining POD fields is trivial and intentionally NOT enumerated
// field-by-field — a single round-trip sanity check stands in for the lot.

#include <QtTest>
#include <cstdint>
#include "core/models/FileItem.h"

using fsnext::FileItem;

class TestFileItem : public QObject
{
    Q_OBJECT
private slots:
    // ATM-0314 — type=="folder" drives isFolder()==true / isFile()==false.
    void isFolderTrueForFolderType()
    {
        FileItem it;
        it.type = QStringLiteral("folder");
        QVERIFY(it.isFolder());
        QVERIFY(!it.isFile());
    }

    // ATM-0314 — type=="file" drives isFile()==true / isFolder()==false.
    void isFileTrueForFileType()
    {
        FileItem it;
        it.type = QStringLiteral("file");
        QVERIFY(it.isFile());
        QVERIFY(!it.isFolder());
    }

    // ATM-0314 — isFile() is defined as !isFolder(): the two helpers must always
    // be mutually exclusive, and anything that isn't the literal "folder"
    // (default-empty type, unexpected strings, wrong case) classifies as a file.
    void isFileIsComplementOfIsFolder_data()
    {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("expectFolder");
        QTest::newRow("default-empty") << QString()                    << false;
        QTest::newRow("file")          << QStringLiteral("file")       << false;
        QTest::newRow("folder")        << QStringLiteral("folder")     << true;
        QTest::newRow("case-Folder")   << QStringLiteral("Folder")     << false; // exact match only
        QTest::newRow("garbage")       << QStringLiteral("directory")  << false;
    }

    void isFileIsComplementOfIsFolder()
    {
        QFETCH(QString, type);
        QFETCH(bool, expectFolder);
        FileItem it;
        it.type = type;
        QCOMPARE(it.isFolder(), expectFolder);
        QCOMPARE(it.isFile(), !expectFolder);
    }

    // ATM-0315 / ATM-0317..0320 / ATM-0333 / ATM-0334 — default-ctor invariants.
    // Verifies every field that declares an explicit initializer: numeric zero,
    // all flags false, all QStrings empty. A fresh FileItem is therefore an
    // inert "file" with no identity, which downstream cache/transfer code relies
    // on. This single test stands in for the per-field POD default cases.
    void defaultConstructedInvariants()
    {
        FileItem it;
        QCOMPARE(it.id, static_cast<uint64_t>(0));
        QCOMPARE(it.size, static_cast<int64_t>(0));
        QCOMPARE(it.downloadCount, 0);

        QVERIFY(!it.secure);
        QVERIFY(!it.isPublic);
        QVERIFY(!it.directlink);
        QVERIFY(!it.hasPassword);
        QVERIFY(!it.deleted);
        QVERIFY(!it.copied);
        QVERIFY(!it.shared);

        QVERIFY(it.linkcode.isEmpty());
        QVERIFY(it.name.isEmpty());
        QVERIFY(it.type.isEmpty());
        QVERIFY(it.path.isEmpty());
        QVERIFY(it.hashIndex.isEmpty());
        QVERIFY(it.ownerId.isEmpty());
        QVERIFY(it.parentId.isEmpty());
        QVERIFY(it.description.isEmpty());
        QVERIFY(it.created.isEmpty());
        QVERIFY(it.modified.isEmpty());
        QVERIFY(it.lastDownload.isEmpty());
        QVERIFY(it.tIndex.isEmpty());

        // A default item is classified as a file (type is empty, not "folder").
        QVERIFY(it.isFile());
        QVERIFY(!it.isFolder());
    }

    // ATM-0313 / ATM-0333 / ATM-0334 / ATM-0315 — field round-trip sanity.
    // FileItem is a plain aggregate, so one representative round-trip (incl. the
    // full uint64 range for id, an int64 size, and a Unicode/special-char name)
    // confirms storage width and that no field mangles its value. The remaining
    // trivial scalar/string fields share the same storage mechanics and are not
    // enumerated individually.
    void fieldsRoundTrip()
    {
        FileItem it;
        it.id = UINT64_MAX;                                  // no truncation to int
        it.size = Q_INT64_C(1099511627776);                 // 1 TiB
        it.name = QStringLiteral("tài liệu @#&.pdf");        // unicode + specials
        it.linkcode = QStringLiteral("ABC123xyz");

        QCOMPARE(it.id, UINT64_MAX);
        QCOMPARE(it.size, Q_INT64_C(1099511627776));
        QCOMPARE(it.name, QStringLiteral("tài liệu @#&.pdf"));
        QCOMPARE(it.linkcode, QStringLiteral("ABC123xyz"));
    }
};

QTEST_GUILESS_MAIN(TestFileItem)
#include "test_file_item.moc"
