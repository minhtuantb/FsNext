/*
 * StreamIX TV — Player full-screen (S5) với Media3 ExoPlayer + custom Compose Overlay
 * Reference: 13 §10-§14 + 14 §7
 *
 * Tích hợp:
 *   - PlayerOverlay (S5a) — controls auto-hide 5s
 *   - TrackSelectionSheet (S5b) — chọn subtitle/audio
 *   - PlayerErrorOverlay (S5c) — 4 variants lỗi
 *   - ResumePromptDialog (S5d) — hỏi resume từ saved position
 *   - StopPlaybackDialog (D2) — confirm Back
 *   - PlayerBanner (P1-P5) — non-toast banners
 */
package vn.streamix.tv.feature.player

import android.view.ViewGroup
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.viewinterop.AndroidView
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.common.TrackSelectionOverride
import androidx.media3.datasource.DefaultHttpDataSource
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory
import androidx.media3.ui.PlayerView
import kotlinx.coroutines.delay
import timber.log.Timber
import vn.streamix.tv.core.ui.dialogs.StopPlaybackDialog
import vn.streamix.tv.core.ui.theme.FsColors

@Composable
fun PlayerScreen(
    linkcode: String,
    resumeMs: Long,
    fileName: String = "",
    onExit: () -> Unit,
    viewModel: PlayerViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    val context = LocalContext.current

    // Local UI state
    var showStopDialog by remember { mutableStateOf(false) }
    var showResumePrompt by remember { mutableStateOf(false) }
    var resumePosFromState by remember { mutableLongStateOf(0L) }
    var overlayVisible by remember { mutableStateOf(true) }
    var trackSheet by remember { mutableStateOf<TrackKind?>(null) }
    var playerBanner by remember { mutableStateOf<PlayerBannerMessage?>(null) }
    var currentMs by remember { mutableLongStateOf(0L) }
    var durationMs by remember { mutableLongStateOf(0L) }
    var bufferedMs by remember { mutableLongStateOf(0L) }
    var isPlaying by remember { mutableStateOf(false) }

    BackHandler {
        when {
            trackSheet != null -> trackSheet = null
            showResumePrompt -> showResumePrompt = false
            else -> showStopDialog = true
        }
    }

    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        when (val s = state) {
            PlayerUiState.LoadingStream -> Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center,
            ) { CircularProgressIndicator(color = FsColors.AccentPrimary) }

            is PlayerUiState.PasswordRequired -> PasswordRequiredOverlay(
                failedPassword = s.failedPassword,
                message = s.message,
                onSubmit = { pwd -> viewModel.retryWithPassword(pwd) },
                onCancel = onExit,
            )

            is PlayerUiState.Error -> PlayerErrorOverlay(
                cause = s.cause,
                detailMessage = s.message,
                onRetry = { viewModel.loadStream() },
                onSwitchEngine = null,   // V1 chưa có libVLC; V1.1 wire
                onBack = onExit,
            )

            is PlayerUiState.Ready -> {
                LaunchedEffect(s) {
                    if (s.needsResumePrompt && s.initialPosition > 0) {
                        showResumePrompt = true
                        resumePosFromState = s.initialPosition
                    }
                }

                val exoPlayer = remember(s.streamUrl.url) {
                    ExoPlayer.Builder(context)
                        .setMediaSourceFactory(
                            DefaultMediaSourceFactory(context)
                                .setDataSourceFactory(
                                    DefaultHttpDataSource.Factory()
                                        .setUserAgent("Fshare/androidtv/1.0")
                                        .setConnectTimeoutMs(15_000)
                                        .setReadTimeoutMs(30_000)
                                        .setAllowCrossProtocolRedirects(true)
                                )
                        )
                        .build().apply {
                            setMediaItem(MediaItem.fromUri(s.streamUrl.url))
                            // Seek đến resume pos NẾU không cần prompt (đã pass từ Continue Watching)
                            if (!s.needsResumePrompt && s.initialPosition > 0) {
                                seekTo(s.initialPosition)
                            }
                            playWhenReady = false   // chờ user qua resume prompt nếu có
                            prepare()
                        }
                }

                // Listener — observe state changes
                DisposableEffect(exoPlayer) {
                    val listener = object : Player.Listener {
                        override fun onIsPlayingChanged(playing: Boolean) {
                            isPlaying = playing
                            if (playing) overlayVisible = true
                        }
                        override fun onPlayerError(error: PlaybackException) {
                            Timber.e(error, "Player error code=${error.errorCode}")
                            val cause = when (error.errorCode) {
                                PlaybackException.ERROR_CODE_DECODER_INIT_FAILED,
                                PlaybackException.ERROR_CODE_DECODER_QUERY_FAILED ->
                                    ErrorCause.Codec
                                PlaybackException.ERROR_CODE_IO_BAD_HTTP_STATUS,
                                PlaybackException.ERROR_CODE_IO_FILE_NOT_FOUND ->
                                    ErrorCause.UrlExpired
                                PlaybackException.ERROR_CODE_IO_NETWORK_CONNECTION_FAILED,
                                PlaybackException.ERROR_CODE_IO_NETWORK_CONNECTION_TIMEOUT ->
                                    ErrorCause.Network
                                else -> ErrorCause.Network
                            }
                            viewModel.onPlayerError(cause, error.message ?: "Player error")
                        }
                    }
                    exoPlayer.addListener(listener)
                    onDispose {
                        if (exoPlayer.duration > 0) {
                            viewModel.savePosition(exoPlayer.currentPosition, exoPlayer.duration, fileName.ifBlank { "Đang xem..." })
                        }
                        exoPlayer.removeListener(listener)
                        exoPlayer.release()
                    }
                }

                // Position tick
                LaunchedEffect(exoPlayer) {
                    while (true) {
                        delay(500)
                        currentMs = exoPlayer.currentPosition
                        durationMs = exoPlayer.duration.coerceAtLeast(0)
                        bufferedMs = exoPlayer.bufferedPosition

                        // Mỗi 5s save position
                        if (exoPlayer.isPlaying && durationMs > 0 && currentMs % 5_000 < 500) {
                            viewModel.savePosition(currentMs, durationMs, fileName.ifBlank { "Đang xem..." })
                        }
                    }
                }

                // Auto-hide overlay sau 5s không tương tác
                LaunchedEffect(overlayVisible, currentMs) {
                    if (overlayVisible) {
                        delay(5_000)
                        if (isPlaying) overlayVisible = false
                    }
                }

                // PlayerView native hiển thị video
                AndroidView(
                    modifier = Modifier
                        .fillMaxSize()
                        .clickable(
                            interactionSource = remember { MutableInteractionSource() },
                            indication = null,
                        ) {
                            // Click vào video → toggle overlay
                            overlayVisible = !overlayVisible
                        },
                    factory = { ctx ->
                        PlayerView(ctx).apply {
                            player = exoPlayer
                            useController = false   // CUSTOM Compose overlay thay default controller
                            setShowBuffering(PlayerView.SHOW_BUFFERING_WHEN_PLAYING)
                            resizeMode = androidx.media3.ui.AspectRatioFrameLayout.RESIZE_MODE_FIT
                            layoutParams = ViewGroup.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT,
                            )
                        }
                    },
                )

                // === Custom Compose Overlay (S5a) ===
                PlayerOverlay(
                    visible = overlayVisible,
                    title = fileName,
                    isPlaying = isPlaying,
                    currentMs = currentMs,
                    durationMs = durationMs,
                    bufferedMs = bufferedMs,
                    onPlayPause = {
                        if (exoPlayer.isPlaying) exoPlayer.pause() else exoPlayer.play()
                        overlayVisible = true
                    },
                    onSeekBack10 = {
                        exoPlayer.seekTo((exoPlayer.currentPosition - 10_000).coerceAtLeast(0))
                        overlayVisible = true
                        if (exoPlayer.currentPosition <= 0) {
                            playerBanner = PlayerBannerMessage(PlayerBannerType.SeekStart, "Đã đến đầu video", 1_500)
                        }
                    },
                    onSeekForward10 = {
                        val target = exoPlayer.currentPosition + 10_000
                        exoPlayer.seekTo(target.coerceAtMost(exoPlayer.duration))
                        overlayVisible = true
                        if (target >= exoPlayer.duration) {
                            playerBanner = PlayerBannerMessage(PlayerBannerType.SeekEnd, "Đã đến cuối video", 1_500)
                        }
                    },
                    onClickSubtitle = { trackSheet = TrackKind.Subtitle; overlayVisible = true },
                    onClickAudio = { trackSheet = TrackKind.Audio; overlayVisible = true },
                    onBack = { showStopDialog = true },
                )

                // === Track Selection BottomSheet (S5b) ===
                if (trackSheet != null) {
                    val tracks = remember(trackSheet, exoPlayer.currentTracks) {
                        extractTracks(exoPlayer, trackSheet!!)
                    }
                    TrackSelectionSheet(
                        visible = true,
                        kind = trackSheet!!,
                        tracks = tracks,
                        onSelect = { selected ->
                            applyTrackSelection(exoPlayer, trackSheet!!, selected)
                        },
                        onDismiss = { trackSheet = null },
                    )
                }

                // === Resume Prompt (S5d) ===
                ResumePromptDialog(
                    visible = showResumePrompt,
                    positionMs = resumePosFromState,
                    onResume = {
                        exoPlayer.seekTo(resumePosFromState)
                        exoPlayer.play()
                        showResumePrompt = false
                    },
                    onRestart = {
                        exoPlayer.seekTo(0)
                        exoPlayer.play()
                        showResumePrompt = false
                    },
                )

                // === In-screen banner (P1-P5) ===
                PlayerBanner(message = playerBanner, onDismiss = { playerBanner = null })

                // === D2 Stop confirm dialog ===
                StopPlaybackDialog(
                    visible = showStopDialog,
                    onConfirmStop = {
                        if (exoPlayer.duration > 0) {
                            viewModel.savePosition(exoPlayer.currentPosition, exoPlayer.duration, fileName.ifBlank { "Đang xem..." })
                        }
                        onExit()
                    },
                    onDismiss = { showStopDialog = false },
                )
            }
        }
    }
}

// === Helpers extract + apply track selection ===

private fun extractTracks(player: ExoPlayer, kind: TrackKind): List<PlayerTrack> {
    val targetType = if (kind == TrackKind.Subtitle) C.TRACK_TYPE_TEXT else C.TRACK_TYPE_AUDIO
    val out = mutableListOf<PlayerTrack>()
    player.currentTracks.groups.forEachIndexed { groupIdx, group ->
        if (group.type != targetType) return@forEachIndexed
        for (trackIdx in 0 until group.length) {
            val format = group.getTrackFormat(trackIdx)
            out += PlayerTrack(
                groupIndex = groupIdx,
                trackIndex = trackIdx,
                language = format.language ?: "",
                codec = format.sampleMimeType?.substringAfter('/'),
                isSelected = group.isTrackSelected(trackIdx),
            )
        }
    }
    return out
}

private fun applyTrackSelection(player: ExoPlayer, kind: TrackKind, selected: PlayerTrack?) {
    val targetType = if (kind == TrackKind.Subtitle) C.TRACK_TYPE_TEXT else C.TRACK_TYPE_AUDIO
    val params = player.trackSelectionParameters.buildUpon()
    if (selected == null) {
        // Tắt subtitle (chỉ cho kind=Subtitle)
        params.setTrackTypeDisabled(targetType, true)
    } else {
        params.setTrackTypeDisabled(targetType, false)
        val trackGroup = player.currentTracks.groups.getOrNull(selected.groupIndex) ?: return
        params.setOverrideForType(
            TrackSelectionOverride(trackGroup.mediaTrackGroup, listOf(selected.trackIndex))
        )
    }
    player.trackSelectionParameters = params.build()
}
