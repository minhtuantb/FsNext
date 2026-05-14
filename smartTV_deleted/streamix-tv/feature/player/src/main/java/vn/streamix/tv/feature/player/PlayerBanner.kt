/*
 * StreamIX TV — Player banners (P1-P5) — in-screen, không phải toàn cục Snackbar
 * Reference: 14 §9
 *
 * Banner xuất hiện trong Player KHÔNG dùng FsSnackbar (sẽ vướng video).
 * Position: top hoặc center (top cho status, center cho seek hint).
 */
package vn.streamix.tv.feature.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

enum class PlayerBannerType {
    /** P1 — Mạng yếu giữa stream */
    NetworkWeak,
    /** P2 — Đang refresh URL stream silently */
    UrlRefreshing,
    /** P3 — Audio passthrough warning (AVR không support codec) */
    AudioWarning,
    /** P4 — Tua đến cuối */
    SeekEnd,
    /** P5 — Tua quá đầu */
    SeekStart,
}

data class PlayerBannerMessage(
    val type: PlayerBannerType,
    val text: String,
    val autoHideMs: Long = 5_000,
)

@Composable
fun PlayerBanner(
    message: PlayerBannerMessage?,
    onDismiss: () -> Unit,
) {
    val isCenter = message?.type == PlayerBannerType.SeekEnd || message?.type == PlayerBannerType.SeekStart

    LaunchedEffect(message) {
        if (message != null) {
            delay(message.autoHideMs)
            onDismiss()
        }
    }

    Box(modifier = Modifier.fillMaxWidth(), contentAlignment = if (isCenter) Alignment.Center else Alignment.TopCenter) {
        AnimatedVisibility(
            visible = message != null,
            enter = if (isCenter) fadeIn() else slideInVertically { -it } + fadeIn(),
            exit = if (isCenter) fadeOut() else slideOutVertically { -it } + fadeOut(),
        ) {
            message?.let { m ->
                Row(
                    modifier = Modifier
                        .padding(top = if (isCenter) 0.dp else 100.dp)
                        .clip(RoundedCornerShape(FsRadius.Md))
                        .background(Color.Black.copy(alpha = 0.7f))
                        .padding(horizontal = FsSpacing.S5, vertical = FsSpacing.S3),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3),
                ) {
                    Text(iconFor(m.type), style = FsType.TitleMd)
                    Text(m.text, style = FsType.BodyMd, color = FsColors.TextPrimary)
                }
            }
        }
    }
}

private fun iconFor(type: PlayerBannerType): String = when (type) {
    PlayerBannerType.NetworkWeak -> "📶"
    PlayerBannerType.UrlRefreshing -> "↻"
    PlayerBannerType.AudioWarning -> "🔊"
    PlayerBannerType.SeekEnd -> "⏭"
    PlayerBannerType.SeekStart -> "⏮"
}
