// SPDX-License-Identifier: Proprietary
//
// Unit tests for FileCacheDB — the SQLite-backed metadata cache behind the
// file manager. Uses in-memory SQLite (":memory:") so tests are hermetic.
//
// Regression focus:
//   * Root folder lookup: parentId may arrive as a null QString (QString())
//     OR as the empty string (""); both must match rows stored with
//     parent_id='' (NOT NULL DEFAULT ''). The nonNullStr() helper in
//     FileCacheDB was added to fix a production bug where null QString bound
//     as SQL NULL and never matched.
//   * Folder-first ordering when sorting by name/size (Windows Explorer
//     behavior).
//   * Type normalization: legacy rows may store "0"/"1"; current code expects
//     "folder"/"file".

#include "core/cache/FileCacheDB.h"
#include "core/models/FileItem.h"

#include <QObject>
#include <QTest>
#include <QVector>

using fsnext::FileCacheDB;
using fsnext::FileItem;
using fsnext::FolderSyncState;

namespace {

FileItem makeItem(const QString &linkcode,
                  const QString &name,
                  const QString &type,
                  const QString &parentId = QStringLiteral(""),
                  qint64 size = 0,
                  const QString &modified = QStringLiteral("0"))
{
    FileItem f;
    f.linkcode = linkcode;
    f.name     = name;
    f.type     = type;
    f.parentId = parentId;
    f.size     = size;
    f.modified = modified;
    f.created  = QStringLiteral("0");
    return f;
}

} // namespace

class TestFileCacheDB : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // ── read/write roundtrip ────────────────────────────────
    void upsertAndQueryRoundtrip();
    void upsertPreservesFields();

    // ── root folder handling (the big regression) ───────────
    void rootFolderQueryWithNullParent();
    void rootFolderQueryWithEmptyParent();
    void rootFolderCountMatchesQuery();

    // ── sort semantics ──────────────────────────────────────
    void folderFirstSortByName();
    void folderFirstSortBySize();
    void sortByDateDoesNotGroupFolders();
    void sortByNameDescReversesFiles();

    // ── type filter ─────────────────────────────────────────
    void typeFilterFolderOnly();
    void typeFilterFileOnly();

    // ── mutations ───────────────────────────────────────────
    void renameFileUpdatesRow();
    void removeFilesDropsRows();
    void updateParentMovesRows();

    // ── sync state ──────────────────────────────────────────
    void syncStateTransitions();
    void clearFolderSyncResets();

    // ── FTS5 search ─────────────────────────────────────────
    void searchFindsByName();

    // ── folder tree ─────────────────────────────────────────
    void getFolderTreeReturnsFoldersOnly();

    // ── multi-user isolation ────────────────────────────────
    void userIdIsolatesRows();

private:
    FileCacheDB *m_db = nullptr;
    const QString kUser = QStringLiteral("110");
};

void TestFileCacheDB::init()
{
    m_db = new FileCacheDB;
    QVERIFY(m_db->open(QStringLiteral(":memory:")));
    QVERIFY(m_db->isOpen());
}

void TestFileCacheDB::cleanup()
{
    delete m_db;
    m_db = nullptr;
}

// ── read/write roundtrip ────────────────────────────────────

void TestFileCacheDB::upsertAndQueryRoundtrip()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("lc1"), QStringLiteral("hello.txt"),
                      QStringLiteral("file"), QStringLiteral(""), 42);
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].linkcode, QStringLiteral("lc1"));
    QCOMPARE(rows[0].name, QStringLiteral("hello.txt"));
    QCOMPARE(rows[0].size, qint64{42});
    QVERIFY(rows[0].isFile());
}

void TestFileCacheDB::upsertPreservesFields()
{
    FileItem f;
    f.linkcode    = QStringLiteral("lc2");
    f.name        = QStringLiteral("secret.zip");
    f.type        = QStringLiteral("file");
    f.parentId    = QStringLiteral("");
    f.size        = 1234567;
    f.secure      = true;
    f.hasPassword = true;
    f.directlink  = true;
    f.isPublic    = false;
    f.hashIndex   = QStringLiteral("abc123");
    f.description = QStringLiteral("Quarterly report");
    f.created     = QStringLiteral("1700000000");
    f.modified    = QStringLiteral("1700000500");

    m_db->upsertFiles(kUser, {f});
    auto rows = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(rows.size(), 1);

    const auto &g = rows[0];
    QCOMPARE(g.name, f.name);
    QCOMPARE(g.size, f.size);
    QVERIFY(g.secure);
    QVERIFY(g.hasPassword);
    QVERIFY(g.directlink);
    QVERIFY(!g.isPublic);
    QCOMPARE(g.hashIndex, f.hashIndex);
    QCOMPARE(g.description, f.description);
    // Timestamps are stored as epoch seconds, returned as decimal strings
    QCOMPARE(g.created, QStringLiteral("1700000000"));
    QCOMPARE(g.modified, QStringLiteral("1700000500"));
}

// ── root folder handling ────────────────────────────────────
//
// Root is represented throughout the app as QString() (null). SQLite stores
// parent_id as TEXT NOT NULL DEFAULT '', so queries must coerce null → ''.
// Before the nonNullStr() fix, root queries silently returned 0 rows even
// when the cache had fresh data.

void TestFileCacheDB::rootFolderQueryWithNullParent()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("r1"), QStringLiteral("A.txt"),
                      QStringLiteral("file"), QString());  // explicit null
    items << makeItem(QStringLiteral("r2"), QStringLiteral("B.txt"),
                      QStringLiteral("file"), QStringLiteral(""));
    m_db->upsertFiles(kUser, items);

    // The bug: querying with null parentId would bind as SQL NULL and miss
    // every row. After the fix, it must find both.
    auto rows = m_db->queryFiles(kUser, QString());
    QCOMPARE(rows.size(), 2);
}

void TestFileCacheDB::rootFolderQueryWithEmptyParent()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("r1"), QStringLiteral("A.txt"),
                      QStringLiteral("file"), QString());
    items << makeItem(QStringLiteral("r2"), QStringLiteral("B.txt"),
                      QStringLiteral("file"), QStringLiteral(""));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(rows.size(), 2);
}

void TestFileCacheDB::rootFolderCountMatchesQuery()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("r1"), QStringLiteral("A"),
                      QStringLiteral("folder"), QString());
    items << makeItem(QStringLiteral("r2"), QStringLiteral("B.txt"),
                      QStringLiteral("file"), QString());
    items << makeItem(QStringLiteral("r3"), QStringLiteral("C.txt"),
                      QStringLiteral("file"), QString());
    m_db->upsertFiles(kUser, items);

    const int cntNull  = m_db->countFiles(kUser, QString());
    const int cntEmpty = m_db->countFiles(kUser, QStringLiteral(""));
    QCOMPARE(cntNull, 3);
    QCOMPARE(cntEmpty, 3);
    QCOMPARE(m_db->queryFiles(kUser, QString()).size(), 3);
}

// ── sort semantics ──────────────────────────────────────────

void TestFileCacheDB::folderFirstSortByName()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("zebra.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("alpha.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("mango"),
                      QStringLiteral("folder"));
    items << makeItem(QStringLiteral("4"), QStringLiteral("banana"),
                      QStringLiteral("folder"));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("name"), true);
    QCOMPARE(rows.size(), 4);
    // Folders first (alphabetical), then files (alphabetical)
    QCOMPARE(rows[0].name, QStringLiteral("banana"));
    QVERIFY(rows[0].isFolder());
    QCOMPARE(rows[1].name, QStringLiteral("mango"));
    QVERIFY(rows[1].isFolder());
    QCOMPARE(rows[2].name, QStringLiteral("alpha.txt"));
    QVERIFY(rows[2].isFile());
    QCOMPARE(rows[3].name, QStringLiteral("zebra.txt"));
    QVERIFY(rows[3].isFile());
}

void TestFileCacheDB::folderFirstSortBySize()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("big.bin"),
                      QStringLiteral("file"), QStringLiteral(""), 9999);
    items << makeItem(QStringLiteral("2"), QStringLiteral("folderX"),
                      QStringLiteral("folder"), QStringLiteral(""), 0);
    items << makeItem(QStringLiteral("3"), QStringLiteral("small.bin"),
                      QStringLiteral("file"), QStringLiteral(""), 10);
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("size"), true);
    QCOMPARE(rows.size(), 3);
    // Folder first even though its size=0 would normally put it anywhere
    QVERIFY(rows[0].isFolder());
    // Then files asc by size
    QCOMPARE(rows[1].name, QStringLiteral("small.bin"));
    QCOMPARE(rows[2].name, QStringLiteral("big.bin"));
}

void TestFileCacheDB::sortByDateDoesNotGroupFolders()
{
    // By design, date sort is chronological — folders interleave with files.
    // This pins that contract so it doesn't silently regress.
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("new_file.txt"),
                      QStringLiteral("file"), QStringLiteral(""), 0,
                      QStringLiteral("3000"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("old_folder"),
                      QStringLiteral("folder"), QStringLiteral(""), 0,
                      QStringLiteral("1000"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("mid_file.txt"),
                      QStringLiteral("file"), QStringLiteral(""), 0,
                      QStringLiteral("2000"));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("date"), false); // desc
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].name, QStringLiteral("new_file.txt"));
    QCOMPARE(rows[1].name, QStringLiteral("mid_file.txt"));
    QCOMPARE(rows[2].name, QStringLiteral("old_folder"));
}

void TestFileCacheDB::sortByNameDescReversesFiles()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("Alpha.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("Beta.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("Zeta"),
                      QStringLiteral("folder"));
    items << makeItem(QStringLiteral("4"), QStringLiteral("Apple"),
                      QStringLiteral("folder"));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("name"), false);
    QCOMPARE(rows.size(), 4);
    // Folders still first, but in reverse alphabetical
    QVERIFY(rows[0].isFolder());
    QCOMPARE(rows[0].name, QStringLiteral("Zeta"));
    QVERIFY(rows[1].isFolder());
    QCOMPARE(rows[1].name, QStringLiteral("Apple"));
    // Then files reversed
    QCOMPARE(rows[2].name, QStringLiteral("Beta.txt"));
    QCOMPARE(rows[3].name, QStringLiteral("Alpha.txt"));
}

// ── type filter ─────────────────────────────────────────────

void TestFileCacheDB::typeFilterFolderOnly()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("doc.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("photos"),
                      QStringLiteral("folder"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("music"),
                      QStringLiteral("folder"));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("name"), true,
                                 QStringLiteral("folder"));
    QCOMPARE(rows.size(), 2);
    for (const auto &r : rows)
        QVERIFY(r.isFolder());
}

void TestFileCacheDB::typeFilterFileOnly()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("doc.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("photos"),
                      QStringLiteral("folder"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("song.mp3"),
                      QStringLiteral("file"));
    m_db->upsertFiles(kUser, items);

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""),
                                 QStringLiteral("name"), true,
                                 QStringLiteral("file"));
    QCOMPARE(rows.size(), 2);
    for (const auto &r : rows)
        QVERIFY(r.isFile());
}

// ── mutations ───────────────────────────────────────────────

void TestFileCacheDB::renameFileUpdatesRow()
{
    m_db->upsertFiles(kUser, {makeItem(QStringLiteral("lc"),
                                        QStringLiteral("old.txt"),
                                        QStringLiteral("file"))});

    m_db->renameFile(QStringLiteral("lc"), QStringLiteral("new.txt"));

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].name, QStringLiteral("new.txt"));
}

void TestFileCacheDB::removeFilesDropsRows()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("a"), QStringLiteral("A.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("b"), QStringLiteral("B.txt"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("c"), QStringLiteral("C.txt"),
                      QStringLiteral("file"));
    m_db->upsertFiles(kUser, items);

    m_db->removeFiles({QStringLiteral("a"), QStringLiteral("c")});

    auto rows = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].linkcode, QStringLiteral("b"));
}

void TestFileCacheDB::updateParentMovesRows()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("folderA"), QStringLiteral("A"),
                      QStringLiteral("folder"), QStringLiteral(""));
    items << makeItem(QStringLiteral("file1"), QStringLiteral("x.txt"),
                      QStringLiteral("file"), QStringLiteral(""));
    m_db->upsertFiles(kUser, items);

    // Move file1 into folderA
    m_db->updateParent({QStringLiteral("file1")}, QStringLiteral("folderA"));

    auto root = m_db->queryFiles(kUser, QStringLiteral(""));
    QCOMPARE(root.size(), 1);
    QCOMPARE(root[0].linkcode, QStringLiteral("folderA"));

    auto child = m_db->queryFiles(kUser, QStringLiteral("folderA"));
    QCOMPARE(child.size(), 1);
    QCOMPARE(child[0].linkcode, QStringLiteral("file1"));
}

// ── sync state ──────────────────────────────────────────────

void TestFileCacheDB::syncStateTransitions()
{
    const QString folder = QStringLiteral("");

    // Before any sync: no row
    auto before = m_db->getSyncState(kUser, folder);
    QVERIFY(!before.exists);

    m_db->setFolderSyncPartial(kUser, folder);
    auto partial = m_db->getSyncState(kUser, folder);
    QVERIFY(partial.exists);
    QVERIFY(!partial.isComplete);

    m_db->setFolderSyncComplete(kUser, folder);
    auto complete = m_db->getSyncState(kUser, folder);
    QVERIFY(complete.exists);
    QVERIFY(complete.isComplete);
    QVERIFY(complete.syncedAt > 0);
}

void TestFileCacheDB::clearFolderSyncResets()
{
    const QString folder = QStringLiteral("abc");
    m_db->setFolderSyncComplete(kUser, folder);
    QVERIFY(m_db->getSyncState(kUser, folder).exists);

    m_db->clearFolderSync(kUser, folder);
    QVERIFY(!m_db->getSyncState(kUser, folder).exists);
}

// ── FTS5 search ─────────────────────────────────────────────

void TestFileCacheDB::searchFindsByName()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("1"), QStringLiteral("report_q1.pdf"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("2"), QStringLiteral("photo.jpg"),
                      QStringLiteral("file"));
    items << makeItem(QStringLiteral("3"), QStringLiteral("report_q2.pdf"),
                      QStringLiteral("file"));
    m_db->upsertFiles(kUser, items);

    auto hits = m_db->searchFts(kUser, QStringLiteral("report"));
    QCOMPARE(hits.size(), 2);

    auto none = m_db->searchFts(kUser, QStringLiteral("nothing_here"));
    QCOMPARE(none.size(), 0);
}

// ── folder tree ─────────────────────────────────────────────

void TestFileCacheDB::getFolderTreeReturnsFoldersOnly()
{
    QVector<FileItem> items;
    items << makeItem(QStringLiteral("f1"), QStringLiteral("docs"),
                      QStringLiteral("folder"), QStringLiteral(""));
    items << makeItem(QStringLiteral("f2"), QStringLiteral("pics"),
                      QStringLiteral("folder"), QStringLiteral(""));
    items << makeItem(QStringLiteral("x1"), QStringLiteral("note.txt"),
                      QStringLiteral("file"), QStringLiteral(""));
    m_db->upsertFiles(kUser, items);

    auto tree = m_db->getFolderTree(kUser);
    QCOMPARE(tree.size(), 2);
    for (const auto &n : tree)
        QVERIFY(n.isFolder());
}

// ── multi-user isolation ────────────────────────────────────

void TestFileCacheDB::userIdIsolatesRows()
{
    m_db->upsertFiles(QStringLiteral("userA"),
                      {makeItem(QStringLiteral("a1"), QStringLiteral("a.txt"),
                                QStringLiteral("file"))});
    m_db->upsertFiles(QStringLiteral("userB"),
                      {makeItem(QStringLiteral("b1"), QStringLiteral("b.txt"),
                                QStringLiteral("file"))});

    auto a = m_db->queryFiles(QStringLiteral("userA"), QStringLiteral(""));
    auto b = m_db->queryFiles(QStringLiteral("userB"), QStringLiteral(""));
    QCOMPARE(a.size(), 1);
    QCOMPARE(b.size(), 1);
    QCOMPARE(a[0].linkcode, QStringLiteral("a1"));
    QCOMPARE(b[0].linkcode, QStringLiteral("b1"));
}

QTEST_MAIN(TestFileCacheDB)
#include "test_file_cache_db.moc"
