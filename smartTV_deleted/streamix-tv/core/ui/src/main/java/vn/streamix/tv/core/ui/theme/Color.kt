/*
 * StreamIX TV — Color tokens
 * Reference: design package v1.0 tokens.style-dictionary.json + 09 §6.1
 */
package vn.streamix.tv.core.ui.theme

import androidx.compose.ui.graphics.Color

object FsColors {
    // Background
    val BgBase = Color(0xFF0F1218)
    val BgSurface = Color(0xFF1A1F2A)
    val BgSurfaceHi = Color(0xFF252B38)
    val BgScrim = Color(0xB8000000)            // 72% alpha

    // Text
    val TextPrimary = Color(0xFFEEF1F6)
    val TextSecondary = Color(0xFFA9B1BD)
    val TextTertiary = Color(0xFF7A8190)
    val TextDisabled = Color(0xFF5A6170)
    val TextInverse = Color(0xFF0F1218)

    // Accent (Fshare amber)
    val AccentPrimary = Color(0xFFFFB12C)
    val AccentPrimaryHi = Color(0xFFFFCB6B)
    val AccentPrimarySoft = Color(0x1FFFB12C)  // 12% alpha

    // State
    val Success = Color(0xFF30A46C)
    val Warning = Color(0xFFF5A524)
    val Danger = Color(0xFFE5484D)
    val Info = Color(0xFFFFB12C)

    // Border
    val BorderSubtle = Color(0x0FFFFFFF)        // 6% alpha
    val BorderDefault = Color(0x1FFFFFFF)       // 12%
    val BorderStrong = Color(0x3DFFFFFF)        // 24%

    // Focus
    val FocusRing = Color(0xFFFFFFFF)
    val FocusGlow = Color(0x59FFB12C)           // 35% alpha
}
