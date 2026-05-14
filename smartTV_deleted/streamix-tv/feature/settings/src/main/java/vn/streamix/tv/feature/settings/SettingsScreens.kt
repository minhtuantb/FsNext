/*
 * StreamIX TV — Settings screens (S7, S7a, S7b, S7d, S7f)
 * Reference: 13 §15-19
 */
package vn.streamix.tv.feature.settings

import android.content.Intent
import android.os.Build
import android.provider.Settings
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.dialogs.LogoutDialog
import vn.streamix.tv.core.ui.dialogs.UpdateGuideDialog
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

// ===== S7 Settings Hub =====

@Composable
fun SettingsHubScreen(
    onOpenAccount: () -> Unit,
    onOpenPlayback: () -> Unit,
    onOpenNetwork: () -> Unit,
    onOpenAbout: () -> Unit,
    onBack: () -> Unit,
) {
    BackHandler { onBack() }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(FsColors.BgBase)
            .padding(FsSpacing.S7),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S5),
    ) {
        Text(
            stringResource(vn.streamix.tv.core.ui.R.string.settings_hub_title),
            style = FsType.DisplayMd, color = FsColors.TextPrimary,
        )
        Spacer(Modifier.height(FsSpacing.S5))
        SettingsRowItem(stringResource(vn.streamix.tv.core.ui.R.string.settings_section_account), onOpenAccount)
        SettingsRowItem(stringResource(vn.streamix.tv.core.ui.R.string.settings_section_playback), onOpenPlayback)
        SettingsRowItem(stringResource(vn.streamix.tv.core.ui.R.string.settings_section_network), onOpenNetwork)
        SettingsRowItem(stringResource(vn.streamix.tv.core.ui.R.string.settings_section_about), onOpenAbout)
    }
}

@Composable
private fun SettingsRowItem(label: String, onClick: () -> Unit) {
    FsButton(
        text = label,
        onClick = onClick,
        variant = FsButtonVariant.Secondary,
        modifier = Modifier.fillMaxWidth(),
    )
}

// ===== S7a Account =====

@Composable
fun SettingsAccountScreen(
    onLoggedOut: () -> Unit,
    onBack: () -> Unit,
    viewModel: SettingsAccountViewModel = hiltViewModel(),
) {
    BackHandler { onBack() }
    val state by viewModel.state.collectAsStateWithLifecycle()
    val scope = rememberCoroutineScope()
    var showLogoutDialog by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier.fillMaxSize().background(FsColors.BgBase).padding(FsSpacing.S7),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S5),
    ) {
        Text(stringResource(vn.streamix.tv.core.ui.R.string.account_title),
            style = FsType.DisplayMd, color = FsColors.TextPrimary)

        state.user?.let { user ->
            // Avatar (initials)
            Box(
                modifier = Modifier.size(120.dp).clip(CircleShape).background(FsColors.AccentPrimary),
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    user.fullName.take(1).uppercase(),
                    style = FsType.DisplayMd, color = FsColors.TextInverse,
                )
            }
            Spacer(Modifier.height(FsSpacing.S3))
            Text(user.fullName, style = FsType.TitleLg, color = FsColors.TextPrimary)
            Text(user.email, style = FsType.BodyMd, color = FsColors.TextSecondary)

            Spacer(Modifier.height(FsSpacing.S5))
            Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S6)) {
                Column {
                    Text(stringResource(vn.streamix.tv.core.ui.R.string.account_label_vip),
                        style = FsType.Caption, color = FsColors.TextTertiary)
                    Text(
                        stringResource(
                            if (user.isVip) vn.streamix.tv.core.ui.R.string.account_vip_active
                            else vn.streamix.tv.core.ui.R.string.account_vip_inactive
                        ),
                        style = FsType.TitleMd,
                        color = if (user.isVip) FsColors.AccentPrimary else FsColors.TextSecondary,
                    )
                }
                Column {
                    Text(stringResource(vn.streamix.tv.core.ui.R.string.account_label_webspace),
                        style = FsType.Caption, color = FsColors.TextTertiary)
                    Text(
                        "${FileSize.format(user.storageUsed)} / ${FileSize.format(user.storageQuota)}",
                        style = FsType.TitleMd, color = FsColors.TextPrimary,
                    )
                }
            }
        }

        Spacer(Modifier.height(FsSpacing.S7))
        FsButton(
            text = stringResource(vn.streamix.tv.core.ui.R.string.account_button_logout),
            onClick = { showLogoutDialog = true },
            variant = FsButtonVariant.Destructive,
        )
    }

    // D3 Logout confirm
    LogoutDialog(
        visible = showLogoutDialog,
        onConfirmLogout = {
            showLogoutDialog = false
            scope.launch {
                viewModel.logout()
                onLoggedOut()
            }
        },
        onDismiss = { showLogoutDialog = false },
    )
}

// ===== S7b Playback =====

@Composable
fun SettingsPlaybackScreen(onBack: () -> Unit, viewModel: SettingsPlaybackViewModel = hiltViewModel()) {
    BackHandler { onBack() }
    val settings by viewModel.settings.collectAsStateWithLifecycle()
    val scope = rememberCoroutineScope()

    Column(
        modifier = Modifier.fillMaxSize().background(FsColors.BgBase).padding(FsSpacing.S7),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        Text(stringResource(vn.streamix.tv.core.ui.R.string.playback_title),
            style = FsType.DisplayMd, color = FsColors.TextPrimary)
        Spacer(Modifier.height(FsSpacing.S5))

        ToggleRow(
            label = stringResource(vn.streamix.tv.core.ui.R.string.playback_audio_passthrough),
            checked = settings.audioPassthroughHdmi,
            onChange = { v -> scope.launch { viewModel.update { it.copy(audioPassthroughHdmi = v) } } },
        )
        ToggleRow(
            label = stringResource(vn.streamix.tv.core.ui.R.string.playback_auto_next),
            checked = settings.autoPlayNext,
            onChange = { v -> scope.launch { viewModel.update { it.copy(autoPlayNext = v) } } },
        )
        ToggleRow(
            label = stringResource(vn.streamix.tv.core.ui.R.string.playback_save_position),
            checked = settings.savePlaybackPosition,
            onChange = { v -> scope.launch { viewModel.update { it.copy(savePlaybackPosition = v) } } },
        )
        ToggleRow(
            label = stringResource(vn.streamix.tv.core.ui.R.string.playback_subtitle_outline),
            checked = settings.subtitleOutline,
            onChange = { v -> scope.launch { viewModel.update { it.copy(subtitleOutline = v) } } },
        )
        // RadioGroup cho engine, sub size, sub color — V1 đơn giản hoá; V1.1 expand
    }
}

@Composable
private fun ToggleRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = FsSpacing.S3),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, style = FsType.BodyLg, color = FsColors.TextPrimary)
        Spacer(Modifier.width(FsSpacing.S5))
        FsButton(
            text = if (checked) "Bật" else "Tắt",
            onClick = { onChange(!checked) },
            variant = if (checked) FsButtonVariant.Primary else FsButtonVariant.Secondary,
        )
    }
}

// ===== S7d Network (info-only, không speed test theo decision) =====

@Composable
fun SettingsNetworkScreen(onBack: () -> Unit) {
    BackHandler { onBack() }
    val context = LocalContext.current

    Column(
        modifier = Modifier.fillMaxSize().background(FsColors.BgBase).padding(FsSpacing.S7),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        Text(stringResource(vn.streamix.tv.core.ui.R.string.network_title),
            style = FsType.DisplayMd, color = FsColors.TextPrimary)
        Spacer(Modifier.height(FsSpacing.S5))

        // V1: chỉ hiển thị nút mở system network settings.
        // Detail Wi-Fi info đọc qua WifiManager — V1.1 implement (cần permission ACCESS_FINE_LOCATION ở Android 9+)
        Text(
            "Trạng thái mạng: kiểm tra qua Cài đặt hệ thống",
            style = FsType.BodyMd, color = FsColors.TextSecondary,
        )

        Spacer(Modifier.height(FsSpacing.S6))
        FsButton(
            text = stringResource(vn.streamix.tv.core.ui.R.string.network_action_open_system_settings),
            onClick = {
                val intent = Intent(Settings.ACTION_WIRELESS_SETTINGS).apply {
                    flags = Intent.FLAG_ACTIVITY_NEW_TASK
                }
                context.startActivity(intent)
            },
            variant = FsButtonVariant.Primary,
        )
    }
}

// ===== S7f About =====

@Composable
fun SettingsAboutScreen(onBack: () -> Unit) {
    BackHandler { onBack() }
    var showUpdateGuide by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier.fillMaxSize().background(FsColors.BgBase).padding(FsSpacing.S7),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        Text(stringResource(vn.streamix.tv.core.ui.R.string.about_title),
            style = FsType.DisplayMd, color = FsColors.TextPrimary)
        Spacer(Modifier.height(FsSpacing.S5))

        Text(stringResource(vn.streamix.tv.core.ui.R.string.app_name),
            style = FsType.TitleLg, color = FsColors.AccentPrimary)
        Text("Phiên bản 1.0.0 (10000)", style = FsType.Mono, color = FsColors.TextSecondary)

        Spacer(Modifier.height(FsSpacing.S5))
        FsButton(
            text = stringResource(vn.streamix.tv.core.ui.R.string.about_update_show_qr),
            onClick = { showUpdateGuide = true },
            variant = FsButtonVariant.Primary,
        )

        Spacer(Modifier.height(FsSpacing.S6))
        Text("Model: ${Build.MODEL}",
            style = FsType.BodyMd, color = FsColors.TextSecondary)
        Text("Android ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT})",
            style = FsType.BodyMd, color = FsColors.TextSecondary)
        Spacer(Modifier.height(FsSpacing.S6))
        Text(stringResource(vn.streamix.tv.core.ui.R.string.about_contact_caption),
            style = FsType.Caption, color = FsColors.TextTertiary)
    }

    // D6 Update guide dialog
    UpdateGuideDialog(
        visible = showUpdateGuide,
        qrPayloadUrl = "https://tv.streamix.vn",
        onDismiss = { showUpdateGuide = false },
    )
}
