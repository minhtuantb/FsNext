/*
 * StreamIX TV — Checkbox component (D-pad focusable).
 *
 * 24×24 box; checked state hiển thị tick (✓) trên amber bg.
 * Focus: 4dp ring + scale 1.05.
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsCheckbox(
    checked: Boolean,
    onToggle: () -> Unit,
    label: String,
    modifier: Modifier = Modifier,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val boxBg = when {
        isFocused && checked -> FsColors.AccentPrimaryHi
        checked -> FsColors.AccentPrimary
        isFocused -> FsColors.BgSurfaceHi
        else -> FsColors.BgSurface
    }
    val boxBorderColor = when {
        isFocused -> FsColors.FocusRing
        checked -> FsColors.AccentPrimary
        else -> FsColors.BorderDefault
    }
    val boxBorderWidth = if (isFocused) 3.dp else 1.dp

    Row(
        modifier = modifier
            .clickable(interactionSource, indication = null, onClick = onToggle),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Start,
    ) {
        Box(
            modifier = Modifier
                .size(28.dp)
                .clip(RoundedCornerShape(FsRadius.Sm))
                .background(boxBg)
                .border(boxBorderWidth, boxBorderColor, RoundedCornerShape(FsRadius.Sm)),
            contentAlignment = Alignment.Center,
        ) {
            if (checked) {
                Text(
                    text = "✓",
                    style = FsType.TitleMd,
                    color = FsColors.TextInverse,
                )
            }
        }
        Spacer(Modifier.width(FsSpacing.S3))
        Text(
            text = label,
            style = FsType.BodyMd,
            color = if (isFocused) FsColors.TextPrimary else FsColors.TextSecondary,
        )
    }
}
