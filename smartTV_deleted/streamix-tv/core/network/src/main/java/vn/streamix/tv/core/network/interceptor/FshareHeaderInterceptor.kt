/*
 * StreamIX TV — FshareHeaderInterceptor: chèn User-Agent + cache-control + X-Request-Id.
 *
 * KHÁC OPEN API GATEWAY:
 * - KHÔNG có `X-App-Key` header (app_key đi trong body request /api/user/login).
 * - User-Agent BẮT BUỘC là "FshareVideoDesktop_23052023" — sai sẽ trả "Invalid User Agent!".
 * - cache-control: no-cache (bắt chước desktop client gốc).
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3 (Nguyên tắc chung) + §3.1.
 */
package vn.streamix.tv.core.network.interceptor

import okhttp3.Interceptor
import okhttp3.Response
import java.util.UUID
import javax.inject.Inject
import javax.inject.Named
import javax.inject.Singleton

@Singleton
class FshareHeaderInterceptor @Inject constructor(
    @Named("userAgent") private val userAgent: String,
) : Interceptor {

    override fun intercept(chain: Interceptor.Chain): Response {
        val req = chain.request()
        val builder = req.newBuilder()
            // Chỉ override nếu request không tự đặt UA riêng
            // (cần thiết cho Timfshare client dùng UA Chrome khác).
            .apply {
                if (req.header("User-Agent") == null) {
                    header("User-Agent", userAgent)
                }
            }
            .header("cache-control", "no-cache")
            .header("Accept", "application/json")

        // Gen X-Request-Id nếu request chưa có (chỉ phục vụ tracing nội bộ).
        if (req.header("X-Request-Id") == null) {
            builder.header("X-Request-Id", generateRequestId())
        }
        return chain.proceed(builder.build())
    }

    /** Sinh ULID-like ID đơn giản — không cần crypto-strength. */
    private fun generateRequestId(): String {
        val ts = System.currentTimeMillis().toString(36).uppercase().padStart(10, '0')
        val rand = UUID.randomUUID().toString().replace("-", "").take(16).uppercase()
        return (ts + rand).take(26)
    }
}
