/*
 * StreamIX TV — Login ViewModel (chỉ email/password, V1)
 * Reference: 13 §6 + 14 (S1 states)
 */
package vn.streamix.tv.feature.auth

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.SettingsRepository
import javax.inject.Inject

@HiltViewModel
class LoginViewModel @Inject constructor(
    private val authRepo: AuthRepository,
    private val settingsRepo: SettingsRepository,
) : ViewModel() {

    private val _state = MutableStateFlow(LoginUiState())
    val state: StateFlow<LoginUiState> = _state.asStateFlow()

    fun onEmailChange(email: String) {
        _state.update { it.copy(email = email, errorKey = null) }
    }

    fun onPasswordChange(password: String) {
        _state.update { it.copy(password = password, errorKey = null) }
    }

    fun togglePasswordVisible() {
        _state.update { it.copy(passwordVisible = !it.passwordVisible) }
    }

    fun toggleRememberMe() {
        _state.update { it.copy(rememberMe = !it.rememberMe) }
    }

    fun submit(onSuccess: (onboarded: Boolean) -> Unit) = viewModelScope.launch {
        val s = _state.value
        if (!isEmailValid(s.email)) {
            _state.update { it.copy(errorKey = LoginErrorKey.InvalidEmail) }
            return@launch
        }
        if (s.password.length < 6) {
            _state.update { it.copy(errorKey = LoginErrorKey.PasswordTooShort) }
            return@launch
        }
        _state.update { it.copy(submitting = true, errorKey = null) }
        when (val result = authRepo.login(s.email.trim(), s.password)) {
            is ApiResult.Success -> {
                val settings = settingsRepo.observe().first()
                _state.update { it.copy(submitting = false, success = true) }
                onSuccess(settings.onboardingCompleted)
            }
            is ApiResult.Error -> {
                val key = when (result.errorCode) {
                    // Login: sai email/password — code có thể "405" hoặc "authenticate"
                    ErrorCodes.AUTH_INVALID_CREDENTIALS, "authenticate" -> LoginErrorKey.InvalidCredentials
                    ErrorCodes.AUTH_TOO_MANY_ATTEMPTS -> LoginErrorKey.Throttled
                    ErrorCodes.ACCOUNT_LOCKED, ErrorCodes.TEMPORARILY_LOCKED -> LoginErrorKey.AccountLocked
                    ErrorCodes.INVALID_INPUT -> LoginErrorKey.InvalidEmail
                    ErrorCodes.APP_KEY_INVALID,
                    ErrorCodes.APP_NOT_AUTHORIZED,
                    ErrorCodes.APP_DEVICE_UNSUPPORTED_37,
                    ErrorCodes.APP_DEVICE_UNSUPPORTED_38,
                    ErrorCodes.APP_OUTDATED -> LoginErrorKey.AppKeyError
                    else -> LoginErrorKey.Unknown
                }
                _state.update { it.copy(submitting = false, errorKey = key) }
            }
            ApiResult.NetworkError -> _state.update { it.copy(submitting = false, errorKey = LoginErrorKey.Network) }
            is ApiResult.RateLimited -> _state.update { it.copy(submitting = false, errorKey = LoginErrorKey.Throttled) }
            ApiResult.AuthExpired -> _state.update { it.copy(submitting = false, errorKey = LoginErrorKey.InvalidCredentials) }
        }
    }

    private fun isEmailValid(email: String): Boolean =
        email.matches(Regex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"))
}

data class LoginUiState(
    val email: String = "",
    val password: String = "",
    val passwordVisible: Boolean = false,
    val rememberMe: Boolean = true,
    val submitting: Boolean = false,
    val success: Boolean = false,
    val errorKey: LoginErrorKey? = null,
)

enum class LoginErrorKey {
    InvalidEmail,
    PasswordTooShort,
    InvalidCredentials,
    AccountLocked,
    Throttled,
    AppKeyError,
    Network,
    Unknown,
}
