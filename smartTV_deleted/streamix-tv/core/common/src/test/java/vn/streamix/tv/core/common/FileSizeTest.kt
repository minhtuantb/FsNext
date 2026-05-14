package vn.streamix.tv.core.common

import org.junit.Assert.assertEquals
import org.junit.Test
import java.util.Locale

class FileSizeTest {

    @Test
    fun bytes() {
        assertEquals("0 B", FileSize.format(0, Locale.US))
        assertEquals("999 B", FileSize.format(999, Locale.US))
    }

    @Test
    fun kilobytes() {
        assertEquals("1.0 KB", FileSize.format(1024, Locale.US))
        assertEquals("1.5 KB", FileSize.format(1536, Locale.US))
    }

    @Test
    fun megabytes() {
        assertEquals("1.0 MB", FileSize.format(1024L * 1024, Locale.US))
        assertEquals("250 MB", FileSize.format(262_144_000, Locale.US))
    }

    @Test
    fun gigabytes() {
        assertEquals("12.4 GB", FileSize.format(13_314_898_330, Locale.US))
    }

    @Test
    fun negativeReturnsDash() {
        assertEquals("—", FileSize.format(-1, Locale.US))
    }
}
