/*
 * StreamIX TV — Confirm dialog instances (D1-D7)
 * Reference: 14 §5
 *
 * Mỗi dialog là 1 wrapper convenience của FsDialog với strings + variant + default focus đúng spec.
 */
package vn.streamix.tv.core.ui.dialogs

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.res.stringResource
import vn.streamix.tv.core.ui.R
import vn.streamix.tv.core.ui.components.FsDialog
import vn.streamix.tv.core.ui.components.FsDialogVariant
import vn.streamix.tv.core.ui.components.FsTextField

/**
 * D1 — Exit app confirm (gọi từ Home Back).
 */
@Composable
fun ExitAppDialog(
    visible: Boolean,
    onConfirmExit: () -> Unit,
    onDismiss: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_exit_app_title),
        body = stringResource(R.string.dialog_exit_app_body),
        cancelLabel = stringResource(R.string.action_no),
        confirmLabel = stringResource(R.string.action_yes),
        variant = FsDialogVariant.Standard,
        defaultFocusOnConfirm = false,   // default "Không" (an toàn)
        onConfirm = onConfirmExit,
        onDismiss = onDismiss,
    )
}

/**
 * D2 — Tắt phim confirm (gọi từ Player Back).
 */
@Composable
fun StopPlaybackDialog(
    visible: Boolean,
    onConfirmStop: () -> Unit,
    onDismiss: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_stop_playback_title),
        body = stringResource(R.string.dialog_stop_playback_body),
        cancelLabel = stringResource(R.string.action_no),
        confirmLabel = stringResource(R.string.action_yes),
        variant = FsDialogVariant.Standard,
        defaultFocusOnConfirm = false,
        onConfirm = onConfirmStop,
        onDismiss = onDismiss,
    )
}

/**
 * D3 — Logout confirm (Settings → Account, destructive).
 */
@Composable
fun LogoutDialog(
    visible: Boolean,
    onConfirmLogout: () -> Unit,
    onDismiss: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_logout_title),
        body = stringResource(R.string.dialog_logout_body),
        cancelLabel = stringResource(R.string.action_cancel),
        confirmLabel = stringResource(R.string.dialog_logout_action_confirm),
        variant = FsDialogVariant.Destructive,
        defaultFocusOnConfirm = false,   // default "Hủy" (an toàn cho destructive)
        onConfirm = onConfirmLogout,
        onDismiss = onDismiss,
    )
}

/**
 * D4 — Password input cho file protected.
 *
 * Hold password state internally — caller chỉ nhận password khi user submit.
 */
@Composable
fun FilePasswordDialog(
    visible: Boolean,
    onSubmit: (password: String) -> Unit,
    onDismiss: () -> Unit,
) {
    var password by remember { mutableStateOf("") }
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_file_password_title),
        body = stringResource(R.string.dialog_file_password_body),
        cancelLabel = stringResource(R.string.action_cancel),
        confirmLabel = stringResource(R.string.action_close),
        variant = FsDialogVariant.Standard,
        defaultFocusOnConfirm = false,
        onConfirm = {
            if (password.isNotBlank()) {
                onSubmit(password)
                password = ""
            }
        },
        onDismiss = {
            password = ""
            onDismiss()
        },
        extraContent = {
            FsTextField(
                value = password,
                onValueChange = { password = it },
                placeholder = stringResource(R.string.dialog_file_password_field),
                isPassword = true,
            )
        },
    )
}

/**
 * D6 — Cách cập nhật (Settings → About) — info-only, chỉ có nút Đóng.
 *
 * Render QR code dẫn về tv.streamix.vn cho user scan bằng phone.
 */
@Composable
fun UpdateGuideDialog(
    visible: Boolean,
    qrPayloadUrl: String = "https://tv.streamix.vn",
    onDismiss: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_update_guide_title),
        body = stringResource(R.string.dialog_update_guide_body),
        confirmLabel = stringResource(R.string.action_close),
        variant = FsDialogVariant.Info,
        defaultFocusOnConfirm = true,
        onConfirm = onDismiss,
        onDismiss = onDismiss,
        extraContent = {
            QrCodeView(payload = qrPayloadUrl)
        },
    )
}

/**
 * D7 — Token expired (global) — appearance khi refresh token hết 7 ngày.
 */
@Composable
fun SessionExpiredDialog(
    visible: Boolean,
    onSignInAgain: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(R.string.dialog_session_expired_title),
        body = stringResource(R.string.dialog_session_expired_body),
        confirmLabel = stringResource(R.string.dialog_session_expired_action),
        variant = FsDialogVariant.Standard,
        defaultFocusOnConfirm = true,
        onConfirm = onSignInAgain,
        onDismiss = {},   // KHÔNG cho phép dismiss — phải re-login
    )
}
