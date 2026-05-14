/*
 * StreamIX TV — C1 TopBar
 * Reference: 09 §8 (C1) + 13 §7 S2 Home + 13 §8 S3 Browse
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
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
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

/**
 * TopBar dùng ở Home (S2), Browse (S3), Search (S12).
 *
 * Layout: [logo / breadcrumb trái] -- spacer -- [search] [settings] [avatar]
 */
@Composable
fun FsTopBar(
    title: String,
    titleColor: androidx.compose.ui.graphics.Color = FsColors.AccentPrimary,
    onSearchClick: (() -> Unit)? = null,
    onSettingsClick: (() -> Unit)? = null,
    onAvatarClick: (() -> Unit)? = null,
    avatarInitial: String? = null,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(80.dp)
            .padding(horizontal = FsSpacing.S7),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = title,
            style = FsType.TitleLg,
            color = titleColor,
        )
        Spacer(Modifier.weight(1f))
        Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
            if (onSearchClick != null) IconActionButton("🔍", onSearchClick)
            if (onSettingsClick != null) IconActionButton("⚙", onSettingsClick)
            if (onAvatarClick != null) AvatarActionButton(avatarInitial ?: "?", onAvatarClick)
        }
    }
}

@Composable
private fun IconActionButton(icon: String, onClick: () -> Unit) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    androidx.compose.foundation.layout.Box(
        modifier = Modifier
            .size(48.dp)
            .clip(CircleShape)
            .background(if (isFocused) FsColors.BgSurfaceHi else Color.Transparent)
            .border(
                width = if (isFocused) 3.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = CircleShape,
            )
            .clickable(interactionSource, indication = null, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(icon, style = FsType.TitleMd, color = FsColors.TextPrimary)
    }
}

@Composable
private fun AvatarActionButton(initial: String, onClick: () -> Unit) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    androidx.compose.foundation.layout.Box(
        modifier = Modifier
            .size(48.dp)
            .clip(CircleShape)
            .background(FsColors.AccentPrimary)
            .border(
                width = if (isFocused) 3.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = CircleShape,
            )
            .clickable(interactionSource, indication = null, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(initial.take(1).uppercase(), style = FsType.LabelLg, color = FsColors.TextInverse)
    }
}
