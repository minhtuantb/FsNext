/*
 * StreamIX TV — Spacing & radius tokens
 * Reference: 09 §6.3 + §6.5 (4dp grid)
 */
package vn.streamix.tv.core.ui.theme

import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

object FsSpacing {
    val Zero: Dp = 0.dp
    val S1: Dp = 4.dp
    val S2: Dp = 8.dp
    val S3: Dp = 12.dp
    val S4: Dp = 16.dp
    val S5: Dp = 24.dp
    val S6: Dp = 32.dp
    val S7: Dp = 48.dp        // Overscan padding ngoài screen — 5% safe zone
    val S8: Dp = 64.dp
    val S9: Dp = 96.dp
}

object FsRadius {
    val None: Dp = 0.dp
    val Sm: Dp = 6.dp
    val Md: Dp = 12.dp
    val Lg: Dp = 16.dp
    val Xl: Dp = 24.dp
    val Full: Dp = 9999.dp
}

object FsFocus {
    val RingWidth: Dp = 4.dp
    const val Scale: Float = 1.05f
    val GlowBlur: Dp = 24.dp
    const val GlowAlpha: Float = 0.35f
}
