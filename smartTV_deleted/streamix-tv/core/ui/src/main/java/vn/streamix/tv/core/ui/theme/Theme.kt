/*
 * StreamIX TV — Compose theme entry point
 * Reference: 13 §3.6 mapping JSON → Compose, §4 Common patterns
 *
 * StreamIX dark-only V1 (light theme = V2 nice-to-have).
 */
package vn.streamix.tv.core.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.tv.material3.darkColorScheme as tvDarkColorScheme

private val FsDarkColorScheme = darkColorScheme(
    primary = FsColors.AccentPrimary,
    onPrimary = FsColors.TextInverse,
    secondary = FsColors.AccentPrimaryHi,
    onSecondary = FsColors.TextInverse,
    background = FsColors.BgBase,
    onBackground = FsColors.TextPrimary,
    surface = FsColors.BgSurface,
    onSurface = FsColors.TextPrimary,
    surfaceVariant = FsColors.BgSurfaceHi,
    onSurfaceVariant = FsColors.TextSecondary,
    error = FsColors.Danger,
    onError = FsColors.TextPrimary,
    outline = FsColors.BorderDefault,
    outlineVariant = FsColors.BorderSubtle,
)

private val FsTvDarkColorScheme = tvDarkColorScheme(
    primary = FsColors.AccentPrimary,
    onPrimary = FsColors.TextInverse,
    background = FsColors.BgBase,
    onBackground = FsColors.TextPrimary,
    surface = FsColors.BgSurface,
    onSurface = FsColors.TextPrimary,
    surfaceVariant = FsColors.BgSurfaceHi,
    onSurfaceVariant = FsColors.TextSecondary,
    error = FsColors.Danger,
)

private val FsTypography = Typography(
    displayLarge = FsType.DisplayLg,
    displayMedium = FsType.DisplayMd,
    headlineMedium = FsType.Headline,
    titleLarge = FsType.TitleLg,
    titleMedium = FsType.TitleMd,
    bodyLarge = FsType.BodyLg,
    bodyMedium = FsType.BodyMd,
    labelLarge = FsType.LabelLg,
    labelMedium = FsType.LabelMd,
    labelSmall = FsType.Caption,
)

/** Compose-TV equivalent of MaterialTheme + custom token CompositionLocals. */
@Composable
fun StreamIXTheme(content: @Composable () -> Unit) {
    androidx.tv.material3.MaterialTheme(
        colorScheme = FsTvDarkColorScheme,
        typography = androidx.tv.material3.Typography(
            displayLarge = FsType.DisplayLg,
            displayMedium = FsType.DisplayMd,
            headlineMedium = FsType.Headline,
            titleLarge = FsType.TitleLg,
            titleMedium = FsType.TitleMd,
            bodyLarge = FsType.BodyLg,
            bodyMedium = FsType.BodyMd,
            labelLarge = FsType.LabelLg,
            labelMedium = FsType.LabelMd,
            labelSmall = FsType.Caption,
        ),
    ) {
        // Cũng provide Material3 theme cho component fallback (TextField chưa có TV variant)
        MaterialTheme(
            colorScheme = FsDarkColorScheme,
            typography = FsTypography,
        ) {
            CompositionLocalProvider(
                LocalFsColors provides FsColors,
                LocalFsSpacing provides FsSpacing,
                LocalFsRadius provides FsRadius,
                LocalFsType provides FsType,
                content = content,
            )
        }
    }
}

// === Composition locals cho design tokens custom ===
val LocalFsColors = staticCompositionLocalOf { FsColors }
val LocalFsSpacing = staticCompositionLocalOf { FsSpacing }
val LocalFsRadius = staticCompositionLocalOf { FsRadius }
val LocalFsType = staticCompositionLocalOf { FsType }
