// SPDX-License-Identifier: Proprietary
// SpeedMeter unit tests — rolling-window speed and ETA calculation.
//
// SpeedMeter samples on its internal QElapsedTimer. We can't time-travel that
// timer in pure C++, so the tests use generous tolerances and rely on QTest's
// QTRY_VERIFY-style waiting where necessary. Where exact values matter we
// drive the meter through markProgress() spaced with QTest::qWait() and assert
// the order of magnitude / sign rather than exact bytes/sec.

#include <QtTest>
#include "core/transfer/SpeedMeter.h"

using fsnext::SpeedMeter;

class TestSpeedMeter : public QObject
{
    Q_OBJECT
private slots:
    void zeroBeforeStart()
    {
        SpeedMeter m;
        QCOMPARE(m.speed(), 0.0);
        QCOMPARE(m.etaSeconds(), -1);
        QCOMPARE(m.progress(), 0.0);
    }

    void progressTracksTotal()
    {
        SpeedMeter m;
        m.start(1000);
        m.markProgress(0);
        QCOMPARE(m.progress(), 0.0);
        m.markProgress(500);
        QVERIFY(qFuzzyCompare(m.progress(), 50.0));
        m.markProgress(1000);
        QVERIFY(qFuzzyCompare(m.progress(), 100.0));
    }

    void progressIsClampedTo100()
    {
        SpeedMeter m;
        m.start(1000);
        // markProgress with bytes > total can happen when servers send a
        // slightly different Content-Length than the actual stream. Don't
        // emit > 100 % to QML.
        m.markProgress(2000);
        QVERIFY(m.progress() <= 100.01);
    }

    void resetClearsState()
    {
        SpeedMeter m;
        m.start(1000);
        m.markProgress(500);
        m.reset();
        QCOMPARE(m.speed(), 0.0);
        QCOMPARE(m.progress(), 0.0);
        QCOMPARE(m.etaSeconds(), -1);
    }

    void speedIsPositiveWhenBytesArrive()
    {
        SpeedMeter m;
        m.start(1'000'000);
        m.markProgress(0);
        // Push a couple of samples spaced > sample interval (500 ms) so the
        // window has at least 2 distinct points to compute slope from.
        QTest::qWait(550);
        m.markProgress(100'000);
        QTest::qWait(550);
        m.markProgress(250'000);
        QVERIFY2(m.speed() > 0.0,
                 qPrintable(QStringLiteral("speed=%1 expected > 0").arg(m.speed())));
    }

    void etaFormattedNonEmptyOnceSpeedKnown()
    {
        SpeedMeter m;
        m.start(10'000'000);
        m.markProgress(0);
        QTest::qWait(550);
        m.markProgress(1'000'000);
        QTest::qWait(550);
        m.markProgress(2'000'000);
        const QString eta = m.eta();
        QVERIFY2(!eta.isEmpty(), "ETA expected non-empty after sustained progress");
    }

    void elapsedMonotonic()
    {
        SpeedMeter m;
        m.start(1000);
        const qint64 e0 = m.elapsedMs();
        QTest::qWait(20);
        const qint64 e1 = m.elapsedMs();
        QVERIFY2(e1 >= e0, qPrintable(QStringLiteral("e0=%1 e1=%2").arg(e0).arg(e1)));
    }
};

QTEST_GUILESS_MAIN(TestSpeedMeter)
#include "test_speed_meter.moc"
