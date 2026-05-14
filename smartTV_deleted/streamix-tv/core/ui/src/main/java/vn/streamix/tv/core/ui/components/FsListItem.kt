/*
 * StreamIX TV — C7 ListItem (Settings rows: with-switch / with-radio / with-chevron)
 * Reference: 09 §8 (C7)
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
import androidx.compose.foundation.shape.CircleShape
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

/** ListItem 1-line hoặc 2-line cho Settings list. */
@Composable
fun FsListItem(
    title: String,
    subtitle: String? = null,
    modifier: Modifier = Modifier,
    leadingIcon: (@Composable () -> Unit)? = null,
    trailing: (@Composable () -> Unit)? = null,
    onClick: (() -> Unit)? = null,
    selected: Boolean = false,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val bg = when {
        selected -> FsColors.AccentPrimarySoft
        isFocused -> FsColors.BgSurfaceHi
        else -> FsColors.BgSurface
    }

    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(72.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(bg)
            .border(
                width = if (isFocused) 4.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Md),
            )
            .let { if (onClick != null) it.clickable(interactionSource, indication = null, onClick = onClick) else it }
            .padding(horizontal = FsSpacing.S4, vertical = FsSpacing.S3),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3),
    ) {
        if (leadingIcon != null) {
            leadingIcon()
        }
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = FsType.BodyLg,
                color = if (selected) FsColors.AccentPrimary else FsColors.TextPrimary,
            )
            if (subtitle != null) {
                Text(text = subtitle, style = FsType.Caption, color = FsColors.TextSecondary)
            }
        }
        trailing?.invoke()
    }
}

/** Switch toggle — component C17. */
@Composable
fun FsSwitch(
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val trackColor = when {
        !enabled -> FsColors.BorderSubtle
        checked -> FsColors.AccentPrimary
        else -> FsColors.BorderDefault
    }
    val thumbColor = when {
        !enabled -> FsColors.TextDisabled
        else -> FsColors.TextPrimary
    }

    Box(
        modifier = modifier
            .width(56.dp)
            .height(32.dp)
            .clip(CircleShape)
            .background(trackColor)
            .border(
                width = if (isFocused) 3.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = CircleShape,
            )
            .clickable(interactionSource, indication = null, enabled = enabled) {
                onCheckedChange(!checked)
            },
    ) {
        Box(
            modifier = Modifier
                .padding(horizontal = if (checked) 28.dp else 4.dp, vertical = 4.dp)
                .width(24.dp)
                .height(24.dp)
                .clip(CircleShape)
                .background(thumbColor),
        )
    }
}

/** Radio item dùng trong RadioGroup — component C18 sub-element */
@Composable
fun FsRadioItem(
    label: String,
    selected: Boolean,
    onSelect: () -> Unit,
    modifier: Modifier = Modifier,
) {
    FsListItem(
        title = label,
        modifier = modifier,
        onClick = onSelect,
        selected = selected,
        trailing = {
            if (selected) {
                Text("✓", style = FsType.TitleMd, color = FsColors.AccentPrimary)
            }
        },
    )
}

/** RadioGroup container — component C18. */
@Composable
fun <T> FsRadioGroup(
    options: List<T>,
    selected: T?,
    label: (T) -> String,
    onSelect: (T) -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier, verticalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
        options.forEach { item ->
            FsRadioItem(
                label = label(item),
                selected = item == selected,
                onSelect = { onSelect(item) },
            )
        }
    }
}
