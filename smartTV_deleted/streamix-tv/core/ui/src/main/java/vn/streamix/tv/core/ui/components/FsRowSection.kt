/*
 * StreamIX TV — C6 RowSection (horizontal list with title)
 * Reference: 09 §8 (C6) + 13 §7 S2 Home
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsRowSection(
    title: String,
    modifier: Modifier = Modifier,
    listState: LazyListState = rememberLazyListState(),
    content: @Composable () -> Unit,
) {
    Column(modifier = modifier.fillMaxWidth()) {
        Text(
            text = title,
            style = FsType.TitleMd,
            color = FsColors.TextPrimary,
            modifier = Modifier.padding(start = FsSpacing.S7, end = FsSpacing.S7),
        )
        Spacer(modifier = Modifier.height(FsSpacing.S4))
        LazyRow(
            state = listState,
            contentPadding = PaddingValues(horizontal = FsSpacing.S7),
            horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
        ) {
            // Caller wraps content trong items() block — but to keep API simple,
            // we expose a Composable lambda; real use will use LazyRow directly khi cần items() control.
        }
        content()
    }
}
