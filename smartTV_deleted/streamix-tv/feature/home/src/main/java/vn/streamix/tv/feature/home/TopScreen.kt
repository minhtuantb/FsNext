/*
 * StreamIX TV — Top ranking screen.
 *
 * Reference: design system mockup S2c (vertical list 100 phim với rank numbers,
 * filter source IMDB/RT/Fshare).
 *
 * Layout:
 *   ┌─────────────────────────────────────────────────────────┐
 *   │ Top                          [⚙] [HN]                   │  ← top bar
 *   │ 🏆 BẢNG XẾP HẠNG     [IMDB] [RT] [FSHARE]               │
 *   │ Top · 100 phim hàng đầu mọi thời đại                    │
 *   │                                                         │
 *   │ 01  [thumb]  The Godfather                              │
 *   │              1972 · 9.4 GB · MKV                ›       │
 *   │ 02  [thumb]  The Dark Knight                            │
 *   │              2008 · 8.7 GB · MKV                ›       │
 *   │ 03  ...                                                 │
 *   └─────────────────────────────────────────────────────────┘
 */
package vn.streamix.tv.feature.home

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
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.common.FileSize
import vn.streamix.tv.core.domain.model.TopMovie
import vn.streamix.tv.core.domain.model.TopSource
import vn.streamix.tv.core.ui.components.FsCardSkeleton
import vn.streamix.tv.core.ui.components.FsEmptyState
import vn.streamix.tv.core.ui.components.FsErrorState
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsFocus
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun TopScreen(
    onOpenFile: (linkcode: String) -> Unit,
    onBack: () -> Unit,
    viewModel: TopViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()

    Column(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        // Header
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = FsSpacing.S7, vertical = FsSpacing.S5),
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "🏆",
                    style = FsType.TitleMd,
                    color = FsColors.AccentPrimary,
                )
                Spacer(Modifier.width(FsSpacing.S3))
                Text(
                    text = "BẢNG XẾP HẠNG",
                    style = FsType.Caption,
                    color = FsColors.TextSecondary,
                )
                Spacer(Modifier.weight(1f))
                SourceFilter(
                    selected = state.source,
                    onSelect = viewModel::selectSource,
                )
            }
            Spacer(Modifier.height(FsSpacing.S3))
            Text(
                text = "Top",
                style = FsType.DisplayLg,
                color = FsColors.TextPrimary,
            )
            Spacer(Modifier.height(FsSpacing.S2))
            Text(
                text = "100 phim hàng đầu mọi thời đại",
                style = FsType.BodyMd,
                color = FsColors.TextSecondary,
            )
        }

        // Content
        Box(modifier = Modifier.fillMaxSize()) {
            when {
                state.loading -> LazyColumn(
                    contentPadding = PaddingValues(horizontal = FsSpacing.S7),
                    verticalArrangement = Arrangement.spacedBy(FsSpacing.S3),
                ) {
                    items(8) { FsCardSkeleton() }
                }
                state.error != null -> FsErrorState(
                    title = "Không tải được",
                    body = state.error!!,
                    onRetry = { viewModel.selectSource(state.source) },
                )
                state.items.isEmpty() -> FsEmptyState(
                    title = "Bảng xếp hạng đang trống",
                    body = "Đội biên tập đang cập nhật. Quay lại sau ít phút.",
                )
                else -> LazyColumn(
                    contentPadding = PaddingValues(horizontal = FsSpacing.S7, vertical = FsSpacing.S3),
                    verticalArrangement = Arrangement.spacedBy(FsSpacing.S3),
                ) {
                    items(state.items, key = { it.rank }) { movie ->
                        TopMovieRow(
                            movie = movie,
                            onClick = {
                                val lc = movie.linkcode ?: return@TopMovieRow
                                onOpenFile(lc)
                            },
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun SourceFilter(
    selected: TopSource,
    onSelect: (TopSource) -> Unit,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(FsSpacing.S2)) {
        SourceChip(label = "IMDB", selected = selected == TopSource.IMDB) {
            onSelect(TopSource.IMDB)
        }
        SourceChip(label = "RT", selected = selected == TopSource.RT) {
            onSelect(TopSource.RT)
        }
        SourceChip(label = "FSHARE", selected = selected == TopSource.FSHARE) {
            onSelect(TopSource.FSHARE)
        }
    }
}

@Composable
private fun SourceChip(label: String, selected: Boolean, onClick: () -> Unit) {
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

@Composable
private fun TopMovieRow(
    movie: TopMovie,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val bgColor = if (isFocused) FsColors.BgSurfaceHi else FsColors.BgSurface

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(96.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(bgColor)
            .border(
                width = if (isFocused) FsFocus.RingWidth else 0.dp,
                color = if (isFocused) FsColors.FocusRing else Color.Transparent,
                shape = RoundedCornerShape(FsRadius.Md),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(horizontal = FsSpacing.S5),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        // Rank
        Text(
            text = movie.rank.toString().padStart(2, '0'),
            style = FsType.DisplayMd,
            color = if (isFocused) FsColors.AccentPrimary else FsColors.TextSecondary,
            modifier = Modifier.width(72.dp),
        )

        // Thumbnail placeholder
        Box(
            modifier = Modifier
                .size(width = 96.dp, height = 56.dp)
                .clip(RoundedCornerShape(FsRadius.Sm))
                .background(FsColors.BgBase),
        )

        Spacer(Modifier.width(FsSpacing.S5))

        // Title + meta
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = movie.title,
                style = FsType.TitleMd,
                color = FsColors.TextPrimary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            Spacer(Modifier.height(FsSpacing.S1))
            Text(
                text = buildMeta(movie),
                style = FsType.Caption,
                color = FsColors.TextSecondary,
            )
        }

        Text(
            text = "›",
            style = FsType.TitleLg,
            color = FsColors.TextSecondary,
        )
    }
}

private fun buildMeta(movie: TopMovie): String {
    val parts = mutableListOf<String>()
    movie.year?.let { parts += it.toString() }
    movie.size?.let { parts += FileSize.format(it) }
    movie.format?.let { parts += it }
    return parts.joinToString(" · ").ifBlank { "—" }
}
