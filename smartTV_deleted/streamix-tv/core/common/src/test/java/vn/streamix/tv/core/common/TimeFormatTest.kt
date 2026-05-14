package vn.streamix.tv.core.common

import org.junit.Assert.assertEquals
import org.junit.Test

class TimeFormatTest {

    @Test
    fun zeroAndNegative() {
        assertEquals("00:00", TimeFormat.forPlayer(0))
        assertEquals("00:00", TimeFormat.forPlayer(-100))
    }

    @Test
    fun underHour_MMSS() {
        assertEquals("00:30", TimeFormat.forPlayer(30_000))
        assertEquals("05:00", TimeFormat.forPlayer(300_000))
        assertEquals("59:59", TimeFormat.forPlayer(59 * 60_000 + 59 * 1000L))
    }

    @Test
    fun overHour_HHMMSS() {
        assertEquals("01:00:00", TimeFormat.forPlayer(60 * 60 * 1000L))
        assertEquals("02:18:00", TimeFormat.forPlayer((2 * 3600 + 18 * 60) * 1000L))
    }
}
