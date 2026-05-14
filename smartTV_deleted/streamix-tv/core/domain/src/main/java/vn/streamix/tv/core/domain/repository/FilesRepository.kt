/*
 * StreamIX TV — FilesRepository interface
 * Reference: 13 §3.1 API-5/6/7/8/9
 */
package vn.streamix.tv.core.domain.repository

import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FileItem

interface FilesRepository {
    /** API-5: GET /api/v1/files?path=&page=&per_page=&dir_only=&ext= */
    suspend fun listFiles(
        path: String = "/",
        page: Int = 1,
        perPage: Int = 50,
        dirOnly: Boolean = false,
        ext: String? = null,
    ): ApiResult<List<FileItem>>

    /** API-6: GET /api/v1/files/{linkcode} */
    suspend fun getFile(linkcode: String): ApiResult<FileItem>

    /** API-7: GET /api/v1/files/{linkcode}/metadata */
    suspend fun getFileMetadata(linkcode: String): ApiResult<FileItem>

    /** API-8: GET /api/v1/files/search?q=&path=&ext=&page=&per_page= */
    suspend fun searchFiles(
        query: String,
        path: String? = null,
        ext: String? = null,
        page: Int = 1,
        perPage: Int = 50,
    ): ApiResult<List<FileItem>>

    /** API-9: GET /api/v1/folders */
    suspend fun listFolderTree(): ApiResult<List<FileItem>>
}
