/*
 * StreamIX TV — Global overlays G1 / G2 / G3
 * Reference: 14 §6
 *
 * G1 No Network — mất kết nối > 5s
 * G2 Server Unavailable — API trả 503 trong 30s
 * G3 Fatal Init — splash > 15s, Hilt fail, app_key invalid
 *
 * Render fullscreen, render trên toàn bộ NavHost (đặt trong MainActivity Box).
 */
package vn.streamix.tv.core.ui.overlays

import android.content.Intent
import android.provider.Settings
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

/** G1 — No Network */
@Composable
fun NoNetworkOverlay(
    onRetry: () -> Unit,
) {
    val context = LocalContext.current
    GlobalOverlayContainer(
        icon = "📡",
        iconBg = FsColors.Warning.copy(alpha = 0.2f),
        title = "Không có kết nối",
        body = "Vui lòng kiểm tra mạng và thử lại.",
        actions = {
            Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                FsButton(
                    text = "Cài đặt mạng",
                    onClick = {
                        val intent = Intent(Settings.ACTION_WIRELESS_SETTINGS).apply {
                            flags = Intent.FLAG_ACTIVITY_NEW_TASK
                        }
                        context.startActivity(intent)
                    },
                    variant = FsButtonVariant.Secondary,
                )
                FsButton(text = "Thử lại", onClick = onRetry, variant = FsButtonVariant.Primary)
            }
        },
    )
}

/** G2 — Server Unavailable */
@Composable
fun ServerUnavailableOverlay(
    onRetry: () -> Unit,
) {
    GlobalOverlayContainer(
        icon = "☁",
        iconBg = FsColors.Warning.copy(alpha = 0.2f),
        title = "Máy chủ tạm ngưng",
        body = "Vui lòng thử lại sau ít phút.",
        actions = {
            FsButton(text = "Thử lại", onClick = onRetry, variant = FsButtonVariant.Primary)
        },
    )
}

/** G3 — Fatal Init Error */
@Composable
fun FatalErrorOverlay(
    detailMessage: String? = null,
    onRetry: (() -> Unit)? = null,
    onExit: () -> Unit,
) {
    GlobalOverlayContainer(
        icon = "✕",
        iconBg = FsColors.Danger.copy(alpha = 0.2f),
        iconColor = FsColors.Danger,
        title = "Khởi động thất bại",
        body = detailMessage ?: "Có lỗi xảy ra. Vui lòng thử lại.",
        actions = {
            Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                FsButton(text = "Thoát", onClick = onExit, variant = FsButtonVariant.Secondary)
                if (onRetry != null) {
                    FsButton(text = "Thử lại", onClick = onRetry, variant = FsButtonVariant.Primary)
                }
            }
        },
    )
}

@Composable
private fun GlobalOverlayContainer(
    icon: String,
    iconBg: androidx.compose.ui.graphics.Color,
    iconColor: androidx.compose.ui.graphics.Color = FsColors.TextPrimary,
    title: String,
    body: String,
    actions: @Composable () -> Unit,
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(FsColors.BgBase),
        contentAlignment = Alignment.Center,
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Box(
                modifier = Modifier
                    .size(128.dp)
                    .clip(CircleShape)
                    .background(iconBg),
                contentAlignment = Alignment.Center,
            ) {
                Text(icon, style = FsType.DisplayLg, color = iconColor)
            }
            Spacer(Modifier.height(FsSpacing.S6))
            Text(
                text = title,
                style = FsType.Headline,
                color = FsColors.TextPrimary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S3))
            Text(
                text = body,
                style = FsType.BodyLg,
                color = FsColors.TextSecondary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S7))
            actions()
        }
    }
}
