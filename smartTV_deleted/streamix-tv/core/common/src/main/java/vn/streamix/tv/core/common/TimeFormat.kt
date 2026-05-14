/*
 * StreamIX TV — Time formatters cho player
 * Reference: 13 S5a — caption-mono `tnum` cho time elapsed/remaining
 */
package vn.streamix.tv.core.common

import java.util.Locale

object TimeFormat {
    /**
     * Format millis -> HH:MM:SS (nếu > 1h) hoặc MM:SS (nếu < 1h).
     * Dùng cho Player timeline.
     */
    fun forPlayer(millis: Long): String {
        if (millis < 0) return "00:00"
        val totalSec = millis / 1000
        val hours = totalSec / 3600
        val minutes = (totalSec % 3600) / 60
        val seconds = totalSec % 60
        return if (hours > 0) {
            String.format(Locale.US, "%02d:%02d:%02d", hours, minutes, seconds)
        } else {
            String.format(Locale.US, "%02d:%02d", minutes, seconds)
        }
    }
}
