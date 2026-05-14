/*
 * StreamIX TV — S5c Player Error overlay (4 variants)
 * Reference: 13 §13 + 14 §7 (S5c codec/network/url-expired/forbidden)
 */
package vn.streamix.tv.feature.player

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun PlayerErrorOverlay(
    cause: ErrorCause,
    detailMessage: String? = null,
    onRetry: () -> Unit,
    onSwitchEngine: (() -> Unit)? = null,
    onBack: () -> Unit,
) {
    val bodyRes = when (cause) {
        ErrorCause.Codec -> vn.streamix.tv.core.ui.R.string.player_error_codec_body
        ErrorCause.Network -> vn.streamix.tv.core.ui.R.string.player_error_network_body
        ErrorCause.Auth -> vn.streamix.tv.core.ui.R.string.player_error_auth_body
        ErrorCause.UrlExpired -> vn.streamix.tv.core.ui.R.string.player_error_url_expired_body
        ErrorCause.Forbidden -> vn.streamix.tv.core.ui.R.string.player_error_network_body
        ErrorCause.NotFound -> vn.streamix.tv.core.ui.R.string.player_error_not_found_body
        ErrorCause.TooManySessions -> vn.streamix.tv.core.ui.R.string.player_error_too_many_sessions_body
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.85f)),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            // Icon error 96dp circle
            Box(
                modifier = Modifier
                    .size(96.dp)
                    .clip(CircleShape)
                    .background(FsColors.Danger.copy(alpha = 0.2f)),
                contentAlignment = Alignment.Center,
            ) {
                Text("✕", style = FsType.DisplayMd, color = FsColors.Danger)
            }
            Spacer(Modifier.height(FsSpacing.S5))
            Text(
                text = "Không phát được video",
                style = FsType.Headline,
                color = FsColors.TextPrimary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S3))
            Text(
                text = detailMessage ?: stringResource(bodyRes),
                style = FsType.BodyLg,
                color = FsColors.TextSecondary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S7))

            Column(verticalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                FsButton(
                    text = stringResource(vn.streamix.tv.core.ui.R.string.player_error_action_retry),
                    onClick = onRetry,
                    variant = FsButtonVariant.Primary,
                )
                if (cause == ErrorCause.Codec && onSwitchEngine != null) {
                    FsButton(
                        text = "Đổi sang VLC engine",
                        onClick = onSwitchEngine,
                        variant = FsButtonVariant.Secondary,
                    )
                }
                FsButton(
                    text = stringResource(vn.streamix.tv.core.ui.R.string.player_error_action_back),
                    onClick = onBack,
                    variant = FsButtonVariant.Ghost,
                )
            }
        }
    }
}
