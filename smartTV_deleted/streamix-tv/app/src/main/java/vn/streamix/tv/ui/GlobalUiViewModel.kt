/*
 * StreamIX TV — Global UI ViewModel
 *
 * Theo dõi network state + session state để trigger overlay G1/G2/D7.
 *
 * Reference: 14 §6 (Global overlays G1-G3)
 */
package vn.streamix.tv.ui

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import vn.streamix.tv.core.domain.repository.AuthRepository
import javax.inject.Inject

@HiltViewModel
class GlobalUiViewModel @Inject constructor(
    @ApplicationContext private val context: Context,
    private val authRepo: AuthRepository,
) : ViewModel() {

    private val _isConnected = MutableStateFlow(true)
    private val _serverDown = MutableStateFlow(false)
    private val _fatal = MutableStateFlow<String?>(null)

    val state: StateFlow<GlobalUiState> = combine(
        _isConnected, _serverDown, _fatal, authRepo.observeSession(),
    ) { connected, serverDown, fatal, _ ->
        when {
            fatal != null -> GlobalUiState.Fatal(fatal)
            !connected -> GlobalUiState.NoNetwork
            serverDown -> GlobalUiState.ServerDown
            else -> GlobalUiState.Normal
        }
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5_000), GlobalUiState.Normal)

    private val cm by lazy { context.getSystemService(ConnectivityManager::class.java) }

    private val networkCallback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) { _isConnected.value = true }
        override fun onLost(network: Network) { _isConnected.value = false }
        override fun onCapabilitiesChanged(network: Network, cap: NetworkCapabilities) {
            _isConnected.value = cap.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) &&
                                 cap.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
        }
    }

    init {
        val req = NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build()
        runCatching {
            cm.registerNetworkCallback(req, networkCallback)
            val active = cm.activeNetwork
            val cap = active?.let { cm.getNetworkCapabilities(it) }
            _isConnected.value = cap?.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) == true
        }
    }

    fun reportServerDown(down: Boolean) { _serverDown.value = down }
    fun reportFatal(message: String) { _fatal.value = message }
    fun checkNow() {
        _fatal.value = null
        _serverDown.value = false
    }

    override fun onCleared() {
        super.onCleared()
        runCatching { cm.unregisterNetworkCallback(networkCallback) }
    }
}

sealed interface GlobalUiState {
    data object Normal : GlobalUiState
    data object NoNetwork : GlobalUiState
    data object ServerDown : GlobalUiState
    data class Fatal(val message: String) : GlobalUiState
    data object SessionExpired : GlobalUiState
}
