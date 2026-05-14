/*
 * StreamIX TV — File Detail screen (S4)
 * Reference: 13 §9
 */
package vn.streamix.tv.feature.browse

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import coil.compose.AsyncImage
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.components.FsErrorState
import vn.streamix.tv.core.ui.dialogs.FilePasswordDialog
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FileDetailScreen(
    linkcode: String,
    onPlay: (linkcode: String, fileName: String) -> Unit,
    onBack: () -> Unit,
    viewModel: FileDetailViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    var passwordDialog by remember { mutableStateOf(false) }

    Box(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        when (val s = state) {
            FileDetailUiState.Loading -> Text(
                "Đang tải…",
                style = FsType.BodyLg,
                color = FsColors.TextSecondary,
                modifier = Modifier.padding(FsSpacing.S7),
            )
            is FileDetailUiState.Error -> FsErrorState(
                title = "Không tải được",
                body = s.message,
                onRetry = { viewModel.load() },
            )
            is FileDetailUiState.Loaded -> Row(
                modifier = Modifier.fillMaxSize().padding(FsSpacing.S7),
                horizontalArrangement = Arrangement.spacedBy(FsSpacing.S6),
            ) {
                // Thumbnail (col trái 40%)
                Box(
                    modifier = Modifier
                        .width(720.dp)
                        .aspectRatio(16f / 9f)
                        .clip(RoundedCornerShape(FsRadius.Lg))
                        .background(FsColors.BgSurface),
                    contentAlignment = Alignment.Center,
                ) {
                    if (s.file.thumbnail != null) {
                        AsyncImage(
                            model = s.file.thumbnail,
                            contentDescription = s.file.name,
                            modifier = Modifier.fillMaxSize(),
                        )
                    } else {
                        Text("📹", style = FsType.DisplayLg)
                    }
                }

                // Metadata + actions (col phải 60%)
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = s.file.name,
                        style = FsType.DisplayMd,
                        color = FsColors.TextPrimary,
                    )
                    Spacer(Modifier.height(FsSpacing.S3))
                    Text(
                        text = "${FileSize.format(s.file.size)} · ${s.file.lastUpdatedAt ?: ""}",
                        style = FsType.BodyMd,
                        color = FsColors.TextSecondary,
                    )
                    if (s.file.description != null) {
                        Spacer(Modifier.height(FsSpacing.S5))
                        Text(
                            text = s.file.description!!,
                            style = FsType.BodyLg,
                            color = FsColors.TextSecondary,
                            maxLines = 4,
                        )
                    }
                    Spacer(Modifier.height(FsSpacing.S7))

                    FsButton(
                        text = stringResource(vn.streamix.tv.core.ui.R.string.detail_button_play),
                        onClick = { onPlay(s.file.linkcode, s.file.name) },
                        variant = FsButtonVariant.Primary,
                        enabled = s.playable,
                        modifier = Modifier.fillMaxWidth(0.5f),
                    )
                    if (!s.playable) {
                        Spacer(Modifier.height(FsSpacing.S2))
                        Text(
                            text = stringResource(vn.streamix.tv.core.ui.R.string.detail_error_not_playable),
                            style = FsType.Caption,
                            color = FsColors.TextTertiary,
                        )
                    }
                    Spacer(Modifier.height(FsSpacing.S3))
                    FsButton(
                        text = stringResource(vn.streamix.tv.core.ui.R.string.detail_button_favorite),
                        onClick = { /* TODO favorite local Room */ },
                        variant = FsButtonVariant.Secondary,
                        modifier = Modifier.fillMaxWidth(0.5f),
                    )
                }
            }
            is FileDetailUiState.PasswordRequired -> {
                LaunchedEffect(Unit) { passwordDialog = true }
            }
        }

        // D4 File password dialog
        FilePasswordDialog(
            visible = passwordDialog,
            onSubmit = { _ ->
                passwordDialog = false
                // TODO: pass password qua Routes.Player.password param khi backend ship
                val fileName = (state as? FileDetailUiState.Loaded)?.file?.name
                    ?: (state as? FileDetailUiState.PasswordRequired)?.file?.name
                    ?: ""
                onPlay(linkcode, fileName)
            },
            onDismiss = { passwordDialog = false },
        )
    }
}
