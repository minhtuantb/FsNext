/*
 * StreamIX TV — Auth API (Fshare LEGACY).
 *
 * Endpoints:
 *  - POST /api/user/login — đăng nhập, trả token + session_id (flat).
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.1.
 *
 * KHÔNG có refresh / logout endpoint. Khi server trả 201 ⇒ client clear local
 * và force re-login. Logout = clear local store.
 */
package vn.streamix.tv.core.network.service

import com.squareup.moshi.Json
import com.squareup.moshi.JsonClass
import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.POST
import vn.streamix.tv.core.network.envelope.IntOrString
import vn.streamix.tv.core.network.envelope.LegacyResponse

interface AuthApi {

    @POST("api/user/login")
    suspend fun login(@Body body: LoginRequestDto): Response<LoginResponseDto>
}

// ─── DTOs ─────────────────────────────────────────────────────────────

@JsonClass(generateAdapter = true)
data class LoginRequestDto(
    @Json(name = "app_key") val appKey: String,
    @Json(name = "user_email") val userEmail: String,
    @Json(name = "password") val password: String,
)

/**
 * Response /api/user/login (flat).
 * Success body: `{"code": 200, "msg": "Login successful!", "token": "...", "session_id": "..."}`
 *
 * Error: `{"code": 405, "msg": "..."}` hoặc `{"code": "authenticate", "msg": "..."}`.
 * Đã được [IntOrString] adapter coerce về String đồng nhất.
 */
@JsonClass(generateAdapter = true)
data class LoginResponseDto(
    @Json(name = "code") @IntOrString override val code: String? = null,
    @Json(name = "msg") override val msg: String? = null,
    @Json(name = "token") val token: String? = null,
    @Json(name = "session_id") val sessionId: String? = null,
) : LegacyResponse
