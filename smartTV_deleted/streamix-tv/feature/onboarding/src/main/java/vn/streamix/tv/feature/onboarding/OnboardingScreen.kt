/*
 * StreamIX TV — Onboarding (S9, 2 steps sau cuts D-2)
 * Reference: 13 §20 + 14 §4
 */
package vn.streamix.tv.feature.onboarding

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import kotlinx.coroutines.launch
import vn.streamix.tv.core.ui.components.FsButton
import vn.streamix.tv.core.ui.components.FsButtonVariant
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun OnboardingScreen(
    onCompleted: () -> Unit,
    viewModel: OnboardingViewModel = hiltViewModel(),
) {
    var step by remember { mutableIntStateOf(0) }
    val scope = rememberCoroutineScope()

    val steps = listOf(
        OnboardingStep(
            title = vn.streamix.tv.core.ui.R.string.onboarding_step1_title,
            body = vn.streamix.tv.core.ui.R.string.onboarding_step1_body,
            icon = "🎮",  // placeholder cho ic_dpad
        ),
        OnboardingStep(
            title = vn.streamix.tv.core.ui.R.string.onboarding_step2_title,
            body = vn.streamix.tv.core.ui.R.string.onboarding_step2_body,
            icon = "▶",   // placeholder cho ic_replay
        ),
    )

    val current = steps[step]

    Box(
        modifier = Modifier.fillMaxSize().background(FsColors.BgBase),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            modifier = Modifier.widthIn(max = 720.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(text = current.icon, style = FsType.DisplayLg)
            Spacer(Modifier.height(FsSpacing.S6))
            Text(
                text = stringResource(current.title),
                style = FsType.DisplayMd,
                color = FsColors.TextPrimary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S4))
            Text(
                text = stringResource(current.body),
                style = FsType.BodyLg,
                color = FsColors.TextSecondary,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(FsSpacing.S7))

            // Page indicator
            Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
                steps.forEachIndexed { i, _ ->
                    Box(
                        modifier = Modifier
                            .size(if (i == step) 12.dp else 8.dp)
                            .clip(CircleShape)
                            .background(if (i == step) FsColors.AccentPrimary else FsColors.BorderDefault),
                    )
                }
            }
            Spacer(Modifier.height(FsSpacing.S6))

            // Buttons
            Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
                FsButton(
                    text = stringResource(vn.streamix.tv.core.ui.R.string.onboarding_action_skip),
                    onClick = {
                        scope.launch { viewModel.markCompleted(); onCompleted() }
                    },
                    variant = FsButtonVariant.Ghost,
                )
                FsButton(
                    text = stringResource(
                        if (step == steps.lastIndex)
                            vn.streamix.tv.core.ui.R.string.onboarding_action_start
                        else
                            vn.streamix.tv.core.ui.R.string.onboarding_action_next
                    ),
                    onClick = {
                        if (step < steps.lastIndex) step++
                        else scope.launch { viewModel.markCompleted(); onCompleted() }
                    },
                    variant = FsButtonVariant.Primary,
                )
            }
        }
    }
}

private data class OnboardingStep(
    val title: Int,
    val body: Int,
    val icon: String,
)
