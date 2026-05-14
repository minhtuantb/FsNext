/*
 * StreamIX TV — AuthRepository implementation (Fshare LEGACY).
 *
 * Login → server trả `{code, msg, token, session_id}` flat. Nếu code != 200,
 * apiCall đã chuyển sang ApiResult.Error trước khi tới đây.
 *
 * Logout → KHÔNG có endpoint server-side. Chỉ clear local AuthStore.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.1.
 */
package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.domain.model.Session
import vn.streamix.tv.core.domain.model.User
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.LoginResult
import vn.streamix.tv.core.network.envelope.apiCall
import vn.streamix.tv.core.network.service.AuthApi
import vn.streamix.tv.core.network.service.LoginRequestDto
import javax.inject.Inject
import javax.inject.Named
import javax.inject.Singleton

@Singleton
class AuthRepositoryImpl @Inject constructor(
    private val authApi: AuthApi,
    private val authStore: AuthStore,
    private val moshi: Moshi,
    @Named("appKey") private val appKey: String,
) : AuthRepository {

    override suspend fun login(email: String, password: String): ApiResult<LoginResult> {
        val result = apiCall(moshi) {
            authApi.login(LoginRequestDto(appKey = appKey, userEmail = email, password = password))
        }
        return when (result) {
            is ApiResult.Success -> {
                val data = result.data
                val token = data.token
                val sid = data.sessionId
                if (token.isNullOrBlank() || sid.isNullOrBlank()) {
                    ApiResult.Error(
                        errorCode = ErrorCodes.PARSE_ERROR,
                        message = "Login response thiếu token/session_id",
                        httpStatus = 200,
                    )
                } else {
                    val session = Session(token = token, sessionId = sid, email = email)
                    authStore.save(session)
                    // V1: Login chỉ lưu session — User profile sẽ fetch sau qua refreshMe().
                    // Tạo placeholder User minimal để giữ chữ ký LoginResult.
                    val user = User(
                        id = "",
                        email = email,
                        fullName = email.substringBefore('@'),
                        phone = null,
                        isVip = false,
                        vipLevel = 0,
                        vipExpiresAt = null,
                        storageUsed = 0,
                        storageQuota = 0,
                        createdAt = null,
                    )
                    ApiResult.Success(LoginResult(session, user), result.meta, result.requestId)
                }
            }
            is ApiResult.Error -> result
            ApiResult.NetworkError -> ApiResult.NetworkError
            ApiResult.AuthExpired -> ApiResult.AuthExpired
            is ApiResult.RateLimited -> result
        }
    }

    override suspend fun logout(): ApiResult<Unit> {
        // Fshare legacy không có endpoint /logout. Chỉ clear local.
        authStore.clear()
        return ApiResult.Success(Unit)
    }

    override suspend fun currentSession(): Session? = authStore.session()

    override fun observeSession(): Flow<Session?> = authStore.sessionFlow

    override suspend fun clearSession() = authStore.clear()
}
