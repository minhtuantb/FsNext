/*
 * StreamIX TV — Splash ViewModel
 * Reference: 13 §5
 */
package vn.streamix.tv.feature.splash

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.UserRepository
import javax.inject.Inject

@HiltViewModel
class SplashViewModel @Inject constructor(
    private val authRepo: AuthRepository,
    private val userRepo: UserRepository,
) : ViewModel() {

    private val _state = MutableStateFlow<SplashState>(SplashState.Initializing)
    val state: StateFlow<SplashState> = _state.asStateFlow()

    init {
        viewModelScope.launch { route() }
    }

    private suspend fun route() {
        val session = authRepo.currentSession()
        if (session == null) {
            _state.value = SplashState.Routing(SplashDestination.Login)
            return
        }
        // Có session → verify qua /me. Authenticator sẽ tự refresh nếu access token expired.
        _state.value = SplashState.Verifying
        when (userRepo.refreshMe()) {
            is ApiResult.Success -> _state.value = SplashState.Routing(SplashDestination.Home)
            ApiResult.AuthExpired -> {
                authRepo.clearSession()
                _state.value = SplashState.Routing(SplashDestination.Login)
            }
            ApiResult.NetworkError -> {
                // Cache fallback: nếu Room có user_cache thì vẫn vào Home (offline-first)
                _state.value = SplashState.Routing(SplashDestination.Home)
            }
            else -> _state.value = SplashState.Routing(SplashDestination.Login)
        }
    }
}

sealed interface SplashState {
    data object Initializing : SplashState
    data object Verifying : SplashState
    data class Routing(val destination: SplashDestination) : SplashState
    data class InitFailed(val message: String) : SplashState
}

enum class SplashDestination { Login, Home }
