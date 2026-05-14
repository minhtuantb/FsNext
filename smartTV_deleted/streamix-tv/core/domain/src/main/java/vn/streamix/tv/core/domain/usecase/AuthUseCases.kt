/*
 * StreamIX TV — Auth use cases
 * Reference: 02 §2.2 (UseCase layer)
 */
package vn.streamix.tv.core.domain.usecase

import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.LoginResult
import javax.inject.Inject

class LoginUseCase @Inject constructor(
    private val repo: AuthRepository,
) {
    suspend operator fun invoke(email: String, password: String): ApiResult<LoginResult> =
        repo.login(email.trim(), password)
}

class LogoutUseCase @Inject constructor(
    private val repo: AuthRepository,
) {
    suspend operator fun invoke(): ApiResult<Unit> = repo.logout()
}
