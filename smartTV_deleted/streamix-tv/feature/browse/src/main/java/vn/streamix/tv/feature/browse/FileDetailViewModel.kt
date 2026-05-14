/*
 * StreamIX TV — File Detail ViewModel (S4)
 * Reference: 13 §9
 */
package vn.streamix.tv.feature.browse

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FileCategory
import vn.streamix.tv.core.domain.model.FileItem
import vn.streamix.tv.core.domain.repository.FilesRepository
import javax.inject.Inject

@HiltViewModel
class FileDetailViewModel @Inject constructor(
    handle: SavedStateHandle,
    private val filesRepo: FilesRepository,
) : ViewModel() {

    val linkcode: String = handle["linkcode"] ?: ""

    private val _state = MutableStateFlow<FileDetailUiState>(FileDetailUiState.Loading)
    val state: StateFlow<FileDetailUiState> = _state.asStateFlow()

    init {
        load()
    }

    fun load() = viewModelScope.launch {
        _state.value = FileDetailUiState.Loading
        when (val r = filesRepo.getFile(linkcode)) {
            is ApiResult.Success -> {
                val file = r.data
                val playable = FileCategory.isPlayable(FileCategory.fromExtension(file.name))
                _state.value = FileDetailUiState.Loaded(file, playable)
            }
            is ApiResult.Error -> _state.value = FileDetailUiState.Error(r.message)
            ApiResult.NetworkError -> _state.value = FileDetailUiState.Error("Mất kết nối")
            else -> _state.value = FileDetailUiState.Error("Không tải được")
        }
    }
}

sealed interface FileDetailUiState {
    data object Loading : FileDetailUiState
    data class Loaded(val file: FileItem, val playable: Boolean) : FileDetailUiState
    data class Error(val message: String) : FileDetailUiState
    data class PasswordRequired(val file: FileItem) : FileDetailUiState
}
