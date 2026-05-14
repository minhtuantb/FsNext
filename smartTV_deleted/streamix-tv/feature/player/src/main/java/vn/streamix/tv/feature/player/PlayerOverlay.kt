/*
 * StreamIX TV — Player Overlay (S5a)
 *
 * Custom Compose overlay phủ trên video. Hiển thị on-demand 5s rồi auto-hide.
 * Reference: 13 §11 + 14 §7 (S5a 3 states)
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────┐
 *   │ ← Back                       Title file              │  ← top zone (gradient đen)
 *   │                                                      │
 *   │              [video clean view]                      │
 *   │                                                      │
 *   │ ━━━━━━━━━━━●─────────────                            │
 *   │ 00:42:13                              02:18:00       │
 *   │ ⏮ ⏯ ⏭        [Phụ đề] [Audio] [⚙]                   │  ← bottom zone (gradient đen)
 *   └──────────────────────────────────────────────────────┘
 */
package vn.streamix.tv.feature.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.common.TimeFormat
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun PlayerOverlay(
    visible: Boolean,
    title: String,
    isPlaying: Boolean,
    currentMs: Long,
    durationMs: Long,
    bufferedMs: Long = 0L,
    onPlayPause: () -> Unit,
    onSeekBack10: () -> Unit,
    onSeekForward10: () -> Unit,
    onClickSubtitle: () -> Unit,
    onClickAudio: () -> Unit,
    onBack: () -> Unit,
    modifier: Modifier = Modifier,
) {
    AnimatedVisibility(
        visible = visible,
        enter = fadeIn(),
        exit = fadeOut(),
        modifier = modifier,
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            // === TOP ZONE (gradient từ đen → trong suốt) ===
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(
                        Brush.verticalGradient(
                            listOf(Color.Black.copy(alpha = 0.7f), Color.Transparent)
                        )
                    )
                    .padding(horizontal = FsSpacing.S7, vertical = FsSpacing.S5),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                IconCircleButton("←", onClick = onBack)
                Spacer(Modifier.width(FsSpacing.S5))
                Text(
                    text = title,
                    style = FsType.TitleMd,
                    color = FsColors.TextPrimary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f),
                )
            }

            // === BOTTOM ZONE ===
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.BottomCenter)
                    .background(
                        Brush.verticalGradient(
                            listOf(Color.Transparent, Color.Black.copy(alpha = 0.85f))
                        )
                    )
                    .padding(horizontal = FsSpacing.S7, vertical = FsSpacing.S5),
            ) {
                // Time row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                ) {
                    Text(
                        text = TimeFormat.forPlayer(currentMs),
                        style = FsType.Mono,
                        color = FsColors.TextPrimary,
                    )
                    Text(
                        text = TimeFormat.forPlayer(durationMs),
                        style = FsType.Mono,
                        color = FsColors.TextSecondary,
                    )
                }
                Spacer(Modifier.height(FsSpacing.S2))

                // Progress bar (timeline)
                val progress = if (durationMs > 0) currentMs.toFloat() / durationMs else 0f
                val bufferProgress = if (durationMs > 0) bufferedMs.toFloat() / durationMs else 0f
                Box(modifier = Modifier.fillMaxWidth().height(8.dp)) {
                    // Buffer track
                    LinearProgressIndicator(
                        progress = { bufferProgress },
                        modifier = Modifier.fillMaxWidth().height(8.dp),
                        color = FsColors.BorderStrong,
                        trackColor = FsColors.BorderSubtle,
                        gapSize = 0.dp,
                        drawStopIndicator = {},
                    )
                    // Played track
                    LinearProgressIndicator(
                        progress = { progress },
                        modifier = Modifier.fillMaxWidth().height(8.dp),
                        color = FsColors.AccentPrimary,
                        trackColor = Color.Transparent,
                        gapSize = 0.dp,
                        drawStopIndicator = {},
                    )
                }
                Spacer(Modifier.height(FsSpacing.S5))

                // Buttons row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                        IconCircleButton("⏪", onClick = onSeekBack10)
                        IconCircleButton(
                            icon = if (isPlaying) "⏸" else "▶",
                            onClick = onPlayPause,
                            primary = true,
                        )
                        IconCircleButton("⏩", onClick = onSeekForward10)
                    }
                    Spacer(Modifier.weight(1f))
                    Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                        IconCircleButton("CC", onClick = onClickSubtitle, label = "Phụ đề")
                        IconCircleButton("♪", onClick = onClickAudio, label = "Audio")
                    }
                }
            }
        }
    }
}

@Composable
private fun IconCircleButton(
    icon: String,
    onClick: () -> Unit,
    primary: Boolean = false,
    label: String? = null,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val bg = when {
        isFocused && primary -> FsColors.AccentPrimaryHi
        primary -> FsColors.AccentPrimary
        isFocused -> FsColors.BgSurfaceHi
        else -> FsColors.BgSurface.copy(alpha = 0.6f)
    }
    val fg = if (primary) FsColors.TextInverse else FsColors.TextPrimary

    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Box(
            modifier = Modifier
                .size(if (primary) 64.dp else 56.dp)
                .clip(CircleShape)
                .background(bg)
                .border(
                    width = if (isFocused) 3.dp else 0.dp,
                    color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                    shape = CircleShape,
                )
                .clickable(interactionSource, indication = null, onClick = onClick),
            contentAlignment = Alignment.Center,
        ) {
            Text(icon, style = FsType.TitleMd, color = fg)
        }
        if (label != null && isFocused) {
            Spacer(Modifier.height(FsSpacing.S1))
            Text(label, style = FsType.Caption, color = FsColors.TextPrimary)
        }
    }
}
