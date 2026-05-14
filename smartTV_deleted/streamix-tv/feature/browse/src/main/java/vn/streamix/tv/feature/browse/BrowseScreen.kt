/*
 * StreamIX TV — Browse screen (S3 grid 4 columns)
 * Reference: 13 §8
 */
package vn.streamix.tv.feature.browse

import androidx.compose.foundation.background
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.grid.rememberLazyGridState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.runtime.snapshotFlow
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import kotlinx.coroutines.flow.distinctUntilChanged
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.domain.model.FileItem
import vn.streamix.tv.core.domain.model.ItemType
import vn.streamix.tv.core.ui.components.FsCardSkeleton
import vn.streamix.tv.core.ui.components.FsEmptyState
import vn.streamix.tv.core.ui.components.FsErrorState
import vn.streamix.tv.core.ui.components.FsFileCard
import vn.streamix.tv.core.ui.components.FsFolderCard
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun BrowseScreen(
    folderPath: String,
    folderName: String,
    onOpenFolder: (FileItem) -> Unit,
    onOpenFile: (linkcode: String) -> Unit,
    onBack: () -> Unit,
    viewModel: BrowseViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    val gridState = rememberLazyGridState()

    // Auto-loadMore khi scroll cuối
    LaunchedEffect(gridState) {
        snapshotEnd(gridState).collect { atEnd ->
            if (atEnd) viewModel.loadMore()
        }
    }

    Column(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        Row(
            modifier = Modifier.fillMaxWidth().height(80.dp).padding(horizontal = FsSpacing.S7),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                text = folderName.ifBlank { folderPath },
                style = FsType.TitleLg,
                color = FsColors.TextPrimary,
            )
        }
        Box(modifier = Modifier.fillMaxSize()) {
            when {
                state.loading -> {
                    LazyVerticalGrid(
                        columns = GridCells.Fixed(4),
                        contentPadding = PaddingValues(FsSpacing.S7),
                        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                    ) {
                        items(12) { FsCardSkeleton() }
                    }
                }
                state.error != null -> FsErrorState(
                    title = stringResource(vn.streamix.tv.core.ui.R.string.browse_error_title),
                    body = state.error!!,
                    onRetry = { viewModel.loadInitial() },
                )
                state.items.isEmpty() -> FsEmptyState(
                    title = stringResource(vn.streamix.tv.core.ui.R.string.browse_empty_title),
                    body = stringResource(vn.streamix.tv.core.ui.R.string.browse_empty_body),
                )
                else -> LazyVerticalGrid(
                    state = gridState,
                    columns = GridCells.Fixed(4),
                    contentPadding = PaddingValues(FsSpacing.S7),
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                    verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                ) {
                    items(state.items, key = { it.linkcode }) { item ->
                        when (item.type) {
                            ItemType.FOLDER -> FsFolderCard(
                                name = item.name,
                                onClick = { onOpenFolder(item) },
                            )
                            ItemType.FILE -> FsFileCard(
                                title = item.name,
                                subtitle = FileSize.format(item.size),
                                thumbnailUrl = item.thumbnail,
                                onClick = { onOpenFile(item.linkcode) },
                            )
                        }
                    }
                    if (state.loadingMore) {
                        items(4) { FsCardSkeleton() }
                    }
                }
            }
        }
    }
}

private fun snapshotEnd(gridState: androidx.compose.foundation.lazy.grid.LazyGridState) =
    snapshotFlow {
        val info = gridState.layoutInfo
        val total = info.totalItemsCount
        if (total == 0) false else
            (info.visibleItemsInfo.lastOrNull()?.index ?: 0) >= total - 8
    }.distinctUntilChanged()
