/*
 * StreamIX TV — Session API (Fshare LEGACY).
 *
 * Endpoints:
 *  - POST /api/session/download — Lấy direct stream URL (zipflag=0).
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.3.
 *
 * URL Fshare GỬI vào endpoint phải có `?share=8805984` (referral) — Repository tự chèn.
 *
 * Response success: `{"code": 200, "location": "https://download.fshare.vn/.../stream-key"}`
 * Lỗi đáng chú ý:
 *   200 + code 123 → File yêu cầu mật khẩu
 *   201            → Session expired
 *   404            → Link không tồn tại
 *   471            → Quá nhiều phiên tải
 */
package vn.streamix.tv.core.network.service

import com.squareup.moshi.Json
import com.squareup.moshi.JsonClass
import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.POST
import vn.streamix.tv.core.network.envelope.IntOrString
import vn.streamix.tv.core.network.envelope.LegacyResponse

interface SessionApi {

    @POST("api/session/download")
    suspend fun createDownloadSession(
        @Body body: SessionDownloadRequestDto,
    ): Response<SessionDownloadResponseDto>
}

@JsonClass(generateAdapter = true)
data class SessionDownloadRequestDto(
    /** 0 = single file, 1 = zip multiple. */
    @Json(name = "zipflag") val zipflag: Int = 0,
    /** Full Fshare URL — phải có `?share=<referral_id>`. */
    @Json(name = "url") val url: String,
    /** Mật khẩu file (rỗng nếu không có). */
    @Json(name = "password") val password: String = "",
    /** Token user (từ AuthStore.token()). */
    @Json(name = "token") val token: String,
)

@JsonClass(generateAdapter = true)
data class SessionDownloadResponseDto(
    @Json(name = "code") @IntOrString override val code: String? = null,
    @Json(name = "msg") override val msg: String? = null,
    /** Direct stream URL — chỉ có khi success. */
    @Json(name = "location") val location: String? = null,
    /** Một số endpoint cũ trả `url` thay `location`. */
    @Json(name = "url") val url: String? = null,
) : LegacyResponse
