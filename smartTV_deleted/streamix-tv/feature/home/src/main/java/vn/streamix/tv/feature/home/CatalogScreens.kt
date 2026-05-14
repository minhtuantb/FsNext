/*
 * StreamIX TV — SuggestedScreen + TrendingScreen với advanced UI.
 *
 * Reference: mockups S2a (Gợi Ý) + S2b (Xu Hướng).
 *
 * V1.1 features:
 *  - Folder cards (FsFolderCard) cho item là folder URL.
 *  - Video badges (HD/4K/NEW) derive từ filename pattern.
 *  - Sort filter Suggested: SẮP XẾP MỚI NHẤT / TẤT CẢ.
 *  - Time period tabs Trending: HÔM NAY / 7 NGÀY / 30 NGÀY (visual V1, data như nhau).
 */
package vn.streamix.tv.feature.home

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
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
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.core.domain.model.FeaturedItem
import vn.streamix.tv.core.domain.model.FeaturedSource
import vn.streamix.tv.core.ui.components.FsCardSkeleton
import vn.streamix.tv.core.ui.components.FsEmptyState
import vn.streamix.tv.core.ui.components.FsErrorState
import vn.streamix.tv.core.ui.components.FsFileCard
import vn.streamix.tv.core.ui.components.FsFolderCard
import vn.streamix.tv.core.ui.components.deriveBadgeVariant
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

private enum class SuggestedSort(val label: String) {
    Newest("Mới nhất"),
    All("Tất cả"),
}

private enum class TrendingPeriod(val label: String) {
    Today("Hôm nay"),
    Week("7 ngày"),
    Month("30 ngày"),
}

@Composable
fun SuggestedScreen(
    onOpenItem: (FeaturedItem) -> Unit,
    onBack: () -> Unit,
    viewModel: SuggestedViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    var sort by remember { mutableStateOf(SuggestedSort.Newest) }

    // V1: sort = newest giữ nguyên thứ tự sheet (đã ordered theo team biên tập);
    // sort = All cũng hiển thị giống. Backend chưa cung cấp updatedAt nên 2 chế độ
    // hiện như nhau — placeholder cho V1.2 khi có metadata.
    val sortedItems = state.items

    CatalogContent(
        title = "Gợi Ý",
        eyebrow = "ĐỀ XUẤT CHO BẠN",
        eyebrowIcon = "⭐",
        countText = if (state.items.isNotEmpty()) "${state.items.size} mục · cá nhân hoá" else null,
        loading = state.loading,
        items = sortedItems,
        error = state.error,
        onRetry = viewModel::load,
        onOpenItem = onOpenItem,
        topRightSlot = {
            SortChips(
                active = sort,
                onSelect = { sort = it },
            )
        },
    )
}

@Composable
fun TrendingScreen(
    onOpenItem: (FeaturedItem) -> Unit,
    onBack: () -> Unit,
    viewModel: TrendingViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    var period by remember { mutableStateOf(TrendingPeriod.Week) }

    CatalogContent(
        title = "Xu Hướng",
        eyebrow = "BẢNG XẾP HẠNG NHANH",
        eyebrowIcon = "🔥",
        countText = if (state.items.isNotEmpty()) "${state.items.size} mục · ${period.label.lowercase()} qua" else null,
        loading = state.loading,
        items = state.items,
        error = state.error,
        onRetry = viewModel::load,
        onOpenItem = onOpenItem,
        topRightSlot = {
            PeriodTabs(
                active = period,
                onSelect = { period = it },
            )
        },
    )
}

@Composable
private fun CatalogContent(
    title: String,
    eyebrow: String,
    eyebrowIcon: String,
    countText: String?,
    loading: Boolean,
    items: List<FeaturedItem>,
    error: String?,
    onRetry: () -> Unit,
    onOpenItem: (FeaturedItem) -> Unit,
    topRightSlot: @Composable (() -> Unit)? = null,
) {
    Column(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        // Header
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = FsSpacing.S7, vertical = FsSpacing.S5),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(text = eyebrowIcon, style = FsType.TitleMd, color = FsColors.AccentPrimary)
                    Spacer(Modifier.width(FsSpacing.S3))
                    Text(text = eyebrow, style = FsType.Caption, color = FsColors.TextSecondary)
                }
                Spacer(Modifier.height(FsSpacing.S3))
                Text(text = title, style = FsType.DisplayLg, color = FsColors.TextPrimary)
                if (countText != null) {
                    Spacer(Modifier.height(FsSpacing.S2))
                    Text(text = countText, style = FsType.BodyMd, color = FsColors.TextSecondary)
                }
            }
            topRightSlot?.invoke()
        }

        Box(modifier = Modifier.fillMaxSize()) {
            when {
                loading -> LazyVerticalGrid(
                    columns = GridCells.Fixed(4),
                    contentPadding = PaddingValues(FsSpacing.S7),
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                    verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                ) {
                    items(8) { FsCardSkeleton() }
                }
                error != null -> FsErrorState(
                    title = "Không tải được",
                    body = error,
                    onRetry = onRetry,
                )
                items.isEmpty() -> FsEmptyState(
                    title = "Chưa có mục nào",
                    body = "Đội biên tập đang cập nhật. Quay lại sau.",
                )
                else -> LazyVerticalGrid(
                    columns = GridCells.Fixed(4),
                    contentPadding = PaddingValues(FsSpacing.S7),
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                    verticalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                ) {
                    items(items, key = { it.url }) { item ->
                        if (FshareUrl.isFolderUrl(item.url)) {
                            FsFolderCard(
                                name = item.title,
                                subtitle = "Thư mục",
                                onClick = { onOpenItem(item) },
                            )
                        } else {
                            FsFileCard(
                                title = item.title,
                                subtitle = item.size?.let { FileSize.format(it) },
                                thumbnailUrl = item.thumbnail,
                                badge = deriveBadgeVariant(
                                    title = item.title,
                                    isNew = item.source == FeaturedSource.TRENDING,
                                ),
                                onClick = { onOpenItem(item) },
                            )
                        }
                    }
                }
            }
        }
    }
}

// ─── Sort chips (Suggested) ───────────────────────────────────────────

@Composable
private fun SortChips(
    active: SuggestedSort,
    onSelect: (SuggestedSort) -> Unit,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
        SuggestedSort.entries.forEach { s ->
            CatalogChip(
                label = if (s == SuggestedSort.Newest) "SẮP XẾP · ${s.label}" else s.label.uppercase(),
                selected = s == active,
                onClick = { onSelect(s) },
            )
        }
    }
}

// ─── Period tabs (Trending) ───────────────────────────────────────────

@Composable
private fun PeriodTabs(
    active: TrendingPeriod,
    onSelect: (TrendingPeriod) -> Unit,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
        TrendingPeriod.entries.forEach { p ->
            CatalogChip(
                label = p.label.uppercase(),
                selected = p == active,
                onClick = { onSelect(p) },
            )
        }
    }
}

@Composable
private fun CatalogChip(label: String, selected: Boolean, onClick: () -> Unit) {
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
            .padding(horizontal = FsSpacing.S3, vertical = FsSpacing.S2),
    ) {
        Text(text = label, style = FsType.Caption, color = fg)
    }
}
