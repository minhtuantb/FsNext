/*
 * StreamIX TV — Typography tokens
 * Reference: 09 §6.2 (sau quyết định bump body 22sp min — audit 11 AI-6)
 */
package vn.streamix.tv.core.ui.theme

import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

object FsType {
    // Display
    val DisplayLg = TextStyle(fontSize = 56.sp, lineHeight = 64.sp, fontWeight = FontWeight.Bold)
    val DisplayMd = TextStyle(fontSize = 48.sp, lineHeight = 56.sp, fontWeight = FontWeight.Bold)

    // Headline (restored sau audit 11 AI-6)
    val Headline = TextStyle(fontSize = 36.sp, lineHeight = 44.sp, fontWeight = FontWeight.SemiBold)

    // Title
    val TitleLg = TextStyle(fontSize = 32.sp, lineHeight = 40.sp, fontWeight = FontWeight.SemiBold)
    val TitleMd = TextStyle(fontSize = 28.sp, lineHeight = 36.sp, fontWeight = FontWeight.SemiBold)

    // Body — bumped lên min 22sp theo AI-6
    val BodyLg = TextStyle(fontSize = 24.sp, lineHeight = 32.sp, fontWeight = FontWeight.Medium)
    val BodyMd = TextStyle(fontSize = 22.sp, lineHeight = 30.sp, fontWeight = FontWeight.Normal)

    // Label
    val LabelLg = TextStyle(fontSize = 22.sp, lineHeight = 28.sp, fontWeight = FontWeight.SemiBold)
    val LabelMd = TextStyle(fontSize = 20.sp, lineHeight = 26.sp, fontWeight = FontWeight.Medium)

    // Button
    val ButtonLg = TextStyle(fontSize = 22.sp, lineHeight = 28.sp, fontWeight = FontWeight.SemiBold, letterSpacing = 0.4.sp)

    // Caption
    val Caption = TextStyle(fontSize = 18.sp, lineHeight = 24.sp, fontWeight = FontWeight.Normal)

    // Mono (dùng cho time, version)
    val Mono = TextStyle(fontSize = 22.sp, lineHeight = 30.sp, fontWeight = FontWeight.Medium)
}
