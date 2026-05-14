/*
 * StreamIX TV — Settings ViewModels
 */
package vn.streamix.tv.feature.settings

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import vn.streamix.tv.core.domain.model.AppSettings
import vn.streamix.tv.core.domain.model.User
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.SettingsRepository
import vn.streamix.tv.core.domain.repository.UserRepository
import javax.inject.Inject

@HiltViewModel
class SettingsAccountViewModel @Inject constructor(
    private val authRepo: AuthRepository,
    private val userRepo: UserRepository,
) : ViewModel() {

    val state: StateFlow<AccountUiState> = userRepo.observeMe()
        .map { AccountUiState(user = it) }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5_000), AccountUiState())

    init {
        viewModelScope.launch { userRepo.refreshMe() }
    }

    suspend fun logout() {
        authRepo.logout()
    }
}

data class AccountUiState(val user: User? = null)

@HiltViewModel
class SettingsPlaybackViewModel @Inject constructor(
    private val settingsRepo: SettingsRepository,
) : ViewModel() {

    val settings: StateFlow<AppSettings> = settingsRepo.observe()
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5_000), AppSettings())

    suspend fun update(transform: (AppSettings) -> AppSettings) {
        settingsRepo.update(transform)
    }
}
