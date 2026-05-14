/*
 * StreamIX TV — ViewModels for Suggested + Trending detail screens.
 *
 * Reference: design system mockups S2a (Gợi Ý detail) + S2b (Xu Hướng detail).
 * Implementation V1: grid view của FeaturedRepository data.
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
import vn.streamix.tv.core.domain.model.FeaturedItem
import vn.streamix.tv.core.domain.repository.FeaturedRepository
import javax.inject.Inject

@HiltViewModel
class SuggestedViewModel @Inject constructor(
    private val repo: FeaturedRepository,
) : ViewModel() {
    private val _state = MutableStateFlow(CatalogUiState())
    val state: StateFlow<CatalogUiState> = _state.asStateFlow()

    init { load() }

    fun load() = viewModelScope.launch {
        _state.value = _state.value.copy(loading = true, error = null)
        when (val r = repo.suggested()) {
            is ApiResult.Success -> _state.value = CatalogUiState(loading = false, items = r.data)
            is ApiResult.Error -> _state.value = _state.value.copy(loading = false, error = r.message)
            ApiResult.NetworkError -> _state.value = _state.value.copy(loading = false, error = "Không có kết nối mạng")
            ApiResult.AuthExpired -> _state.value = _state.value.copy(loading = false, error = "Phiên hết hạn")
            is ApiResult.RateLimited -> _state.value = _state.value.copy(loading = false, error = "Quá nhiều yêu cầu")
        }
    }
}

@HiltViewModel
class TrendingViewModel @Inject constructor(
    private val repo: FeaturedRepository,
) : ViewModel() {
    private val _state = MutableStateFlow(CatalogUiState())
    val state: StateFlow<CatalogUiState> = _state.asStateFlow()

    init { load() }

    fun load() = viewModelScope.launch {
        _state.value = _state.value.copy(loading = true, error = null)
        when (val r = repo.trending()) {
            is ApiResult.Success -> _state.value = CatalogUiState(loading = false, items = r.data)
            is ApiResult.Error -> _state.value = _state.value.copy(loading = false, error = r.message)
            ApiResult.NetworkError -> _state.value = _state.value.copy(loading = false, error = "Không có kết nối mạng")
            ApiResult.AuthExpired -> _state.value = _state.value.copy(loading = false, error = "Phiên hết hạn")
            is ApiResult.RateLimited -> _state.value = _state.value.copy(loading = false, error = "Quá nhiều yêu cầu")
        }
    }
}

data class CatalogUiState(
    val loading: Boolean = true,
    val items: List<FeaturedItem> = emptyList(),
    val error: String? = null,
)
