/*
 * StreamIX TV — File size formatter
 * Hiển thị file size theo locale VN (1.2 GB, 250 MB, ...)
 */
package vn.streamix.tv.core.common

import java.util.Locale

object FileSize {
    private val units = arrayOf("B", "KB", "MB", "GB", "TB", "PB")

    fun format(bytes: Long, locale: Locale = Locale.getDefault()): String {
        if (bytes < 0) return "—"
        if (bytes < 1024) return "$bytes B"
        var value = bytes.toDouble()
        var unitIdx = 0
        while (value >= 1024 && unitIdx < units.size - 1) {
            value /= 1024
            unitIdx++
        }
        val precision = if (value >= 100 || unitIdx == 0) 0 else 1
        return String.format(locale, "%.${precision}f %s", value, units[unitIdx])
    }
}
