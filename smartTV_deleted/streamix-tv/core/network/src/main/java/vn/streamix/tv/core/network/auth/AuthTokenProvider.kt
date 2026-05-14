/*
 * StreamIX TV — Token + session provider interface (data layer implements).
 *
 * Lý do tách interface ở :core:network thay vì để :core:data trực tiếp:
 *   network module không phụ thuộc data module → tránh circular dep.
 *   data module sẽ implement interface này (AuthStore) và bind qua Hilt.
 *
 * Khác Open API Gateway (chỉ có JWT access + refresh):
 *   Fshare legacy có 2 thứ riêng biệt:
 *     - `token` (dài, được gửi trong request body field "token") — dùng trong body.
 *     - `session_id` (cookie value) — dùng trong header `Cookie: session_id=<sid>`.
 *
 * Cả 2 đều phải có. KHÔNG có refresh — khi server trả 201 ⇒ user re-login.
 */
package vn.streamix.tv.core.network.auth

interface AuthTokenProvider {
    /** Token Fshare để inject vào body request (`{"token": "<token>"}`). Null nếu chưa login. */
    suspend fun token(): String?

    /** Session ID Fshare để inject vào cookie (`session_id=<sid>`). Null nếu chưa login. */
    suspend fun sessionId(): String?

    /** Lưu credentials sau khi login thành công. */
    suspend fun saveCredentials(token: String, sessionId: String, email: String)

    /** Xoá credentials — khi logout hoặc session expired. */
    suspend fun clear()
}
