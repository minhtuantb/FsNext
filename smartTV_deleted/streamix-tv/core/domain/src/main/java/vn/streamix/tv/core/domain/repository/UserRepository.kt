package vn.streamix.tv.core.domain.repository

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.User

interface UserRepository {
    /** Fetch user info từ API + cache vào Room. Trả Flow để UI tự update khi cache đổi. */
    fun observeMe(): Flow<User?>
    suspend fun refreshMe(): ApiResult<User>
}
