/*
 * StreamIX TV — Badge labels (HD, 4K, NEW, HOT, #1, OSCAR).
 *
 * Hiển thị overlay trên thumbnail card. Color/text tuỳ variant.
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.background
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

enum class FsBadgeVariant { HD, FourK, New, Hot, Top, Oscar }

@Composable
fun FsBadge(
    variant: FsBadgeVariant,
    modifier: Modifier = Modifier,
) {
    val (text, bg, fg) = when (variant) {
        FsBadgeVariant.HD -> Triple("HD", FsColors.AccentPrimary, FsColors.TextInverse)
        FsBadgeVariant.FourK -> Triple("4K", Color(0xFF6FCF97), FsColors.TextInverse)
        FsBadgeVariant.New -> Triple("NEW", Color(0xFFEB5757), FsColors.TextInverse)
        FsBadgeVariant.Hot -> Triple("HOT", Color(0xFFE5484D), FsColors.TextInverse)
        FsBadgeVariant.Top -> Triple("#1", FsColors.AccentPrimary, FsColors.TextInverse)
        FsBadgeVariant.Oscar -> Triple("OSCAR", Color(0xFFB28E40), FsColors.TextInverse)
    }
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(FsRadius.Sm))
            .background(bg)
            .padding(horizontal = FsSpacing.S2, vertical = 2.dp),
    ) {
        Text(text = text, style = FsType.Caption, color = fg)
    }
}

/**
 * Derive badge variant from FeaturedItem (V1: dùng filename pattern + tuổi update).
 * Trả null nếu không có badge.
 */
fun deriveBadgeVariant(
    title: String,
    isNew: Boolean = false,
    rank: Int? = null,
): FsBadgeVariant? {
    val n = title.lowercase()
    return when {
        rank == 1 -> FsBadgeVariant.Top
        isNew -> FsBadgeVariant.New
        "4k" in n || "2160p" in n -> FsBadgeVariant.FourK
        "1080p" in n || "hd" in n -> FsBadgeVariant.HD
        else -> null
    }
}
