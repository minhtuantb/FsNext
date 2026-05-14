/*
 * StreamIX TV — Search screen (S12) theo design system mockup.
 *
 * Layout:
 *   ┌────────────────────────────────────────────────────────┐
 *   │ Tìm kiếm                       [⚙] [Avatar]            │
 *   │                                                        │
 *   │ ┌────────────────────────────────────────────────────┐ │
 *   │ │ 🔍  dune part two|              14 KẾT QUẢ · 6.3s  │ │  ← input
 *   │ └────────────────────────────────────────────────────┘ │
 *   │                                                        │
 *   │ [Tất cả · 14] [Phim · 9] [Series · 2] [Folder · 3]    │
 *   │                                                        │
 *   │ Gợi ý cho bạn                                          │
 *   │ + the bear   + spirited away   + christopher nolan    │
 *   │                                                        │
 *   │ [thumb1] [thumb2] [thumb3] [thumb4]                    │  ← grid
 *   │ [thumb5] [thumb6] [thumb7] [thumb8]                    │
 *   └────────────────────────────────────────────────────────┘
 */
package vn.streamix.tv.feature.search

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.core.ui.components.FsCardSkeleton
import vn.streamix.tv.core.ui.components.FsEmptyState
import vn.streamix.tv.core.ui.components.FsFileCard
import vn.streamix.tv.core.ui.components.FsKeyEvent
import vn.streamix.tv.core.ui.components.FsKeyboard
import vn.streamix.tv.core.ui.components.FsTextField
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsFocus
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

private val DEFAULT_SUGGESTIONS = listOf(
    "the bear",
    "spirited away",
    "christopher nolan",
    "hợp cổ",
)

private enum class SearchFilter(val label: String) {
    All("Tất cả"),
    Movie("Phim"),
    Series("Series"),
    Folder("Folder"),
}

/**
 * Phân loại 1 FeaturedItem.
 *  - FOLDER: URL chứa /folder/.
 *  - SERIES: name match pattern "S01E", "Tập N", "EP N", "Episode".
 *  - MOVIE: file URL không phải series.
 */
private fun classifyItem(item: vn.streamix.tv.core.domain.model.FeaturedItem): SearchFilter {
    if (vn.streamix.tv.core.common.FshareUrl.isFolderUrl(item.url)) return SearchFilter.Folder
    val n = item.title.lowercase()
    val seriesPatterns = listOf(
        Regex("""s\d{1,2}e\d{1,2}"""),         // S01E02
        Regex("""tập\s*\d+"""),                  // Tập 5
        Regex("""ep\.?\s*\d+"""),                // EP 5 / EP.5
        Regex("""episode\s*\d+"""),              // Episode 5
        Regex("""season\s*\d+"""),               // Season 1
    )
    if (seriesPatterns.any { it.containsMatchIn(n) }) return SearchFilter.Series
    return SearchFilter.Movie
}

private fun List<vn.streamix.tv.core.domain.model.FeaturedItem>.filterBy(
    filter: SearchFilter,
): List<vn.streamix.tv.core.domain.model.FeaturedItem> = when (filter) {
    SearchFilter.All -> this
    else -> filter { classifyItem(it) == filter }
}

private fun List<vn.streamix.tv.core.domain.model.FeaturedItem>.countBy(
    filter: SearchFilter,
): Int = when (filter) {
    SearchFilter.All -> size
    else -> count { classifyItem(it) == filter }
}

@Composable
fun SearchScreen(
    onOpenFile: (linkcode: String) -> Unit,
    onBack: () -> Unit,
    viewModel: SearchViewModel = hiltViewModel(),
) {
    val query by viewModel.query.collectAsStateWithLifecycle()
    val results by viewModel.results.collectAsStateWithLifecycle()

    val allItems = (results as? SearchResults.Loaded)?.items.orEmpty()
    val totalCount = allItems.size
    var activeFilter by remember { mutableStateOf(SearchFilter.All) }
    val filteredItems = allItems.filterBy(activeFilter)

    Column(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        // Title
        Text(
            text = "Tìm kiếm",
            style = FsType.DisplayLg,
            color = FsColors.TextPrimary,
            modifier = Modifier.padding(horizontal = FsSpacing.S7, vertical = FsSpacing.S5),
        )

        // Search input + result count
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = FsSpacing.S7),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            FsTextField(
                value = query,
                onValueChange = viewModel::setQuery,
                placeholder = "Tìm phim, tên file, hoặc bộ phim...",
                modifier = Modifier.weight(1f),
            )
            if (results is SearchResults.Loaded && totalCount > 0) {
                Spacer(Modifier.width(FsSpacing.S4))
                Text(
                    text = "$totalCount KẾT QUẢ",
                    style = FsType.Caption,
                    color = FsColors.TextSecondary,
                )
            }
        }

        Spacer(Modifier.height(FsSpacing.S4))

        // Filter chips — counts theo từng category, click thì filter visible items.
        if (results is SearchResults.Loaded) {
            FilterChipsRow(
                active = activeFilter,
                allItems = allItems,
                onSelect = { activeFilter = it },
                modifier = Modifier.padding(horizontal = FsSpacing.S7),
            )
            Spacer(Modifier.height(FsSpacing.S4))
        }

        // Content area
        Box(modifier = Modifier.fillMaxSize()) {
            when (val s = results) {
                SearchResults.Idle -> SearchIdleContent(
                    currentQuery = query,
                    onQueryUpdate = viewModel::setQuery,
                    onSelectSuggestion = viewModel::setQuery,
                )
                SearchResults.Searching -> LoadingGrid()
                is SearchResults.Loaded -> {
                    if (filteredItems.isEmpty()) {
                        FsEmptyState(
                            title = "Không có kết quả ở mục này",
                            body = "Thử chọn mục khác hoặc đổi từ khoá.",
                        )
                    } else {
                        ResultGrid(
                            items = filteredItems,
                            onClickItem = { item ->
                                val lc = item.linkcode ?: FshareUrl.extractLinkcode(item.url)
                                if (lc != null) onOpenFile(lc)
                            },
                        )
                    }
                }
                SearchResults.Empty -> FsEmptyState(
                    title = "Không tìm thấy kết quả",
                    body = "Thử với từ khoá khác hoặc kiểm tra chính tả.",
                )
                is SearchResults.Error -> ErrorContent(
                    message = s.message,
                    onSelectSuggestion = viewModel::setQuery,
                )
            }
        }
    }
}

@Composable
private fun SearchIdleContent(
    currentQuery: String,
    onQueryUpdate: (String) -> Unit,
    onSelectSuggestion: (String) -> Unit,
) {
    Row(modifier = Modifier.fillMaxSize().padding(FsSpacing.S7)) {
        // Left: keyboard
        Column(modifier = Modifier.weight(1.6f)) {
            Text(
                text = "Bàn phím",
                style = FsType.Caption,
                color = FsColors.TextSecondary,
            )
            Spacer(Modifier.height(FsSpacing.S3))
            FsKeyboard(
                onKey = { event ->
                    when (event) {
                        is FsKeyEvent.Char -> onQueryUpdate(currentQuery + event.value)
                        FsKeyEvent.Space -> onQueryUpdate("$currentQuery ")
                        FsKeyEvent.Backspace -> onQueryUpdate(currentQuery.dropLast(1))
                        FsKeyEvent.Enter -> Unit  // Search trigger qua debounce
                    }
                },
            )
        }

        Spacer(Modifier.width(FsSpacing.S6))

        // Right: suggestions
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = "Gợi ý cho bạn",
                style = FsType.TitleMd,
                color = FsColors.TextPrimary,
            )
            Spacer(Modifier.height(FsSpacing.S4))
            DEFAULT_SUGGESTIONS.forEach { kw ->
                SuggestionPill(text = kw, onClick = { onSelectSuggestion(kw) })
                Spacer(Modifier.height(FsSpacing.S2))
            }
            Spacer(Modifier.height(FsSpacing.S5))
            Text(
                text = "Nhập tối thiểu 2 ký tự để tìm kiếm.",
                style = FsType.Caption,
                color = FsColors.TextTertiary,
            )
        }
    }
}

@Composable
private fun ErrorContent(
    message: String,
    onSelectSuggestion: (String) -> Unit,
) {
    Column(modifier = Modifier.fillMaxSize().padding(FsSpacing.S7)) {
        Text(
            text = "Không tìm được",
            style = FsType.TitleLg,
            color = FsColors.Danger,
        )
        Spacer(Modifier.height(FsSpacing.S2))
        Text(
            text = message,
            style = FsType.BodyMd,
            color = FsColors.TextSecondary,
        )
        Spacer(Modifier.height(FsSpacing.S5))
        Text(
            text = "Thử các gợi ý:",
            style = FsType.TitleMd,
            color = FsColors.TextPrimary,
        )
        Spacer(Modifier.height(FsSpacing.S3))
        LazyRow(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3)) {
            items(DEFAULT_SUGGESTIONS) { kw ->
                SuggestionPill(text = kw, onClick = { onSelectSuggestion(kw) })
            }
        }
    }
}

@Composable
private fun LoadingGrid() {
    LazyVerticalGrid(
        columns = GridCells.Fixed(4),
        contentPadding = PaddingValues(FsSpacing.S7),
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        items(8) { FsCardSkeleton() }
    }
}

@Composable
private fun ResultGrid(
    items: List<vn.streamix.tv.core.domain.model.FeaturedItem>,
    onClickItem: (vn.streamix.tv.core.domain.model.FeaturedItem) -> Unit,
) {
    LazyVerticalGrid(
        columns = GridCells.Fixed(4),
        contentPadding = PaddingValues(FsSpacing.S7),
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
        verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        items(items, key = { it.url }) { item ->
            FsFileCard(
                title = item.title,
                subtitle = item.size?.let { FileSize.format(it) },
                thumbnailUrl = item.thumbnail,
                onClick = { onClickItem(item) },
            )
        }
    }
}

@Composable
private fun FilterChipsRow(
    active: SearchFilter,
    allItems: List<vn.streamix.tv.core.domain.model.FeaturedItem>,
    onSelect: (SearchFilter) -> Unit,
    modifier: Modifier = Modifier,
) {
    Row(modifier = modifier, horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
        SearchFilter.entries.forEach { filter ->
            val count = allItems.countBy(filter)
            FilterChip(
                label = filter.label,
                count = count.takeIf { it > 0 },
                selected = filter == active,
                onClick = { onSelect(filter) },
            )
        }
    }
}

@Composable
private fun FilterChip(
    label: String,
    count: Int?,
    selected: Boolean,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    val bg = when {
        isFocused -> FsColors.AccentPrimary
        selected -> FsColors.BgSurfaceHi
        else -> FsColors.BgSurface
    }
    val fg = when {
        isFocused -> FsColors.TextInverse
        selected -> FsColors.TextPrimary
        else -> FsColors.TextSecondary
    }
    val text = if (count != null) "$label · $count" else label
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(FsRadius.Sm))
            .background(bg)
            .border(
                width = if (isFocused) 3.dp else if (selected) 1.dp else 0.dp,
                color = if (isFocused) FsColors.FocusRing else FsColors.BorderDefault,
                shape = RoundedCornerShape(FsRadius.Sm),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(horizontal = FsSpacing.S4, vertical = FsSpacing.S2),
    ) {
        Text(text = text, style = FsType.Caption, color = fg)
    }
}

@Composable
private fun SuggestionPill(
    text: String,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    val bg = if (isFocused) FsColors.AccentPrimary else FsColors.BgSurface
    val fg = if (isFocused) FsColors.TextInverse else FsColors.TextPrimary
    Row(
        modifier = Modifier
            .height(48.dp)
            .clip(RoundedCornerShape(FsRadius.Full))
            .background(bg)
            .border(
                width = if (isFocused) FsFocus.RingWidth else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Full),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(horizontal = FsSpacing.S4),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(text = "+", style = FsType.TitleMd, color = fg)
        Spacer(Modifier.width(FsSpacing.S2))
        Text(text = text, style = FsType.BodyMd, color = fg)
    }
}
