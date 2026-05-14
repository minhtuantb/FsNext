/*
 * StreamIX TV — Auth session model (Fshare LEGACY API).
 *
 * KHÁC OPEN API GATEWAY:
 * - Không có access/refresh token. Login trả về `token` + `session_id`.
 * - Không có TTL chính thức ở client (server tự quyết). Khi server trả 201
 *   (HTTP hoặc body code) ⇒ session expired, user phải re-login.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.1.
 */
package vn.streamix.tv.core.domain.model

data class Session(
    /** Token Fshare — đi trong body request `{"token": "<token>"}`. */
    val token: String,
    /** Session ID — đi trong header `Cookie: session_id=<sid>`. */
    val sessionId: String,
    /** Email user — phục vụ hiển thị + ghi log analytics. */
    val email: String,
)
