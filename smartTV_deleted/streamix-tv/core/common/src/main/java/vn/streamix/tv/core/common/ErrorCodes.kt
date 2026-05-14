/*
 * StreamIX TV — Error code constants từ Fshare LEGACY API.
 *
 * Reference: D:\Work\FsNext\smartTV\streamix-tv\documents\API_REFERENCE_FOR_REUSE.md §3.1, §3.3
 *
 * KHÁC BIỆT VỚI OPEN API GATEWAY:
 * - Legacy trả `code` ở body, có thể là Int (200, 201, 405, 471) hoặc String ("authenticate", "too many").
 * - StreamIX TV chuẩn hoá tất cả về String để consume thống nhất.
 * - HTTP status `201` (mọi endpoint) = session expired ⇒ AuthExpired.
 *
 * Code app-side dùng prefix "_" để không trùng với code từ server.
 */
package vn.streamix.tv.core.common

object ErrorCodes {
    // ─── Server-side codes (numeric, chuẩn hoá thành String) ─────────

    /** Thành công — body code = 200. */
    const val OK = "200"

    /** Session/token hết hạn — HTTP 201 hoặc body code 201. App phải force re-login. */
    const val SESSION_EXPIRED = "201"

    /** File yêu cầu mật khẩu — body code 123 (chỉ ở /api/session/download). */
    const val FILE_PASSWORD_REQUIRED = "123"

    /** Email hoặc mật khẩu sai định dạng. */
    const val INVALID_INPUT = "400"

    /** App chưa được cấp phép / dùng bản giả. */
    const val APP_NOT_AUTHORIZED = "33"

    /** Thiết bị / môi trường không được hỗ trợ. */
    const val APP_DEVICE_UNSUPPORTED_37 = "37"
    const val APP_DEVICE_UNSUPPORTED_38 = "38"

    /** App key không hợp lệ — yêu cầu cập nhật app. */
    const val APP_KEY_INVALID = "39"

    /** Forbidden / Email invalid / Vip-only. */
    const val FORBIDDEN = "403"

    /** Resource not found (folder/file không tồn tại). */
    const val NOT_FOUND = "404"

    /** Login: sai email/password. Body code có thể là 405 hoặc "authenticate". */
    const val AUTH_INVALID_CREDENTIALS = "405"

    /** Login: chặn vì gõ sai quá nhiều lần. Body code = "too many". */
    const val AUTH_TOO_MANY_ATTEMPTS = "too many"

    /** Tài khoản chưa kích hoạt. */
    const val ACCOUNT_NOT_ACTIVATED = "406"

    /** Bắt buộc đổi mật khẩu (HTTP 408 + body 407). */
    const val MUST_CHANGE_PASSWORD = "407"

    /** Mật khẩu trùng số điện thoại. */
    const val PASSWORD_EQUALS_PHONE = "408"

    /** Tài khoản bị khóa. */
    const val ACCOUNT_LOCKED = "409"

    /** Domain email bị cấm. */
    const val EMAIL_DOMAIN_BANNED = "410"

    /** Bị tạm khóa do nhập sai nhiều lần (đợi 10 phút). */
    const val TEMPORARILY_LOCKED = "424"

    /** Quá nhiều phiên tải — user phải vào fshare.vn → Bảo mật → Xóa phiên. */
    const val TOO_MANY_SESSIONS = "471"

    /** App phiên bản chưa tương thích. */
    const val APP_OUTDATED = "502"

    /** Internal server error. */
    const val INTERNAL_ERROR = "500"

    /** Service unavailable (maintenance). */
    const val SERVICE_UNAVAILABLE = "503"

    // ─── App-side fallbacks (prefix "_" để không trùng server) ────────

    /** Không xác định / fallback. */
    const val UNKNOWN = "_unknown"

    /** Parse lỗi response body. */
    const val PARSE_ERROR = "_parse_error"

    /** IO error / timeout — sẽ map sang ApiResult.NetworkError. */
    const val NETWORK_ERROR = "_network_error"

    // ─── Sets phục vụ logic dispatch ──────────────────────────────────

    /**
     * Codes mà ApiCall map sang ApiResult.AuthExpired.
     * UI phải force user re-login.
     */
    val AUTH_FATAL_CODES: Set<String> = setOf(
        SESSION_EXPIRED,
        APP_KEY_INVALID,
        APP_NOT_AUTHORIZED,
    )

    /** Codes nên retry với exponential backoff. */
    val RETRYABLE_CODES: Set<String> = setOf(
        INTERNAL_ERROR,
        SERVICE_UNAVAILABLE,
    )

    /** Login error codes hiển thị message thân thiện. */
    val LOGIN_ERROR_CODES: Set<String> = setOf(
        AUTH_INVALID_CREDENTIALS,
        AUTH_TOO_MANY_ATTEMPTS,
        ACCOUNT_NOT_ACTIVATED,
        MUST_CHANGE_PASSWORD,
        PASSWORD_EQUALS_PHONE,
        ACCOUNT_LOCKED,
        EMAIL_DOMAIN_BANNED,
        TEMPORARILY_LOCKED,
    )
}
