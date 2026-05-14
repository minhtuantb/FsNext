/*
 * StreamIX TV — Files use cases
 */
package vn.streamix.tv.core.domain.usecase

import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FileItem
import vn.streamix.tv.core.domain.model.ItemType
import vn.streamix.tv.core.domain.repository.FilesRepository
import javax.inject.Inject

class ListFolderUseCase @Inject constructor(
    private val repo: FilesRepository,
) {
    suspend operator fun invoke(
        path: String,
        page: Int = 1,
        perPage: Int = 50,
    ): ApiResult<List<FileItem>> = repo.listFiles(path, page, perPage)
}

class ListRootFoldersUseCase @Inject constructor(
    private val repo: FilesRepository,
) {
    suspend operator fun invoke(limit: Int = 30): ApiResult<List<FileItem>> {
        // Try /api/v1/folders first (folder tree)
        return when (val r = repo.listFolderTree()) {
            is ApiResult.Success -> ApiResult.Success(
                r.data.filter { it.type == ItemType.FOLDER }.take(limit),
                r.meta, r.requestId,
            )
            // Fallback /api/v1/files?dir_only=1 nếu /folders fail (chưa implement backend)
            else -> repo.listFiles(path = "/", perPage = limit, dirOnly = true)
        }
    }
}

class ListRecentFilesUseCase @Inject constructor(
    private val repo: FilesRepository,
) {
    suspend operator fun invoke(limit: Int = 20): ApiResult<List<FileItem>> {
        val r = repo.listFiles(path = "/", perPage = limit, dirOnly = false)
        return if (r is ApiResult.Success) {
            ApiResult.Success(
                r.data.filter { it.type == ItemType.FILE }
                    .sortedByDescending { it.lastUpdatedAt ?: "" },
                r.meta, r.requestId,
            )
        } else r
    }
}

class GetFileInfoUseCase @Inject constructor(
    private val repo: FilesRepository,
) {
    suspend operator fun invoke(linkcode: String): ApiResult<FileItem> = repo.getFile(linkcode)
}

class SearchFilesUseCase @Inject constructor(
    private val repo: FilesRepository,
) {
    suspend operator fun invoke(query: String, page: Int = 1): ApiResult<List<FileItem>> =
        repo.searchFiles(query, page = page)
}
