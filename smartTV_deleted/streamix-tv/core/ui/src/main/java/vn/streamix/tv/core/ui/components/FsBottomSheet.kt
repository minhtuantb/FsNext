/*
 * StreamIX TV — C9 BottomSheet (slide-up panel ~40% screen height)
 * Reference: 09 §8 (C9) + 13 §12 (S5b Track Selection)
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsBottomSheet(
    visible: Boolean,
    title: String,
    onDismiss: () -> Unit,
    heightFraction: Float = 0.40f,
    content: @Composable () -> Unit,
) {
    AnimatedVisibility(
        visible = visible,
        enter = fadeIn(),
        exit = fadeOut(),
    ) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(FsColors.BgScrim)
                .clickable(
                    interactionSource = remember { MutableInteractionSource() },
                    indication = null,
                    onClick = onDismiss,
                ),
            contentAlignment = Alignment.BottomCenter,
        ) {
            AnimatedVisibility(
                visible = visible,
                enter = slideInVertically { it },
                exit = slideOutVertically { it },
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .fillMaxHeight(heightFraction)
                        .clip(RoundedCornerShape(topStart = FsRadius.Lg, topEnd = FsRadius.Lg))
                        .background(FsColors.BgSurface)
                        .clickable(
                            interactionSource = remember { MutableInteractionSource() },
                            indication = null,
                            onClick = {},
                        )
                        .padding(FsSpacing.S5),
                ) {
                    // Title
                    Text(
                        text = title,
                        style = FsType.TitleMd,
                        color = FsColors.TextPrimary,
                    )
                    Spacer(Modifier.height(FsSpacing.S4))
                    content()
                }
            }
        }
    }
}
