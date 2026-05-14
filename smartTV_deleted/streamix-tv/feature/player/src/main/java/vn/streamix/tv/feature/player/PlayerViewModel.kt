/*
 * StreamIX TV — Player ViewModel (S5) — Wave M4 update.
 *
 * Stream URL từ /api/session/download (legacy). Body code = "123" ⇒ file yêu cầu password.
 * Tách state riêng [PlayerUiState.PasswordRequired] để UI hiển thị FsTextField + retry.
 *
 * Session expired (HTTP 201 hoặc body code 201) → [PlayerUiState.Error(cause = Auth)].
 * UI thì navigate về Login (handled global hoặc qua callback onAuthExpired).
 *
 * Reference: 13 §10 + API_REFERENCE_FOR_REUSE.md §3.3.
 */
package vn.streamix.tv.feature.player

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.domain.model.PlaybackPosition
import vn.streamix.tv.core.domain.model.StreamUrl
import vn.streamix.tv.core.domain.repository.FilesRepository
import vn.streamix.tv.core.domain.repository.PlaybackPositionRepository
import vn.streamix.tv.core.domain.repository.PlayerRepository
import javax.inject.Inject

@HiltViewModel
class PlayerViewModel @Inject constructor(
    handle: SavedStateHandle,
    private val playerRepo: PlayerRepository,
    private val playbackRepo: PlaybackPositionRepository,
    private val filesRepo: FilesRepository,
) : ViewModel() {

    val linkcode: String = handle["linkcode"] ?: ""
    val resumeMs: Long = handle["resumeMs"] ?: 0L

    private val _state = MutableStateFlow<PlayerUiState>(PlayerUiState.LoadingStream)
    val state: StateFlow<PlayerUiState> = _state.asStateFlow()

    init {
        loadStream()
    }

    fun loadStream(password: String? = null) = viewModelScope.launch {
        _state.value = PlayerUiState.LoadingStream
        when (val r = playerRepo.fetchStreamUrl(linkcode, password)) {
            is ApiResult.Success -> {
                // Check resume position từ Room
                val savedPos = playbackRepo.observe(linkcode).first()
                _state.value = PlayerUiState.Ready(
                    streamUrl = r.data,
                    initialPosition = resumeMs.takeIf { it > 0 } ?: savedPos?.positionMs ?: 0L,
                    needsResumePrompt = savedPos?.isResumable == true && resumeMs == 0L,
                )
            }
            is ApiResult.Error -> when (r.errorCode) {
                ErrorCodes.FILE_PASSWORD_REQUIRED -> _state.value = PlayerUiState.PasswordRequired(
                    failedPassword = password,
                    message = r.message,
                )
                ErrorCodes.NOT_FOUND -> _state.value = PlayerUiState.Error(
                    "Link không tồn tại hoặc đã bị xoá", ErrorCause.NotFound,
                )
                ErrorCodes.TOO_MANY_SESSIONS -> _state.value = PlayerUiState.Error(
                    "Quá nhiều phiên tải. Hãy vào fshare.vn → Bảo mật → Xoá phiên.",
                    ErrorCause.TooManySessions,
                )
                else -> _state.value = PlayerUiState.Error(r.message, ErrorCause.Network)
            }
            ApiResult.NetworkError -> _state.value = PlayerUiState.Error("Mất kết nối", ErrorCause.Network)
            ApiResult.AuthExpired -> _state.value = PlayerUiState.Error("Phiên hết hạn", ErrorCause.Auth)
            is ApiResult.RateLimited -> _state.value = PlayerUiState.Error("Quá nhiều yêu cầu", ErrorCause.Network)
        }
    }

    /** UI gọi khi user submit password trong dialog. */
    fun retryWithPassword(password: String) {
        loadStream(password = password.takeIf { it.isNotBlank() })
    }

    fun savePosition(positionMs: Long, durationMs: Long, fileName: String, thumbnail: String? = null) {
        if (positionMs <= 0 || durationMs <= 0) return
        viewModelScope.launch {
            playbackRepo.upsert(
                PlaybackPosition(
                    linkcode = linkcode,
                    positionMs = positionMs,
                    durationMs = durationMs,
                    updatedAt = System.currentTimeMillis(),
                    fileName = fileName,
                    thumbnail = thumbnail,
                )
            )
        }
    }

    fun onPlayerError(cause: ErrorCause, message: String) {
        _state.value = PlayerUiState.Error(message, cause)
    }
}

sealed interface PlayerUiState {
    data object LoadingStream : PlayerUiState
    data class Ready(
        val streamUrl: StreamUrl,
        val initialPosition: Long,
        val needsResumePrompt: Boolean,
    ) : PlayerUiState
    /** body code = 123. UI hiển thị FsTextField nhập password rồi gọi retryWithPassword. */
    data class PasswordRequired(
        val failedPassword: String?,
        val message: String,
    ) : PlayerUiState
    data class Error(val message: String, val cause: ErrorCause) : PlayerUiState
}

enum class ErrorCause { Network, Codec, UrlExpired, Forbidden, Auth, NotFound, TooManySessions }
