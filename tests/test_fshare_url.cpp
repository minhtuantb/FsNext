// SPDX-License-Identifier: Proprietary
// FshareUrl parser unit tests — cover the tolerance matrix.

#include <QtTest>
#include "core/util/FshareUrl.h"

using fsnext::FshareUrl::Kind;
using fsnext::FshareUrl::parse;
using fsnext::FshareUrl::canonicalUrl;
using fsnext::FshareUrl::linkcodeOf;
using fsnext::FshareUrl::isFshareUrl;
using fsnext::FshareUrl::isFolderUrl;
using fsnext::FshareUrl::isFileUrl;

class TestFshareUrl : public QObject
{
    Q_OBJECT
private slots:
    void parsesPlainFileLink()
    {
        const auto p = parse(QStringLiteral("https://www.fshare.vn/file/ABCDEF1234"));
        QCOMPARE(p.kind, Kind::File);
        QCOMPARE(p.linkcode, QStringLiteral("ABCDEF1234"));
    }

    void parsesPlainFolderLink()
    {
        const auto p = parse(QStringLiteral("https://www.fshare.vn/folder/QWERTY9876"));
        QCOMPARE(p.kind, Kind::Folder);
        QCOMPARE(p.linkcode, QStringLiteral("QWERTY9876"));
    }

    void caseInsensitiveHostAndPath()
    {
        const auto p = parse(QStringLiteral("HTTPS://Www.FSHARE.VN/File/AbCdEf1234"));
        QCOMPARE(p.kind, Kind::File);
        QCOMPARE(p.linkcode, QStringLiteral("AbCdEf1234"));
    }

    void tolerantToWhitespaceAndScheme()
    {
        QCOMPARE(parse(QStringLiteral("  fshare.vn/file/AAA111  ")).kind, Kind::File);
        QCOMPARE(parse(QStringLiteral("//fshare.vn/file/BBB222")).kind, Kind::File);
        QCOMPARE(parse(QStringLiteral("http://fshare.vn/file/CCC333")).kind, Kind::File);
    }

    void trailingSlashOk()
    {
        const auto p = parse(QStringLiteral("https://fshare.vn/file/XYZ/"));
        QCOMPARE(p.kind, Kind::File);
        QCOMPARE(p.linkcode, QStringLiteral("XYZ"));
    }

    void rejectsNonFshareHost()
    {
        QCOMPARE(parse(QStringLiteral("https://example.com/file/ABC")).kind, Kind::Invalid);
        QCOMPARE(parse(QStringLiteral("https://fshare.evil.com/file/ABC")).kind, Kind::Invalid);
    }

    void rejectsMissingLinkcode()
    {
        QCOMPARE(parse(QStringLiteral("https://fshare.vn/file/")).kind,   Kind::Invalid);
        QCOMPARE(parse(QStringLiteral("https://fshare.vn/folder")).kind,  Kind::Invalid);
        QCOMPARE(parse(QStringLiteral("https://fshare.vn/")).kind,        Kind::Invalid);
        QCOMPARE(parse(QStringLiteral("")).kind,                          Kind::Invalid);
    }

    void canonicalUrlPreservesShareToken()
    {
        const QString out = canonicalUrl(
            QStringLiteral("HTTPS://fshare.vn/folder/ABC?token=secret123"));
        QVERIFY2(out.contains(QStringLiteral("token=secret123")), qPrintable(out));
        QVERIFY2(out.contains(QStringLiteral("/folder/ABC")), qPrintable(out));
    }

    void canonicalUrlDropsOtherQueryAndFragment()
    {
        const QString out = canonicalUrl(
            QStringLiteral("https://fshare.vn/file/ABC?utm_source=x&ref=y#section"));
        QVERIFY2(!out.contains('?') && !out.contains('#'), qPrintable(out));
        QVERIFY2(out.endsWith(QStringLiteral("/file/ABC")), qPrintable(out));
    }

    void linkcodeOfStripsToken()
    {
        QCOMPARE(linkcodeOf(QStringLiteral("https://fshare.vn/file/ABC?token=secret")),
                 QStringLiteral("ABC"));
        QCOMPARE(linkcodeOf(QStringLiteral("https://fshare.vn/folder/XYZ/")),
                 QStringLiteral("XYZ"));
    }

    void inlineHelpersAgreeWithParse()
    {
        QVERIFY(isFshareUrl(QStringLiteral("https://fshare.vn/file/AA1")));
        QVERIFY(isFileUrl(  QStringLiteral("https://fshare.vn/file/AA1")));
        QVERIFY(!isFolderUrl(QStringLiteral("https://fshare.vn/file/AA1")));
        QVERIFY(isFolderUrl(QStringLiteral("https://fshare.vn/folder/BB2")));
        QVERIFY(!isFshareUrl(QStringLiteral("https://example.com/file/AA1")));
    }
};

QTEST_GUILESS_MAIN(TestFshareUrl)
#include "test_fshare_url.moc"
