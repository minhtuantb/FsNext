/*
 * StreamIX TV — On-screen QWERTY keyboard cho TV remote D-pad input.
 *
 * 4 rows:
 *   Row 0: 1 2 3 4 5 6 7 8 9 0
 *   Row 1: Q W E R T Y U I O P
 *   Row 2: A S D F G H J K L
 *   Row 3: Z X C V B N M ⌫
 *   Row 4: SPACE   OK
 *
 * Mỗi key D-pad focusable. Center button → onKey callback.
 * Focus state: amber bg + scale.
 *
 * Reference: design system mockup S12.
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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

/** Sự kiện từ keyboard. */
sealed interface FsKeyEvent {
    data class Char(val value: Char) : FsKeyEvent
    data object Backspace : FsKeyEvent
    data object Space : FsKeyEvent
    data object Enter : FsKeyEvent
}

private val ROW_DIGITS = listOf("1", "2", "3", "4", "5", "6", "7", "8", "9", "0")
private val ROW_TOP = listOf("Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P")
private val ROW_MID = listOf("A", "S", "D", "F", "G", "H", "J", "K", "L")
private val ROW_BOT = listOf("Z", "X", "C", "V", "B", "N", "M")

@Composable
fun FsKeyboard(
    modifier: Modifier = Modifier,
    onKey: (FsKeyEvent) -> Unit,
) {
    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S2),
    ) {
        KeyRow(ROW_DIGITS, onKey)
        KeyRow(ROW_TOP, onKey)
        KeyRow(ROW_MID, onKey)
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2),
        ) {
            ROW_BOT.forEach { key -> CharKey(key, onKey, modifier = Modifier.weight(1f)) }
            ActionKey(
                label = "⌫",
                modifier = Modifier.weight(1.5f),
                onClick = { onKey(FsKeyEvent.Backspace) },
            )
        }
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2),
        ) {
            ActionKey(
                label = "SPACE",
                modifier = Modifier.weight(3f),
                onClick = { onKey(FsKeyEvent.Space) },
            )
            ActionKey(
                label = "OK",
                modifier = Modifier.weight(1f),
                primary = true,
                onClick = { onKey(FsKeyEvent.Enter) },
            )
        }
    }
}

@Composable
private fun KeyRow(keys: List<String>, onKey: (FsKeyEvent) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2),
    ) {
        keys.forEach { key ->
            CharKey(key, onKey, modifier = Modifier.weight(1f))
        }
    }
}

@Composable
private fun CharKey(
    label: String,
    onKey: (FsKeyEvent) -> Unit,
    modifier: Modifier = Modifier,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val bg = if (isFocused) FsColors.AccentPrimary else FsColors.BgSurface
    val fg = if (isFocused) FsColors.TextInverse else FsColors.TextPrimary

    Box(
        modifier = modifier
            .height(48.dp)
            .clip(RoundedCornerShape(FsRadius.Sm))
            .background(bg)
            .border(
                width = if (isFocused) 3.dp else 1.dp,
                color = if (isFocused) FsColors.FocusRing else FsColors.BorderDefault,
                shape = RoundedCornerShape(FsRadius.Sm),
            )
            .clickable(
                interactionSource = interactionSource,
                indication = null,
                onClick = {
                    val ch = label.first().lowercaseChar()
                    onKey(FsKeyEvent.Char(ch))
                },
            ),
        contentAlignment = Alignment.Center,
    ) {
        Text(text = label, style = FsType.BodyMd, color = fg)
    }
}

@Composable
private fun ActionKey(
    label: String,
    modifier: Modifier = Modifier,
    primary: Boolean = false,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val bg = when {
        isFocused -> FsColors.AccentPrimaryHi
        primary -> FsColors.AccentPrimary
        else -> FsColors.BgSurfaceHi
    }
    val fg = when {
        isFocused -> FsColors.TextInverse
        primary -> FsColors.TextInverse
        else -> FsColors.TextPrimary
    }

    Box(
        modifier = modifier
            .height(48.dp)
            .clip(RoundedCornerShape(FsRadius.Sm))
            .background(bg)
            .border(
                width = if (isFocused) 3.dp else 1.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Sm),
            )
            .clickable(
                interactionSource = interactionSource,
                indication = null,
                onClick = onClick,
            )
            .padding(horizontal = FsSpacing.S3),
        contentAlignment = Alignment.Center,
    ) {
        Text(text = label, style = FsType.Caption, color = fg)
    }
}
