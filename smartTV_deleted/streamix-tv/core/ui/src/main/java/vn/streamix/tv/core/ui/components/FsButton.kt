/*
 * StreamIX TV — C2 Button (primary / secondary / ghost)
 * Reference: 09 §8 + 14 §10
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

enum class FsButtonVariant { Primary, Secondary, Ghost, Destructive }

@Composable
fun FsButton(
    text: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    variant: FsButtonVariant = FsButtonVariant.Primary,
    enabled: Boolean = true,
    loading: Boolean = false,
    leadingIcon: (@Composable () -> Unit)? = null,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val (bg, fg, borderColor) = when (variant) {
        FsButtonVariant.Primary -> Triple(
            if (isFocused) FsColors.AccentPrimaryHi else FsColors.AccentPrimary,
            FsColors.TextInverse,
            Color.Transparent
        )
        FsButtonVariant.Secondary -> Triple(
            if (isFocused) FsColors.BgSurfaceHi else FsColors.BgSurface,
            FsColors.TextPrimary,
            FsColors.BorderDefault
        )
        FsButtonVariant.Ghost -> Triple(
            if (isFocused) FsColors.BgSurface else Color.Transparent,
            FsColors.TextPrimary,
            Color.Transparent
        )
        FsButtonVariant.Destructive -> Triple(
            if (isFocused) Color(0xFFD13D42) else FsColors.Danger,
            FsColors.TextPrimary,
            Color.Transparent
        )
    }

    val ringWidth by animateDpAsState(
        targetValue = if (isFocused) 4.dp else 0.dp,
        label = "ringWidth"
    )

    Row(
        modifier = modifier
            .defaultMinSize(minWidth = 140.dp, minHeight = 56.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(bg)
            .border(
                width = ringWidth,
                color = if (isFocused) FsColors.FocusRing else borderColor,
                shape = RoundedCornerShape(FsRadius.Md)
            )
            .clickable(
                enabled = enabled && !loading,
                interactionSource = interactionSource,
                indication = null,
                onClick = onClick,
            )
            .padding(horizontal = FsSpacing.S5, vertical = FsSpacing.S3),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2),
    ) {
        if (loading) {
            CircularProgressIndicator(
                color = fg,
                strokeWidth = 2.dp,
                modifier = Modifier.defaultMinSize(20.dp, 20.dp)
            )
        } else {
            leadingIcon?.invoke()
        }
        Text(
            text = text,
            style = FsType.ButtonLg,
            color = fg,
            textAlign = TextAlign.Center,
        )
    }
}
