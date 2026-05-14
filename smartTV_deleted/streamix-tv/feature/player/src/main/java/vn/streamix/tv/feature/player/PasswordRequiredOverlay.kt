/*
 * StreamIX TV — Password required overlay (file Fshare có mật khẩu).
 *
 * Body code = 123 ⇒ PlayerViewModel chuyển sang state PasswordRequired.
 * Overlay này hiển thị FsTextField nhập password rồi gọi onSubmit để retry.
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
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.components.FsTextField
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun PasswordRequiredOverlay(
    failedPassword: String?,
    message: String,
    onSubmit: (String) -> Unit,
    onCancel: () -> Unit,
) {
    var password by remember { mutableStateOf("") }
    val isRetry = failedPassword != null

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.85f)),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier.widthIn(max = 480.dp),
        ) {
            Box(
                modifier = Modifier
                    .size(96.dp)
                    .clip(CircleShape)
                    .background(FsColors.AccentPrimary.copy(alpha = 0.2f)),
                contentAlignment = Alignment.Center,
            ) {
                Text("🔒", style = FsType.DisplayMd, color = FsColors.AccentPrimary)
            }
            Spacer(Modifier.height(FsSpacing.S5))
            Text(
                text = if (isRetry) "Mật khẩu không đúng" else "File yêu cầu mật khẩu",
                style = FsType.Headline,
                color = FsColors.TextPrimary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S3))
            Text(
                text = message.ifBlank { "Nhập mật khẩu để xem video." },
                style = FsType.BodyMd,
                color = FsColors.TextSecondary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S5))
            FsTextField(
                value = password,
                onValueChange = { password = it },
                placeholder = "Nhập mật khẩu",
                isError = isRetry && password.isBlank(),
                isPassword = true,
                keyboardType = KeyboardType.Password,
                imeAction = ImeAction.Done,
            )
            Spacer(Modifier.height(FsSpacing.S5))
            Column(verticalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                FsButton(
                    text = "Phát video",
                    onClick = { if (password.isNotBlank()) onSubmit(password) },
                    variant = FsButtonVariant.Primary,
                )
                FsButton(
                    text = "Quay lại",
                    onClick = onCancel,
                    variant = FsButtonVariant.Ghost,
                )
            }
        }
    }
}
