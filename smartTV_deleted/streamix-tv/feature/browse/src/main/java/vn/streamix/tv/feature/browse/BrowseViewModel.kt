/*
 * StreamIX TV — Browse ViewModel (S3)
 * Reference: 13 §8
 */
package vn.streamix.tv.feature.browse

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.navigation.toRoute
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FileItem
import vn.streamix.tv.core.domain.repository.FilesRepository
import javax.inject.Inject

@HiltViewModel
class BrowseViewModel @Inject constructor(
    handle: SavedStateHandle,
    private val filesRepo: FilesRepository,
) : ViewModel() {

    val folderPath: String = handle["path"] ?: "/"
    val folderName: String = handle["name"] ?: ""

    private val _state = MutableStateFlow(BrowseUiState())
    val state: StateFlow<BrowseUiState> = _state.asStateFlow()

    private var currentPage = 1
    private val perPage = 50

    init {
        loadInitial()
    }

    fun loadInitial() = viewModelScope.launch {
        _state.update { it.copy(loading = true, items = emptyList(), error = null) }
        currentPage = 1
        val result = filesRepo.listFiles(folderPath, currentPage, perPage)
        when (result) {
            is ApiResult.Success -> {
                val sorted = sortItems(result.data)
                _state.update {
                    it.copy(
                        loading = false,
                        items = sorted,
                        hasMore = (result.meta?.total ?: sorted.size) > sorted.size,
                        total = result.meta?.total ?: sorted.size,
                    )
                }
            }
            is ApiResult.Error -> _state.update { it.copy(loading = false, error = result.message) }
            ApiResult.NetworkError -> _state.update { it.copy(loading = false, error = "Mất kết nối") }
            else -> _state.update { it.copy(loading = false, error = "Không tải được") }
        }
    }

    fun loadMore() {
        if (_state.value.loadingMore || !_state.value.hasMore) return
        viewModelScope.launch {
            _state.update { it.copy(loadingMore = true) }
            currentPage += 1
            val result = filesRepo.listFiles(folderPath, currentPage, perPage)
            when (result) {
                is ApiResult.Success -> {
                    val combined = sortItems(_state.value.items + result.data)
                    _state.update {
                        it.copy(
                            loadingMore = false,
                            items = combined,
                            hasMore = combined.size < it.total,
                        )
                    }
                }
                else -> _state.update { it.copy(loadingMore = false) }
            }
        }
    }

    private fun sortItems(items: List<FileItem>): List<FileItem> =
        items.sortedWith(compareBy({ it.type.ordinal }, { it.name.lowercase() }))
}

data class BrowseUiState(
    val loading: Boolean = false,
    val loadingMore: Boolean = false,
    val items: List<FileItem> = emptyList(),
    val hasMore: Boolean = false,
    val total: Int = 0,
    val error: String? = null,
)
