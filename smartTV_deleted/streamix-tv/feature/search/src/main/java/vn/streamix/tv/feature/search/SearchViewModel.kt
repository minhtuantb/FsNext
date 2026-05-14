/*
 * StreamIX TV — Search ViewModel (S12) — Wave M4: rewire qua Timfshare partner.
 *
 * Reference: 13 §23 + API_REFERENCE_FOR_REUSE.md §4.1.
 *
 * Lưu ý: SearchRepository (Timfshare) đã sanitize input trước khi gọi API,
 * nên ViewModel chỉ debounce + dispatch.
 */
package vn.streamix.tv.feature.search

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.filter
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FeaturedItem
import vn.streamix.tv.core.domain.repository.SearchRepository
import javax.inject.Inject

@OptIn(FlowPreview::class)
@HiltViewModel
class SearchViewModel @Inject constructor(
    private val searchRepo: SearchRepository,
) : ViewModel() {

    private val _query = MutableStateFlow("")
    val query: StateFlow<String> = _query.asStateFlow()

    private val _results = MutableStateFlow<SearchResults>(SearchResults.Idle)
    val results: StateFlow<SearchResults> = _results.asStateFlow()

    init {
        viewModelScope.launch {
            _query
                .debounce(500)
                .distinctUntilChanged()
                .filter { it.length >= MIN_QUERY_LENGTH }
                .collect { q -> performSearch(q) }
        }
    }

    fun setQuery(q: String) {
        _query.value = q
        if (q.length < MIN_QUERY_LENGTH) _results.value = SearchResults.Idle
    }

    fun clear() {
        _query.value = ""
        _results.value = SearchResults.Idle
    }

    private suspend fun performSearch(q: String) {
        _results.value = SearchResults.Searching
        when (val r = searchRepo.search(q)) {
            is ApiResult.Success -> {
                _results.value = if (r.data.isEmpty()) SearchResults.Empty
                                else SearchResults.Loaded(r.data)
            }
            is ApiResult.Error -> _results.value = SearchResults.Error(r.message)
            ApiResult.NetworkError -> _results.value = SearchResults.Error("Không có kết nối mạng")
            ApiResult.AuthExpired -> _results.value = SearchResults.Error("Phiên hết hạn — vui lòng đăng nhập lại")
            is ApiResult.RateLimited -> _results.value = SearchResults.Error("Quá nhiều yêu cầu, thử lại sau")
        }
    }

    companion object {
        const val MIN_QUERY_LENGTH = 2
    }
}

sealed interface SearchResults {
    data object Idle : SearchResults
    data object Searching : SearchResults
    data class Loaded(val items: List<FeaturedItem>) : SearchResults
    data object Empty : SearchResults
    data class Error(val message: String) : SearchResults
}
