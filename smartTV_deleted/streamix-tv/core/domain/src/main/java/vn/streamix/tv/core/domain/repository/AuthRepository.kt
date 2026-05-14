/*
 * StreamIX TV — AuthRepository interface
 */
package vn.streamix.tv.core.domain.repository

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.Session
import vn.streamix.tv.core.domain.model.User

interface AuthRepository {
    suspend fun login(email: String, password: String): ApiResult<LoginResult>
    suspend fun logout(): ApiResult<Unit>
    suspend fun currentSession(): Session?
    fun observeSession(): Flow<Session?>
    suspend fun clearSession()
}

data class LoginResult(
    val session: Session,
    val user: User,
)
