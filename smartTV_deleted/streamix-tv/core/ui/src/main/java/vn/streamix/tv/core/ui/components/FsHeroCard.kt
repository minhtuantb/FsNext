/*
 * StreamIX TV — Hero card component cho Home (Gợi Ý / Xu Hướng / Top).
 *
 * Mockup: 3 cards lớn cạnh nhau, mỗi card có:
 *  - Background gradient theo theme color (yellow/red/blue)
 *  - Icon lớn ở góc trên trái
 *  - Title bold ở giữa
 *  - Subtitle small dưới title
 *  - Arrow chevron dưới phải
 *
 * Focus state: scale 1.05 + ring sáng + shadow.
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
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsFocus
import vn.streamix.tv.core.ui.theme.FsMotion
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

/**
 * Hero card variant (theme color preset).
 */
enum class FsHeroVariant { Suggested, Trending, Top }

@Composable
fun FsHeroCard(
    title: String,
    subtitle: String,
    icon: String,
    variant: FsHeroVariant,
    modifier: Modifier = Modifier,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val scale by animateFloatAsState(
        targetValue = if (isFocused) FsFocus.Scale else 1f,
        animationSpec = tween(FsMotion.DurationStandardMs),
        label = "hero-scale",
    )

    val (bgStart, bgEnd, fg) = when (variant) {
        FsHeroVariant.Suggested -> Triple(
            Color(0xFFFFB12C),  // amber
            Color(0xFFD68A00),
            Color(0xFF1B1B1B),
        )
        FsHeroVariant.Trending -> Triple(
            Color(0xFFB23A3A),  // dark red
            Color(0xFF6B1F1F),
            Color(0xFFF5F5F5),
        )
        FsHeroVariant.Top -> Triple(
            Color(0xFF1E2C4A),  // dark blue
            Color(0xFF0E1A30),
            Color(0xFFF5F5F5),
        )
    }

    Box(
        modifier = modifier
            .scale(scale)
            .clip(RoundedCornerShape(FsRadius.Lg))
            .background(Brush.verticalGradient(listOf(bgStart, bgEnd)))
            .border(
                width = if (isFocused) 4.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Lg),
            )
            .clickable(
                interactionSource = interactionSource,
                indication = null,
                onClick = onClick,
            )
            .padding(FsSpacing.S6),
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            // Icon emoji at top-left in circle bg
            Box(
                modifier = Modifier
                    .size(64.dp)
                    .clip(RoundedCornerShape(FsRadius.Md))
                    .background(Color.Black.copy(alpha = 0.25f)),
                contentAlignment = Alignment.Center,
            ) {
                Text(text = icon, style = FsType.DisplayMd, color = fg)
            }

            Spacer(modifier = Modifier.height(FsSpacing.S5))

            Text(
                text = title,
                style = FsType.DisplayMd,
                color = fg,
            )
            Spacer(modifier = Modifier.height(FsSpacing.S2))
            Text(
                text = subtitle,
                style = FsType.BodyMd,
                color = fg.copy(alpha = 0.8f),
            )

            Spacer(modifier = Modifier.weight(1f))

            // Arrow chevron at bottom-right
            Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                Spacer(modifier = Modifier.weight(1f))
                Text(
                    text = "›",
                    style = FsType.DisplayMd,
                    color = fg.copy(alpha = 0.7f),
                )
            }
        }
    }
}
