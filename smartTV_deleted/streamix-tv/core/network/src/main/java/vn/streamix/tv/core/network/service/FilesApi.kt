/*
 * StreamIX TV — Files API (Fshare LEGACY).
 *
 * Endpoints:
 *  - POST /api/fileops/get          — lấy info file (theo URL).
 *  - POST /api/fileops/getFolderList — list thư mục (theo canonical folder URL).
 *
 * Response của getFolderList có thể là array thuần hoặc object có `data`/`items`/...
 * Để tránh Moshi JsonDataException khi gặp array, getFolderList trả raw ResponseBody —
 * Repository tự parse linh hoạt (xem FilesRepositoryImpl.parseFolderListResponse).
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.4 + §3.5.
 */
package vn.streamix.tv.core.network.service

import com.squareup.moshi.Json
import com.squareup.moshi.JsonClass
import okhttp3.ResponseBody
import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.POST
import vn.streamix.tv.core.network.envelope.IntOrString
import vn.streamix.tv.core.network.envelope.LegacyResponse

interface FilesApi {

    @POST("api/fileops/get")
    suspend fun getFile(@Body body: GetFileRequestDto): Response<GetFileResponseDto>

    /**
     * Trả raw response body để Repository tự parse — server có thể trả:
     *   1. JSON array thuần các item folder/file.
     *   2. JSON object có code + nested data chứa array.
     *   3. JSON object có code + top-level items array.
     *   4. JSON object có code 201 (session expired).
     *   5. JSON array rỗng (folder rỗng).
     *
     * Mọi nhánh đều phải xử lý ổn định, không crash app.
     */
    @POST("api/fileops/getFolderList")
    suspend fun getFolderList(@Body body: GetFolderListRequestDto): Response<ResponseBody>
}

// ─── Requests ─────────────────────────────────────────────────────────

@JsonClass(generateAdapter = true)
data class GetFileRequestDto(
    @Json(name = "token") val token: String,
    @Json(name = "url") val url: String,
)

@JsonClass(generateAdapter = true)
data class GetFolderListRequestDto(
    @Json(name = "token") val token: String,
    @Json(name = "url") val url: String,
    @Json(name = "dirOnly") val dirOnly: Int = 0,
    @Json(name = "pageIndex") val pageIndex: Int = 0,
    @Json(name = "limit") val limit: Int = 100,
)

// ─── Responses ────────────────────────────────────────────────────────

/**
 * Response /api/fileops/get.
 *
 * Server trả flat hoặc nested. Tất cả field optional; Repository tự chọn fallback.
 */
@JsonClass(generateAdapter = true)
data class GetFileResponseDto(
    @Json(name = "code") @IntOrString override val code: String? = null,
    @Json(name = "msg") override val msg: String? = null,

    // Flat
    @Json(name = "name") val name: String? = null,
    @Json(name = "filename") val filename: String? = null,
    @Json(name = "file_name") val fileName: String? = null,
    @Json(name = "size") val size: Long? = null,
    @Json(name = "file_size") val fileSize: Long? = null,
    @Json(name = "filesize") val filesize: Long? = null,
    @Json(name = "human_size") val humanSize: String? = null,
    @Json(name = "linkcode") val linkcode: String? = null,
    @Json(name = "url") val url: String? = null,
    @Json(name = "thumbnail") val thumbnail: String? = null,
    @Json(name = "description") val description: String? = null,
    @Json(name = "mime_type") val mimeType: String? = null,
    @Json(name = "duration_ms") val durationMs: Long? = null,
    @Json(name = "is_password_protected") val isPasswordProtected: Boolean? = null,

    // Nested fallback
    @Json(name = "data") val data: GetFileResponseDto? = null,
    @Json(name = "file") val file: GetFileResponseDto? = null,

    // Error fallback
    @Json(name = "message") val message: String? = null,
    @Json(name = "error") val error: String? = null,
    @Json(name = "error_msg") val errorMsg: String? = null,
) : LegacyResponse

/**
 * Plain Kotlin model cho item folder list — KHÔNG annotate Moshi để tránh KSP gen.
 * Repository parse manually từ JSON Reader.
 */
data class FolderListItem(
    val name: String? = null,
    val linkcode: String? = null,
    /** "0" = folder, "1" = file. */
    val type: String? = null,
    /** size as String để xử lý cả Long và String từ server. */
    val size: String? = null,
    val id: String? = null,
    val thumbnail: String? = null,
    val description: String? = null,
)

/** Kết quả parse cho /api/fileops/getFolderList. */
data class FolderListParseResult(
    /** Code từ body nếu có (object response); null nếu là array thuần. */
    val code: String? = null,
    val msg: String? = null,
    val items: List<FolderListItem>,
)
