/*
 * StreamIX TV — MainActivity (single-activity)
 * Reference: 13 §4.4
 *
 * Wire:
 *  - StreamIXNavGraph: navigation chính (Splash/Login/Home/...)
 *  - GlobalUiHost: G1/G2/G3 overlays + D7 SessionExpired + Snackbar global
 */
package vn.streamix.tv

import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.navigation.compose.rememberNavController
import dagger.hilt.android.AndroidEntryPoint
import vn.streamix.tv.core.ui.components.FsSnackbarMessage
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.StreamIXTheme
import vn.streamix.tv.navigation.LoginRoute
import vn.streamix.tv.navigation.StreamIXNavGraph
import vn.streamix.tv.ui.GlobalUiHost

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        // PHẢI gọi installSplashScreen() TRƯỚC super.onCreate()
        // để AndroidX SplashScreen compat hoạt động đúng trên mọi API level
        installSplashScreen()
        super.onCreate(savedInstanceState)
        setContent {
            StreamIXAppContent()
        }
    }
}

/**
 * Root composable. Đặt tên khác `StreamIXApp` (Application class) để tránh
 * overload resolution ambiguity giữa class constructor và composable function.
 */
@Composable
private fun StreamIXAppContent() {
    StreamIXTheme {
        val navController = rememberNavController()
        var snackbar by remember { mutableStateOf<FsSnackbarMessage?>(null) }
        val context = LocalContext.current
        val activity = remember(context) { context.findActivity() }

        Box(
            modifier = Modifier.fillMaxSize().background(FsColors.BgBase),
        ) {
            // Main navigation host
            StreamIXNavGraph(navController)

            // Global overlays + dialogs phủ trên
            GlobalUiHost(
                onSignInAgain = {
                    navController.navigate(LoginRoute) {
                        popUpTo(0)
                    }
                },
                onExitApp = { activity?.finish() },
                snackbarMessage = snackbar,
                onSnackbarDismiss = { snackbar = null },
            )
        }
    }
}

/** Walk ContextWrapper chain để tìm Activity. Compose chỉ expose Context. */
private tailrec fun Context.findActivity(): Activity? = when (this) {
    is Activity -> this
    is ContextWrapper -> baseContext.findActivity()
    else -> null
}
