#pragma once
#include <QElapsedTimer>
#include <QString>
#include <cstdint>
#include <deque>

namespace fsnext {

class SpeedMeter {
public:
    SpeedMeter() = default;
    ~SpeedMeter() = default;

    void start(int64_t totalBytes);
    void markProgress(int64_t currentBytes);
    void reset();

    // Getters
    double  speed()      const;  // bytes/sec (rolling window average)
    QString eta()        const;  // formatted "hh:mm:ss" or "—"
    int     etaSeconds() const;  // raw seconds remaining (-1 if unknown)
    double  progress()   const;  // 0.0 – 100.0
    qint64  elapsedMs()  const;

private:
    struct Sample {
        qint64  timestampMs;
        int64_t bytes;
    };

    static constexpr int    kWindowSize    = 10;
    static constexpr qint64 kSampleIntervalMs = 500;

    QElapsedTimer m_timer;
    int64_t       m_total   = 0;
    int64_t       m_current = 0;
    double        m_speed   = 0.0;   // cached rolling average (bytes/sec)

    std::deque<Sample> m_samples;    // sliding window of up to kWindowSize samples
    qint64             m_lastSampleMs = -1;

    void updateSpeed();
};

} // namespace fsnext
