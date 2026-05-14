/*
 * StreamIX TV — Splash screen
 * Reference: 13 §5 + 14 §7 (S0 states)
 */
package vn.streamix.tv.feature.splash

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun SplashScreen(
    onNavigateLogin: () -> Unit,
    onNavigateHome: () -> Unit,
    viewModel: SplashViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()

    LaunchedEffect(state) {
        when (val s = state) {
            is SplashState.Routing -> when (s.destination) {
                SplashDestination.Login -> onNavigateLogin()
                SplashDestination.Home -> onNavigateHome()
            }
            else -> Unit
        }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(FsColors.BgBase),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
        ) {
            // Logo placeholder (chờ designer giao logo_streamix_tv.svg ở v1.1 — block B2)
            Box(
                modifier = Modifier
                    .size(192.dp)
                    .background(FsColors.AccentPrimary),
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    text = "S",
                    style = FsType.DisplayLg.copy(color = FsColors.TextInverse),
                )
            }
            Spacer(Modifier.height(FsSpacing.S5))
            Text(
                text = stringResource(vn.streamix.tv.core.ui.R.string.app_name),
                style = FsType.TitleLg,
                color = FsColors.TextPrimary,
            )
            Spacer(Modifier.height(FsSpacing.S4))
            // Golden accent line theo mockup splash
            androidx.compose.foundation.layout.Box(
                modifier = Modifier
                    .width(120.dp)
                    .height(3.dp)
                    .background(FsColors.AccentPrimary),
            )
            Spacer(Modifier.height(FsSpacing.S5))
            LinearProgressIndicator(
                modifier = Modifier.widthIn(min = 200.dp).height(2.dp),
                color = FsColors.AccentPrimary,
                trackColor = FsColors.BgSurface,
            )
            if (state is SplashState.Verifying) {
                Spacer(Modifier.height(FsSpacing.S3))
                Text(
                    text = stringResource(vn.streamix.tv.core.ui.R.string.splash_loading),
                    style = FsType.Caption,
                    color = FsColors.TextSecondary,
                )
            }
        }
    }
}
