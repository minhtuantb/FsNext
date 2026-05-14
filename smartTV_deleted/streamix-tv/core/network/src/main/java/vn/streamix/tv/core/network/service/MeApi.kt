/*
 * StreamIX TV — Me API (Fshare LEGACY).
 *
 * Endpoints:
 *  - GET /api/user/get — lấy thông tin user hiện tại (level VIP, email, quota).
 *
 * Response: flat hoặc nested trong `data`/`user`. StreamIX desktop đọc theo
 * thứ tự ưu tiên fallback. Cách tiếp cận trên Android: dùng nested optional
 * fields + `firstNonNull` trong Repository.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.2.
 */
package vn.streamix.tv.core.network.service

import com.squareup.moshi.Json
import com.squareup.moshi.JsonClass
import retrofit2.Response
import retrofit2.http.GET
import vn.streamix.tv.core.network.envelope.IntOrString
import vn.streamix.tv.core.network.envelope.LegacyResponse

interface MeApi {
    @GET("api/user/get")
    suspend fun getMe(): Response<MeResponseDto>
}

/**
 * Response `/api/user/get`.
 *
 * Server có thể trả flat (top-level) hoặc nested trong `data` / `user`.
 * Tất cả khoá đọc đều là optional + fallback.
 *
 * Cấp VIP đọc theo ưu tiên: `level` → `user_level` → `account_level`.
 * Email: `email` → `user_email` → `login`.
 * Bytes: `webspace_used` → `storage_used` → `used` → `used_bytes` → `traffic_used`.
 */
@JsonClass(generateAdapter = true)
data class MeResponseDto(
    @Json(name = "code") @IntOrString override val code: String? = null,
    @Json(name = "msg") override val msg: String? = null,

    // Flat — top-level
    @Json(name = "level") val level: Int? = null,
    @Json(name = "user_level") val userLevel: Int? = null,
    @Json(name = "account_level") val accountLevel: Int? = null,
    @Json(name = "email") val email: String? = null,
    @Json(name = "user_email") val userEmail: String? = null,
    @Json(name = "login") val login: String? = null,
    @Json(name = "user_id") val userId: String? = null,
    @Json(name = "userid") val userIdAlt: String? = null,
    @Json(name = "id") val idAlt: String? = null,
    @Json(name = "name") val name: String? = null,
    @Json(name = "full_name") val fullName: String? = null,
    @Json(name = "phone") val phone: String? = null,
    @Json(name = "expire_vip") val expireVip: String? = null,

    // Bytes
    @Json(name = "webspace_used") val webspaceUsed: Long? = null,
    @Json(name = "storage_used") val storageUsed: Long? = null,
    @Json(name = "used") val used: Long? = null,
    @Json(name = "used_bytes") val usedBytes: Long? = null,
    @Json(name = "traffic_used") val trafficUsed: Long? = null,

    @Json(name = "webspace_remaining") val webspaceRemaining: Long? = null,
    @Json(name = "storage_remaining") val storageRemaining: Long? = null,
    @Json(name = "remaining") val remaining: Long? = null,
    @Json(name = "remain") val remain: Long? = null,
    @Json(name = "remain_bytes") val remainBytes: Long? = null,
    @Json(name = "traffic_remaining") val trafficRemaining: Long? = null,

    @Json(name = "webspace") val webspace: Long? = null,
    @Json(name = "storage_total") val storageTotal: Long? = null,
    @Json(name = "quota") val quota: Long? = null,
    @Json(name = "quota_bytes") val quotaBytes: Long? = null,
    @Json(name = "traffic") val traffic: Long? = null,
    @Json(name = "storage_limit") val storageLimit: Long? = null,

    // Nested fallback (rất nhiều endpoint Fshare wrap kết quả vào `data` hoặc `user`)
    @Json(name = "data") val data: MeResponseDto? = null,
    @Json(name = "user") val user: MeResponseDto? = null,
) : LegacyResponse
