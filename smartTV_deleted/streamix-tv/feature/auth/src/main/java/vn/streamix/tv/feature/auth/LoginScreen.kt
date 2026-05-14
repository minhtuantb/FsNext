/*
 * StreamIX TV — Login screen (S1) — split layout theo mockup.
 *
 * Layout:
 *   ┌────────────────────────┬────────────────────────────┐
 *   │ [S] StreamIX TV        │ ĐĂNG NHẬP                  │
 *   │                        │ Tài khoản Fshare           │
 *   │ Cloud của bạn,         │ ┌────────────────────────┐ │
 *   │ trên màn hình lớn.     │ │ EMAIL                  │ │
 *   │                        │ │ [email field]          │ │
 *   │ Đăng nhập bằng tài     │ ├────────────────────────┤ │
 *   │ khoản Fshare để xem... │ │ MẬT KHẨU       Hiện/Ẩn │ │
 *   │                        │ │ [password field]       │ │
 *   │                        │ ├────────────────────────┤ │
 *   │                        │ │ ☑ Nhớ tôi 30 ngày     │ │
 *   │                        │ │ [Đăng nhập button]    │ │
 *   │                        │ │ Quên mật khẩu? ...    │ │
 *   │                        │ └────────────────────────┘ │
 *   │ FSHARE.VN — ACCOUNT    │                            │
 *   └────────────────────────┴────────────────────────────┘
 */
package vn.streamix.tv.feature.auth

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.components.FsCheckbox
import vn.streamix.tv.core.ui.components.FsTextField
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun LoginScreen(
    onLoginSuccess: (onboarded: Boolean) -> Unit,
    viewModel: LoginViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()

    Row(
        modifier = Modifier
            .fillMaxSize()
            .background(FsColors.BgBase),
    ) {
        // ─── LEFT: Hero ──────────────────────────────
        LoginHero(modifier = Modifier.weight(1f).fillMaxHeight())

        // ─── RIGHT: Form ────────────────────────────
        LoginForm(
            modifier = Modifier.weight(1f).fillMaxHeight(),
            state = state,
            onEmailChange = viewModel::onEmailChange,
            onPasswordChange = viewModel::onPasswordChange,
            onTogglePassword = viewModel::togglePasswordVisible,
            onToggleRemember = viewModel::toggleRememberMe,
            onSubmit = { viewModel.submit(onLoginSuccess) },
        )
    }
}

@Composable
private fun LoginHero(modifier: Modifier = Modifier) {
    Column(
        modifier = modifier.padding(FsSpacing.S8),
    ) {
        // Logo + wordmark
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(
                modifier = Modifier
                    .size(48.dp)
                    .clip(RoundedCornerShape(FsRadius.Md))
                    .background(FsColors.AccentPrimary),
                contentAlignment = Alignment.Center,
            ) {
                Text("S", style = FsType.TitleLg, color = FsColors.TextInverse)
            }
            Spacer(Modifier.width(FsSpacing.S3))
            Text(
                text = stringResource(vn.streamix.tv.core.ui.R.string.app_name),
                style = FsType.TitleMd,
                color = FsColors.TextPrimary,
            )
        }

        Spacer(modifier = Modifier.weight(1f))

        // Hero title
        Text(
            text = stringResource(vn.streamix.tv.core.ui.R.string.login_hero_title),
            style = FsType.DisplayLg,
            color = FsColors.TextPrimary,
        )
        Spacer(Modifier.height(FsSpacing.S4))
        Text(
            text = stringResource(vn.streamix.tv.core.ui.R.string.login_hero_subtitle),
            style = FsType.BodyLg,
            color = FsColors.TextSecondary,
            modifier = Modifier.widthIn(max = 480.dp),
        )

        Spacer(modifier = Modifier.weight(1f))

        // Footer
        Text(
            text = stringResource(vn.streamix.tv.core.ui.R.string.login_hero_footer),
            style = FsType.Caption,
            color = FsColors.TextTertiary,
        )
    }
}

@Composable
private fun LoginForm(
    modifier: Modifier = Modifier,
    state: LoginUiState,
    onEmailChange: (String) -> Unit,
    onPasswordChange: (String) -> Unit,
    onTogglePassword: () -> Unit,
    onToggleRemember: () -> Unit,
    onSubmit: () -> Unit,
) {
    Box(modifier = modifier.padding(FsSpacing.S8), contentAlignment = Alignment.Center) {
        Column(
            modifier = Modifier.widthIn(max = 480.dp).fillMaxWidth(),
        ) {
            // Eyebrow
            Text(
                text = stringResource(vn.streamix.tv.core.ui.R.string.login_form_eyebrow),
                style = FsType.Caption,
                color = FsColors.AccentPrimary,
            )
            Spacer(Modifier.height(FsSpacing.S3))
            Text(
                text = stringResource(vn.streamix.tv.core.ui.R.string.login_form_title),
                style = FsType.DisplayMd,
                color = FsColors.TextPrimary,
            )
            Spacer(Modifier.height(FsSpacing.S6))

            // Email
            FieldLabel(stringResource(vn.streamix.tv.core.ui.R.string.login_email_label))
            FsTextField(
                value = state.email,
                onValueChange = onEmailChange,
                placeholder = stringResource(vn.streamix.tv.core.ui.R.string.login_email_placeholder),
                keyboardType = KeyboardType.Email,
                imeAction = ImeAction.Next,
                isError = state.errorKey == LoginErrorKey.InvalidEmail,
            )
            Spacer(Modifier.height(FsSpacing.S4))

            // Password + show/hide trailing button
            FieldLabel(stringResource(vn.streamix.tv.core.ui.R.string.login_password_label))
            FsTextField(
                value = state.password,
                onValueChange = onPasswordChange,
                placeholder = stringResource(vn.streamix.tv.core.ui.R.string.login_password_placeholder),
                keyboardType = KeyboardType.Password,
                imeAction = ImeAction.Done,
                isPassword = !state.passwordVisible,
                isError = state.errorKey == LoginErrorKey.PasswordTooShort,
                trailingIcon = {
                    PasswordToggleButton(
                        showing = state.passwordVisible,
                        onClick = onTogglePassword,
                    )
                },
            )
            Spacer(Modifier.height(FsSpacing.S4))

            // Remember me checkbox
            FsCheckbox(
                checked = state.rememberMe,
                onToggle = onToggleRemember,
                label = stringResource(vn.streamix.tv.core.ui.R.string.login_remember_me),
            )
            Spacer(Modifier.height(FsSpacing.S4))

            // Error message
            state.errorKey?.let { errorKey ->
                Text(
                    text = stringResource(errorKey.toStringRes()),
                    style = FsType.BodyMd,
                    color = FsColors.Danger,
                )
                Spacer(Modifier.height(FsSpacing.S3))
            }

            // Submit button
            FsButton(
                text = stringResource(vn.streamix.tv.core.ui.R.string.login_button_submit),
                onClick = onSubmit,
                variant = FsButtonVariant.Primary,
                loading = state.submitting,
                modifier = Modifier.fillMaxWidth(),
            )
            Spacer(Modifier.height(FsSpacing.S4))

            // Forgot password helper
            Text(
                text = stringResource(vn.streamix.tv.core.ui.R.string.login_forgot_password),
                style = FsType.Caption,
                color = FsColors.TextTertiary,
            )
        }
    }
}

@Composable
private fun FieldLabel(text: String) {
    Text(
        text = text,
        style = FsType.Caption,
        color = FsColors.TextSecondary,
        modifier = Modifier.padding(bottom = FsSpacing.S2),
    )
}

@Composable
private fun PasswordToggleButton(
    showing: Boolean,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    val text = stringResource(
        if (showing) vn.streamix.tv.core.ui.R.string.login_hide_password_text
        else vn.streamix.tv.core.ui.R.string.login_show_password_text
    )
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(FsRadius.Sm))
            .border(
                width = if (isFocused) 2.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else androidx.compose.ui.graphics.Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Sm),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(horizontal = FsSpacing.S3, vertical = FsSpacing.S2),
    ) {
        Text(
            text = text,
            style = FsType.Caption,
            color = if (isFocused) FsColors.AccentPrimary else FsColors.TextSecondary,
        )
    }
}

private fun LoginErrorKey.toStringRes(): Int = when (this) {
    LoginErrorKey.InvalidEmail,
    LoginErrorKey.PasswordTooShort,
    LoginErrorKey.InvalidCredentials -> vn.streamix.tv.core.ui.R.string.login_error_invalid_credentials
    LoginErrorKey.AccountLocked -> vn.streamix.tv.core.ui.R.string.login_error_locked
    LoginErrorKey.Throttled -> vn.streamix.tv.core.ui.R.string.login_error_throttle
    LoginErrorKey.Network -> vn.streamix.tv.core.ui.R.string.login_error_network
    LoginErrorKey.AppKeyError,
    LoginErrorKey.Unknown -> vn.streamix.tv.core.ui.R.string.error_init_title
}
