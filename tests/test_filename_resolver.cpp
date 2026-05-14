// SPDX-License-Identifier: Proprietary
// FileNameSanitizer::resolveConflict — covers ADR 003 D7 policy semantics.
//
// Tests use QTemporaryDir to put real files on disk so the policy logic
// (which uses QFile::exists) is exercised end-to-end.

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include "core/util/FileNameSanitizer.h"

using fsnext::FileNameSanitizer::resolveConflict;
using ConflictPolicy = fsnext::FileNameSanitizer::ConflictPolicy;

class TestFileNameResolver : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_tmp;

    QString writeDummyFile(const QString &name)
    {
        const QString full = QDir(m_tmp.path()).filePath(name);
        QFile f(full);
        f.open(QIODevice::WriteOnly);
        f.write("x");
        f.close();
        return full;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
    }

    void noConflictReturnsBareJoinedPath()
    {
        const QString r = resolveConflict(m_tmp.path(), QStringLiteral("fresh.txt"),
                                          ConflictPolicy::Rename);
        QCOMPARE(r, QDir(m_tmp.path()).filePath(QStringLiteral("fresh.txt")));
    }

    void overwritePolicyReturnsOriginalEvenWhenExists()
    {
        const QString existing = writeDummyFile(QStringLiteral("hit.txt"));
        const QString r = resolveConflict(m_tmp.path(), QStringLiteral("hit.txt"),
                                          ConflictPolicy::Overwrite);
        QCOMPARE(r, existing);
    }

    void skipPolicyReturnsEmptyWhenExists()
    {
        writeDummyFile(QStringLiteral("clash.txt"));
        const QString r = resolveConflict(m_tmp.path(), QStringLiteral("clash.txt"),
                                          ConflictPolicy::Skip);
        QVERIFY(r.isEmpty());
    }

    void renamePolicyAppendsParenNumber()
    {
        writeDummyFile(QStringLiteral("doc.pdf"));
        const QString r1 = resolveConflict(m_tmp.path(), QStringLiteral("doc.pdf"),
                                           ConflictPolicy::Rename);
        QCOMPARE(QFileInfo(r1).fileName(), QStringLiteral("doc (1).pdf"));

        // Now create the (1) too — next attempt should land on (2).
        writeDummyFile(QStringLiteral("doc (1).pdf"));
        const QString r2 = resolveConflict(m_tmp.path(), QStringLiteral("doc.pdf"),
                                           ConflictPolicy::Rename);
        QCOMPARE(QFileInfo(r2).fileName(), QStringLiteral("doc (2).pdf"));
    }

    void renameHandlesNoExtension()
    {
        writeDummyFile(QStringLiteral("README"));
        const QString r = resolveConflict(m_tmp.path(), QStringLiteral("README"),
                                          ConflictPolicy::Rename);
        // No extension = no trailing dot before " (1)".
        QCOMPARE(QFileInfo(r).fileName(), QStringLiteral("README (1)"));
    }

    void renameSanitizesUnsafeNameFirst()
    {
        // Path-traversal via "../" must be neutralised BEFORE conflict resolution.
        const QString r = resolveConflict(m_tmp.path(),
                                          QStringLiteral("../escape.txt"),
                                          ConflictPolicy::Rename);
        QVERIFY2(!r.contains(QStringLiteral("..")), qPrintable(r));
        QVERIFY2(QFileInfo(r).fileName().endsWith(QStringLiteral(".txt")), qPrintable(r));
    }

    void askPolicyMatchesRenameOutcome()
    {
        // Per ADR D7: Ask policy returns the candidate Rename WOULD produce, so
        // the prompt can show the user the suggested new name.
        writeDummyFile(QStringLiteral("ambig.txt"));
        const QString rRename = resolveConflict(m_tmp.path(), QStringLiteral("ambig.txt"),
                                                ConflictPolicy::Rename);
        const QString rAsk    = resolveConflict(m_tmp.path(), QStringLiteral("ambig.txt"),
                                                ConflictPolicy::Ask);
        QCOMPARE(rRename, rAsk);
    }
};

QTEST_GUILESS_MAIN(TestFileNameResolver)
#include "test_filename_resolver.moc"
