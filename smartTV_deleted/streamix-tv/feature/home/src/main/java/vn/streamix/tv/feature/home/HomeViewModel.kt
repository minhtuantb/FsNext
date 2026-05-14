/*
 * StreamIX TV — Home ViewModel (design system layout: search + 3 hero cards + Continue Watching).
 *
 * Mockup S2:
 *   - Search bar (focusable, click → SearchScreen)
 *   - 3 hero cards: Gợi Ý / Xu Hướng / Top
 *   - "Tiếp tục xem" row (PlaybackPosition local Room)
 *
 * Counts hiển thị trong subtitle hero card lấy từ:
 *   - Suggested: featuredRepo.suggested().size
 *   - Trending:  featuredRepo.trending().size
 *   - Top:       topRepo.top().size
 *
 * Reference: 25_design-system-gap-analysis.md §S2.
 */
package vn.streamix.tv.feature.home

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.PlaybackPosition
import vn.streamix.tv.core.domain.model.TopSource
import vn.streamix.tv.core.domain.model.User
import vn.streamix.tv.core.domain.repository.FeaturedRepository
import vn.streamix.tv.core.domain.repository.PlaybackPositionRepository
import vn.streamix.tv.core.domain.repository.TopRepository
import vn.streamix.tv.core.domain.repository.UserRepository
import javax.inject.Inject

@HiltViewModel
class HomeViewModel @Inject constructor(
    private val userRepo: UserRepository,
    private val featuredRepo: FeaturedRepository,
    private val topRepo: TopRepository,
    private val playbackRepo: PlaybackPositionRepository,
) : ViewModel() {

    private val _suggestedCount = MutableStateFlow<Int?>(null)
    private val _trendingCount = MutableStateFlow<Int?>(null)
    private val _topCount = MutableStateFlow<Int?>(null)
    private val _sessionExpired = MutableStateFlow(false)

    val state: StateFlow<HomeUiState> = combine(
        userRepo.observeMe(),
        playbackRepo.observeContinueWatching(20),
        combine(_suggestedCount, _trendingCount, _topCount) { s, t, top ->
            Triple(s, t, top)
        },
        _sessionExpired,
    ) { user, cw, counts, expired ->
        HomeUiState(
            user = user,
            continueWatching = cw,
            suggestedCount = counts.first,
            trendingCount = counts.second,
            topCount = counts.third,
            sessionExpired = expired,
        )
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5_000), HomeUiState())

    init {
        refresh()
    }

    fun refresh() {
        viewModelScope.launch {
            when (userRepo.refreshMe()) {
                ApiResult.AuthExpired -> _sessionExpired.value = true
                else -> Unit
            }
        }
        viewModelScope.launch {
            featuredRepo.suggested().let { r ->
                if (r is ApiResult.Success) _suggestedCount.value = r.data.size
                if (r is ApiResult.AuthExpired) _sessionExpired.value = true
            }
        }
        viewModelScope.launch {
            featuredRepo.trending().let { r ->
                if (r is ApiResult.Success) _trendingCount.value = r.data.size
                if (r is ApiResult.AuthExpired) _sessionExpired.value = true
            }
        }
        viewModelScope.launch {
            topRepo.top(TopSource.IMDB).let { r ->
                if (r is ApiResult.Success) _topCount.value = r.data.size
            }
        }
    }

    fun onSessionExpiredAcknowledged() {
        _sessionExpired.value = false
    }
}

data class HomeUiState(
    val user: User? = null,
    val continueWatching: List<PlaybackPosition> = emptyList(),
    /** null = chưa load xong, 0 = empty, >0 = có items. */
    val suggestedCount: Int? = null,
    val trendingCount: Int? = null,
    val topCount: Int? = null,
    val sessionExpired: Boolean = false,
)
