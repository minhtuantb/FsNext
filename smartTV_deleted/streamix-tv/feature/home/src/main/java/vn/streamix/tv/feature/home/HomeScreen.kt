/*
 * StreamIX TV — Home screen (S2) theo design system mockup.
 *
 * Layout:
 *   ┌────────────────────────────────────────────────────────┐
 *   │ [S] StreamIX TV               [⚙] [Avatar]             │  ← top bar
 *   │                                                        │
 *   │ ┌──────────────────────────────────────────────────┐  │
 *   │ │ 🔍 Tìm phim, tên file, hoặc bộ phim...    OK·... │  │  ← search bar
 *   │ └──────────────────────────────────────────────────┘  │
 *   │                                                        │
 *   │ ┌─────────┐ ┌─────────┐ ┌─────────┐                    │
 *   │ │ ⭐      │ │ 🔥      │ │ 🏆      │                   │
 *   │ │         │ │         │ │         │                    │
 *   │ │ Gợi Ý   │ │ Xu Hướng│ │ Top     │                    │
 *   │ │ 42 mục  │ │ 55 mục  │ │ 100 phim│                    │
 *   │ │       › │ │       › │ │       › │                    │
 *   │ └─────────┘ └─────────┘ └─────────┘                    │
 *   │                                                        │
 *   │ Tiếp tục xem · 3 mục                                   │
 *   │ [card1] [card2] [card3] [card4] ...                    │
 *   └────────────────────────────────────────────────────────┘
 */
package vn.streamix.tv.feature.home

import android.app.Activity
import androidx.activity.compose.BackHandler
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
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import vn.streamix.tv.core.ui.components.FsFileCard
import vn.streamix.tv.core.ui.components.FsHeroCard
import vn.streamix.tv.core.ui.components.FsHeroVariant
import vn.streamix.tv.core.ui.components.FsTopBar
import vn.streamix.tv.core.ui.dialogs.ExitAppDialog
import vn.streamix.tv.core.ui.dialogs.SessionExpiredDialog
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsFocus
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun HomeScreen(
    onOpenSuggested: () -> Unit,
    onOpenTrending: () -> Unit,
    onOpenTop: () -> Unit,
    onOpenContinue: (linkcode: String, resumeMs: Long) -> Unit,
    onOpenSearch: () -> Unit,
    onOpenSettings: () -> Unit,
    onOpenAccount: () -> Unit,
    onSessionExpiredSignIn: () -> Unit,
    viewModel: HomeViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    val context = LocalContext.current
    var showExitDialog by remember { mutableStateOf(false) }

    BackHandler { showExitDialog = true }

    Box(modifier = Modifier.fillMaxSize().background(FsColors.BgBase)) {
        Column(modifier = Modifier.fillMaxSize()) {
            FsTopBar(
                title = "StreamIX TV",
                onSearchClick = onOpenSearch,
                onSettingsClick = onOpenSettings,
                onAvatarClick = onOpenAccount,
                avatarInitial = state.user?.fullName?.take(1)?.uppercase(),
            )

            LazyColumn(
                modifier = Modifier.fillMaxSize(),
                contentPadding = PaddingValues(
                    top = FsSpacing.S5,
                    bottom = FsSpacing.S7,
                ),
                verticalArrangement = Arrangement.spacedBy(FsSpacing.S6),
            ) {
                // Search bar (prominent, click → SearchScreen)
                item {
                    HomeSearchBar(
                        modifier = Modifier.padding(horizontal = FsSpacing.S7),
                        onClick = onOpenSearch,
                    )
                }

                // 3 hero cards
                item {
                    HeroRow(
                        suggestedCount = state.suggestedCount,
                        trendingCount = state.trendingCount,
                        topCount = state.topCount,
                        onOpenSuggested = onOpenSuggested,
                        onOpenTrending = onOpenTrending,
                        onOpenTop = onOpenTop,
                    )
                }

                // Continue watching row
                if (state.continueWatching.isNotEmpty()) {
                    item {
                        Column {
                            SectionLabel(
                                text = "Tiếp tục xem · ${state.continueWatching.size} mục",
                            )
                            Spacer(Modifier.height(FsSpacing.S4))
                            LazyRow(
                                contentPadding = PaddingValues(horizontal = FsSpacing.S7),
                                horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
                            ) {
                                items(state.continueWatching, key = { it.linkcode }) { pos ->
                                    FsFileCard(
                                        title = pos.fileName,
                                        subtitle = "${pos.percent}% đã xem",
                                        thumbnailUrl = pos.thumbnail,
                                        progressPercent = pos.percent,
                                        onClick = { onOpenContinue(pos.linkcode, pos.positionMs) },
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }

        ExitAppDialog(
            visible = showExitDialog,
            onConfirmExit = { (context as? Activity)?.finish() },
            onDismiss = { showExitDialog = false },
        )

        SessionExpiredDialog(
            visible = state.sessionExpired,
            onSignInAgain = {
                viewModel.onSessionExpiredAcknowledged()
                onSessionExpiredSignIn()
            },
        )
    }
}

@Composable
private fun HomeSearchBar(
    modifier: Modifier = Modifier,
    onClick: () -> Unit,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()
    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(72.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(FsColors.BgSurface)
            .border(
                width = if (isFocused) FsFocus.RingWidth else 1.dp,
                color = if (isFocused) FsColors.FocusRing else FsColors.BorderDefault,
                shape = RoundedCornerShape(FsRadius.Md),
            )
            .clickable(interactionSource, indication = null, onClick = onClick)
            .padding(horizontal = FsSpacing.S5),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(text = "🔍", style = FsType.TitleMd)
        Spacer(Modifier.width(FsSpacing.S3))
        Text(
            text = "Tìm phim, tên file, hoặc bộ phim...",
            style = FsType.BodyLg,
            color = FsColors.TextSecondary,
            modifier = Modifier.weight(1f),
        )
        Box(
            modifier = Modifier
                .clip(RoundedCornerShape(FsRadius.Sm))
                .background(FsColors.BgSurfaceHi)
                .padding(horizontal = FsSpacing.S3, vertical = FsSpacing.S2),
        ) {
            Text(
                text = "OK · TÌM KIẾM",
                style = FsType.Caption,
                color = FsColors.TextSecondary,
            )
        }
    }
}

@Composable
private fun HeroRow(
    suggestedCount: Int?,
    trendingCount: Int?,
    topCount: Int?,
    onOpenSuggested: () -> Unit,
    onOpenTrending: () -> Unit,
    onOpenTop: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = FsSpacing.S7),
        horizontalArrangement = Arrangement.spacedBy(FsSpacing.S4),
    ) {
        FsHeroCard(
            modifier = Modifier
                .weight(1f)
                .height(220.dp),
            title = "Gợi Ý",
            subtitle = if (suggestedCount != null) "$suggestedCount mục · cá nhân hoá"
                       else "Đang tải...",
            icon = "⭐",
            variant = FsHeroVariant.Suggested,
            onClick = onOpenSuggested,
        )
        FsHeroCard(
            modifier = Modifier
                .weight(1f)
                .height(220.dp),
            title = "Xu Hướng",
            subtitle = if (trendingCount != null) "$trendingCount mục · 7 ngày qua"
                       else "Đang tải...",
            icon = "🔥",
            variant = FsHeroVariant.Trending,
            onClick = onOpenTrending,
        )
        FsHeroCard(
            modifier = Modifier
                .weight(1f)
                .height(220.dp),
            title = "Top",
            subtitle = if (topCount != null) "${topCount.coerceAtLeast(0)} phim hàng đầu"
                       else "100 phim hàng đầu",
            icon = "🏆",
            variant = FsHeroVariant.Top,
            onClick = onOpenTop,
        )
    }
}

@Composable
private fun SectionLabel(text: String) {
    Text(
        text = text,
        style = FsType.TitleMd,
        color = FsColors.TextPrimary,
        modifier = Modifier.padding(horizontal = FsSpacing.S7),
    )
}
