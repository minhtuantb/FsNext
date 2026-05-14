/*
 * StreamIX TV — Global UI host
 *
 * Render trên toàn bộ NavHost trong MainActivity. Phụ trách:
 *  - G1 No Network overlay khi mất kết nối > 5s
 *  - G2 Server Unavailable
 *  - G3 Fatal Init
 *  - D7 Session Expired dialog (khi token refresh fail)
 *  - FsSnackbarHost cho global toast (T1-T11)
 */
package vn.streamix.tv.ui

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.ui.components.FsSnackbarHost
import vn.streamix.tv.core.ui.components.FsSnackbarMessage
import vn.streamix.tv.core.ui.dialogs.SessionExpiredDialog
import vn.streamix.tv.core.ui.overlays.FatalErrorOverlay
import vn.streamix.tv.core.ui.overlays.NoNetworkOverlay
import vn.streamix.tv.core.ui.overlays.ServerUnavailableOverlay

@Composable
fun GlobalUiHost(
    onSignInAgain: () -> Unit,
    onExitApp: () -> Unit,
    snackbarMessage: FsSnackbarMessage? = null,
    onSnackbarDismiss: () -> Unit = {},
    viewModel: GlobalUiViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()

    Box(modifier = Modifier.fillMaxSize()) {
        when (val s = state) {
            GlobalUiState.Normal -> Unit
            GlobalUiState.NoNetwork -> NoNetworkOverlay(onRetry = viewModel::checkNow)
            GlobalUiState.ServerDown -> ServerUnavailableOverlay(onRetry = viewModel::checkNow)
            is GlobalUiState.Fatal -> FatalErrorOverlay(
                detailMessage = s.message,
                onRetry = viewModel::checkNow,
                onExit = onExitApp,
            )
            GlobalUiState.SessionExpired -> SessionExpiredDialog(
                visible = true,
                onSignInAgain = onSignInAgain,
            )
        }

        FsSnackbarHost(message = snackbarMessage, onDismiss = onSnackbarDismiss)
    }
}
