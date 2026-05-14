/*
 * StreamIX TV — C4 FileCard / C5 FolderCard / C13 Skeleton
 * Reference: 09 §8 (C4, C5, C13) + 14 §10
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.ProgressIndicatorDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsFocus
import vn.streamix.tv.core.ui.theme.FsMotion
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsFileCard(
    title: String,
    subtitle: String? = null,
    thumbnailUrl: String? = null,
    progressPercent: Int? = null,             // 0..100, hiển thị progress bar dưới thumbnail
    badge: FsBadgeVariant? = null,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val scale by animateFloatAsState(
        targetValue = if (isFocused) FsFocus.Scale else 1f,
        animationSpec = tween(FsMotion.DurationQuickMs, easing = FsMotion.EaseQuick),
        label = "scale",
    )

    Column(
        modifier = modifier
            .width(320.dp)
            .scale(scale)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(if (isFocused) FsColors.BgSurfaceHi else FsColors.BgSurface)
            .border(
                width = if (isFocused) FsFocus.RingWidth else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Md),
            )
            .clickable(
                interactionSource = interactionSource,
                indication = null,
                onClick = onClick,
            ),
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(180.dp)
                .background(FsColors.BgSurfaceHi),
        ) {
            if (thumbnailUrl != null) {
                AsyncImage(
                    model = thumbnailUrl,
                    contentDescription = title,
                    modifier = Modifier.fillMaxWidth().height(180.dp),
                )
            } else {
                // Placeholder cho file thuần
                Text("📁", modifier = Modifier.align(Alignment.Center), style = FsType.DisplayLg)
            }
            // Badge overlay (HD/4K/NEW/HOT/etc) — top-left
            if (badge != null) {
                FsBadge(
                    variant = badge,
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .padding(FsSpacing.S2),
                )
            }
            // Progress bar overlay (Continue Watching)
            if (progressPercent != null && progressPercent > 0) {
                LinearProgressIndicator(
                    progress = { progressPercent / 100f },
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .fillMaxWidth()
                        .height(4.dp),
                    color = FsColors.AccentPrimary,
                    trackColor = FsColors.BgScrim,
                    strokeCap = ProgressIndicatorDefaults.LinearStrokeCap,
                    gapSize = 0.dp,
                    drawStopIndicator = {},
                )
            }
        }
        Column(modifier = Modifier.padding(FsSpacing.S3)) {
            Text(
                text = title,
                style = FsType.LabelLg,
                color = FsColors.TextPrimary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (subtitle != null) {
                Text(
                    text = subtitle,
                    style = FsType.Caption,
                    color = FsColors.TextSecondary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
    }
}

@Composable
fun FsFolderCard(
    name: String,
    subtitle: String? = null,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    val scale by animateFloatAsState(
        if (isFocused) FsFocus.Scale else 1f,
        tween(FsMotion.DurationQuickMs, easing = FsMotion.EaseQuick),
        label = "scale",
    )
    Row(
        modifier = modifier
            .width(280.dp)
            .height(120.dp)
            .scale(scale)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(if (isFocused) FsColors.BgSurfaceHi else FsColors.BgSurface)
            .border(
                if (isFocused) FsFocus.RingWidth else 0.dp,
                if (isFocused) FsColors.FocusRing else Color.Transparent,
                RoundedCornerShape(FsRadius.Md),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(FsSpacing.S4),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("📁", style = FsType.DisplayMd, modifier = Modifier.size(48.dp))
        Column(modifier = Modifier.padding(start = FsSpacing.S3)) {
            Text(name, style = FsType.LabelLg, color = FsColors.TextPrimary, maxLines = 1, overflow = TextOverflow.Ellipsis)
            if (subtitle != null) {
                Text(subtitle, style = FsType.Caption, color = FsColors.TextSecondary, maxLines = 1)
            }
        }
    }
}

/** Skeleton placeholder (C13) — pulse opacity */
@Composable
fun FsCardSkeleton(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .width(320.dp)
            .height(260.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(FsColors.BgSurface),
    )
}
