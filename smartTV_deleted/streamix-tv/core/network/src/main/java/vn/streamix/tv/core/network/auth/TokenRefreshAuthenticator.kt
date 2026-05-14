/*
 * StreamIX TV — DEPRECATED.
 *
 * TokenRefreshAuthenticator đã bị bỏ trong Wave M1 (migrate sang Legacy Fshare API).
 *
 * Legacy Fshare API KHÔNG có refresh token mechanism. Khi server trả HTTP 201 hoặc
 * body code = "201" ⇒ user phải re-login. Apparat dispatch nằm ở [apiCall] +
 * UI handler cho `ApiResult.AuthExpired` (D7 SessionExpired dialog).
 *
 * File này giữ rỗng để build không fail; sẽ xoá thực sự ở wave cleanup tiếp theo.
 */
package vn.streamix.tv.core.network.auth
