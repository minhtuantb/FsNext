/*
 * StreamIX TV — Top ranking screen ViewModel.
 *
 * Reference: design system mockup S2c — vertical list 100 phim, filter source
 * (IMDB/RT/Fshare).
 */
package vn.streamix.tv.feature.home

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.TopMovie
import vn.streamix.tv.core.domain.model.TopSource
import vn.streamix.tv.core.domain.repository.TopRepository
import javax.inject.Inject

@HiltViewModel
class TopViewModel @Inject constructor(
    private val topRepo: TopRepository,
) : ViewModel() {

    private val _state = MutableStateFlow(TopUiState())
    val state: StateFlow<TopUiState> = _state.asStateFlow()

    init {
        load(TopSource.IMDB)
    }

    fun selectSource(source: TopSource) {
        if (_state.value.source == source) return
        load(source)
    }

    private fun load(source: TopSource) = viewModelScope.launch {
        _state.value = _state.value.copy(source = source, loading = true, error = null)
        when (val r = topRepo.top(source)) {
            is ApiResult.Success -> _state.value = TopUiState(
                source = source,
                loading = false,
                items = r.data,
            )
            is ApiResult.Error -> _state.value = _state.value.copy(
                loading = false,
                error = r.message,
            )
            ApiResult.NetworkError -> _state.value = _state.value.copy(
                loading = false,
                error = "Không có kết nối mạng",
            )
            ApiResult.AuthExpired -> _state.value = _state.value.copy(
                loading = false,
                error = "Phiên hết hạn",
            )
            is ApiResult.RateLimited -> _state.value = _state.value.copy(
                loading = false,
                error = "Quá nhiều yêu cầu, thử lại sau",
            )
        }
    }
}

data class TopUiState(
    val source: TopSource = TopSource.IMDB,
    val loading: Boolean = true,
    val items: List<TopMovie> = emptyList(),
    val error: String? = null,
)
