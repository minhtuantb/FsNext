/*
 * StreamIX TV — Timfshare API (đối tác search).
 *
 * Hai endpoint trên 2 host khác nhau:
 *  - api.timfshare.com/v1/string-query-search?query=K  (search; cần Bearer + Chrome UA)
 *  - timfshare.com/api/key/data-top                    (trending; chỉ Chrome UA)
 *
 * Tách thành 2 interface riêng để Retrofit/Hilt cấu hình OkHttp client riêng cho mỗi host.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §4.
 */
package vn.streamix.tv.core.network.service

import com.squareup.moshi.Json
import com.squareup.moshi.JsonClass
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.POST
import retrofit2.http.Query

interface TimfshareApi {

    /**
     * Search video theo từ khoá.
     *
     * Request: empty body (Content-Type: application/json giữ default).
     * Response: `{ "data": [ {...}, ... ] }` (object — KHÔNG phải LegacyResponse).
     *
     * Lưu ý:
     *  - Keyword phải URL-encode. StreamIX desktop replace `.` bằng space trước khi encode.
     *  - Filter client-side: chỉ giữ entry có `name` kết thúc bằng video extension.
     */
    @POST("v1/string-query-search")
    suspend fun search(
        @Query("query") query: String,
    ): Response<TimfshareSearchResponseDto>
}

interface TimfshareTrendingApi {

    /**
     * Trending video. KHÔNG cần Bearer.
     * Response: `{ "dataFile": [ {...}, ... ] }`.
     */
    @GET("api/key/data-top")
    suspend fun trending(): Response<TimfshareTrendingResponseDto>
}

// ─── DTOs ─────────────────────────────────────────────────────────────

@JsonClass(generateAdapter = true)
data class TimfshareSearchResponseDto(
    @Json(name = "data") val data: List<TimfshareSearchItemDto>? = null,
)

@JsonClass(generateAdapter = true)
data class TimfshareSearchItemDto(
    @Json(name = "id") val id: Long? = null,
    @Json(name = "name") val name: String? = null,
    @Json(name = "url") val url: String? = null,
    /** "0" = không phát được, khác "0" = playable. */
    @Json(name = "file_type") val fileType: String? = null,
    /** Có thể là Long (raw bytes) hoặc String. Backend không nhất quán. */
    @Json(name = "size") val size: Any? = null,
    @Json(name = "size_display") val sizeDisplay: String? = null,
)

@JsonClass(generateAdapter = true)
data class TimfshareTrendingResponseDto(
    @Json(name = "dataFile") val dataFile: List<TimfshareTrendingItemDto>? = null,
)

@JsonClass(generateAdapter = true)
data class TimfshareTrendingItemDto(
    @Json(name = "id") val id: Long? = null,
    @Json(name = "name") val name: String? = null,
    @Json(name = "linkcode") val linkcode: String? = null,
    @Json(name = "file_extension") val fileExtension: String? = null,
    @Json(name = "size") val size: Long? = null,
    @Json(name = "size_display") val sizeDisplay: String? = null,
)

// ─── Helpers ──────────────────────────────────────────────────────────

/**
 * Whitelist video extension giống StreamIX desktop (.mp4 .avi .mov .mkv .m4v .flv .mpeg .wav).
 * Trả false nếu name không có extension hợp lệ.
 */
fun isVideoFileName(name: String?): Boolean {
    if (name.isNullOrBlank()) return false
    val lower = name.lowercase()
    return VIDEO_EXTENSIONS.any { lower.endsWith(it) }
}

/** Cắt query string khỏi URL (giữ phần trước `?`). */
fun stripUrlQuery(url: String?): String? {
    if (url.isNullOrBlank()) return null
    val q = url.indexOf('?')
    return if (q < 0) url else url.substring(0, q)
}

private val VIDEO_EXTENSIONS = listOf(
    ".mp4", ".avi", ".mov", ".mkv", ".m4v", ".flv", ".mpeg", ".mpg", ".wav", ".webm", ".ts",
)
