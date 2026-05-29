// SPDX-License-Identifier: Proprietary
// FileTypeHelper unit tests — extension extraction, semantic category
// classification, and short icon labels. Pure header-only logic (no .cpp,
// no QObject, no event loop): every method is static and deterministic.

#include <QtTest>
#include "core/util/FileTypeHelper.h"

using fsnext::FileTypeHelper;

class TestFileTypeHelper : public QObject
{
    Q_OBJECT
private slots:
    // ─── extension() ────────────────────────────────────────
    void extensionExtractsLowercaseSuffix()
    {
        // ATM-0407 / TC-0440: take the segment after the LAST dot, lowercased.
        QCOMPARE(FileTypeHelper::extension(QStringLiteral("Report.PDF")),
                 QStringLiteral("pdf"));
        QCOMPARE(FileTypeHelper::extension(QStringLiteral("a.b.TAR.GZ")),
                 QStringLiteral("gz"));
    }

    void extensionEmptyWhenNoSuffix()
    {
        // ATM-0407 / TC-0441: no dot, or a trailing dot, yields no extension.
        QCOMPARE(FileTypeHelper::extension(QStringLiteral("README")), QString());
        QCOMPARE(FileTypeHelper::extension(QStringLiteral("foo.")), QString());
    }

    void extensionDotfileHasNoExtension()
    {
        // ATM-0407 / TC-0442: a leading-dot dotfile (dot at index 0, dot<1)
        // is treated as having no extension, matching Unix convention.
        QCOMPARE(FileTypeHelper::extension(QStringLiteral(".bashrc")), QString());
    }

    // ─── category() ─────────────────────────────────────────
    void categoryClassifiesMainGroups()
    {
        // ATM-0408 / TC-0443: representative extension per primary group.
        QCOMPARE(FileTypeHelper::category(QStringLiteral("png")),
                 QStringLiteral("image"));
        QCOMPARE(FileTypeHelper::category(QStringLiteral("mkv")),
                 QStringLiteral("video"));
        QCOMPARE(FileTypeHelper::category(QStringLiteral("mp3")),
                 QStringLiteral("audio"));
        QCOMPARE(FileTypeHelper::category(QStringLiteral("rar")),
                 QStringLiteral("archive"));
        QCOMPARE(FileTypeHelper::category(QStringLiteral("xlsx")),
                 QStringLiteral("spreadsheet"));
    }

    void categoryUnknownAndEmptyAreGeneric()
    {
        // ATM-0408 / TC-0444: unmapped extension and empty input → "generic"
        // (empty short-circuits the lookup; unknown falls through every set).
        QCOMPARE(FileTypeHelper::category(QStringLiteral("qzx")),
                 QStringLiteral("generic"));
        QCOMPARE(FileTypeHelper::category(QString()),
                 QStringLiteral("generic"));
    }

    // ─── label() ────────────────────────────────────────────
    void labelReturnsCategoryLabelForCommonTypes()
    {
        // ATM-0406 / TC-0437: image/video/archive map to their category label.
        QCOMPARE(FileTypeHelper::label(QStringLiteral("jpg")),
                 QStringLiteral("IMG"));
        QCOMPARE(FileTypeHelper::label(QStringLiteral("mp4")),
                 QStringLiteral("VID"));
        QCOMPARE(FileTypeHelper::label(QStringLiteral("zip")),
                 QStringLiteral("ZIP"));
    }

    void labelUsesExtensionSpecificLabelForDocuments()
    {
        // ATM-0406 / TC-0438: document/spreadsheet families resolve to the
        // more-specific per-extension label rather than the category name.
        QCOMPARE(FileTypeHelper::label(QStringLiteral("pdf")),
                 QStringLiteral("PDF"));
        QCOMPARE(FileTypeHelper::label(QStringLiteral("docx")),
                 QStringLiteral("DOC"));
        QCOMPARE(FileTypeHelper::label(QStringLiteral("xlsx")),
                 QStringLiteral("XLS"));
    }

    void labelEmptyIsFileUnknownIsFirstThreeUpper()
    {
        // ATM-0406 / TC-0439: empty extension → "FILE"; an unmapped extension
        // falls through to the generic path (first 3 chars uppercased).
        QCOMPARE(FileTypeHelper::label(QString()), QStringLiteral("FILE"));
        QCOMPARE(FileTypeHelper::label(QStringLiteral("xyzq")),
                 QStringLiteral("XYZ"));
    }
};

QTEST_GUILESS_MAIN(TestFileTypeHelper)
#include "test_file_type_helper.moc"
