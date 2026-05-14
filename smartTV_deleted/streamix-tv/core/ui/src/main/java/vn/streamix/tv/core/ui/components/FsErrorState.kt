/*
 * StreamIX TV — C14 EmptyState / C15 ErrorState
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsEmptyState(
    title: String,
    body: String,
    icon: @Composable (() -> Unit)? = null,
    actionText: String? = null,
    onActionClick: (() -> Unit)? = null,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
        ) {
            if (icon != null) {
                Box(modifier = Modifier.size(96.dp)) { icon() }
                Spacer(modifier = Modifier.height(FsSpacing.S5))
            }
            Text(title, style = FsType.TitleMd, color = FsColors.TextPrimary, textAlign = TextAlign.Center)
            Spacer(modifier = Modifier.height(FsSpacing.S2))
            Text(body, style = FsType.BodyMd, color = FsColors.TextSecondary, textAlign = TextAlign.Center)
            if (actionText != null && onActionClick != null) {
                Spacer(modifier = Modifier.height(FsSpacing.S5))
                FsButton(text = actionText, onClick = onActionClick)
            }
        }
    }
}

@Composable
fun FsErrorState(
    title: String,
    body: String,
    onRetry: (() -> Unit)? = null,
    modifier: Modifier = Modifier,
) {
    FsEmptyState(
        title = title,
        body = body,
        actionText = if (onRetry != null) "Thử lại" else null,
        onActionClick = onRetry,
        modifier = modifier,
    )
}
