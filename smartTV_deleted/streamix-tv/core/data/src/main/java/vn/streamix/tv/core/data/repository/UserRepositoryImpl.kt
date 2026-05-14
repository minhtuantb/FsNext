/*
 * StreamIX TV — UserRepository (Fshare LEGACY).
 *
 * /api/user/get response: flat hoặc nested trong `data` / `user`.
 * Strategy: gọi merge() để compose top-level + nested fallback rồi đọc ưu tiên
 * theo bảng từ desktop StreamIX (xem API_REFERENCE_FOR_REUSE.md §3.2).
 */
package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.map
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.data.local.room.dao.UserCacheDao
import vn.streamix.tv.core.data.local.room.entity.UserCacheEntity
import vn.streamix.tv.core.domain.model.User
import vn.streamix.tv.core.domain.repository.UserRepository
import vn.streamix.tv.core.network.envelope.apiCall
import vn.streamix.tv.core.network.service.MeApi
import vn.streamix.tv.core.network.service.MeResponseDto
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class UserRepositoryImpl @Inject constructor(
    private val meApi: MeApi,
    private val userCacheDao: UserCacheDao,
    private val authStore: AuthStore,
    private val moshi: Moshi,
) : UserRepository {

    override fun observeMe(): Flow<User?> = userCacheDao.observe().map { it?.toDomain() }

    override suspend fun refreshMe(): ApiResult<User> {
        val result = apiCall(moshi) { meApi.getMe() }
        if (result is ApiResult.Success) {
            val emailHint = authStore.session()?.email
            val merged = result.data.flatten()
            val entity = merged.toEntity(fallbackEmail = emailHint.orEmpty())
            userCacheDao.upsert(entity)
            return ApiResult.Success(entity.toDomain(), result.meta, result.requestId)
        }
        return result.map { it.flatten().toEntity(fallbackEmail = "").toDomain() }
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────

/**
 * Merge top-level + `data` + `user` thành 1 DTO duy nhất bằng cách lấy non-null trước.
 * Lưu ý: nested có thể chính nó còn nest. Đệ quy 1 cấp đủ cho hầu hết trường hợp.
 */
private fun MeResponseDto.flatten(): MeResponseDto {
    val nested = (data ?: user)?.flatten()
    if (nested == null) return this
    return MeResponseDto(
        code = code ?: nested.code,
        msg = msg ?: nested.msg,
        level = level ?: nested.level,
        userLevel = userLevel ?: nested.userLevel,
        accountLevel = accountLevel ?: nested.accountLevel,
        email = email ?: nested.email,
        userEmail = userEmail ?: nested.userEmail,
        login = login ?: nested.login,
        userId = userId ?: nested.userId,
        userIdAlt = userIdAlt ?: nested.userIdAlt,
        idAlt = idAlt ?: nested.idAlt,
        name = name ?: nested.name,
        fullName = fullName ?: nested.fullName,
        phone = phone ?: nested.phone,
        expireVip = expireVip ?: nested.expireVip,
        webspaceUsed = webspaceUsed ?: nested.webspaceUsed,
        storageUsed = storageUsed ?: nested.storageUsed,
        used = used ?: nested.used,
        usedBytes = usedBytes ?: nested.usedBytes,
        trafficUsed = trafficUsed ?: nested.trafficUsed,
        webspaceRemaining = webspaceRemaining ?: nested.webspaceRemaining,
        storageRemaining = storageRemaining ?: nested.storageRemaining,
        remaining = remaining ?: nested.remaining,
        remain = remain ?: nested.remain,
        remainBytes = remainBytes ?: nested.remainBytes,
        trafficRemaining = trafficRemaining ?: nested.trafficRemaining,
        webspace = webspace ?: nested.webspace,
        storageTotal = storageTotal ?: nested.storageTotal,
        quota = quota ?: nested.quota,
        quotaBytes = quotaBytes ?: nested.quotaBytes,
        traffic = traffic ?: nested.traffic,
        storageLimit = storageLimit ?: nested.storageLimit,
        data = null,
        user = null,
    )
}

private fun MeResponseDto.toEntity(fallbackEmail: String): UserCacheEntity {
    val effectiveEmail = email ?: userEmail ?: login ?: fallbackEmail
    val effectiveLevel = level ?: userLevel ?: accountLevel ?: 0
    val storageUsedBytes = webspaceUsed ?: storageUsed ?: used ?: usedBytes ?: trafficUsed ?: 0L
    val storageQuotaBytes = webspace ?: storageTotal ?: quota ?: quotaBytes ?: traffic ?: storageLimit ?: 0L
    val effectiveId = userId ?: userIdAlt ?: idAlt ?: ""
    val effectiveName = fullName ?: name ?: effectiveEmail.substringBefore('@')

    return UserCacheEntity(
        id = effectiveId.ifBlank { effectiveEmail },  // PK fallback theo email
        email = effectiveEmail,
        fullName = effectiveName,
        phone = phone,
        isVip = effectiveLevel >= 1,
        vipLevel = effectiveLevel,
        vipExpiresAt = expireVip,
        storageUsed = storageUsedBytes,
        storageQuota = storageQuotaBytes,
        createdAt = null,
        cachedAt = System.currentTimeMillis(),
    )
}

private fun UserCacheEntity.toDomain() = User(
    id = id, email = email, fullName = fullName, phone = phone,
    isVip = isVip, vipLevel = vipLevel, vipExpiresAt = vipExpiresAt,
    storageUsed = storageUsed, storageQuota = storageQuota, createdAt = createdAt,
)
