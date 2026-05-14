/*
 * StreamIX TV — C19 Chip / Tag (filled / outlined)
 * Reference: 09 §8 (C19) — dùng cho metadata badges (HD, VIP, MKV, AC3)
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

enum class FsChipVariant { Filled, Outlined, Accent }

@Composable
fun FsChip(
    text: String,
    variant: FsChipVariant = FsChipVariant.Filled,
    modifier: Modifier = Modifier,
) {
    val (bg, fg, border) = when (variant) {
        FsChipVariant.Filled -> Triple(FsColors.BgSurfaceHi, FsColors.TextPrimary, Color.Transparent)
        FsChipVariant.Outlined -> Triple(Color.Transparent, FsColors.TextSecondary, FsColors.BorderDefault)
        FsChipVariant.Accent -> Triple(FsColors.AccentPrimarySoft, FsColors.AccentPrimary, Color.Transparent)
    }
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(FsRadius.Sm))
            .background(bg)
            .border(width = if (variant == FsChipVariant.Outlined) 1.dp else 0.dp,
                    color = border, shape = RoundedCornerShape(FsRadius.Sm))
            .padding(horizontal = FsSpacing.S3, vertical = FsSpacing.S1),
    ) {
        Text(text = text, style = FsType.LabelMd, color = fg)
    }
}
