// SPDX-License-Identifier: Proprietary
// FileNameSanitizer unit tests — Windows-reserved chars, DOS device names,
// trailing dots/spaces, control chars, length cap with extension preservation.

#include <QtTest>
#include "core/util/FileNameSanitizer.h"

using fsnext::FileNameSanitizer::sanitize;

class TestFileNameSanitizer : public QObject
{
    Q_OBJECT
private slots:
    void passesPlainName()
    {
        QCOMPARE(sanitize(QStringLiteral("hello.txt")), QStringLiteral("hello.txt"));
    }

    void replacesPathSeparators()
    {
        const QString out = sanitize(QStringLiteral("a/b\\c.txt"));
        QVERIFY2(!out.contains('/') && !out.contains('\\'), qPrintable(out));
        QVERIFY2(out.endsWith(".txt"), qPrintable(out));
    }

    void replacesWindowsReservedChars()
    {
        const QString out = sanitize(QStringLiteral("a:b*c?d\"e<f>g|h.txt"));
        for (QChar c : { QChar(':'), QChar('*'), QChar('?'), QChar('"'),
                         QChar('<'), QChar('>'), QChar('|') }) {
            QVERIFY2(!out.contains(c),
                qPrintable(QStringLiteral("Sanitised name '%1' still contains '%2'")
                           .arg(out, QString(c))));
        }
    }

    void stripsControlChars()
    {
        QString in = QStringLiteral("hello");
        in.append(QChar(0x07));
        in.append(QChar(0x1F));
        in.append(QStringLiteral(".txt"));
        const QString out = sanitize(in);
        for (QChar c : out) QVERIFY(c.unicode() >= 0x20);
        QVERIFY(out.endsWith(".txt"));
    }

    void trimsTrailingDotsAndSpaces()
    {
        QCOMPARE(sanitize(QStringLiteral("name. ")), QStringLiteral("name"));
        QCOMPARE(sanitize(QStringLiteral("name...")), QStringLiteral("name"));
        QCOMPARE(sanitize(QStringLiteral("name   ")), QStringLiteral("name"));
    }

    void escapesDosDeviceNames()
    {
        // Reserved names must NOT be returned bare even when extension is present —
        // Windows treats "CON.txt" the same as "CON".
        for (const QString &reserved : { QStringLiteral("CON"),  QStringLiteral("PRN"),
                                          QStringLiteral("AUX"),  QStringLiteral("NUL"),
                                          QStringLiteral("COM1"), QStringLiteral("LPT9") }) {
            const QString bare    = sanitize(reserved);
            const QString withExt = sanitize(reserved + QStringLiteral(".txt"));
            QVERIFY2(bare.compare(reserved, Qt::CaseInsensitive) != 0,
                qPrintable(QStringLiteral("Reserved '%1' was returned bare").arg(reserved)));
            QVERIFY2(!withExt.startsWith(reserved + QStringLiteral(".")),
                qPrintable(QStringLiteral("'%1' was returned with .txt extension intact").arg(reserved)));
        }
    }

    void emptyInputReturnsDefault()
    {
        QCOMPARE(sanitize(QStringLiteral("")),    QStringLiteral("download"));
        QCOMPARE(sanitize(QStringLiteral("   ")), QStringLiteral("download"));
        QCOMPARE(sanitize(QStringLiteral("///")), QStringLiteral("download"));
    }

    void clampsLongNamesPreservingExtension()
    {
        const QString longBase(300, QLatin1Char('a'));
        const QString in = longBase + QStringLiteral(".tar.gz");
        const QString out = sanitize(in);
        QVERIFY2(out.length() <= 200, qPrintable(QString::number(out.length())));
        // Extension should still be the LAST .ext segment (".gz", per the
        // implementation's "last `.ext`, if present" contract).
        QVERIFY2(out.endsWith(QStringLiteral(".gz")), qPrintable(out));
    }
};

QTEST_GUILESS_MAIN(TestFileNameSanitizer)
#include "test_filename_sanitizer.moc"
