// SPDX-License-Identifier: Proprietary
// FormatUtil unit tests — pure formatting/parsing helpers (no QObject, no
// network, no event loop). Covers humanBytes / humanSpeed / parseTimestamp /
// formatDateTime / formatDate. Date/time output is locale-dependent, so the
// formatted-string tests assert the *contract* (pattern shape, not-fallback,
// presence/absence of a time component) rather than exact localized strings.

#include <QtTest>
#include <QRegularExpression>
#include <cmath>
#include <limits>

#include "core/util/FormatUtil.h"

using namespace fsnext;

class TestFormatUtil : public QObject
{
    Q_OBJECT
private slots:
    // ── ATM-0409 humanBytes ────────────────────────────────
    void humanBytes_scaleAndDecimals()
    {
        // ATM-0409 / TC-0445
        QCOMPARE(FormatUtil::humanBytes(0), QStringLiteral("0 B"));
        // 1536 = 1.5 * 1024 → KB tier, value 1.5 (<10) → 2 decimals.
        QCOMPARE(FormatUtil::humanBytes(1536), QStringLiteral("1.50 KB"));
        // 1572864 = 1.5 * 1024 * 1024 → MB tier, 2 decimals.
        QCOMPARE(FormatUtil::humanBytes(1572864), QStringLiteral("1.50 MB"));
        // Bytes tier never shows decimals.
        QCOMPARE(FormatUtil::humanBytes(512), QStringLiteral("512 B"));
        // ≥100 within a tier → integer (no decimals): 200 KB.
        QCOMPARE(FormatUtil::humanBytes(200 * 1024), QStringLiteral("200 KB"));
        // ≥10 and <100 → 1 decimal: 10.5 KB = 10752 bytes.
        QCOMPARE(FormatUtil::humanBytes(10752), QStringLiteral("10.5 KB"));
    }

    void humanBytes_zeroAndNegative()
    {
        // ATM-0409 / TC-0446
        QVERIFY(FormatUtil::humanBytes(0, /*emptyOnZero*/ true).isEmpty());
        QVERIFY(FormatUtil::humanBytes(-5, /*emptyOnZero*/ true).isEmpty());
        QCOMPARE(FormatUtil::humanBytes(-5, /*emptyOnZero*/ false), QStringLiteral("0 B"));
        // Default arg (emptyOnZero=false) on a negative also yields "0 B".
        QCOMPARE(FormatUtil::humanBytes(-1), QStringLiteral("0 B"));
    }

    // ── ATM-0410 humanSpeed ────────────────────────────────
    void humanSpeed_positiveAppendsPerSecond()
    {
        // ATM-0410 / TC-0447
        QCOMPARE(FormatUtil::humanSpeed(1572864.0), QStringLiteral("1.50 MB/s"));
    }

    void humanSpeed_nonPositiveAndNonFinite()
    {
        // ATM-0410 / TC-0448 — guards bps<=0 and non-finite BEFORE the cast.
        QVERIFY(FormatUtil::humanSpeed(0.0).isEmpty());
        QVERIFY(FormatUtil::humanSpeed(-10.0).isEmpty());
        QVERIFY(FormatUtil::humanSpeed(std::numeric_limits<double>::quiet_NaN()).isEmpty());
        QVERIFY(FormatUtil::humanSpeed(std::numeric_limits<double>::infinity()).isEmpty());
        QVERIFY(FormatUtil::humanSpeed(-std::numeric_limits<double>::infinity()).isEmpty());
    }

    // ── ATM-0411 parseTimestamp ────────────────────────────
    void parseTimestamp_epochAndIso()
    {
        // ATM-0411 / TC-0449
        // Numeric epoch string parses verbatim (toLongLong path).
        QCOMPARE(FormatUtil::parseTimestamp(QStringLiteral("1700000000")), Q_INT64_C(1700000000));
        // ISO-8601 with explicit Z (UTC) → corresponding epoch seconds.
        // 2023-11-14T22:13:20Z == 1700000000.
        QCOMPARE(FormatUtil::parseTimestamp(QStringLiteral("2023-11-14T22:13:20Z")),
                 Q_INT64_C(1700000000));
    }

    void parseTimestamp_emptyAndInvalid()
    {
        // ATM-0411 / TC-0450
        QCOMPARE(FormatUtil::parseTimestamp(QString()), Q_INT64_C(0));
        QCOMPARE(FormatUtil::parseTimestamp(QStringLiteral("")), Q_INT64_C(0));
        QCOMPARE(FormatUtil::parseTimestamp(QStringLiteral("abc")), Q_INT64_C(0));
    }

    // ── ATM-0412 formatDateTime ────────────────────────────
    void formatDateTime_validEpochMatchesPattern()
    {
        // ATM-0412 / TC-0451 — explicit "dd/MM/yyyy HH:mm" format (locale-
        // independent layout), so assert the exact shape rather than a string.
        const QString out = FormatUtil::formatDateTime(QStringLiteral("1700000000"));
        QVERIFY2(out != QStringLiteral("—"), qPrintable(out));
        const QRegularExpression re(QStringLiteral("^\\d{2}/\\d{2}/\\d{4} \\d{2}:\\d{2}$"));
        QVERIFY2(re.match(out).hasMatch(), qPrintable(out));
    }

    void formatDateTime_invalidFallsBack()
    {
        // ATM-0412 / TC-0452
        QCOMPARE(FormatUtil::formatDateTime(QString()), QStringLiteral("—"));
        QCOMPARE(FormatUtil::formatDateTime(QStringLiteral("junk"), QStringLiteral("N/A")),
                 QStringLiteral("N/A"));
    }

    // ── ATM-0413 formatDate ────────────────────────────────
    void formatDate_validEpochHasNoTimeComponent()
    {
        // ATM-0413 / TC-0453 — date-only uses the system short-date locale, so
        // we can't pin the exact string; assert it differs from the fallback
        // and carries no "HH:mm" clock component.
        const QString out = FormatUtil::formatDate(QStringLiteral("1700000000"));
        QVERIFY2(out != QStringLiteral("—"), qPrintable(out));
        QVERIFY2(!out.contains(QRegularExpression(QStringLiteral("\\d{1,2}:\\d{2}"))),
                 qPrintable(out));
        // Sanity: the date-time variant of the same input *does* differ
        // (it appends the clock), confirming formatDate dropped the time.
        QVERIFY(out != FormatUtil::formatDateTime(QStringLiteral("1700000000")));
    }

    void formatDate_invalidFallsBack()
    {
        // ATM-0413 / TC-0454 — "0" parses as epoch 0 → secs<=0 → fallback.
        QCOMPARE(FormatUtil::formatDate(QStringLiteral("0")), QStringLiteral("—"));
        QCOMPARE(FormatUtil::formatDate(QStringLiteral("xx"), QStringLiteral("-")),
                 QStringLiteral("-"));
    }
};

QTEST_GUILESS_MAIN(TestFormatUtil)
#include "test_format_util.moc"
