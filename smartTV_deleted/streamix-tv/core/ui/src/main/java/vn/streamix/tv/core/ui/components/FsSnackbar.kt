/*
 * StreamIX TV — C10 Snackbar (4 variants)
 * Reference: 09 §8 (C10) + 14 §8
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

enum class FsSnackbarType { Info, Success, Warning, Error }

data class FsSnackbarMessage(
    val text: String,
    val type: FsSnackbarType = FsSnackbarType.Info,
    val durationMs: Long = 4_000,
)

/**
 * Hiển thị Snackbar bottom-center, auto-dismiss.
 * Caller pass `current` state với 1 message hoặc null.
 */
@Composable
fun FsSnackbarHost(
    message: FsSnackbarMessage?,
    onDismiss: () -> Unit,
) {
    LaunchedEffect(message) {
        if (message != null) {
            delay(message.durationMs)
            onDismiss()
        }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(bottom = FsSpacing.S7),
        contentAlignment = Alignment.BottomCenter,
    ) {
        AnimatedVisibility(
            visible = message != null,
            enter = fadeIn() + slideInVertically { it },
            exit = fadeOut() + slideOutVertically { it },
        ) {
            message?.let { m ->
                val (bg, fg) = colorsFor(m.type)
                Row(
                    modifier = Modifier
                        .widthIn(min = 360.dp, max = 720.dp)
                        .clip(RoundedCornerShape(FsRadius.Md))
                        .background(bg)
                        .padding(horizontal = FsSpacing.S5, vertical = FsSpacing.S3),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3),
                ) {
                    Text(text = iconFor(m.type), style = FsType.TitleMd)
                    Text(
                        text = m.text,
                        style = FsType.BodyMd,
                        color = fg,
                    )
                }
            }
        }
    }
}

private fun colorsFor(type: FsSnackbarType): Pair<Color, Color> = when (type) {
    FsSnackbarType.Info -> FsColors.BgSurfaceHi to FsColors.TextPrimary
    FsSnackbarType.Success -> FsColors.Success to FsColors.TextPrimary
    FsSnackbarType.Warning -> FsColors.Warning to FsColors.TextInverse
    FsSnackbarType.Error -> FsColors.Danger to FsColors.TextPrimary
}

private fun iconFor(type: FsSnackbarType): String = when (type) {
    FsSnackbarType.Info -> "ℹ"
    FsSnackbarType.Success -> "✓"
    FsSnackbarType.Warning -> "⚠"
    FsSnackbarType.Error -> "✕"
}
