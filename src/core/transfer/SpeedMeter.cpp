#include "SpeedMeter.h"
#include <QStringLiteral>
#include <algorithm>

namespace fsnext {

void SpeedMeter::start(int64_t totalBytes)
{
    reset();
    m_total = totalBytes;
    m_timer.start();
    m_lastSampleMs = 0;
    m_samples.push_back({ 0, 0 });
}

void SpeedMeter::markProgress(int64_t currentBytes)
{
    if (!m_timer.isValid()) {
        m_timer.start();
    }

    m_current = currentBytes;

    const qint64 now = m_timer.elapsed();
    if (now - m_lastSampleMs >= kSampleIntervalMs) {
        m_samples.push_back({ now, currentBytes });
        if (static_cast<int>(m_samples.size()) > kWindowSize) {
            m_samples.pop_front();
        }
        m_lastSampleMs = now;
        updateSpeed();
    }
}

void SpeedMeter::reset()
{
    m_timer.invalidate();
    m_total        = 0;
    m_current      = 0;
    m_speed        = 0.0;
    m_lastSampleMs = -1;
    m_samples.clear();
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void SpeedMeter::updateSpeed()
{
    if (m_samples.size() < 2) {
        m_speed = 0.0;
        return;
    }

    const Sample &oldest = m_samples.front();
    const Sample &newest = m_samples.back();

    const qint64  dtMs    = newest.timestampMs - oldest.timestampMs;
    const int64_t dBytes  = newest.bytes       - oldest.bytes;

    if (dtMs <= 0 || dBytes <= 0) {
        m_speed = 0.0;
        return;
    }

    m_speed = static_cast<double>(dBytes) / (static_cast<double>(dtMs) / 1000.0);
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

double SpeedMeter::speed() const
{
    return m_speed;
}

int SpeedMeter::etaSeconds() const
{
    if (m_speed <= 0.0 || m_total <= 0) return -1;
    const int64_t remaining = m_total - m_current;
    if (remaining <= 0) return 0;
    return static_cast<int>(static_cast<double>(remaining) / m_speed);
}

QString SpeedMeter::eta() const
{
    const int secs = etaSeconds();
    if (secs < 0) return QStringLiteral("—");

    const int h =  secs / 3600;
    const int m = (secs % 3600) / 60;
    const int s =  secs % 60;

    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
               .arg(h, 2, 10, QChar(u'0'))
               .arg(m, 2, 10, QChar(u'0'))
               .arg(s, 2, 10, QChar(u'0'));
    }
    return QStringLiteral("%1:%2")
           .arg(m, 2, 10, QChar(u'0'))
           .arg(s, 2, 10, QChar(u'0'));
}

double SpeedMeter::progress() const
{
    if (m_total <= 0) return 0.0;
    const double pct = static_cast<double>(m_current) / static_cast<double>(m_total) * 100.0;
    return std::clamp(pct, 0.0, 100.0);
}

qint64 SpeedMeter::elapsedMs() const
{
    return m_timer.isValid() ? m_timer.elapsed() : 0;
}

} // namespace fsnext
