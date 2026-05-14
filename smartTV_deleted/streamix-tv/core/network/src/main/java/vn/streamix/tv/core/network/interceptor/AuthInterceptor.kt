/*
 * StreamIX TV — AuthInterceptor: chèn Cookie session_id cho endpoint cần auth.
 *
 * KHÁC OPEN API GATEWAY (Bearer JWT):
 * - Fshare legacy dùng `Cookie: session_id=<sid>` — KHÔNG dùng Authorization header.
 * - `token` Fshare đi trong BODY request (do từng API handler tự inject), KHÔNG đi qua interceptor.
 *
 * Public path (KHÔNG inject cookie):
 *   /api/user/login
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3 + §3.1.
 */
package vn.streamix.tv.core.network.interceptor

import kotlinx.coroutines.runBlocking
import okhttp3.Interceptor
import okhttp3.Response
import vn.streamix.tv.core.network.auth.AuthTokenProvider
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class AuthInterceptor @Inject constructor(
    private val tokenProvider: AuthTokenProvider,
) : Interceptor {

    override fun intercept(chain: Interceptor.Chain): Response {
        val original = chain.request()
        val path = original.url.encodedPath
        if (PUBLIC_PATHS.any { path.endsWith(it) }) return chain.proceed(original)

        // Một số request (Timfshare/Sheets) chạy qua client riêng — đã có UA & token tự,
        // không nên inject session_id của Fshare. Filter theo host.
        val host = original.url.host
        if (!FSHARE_HOSTS.any { host.endsWith(it) }) return chain.proceed(original)

        val sessionId = runBlocking { tokenProvider.sessionId() }
        val req = if (sessionId != null) {
            // Append vào cookie hiện hữu (nếu có) thay vì overwrite.
            val existingCookie = original.header("Cookie")
            val newCookie = if (existingCookie.isNullOrBlank()) {
                "session_id=$sessionId"
            } else {
                "$existingCookie; session_id=$sessionId"
            }
            original.newBuilder().header("Cookie", newCookie).build()
        } else {
            original
        }
        return chain.proceed(req)
    }

    companion object {
        /** Path không cần auth — login. */
        private val PUBLIC_PATHS = listOf("/api/user/login")

        /** Hosts mà interceptor inject Cookie cho. */
        private val FSHARE_HOSTS = listOf("api.fshare.vn", "fshare.vn")
    }
}
