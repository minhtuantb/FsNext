/*
 * StreamIX TV — Player & Playback repositories (Fshare LEGACY).
 *
 * fetchStreamUrl: POST /api/session/download body `{zipflag:0, url, password, token}`.
 *  - URL phải có `?share=<referral_id>` (StreamIX desktop luôn chèn để giữ partner program).
 *  - Trả {code:200, location: "..."} ⇒ Success.
 *  - 200 + code:123 ⇒ FILE_PASSWORD_REQUIRED (UI hiện dialog nhập password).
 *  - 471 ⇒ TOO_MANY_SESSIONS.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.3.
 */
package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.data.local.room.dao.PlaybackPositionDao
import vn.streamix.tv.core.data.local.room.entity.PlaybackPositionEntity
import vn.streamix.tv.core.domain.model.PlaybackPosition
import vn.streamix.tv.core.domain.model.StreamUrl
import vn.streamix.tv.core.domain.repository.PlaybackPositionRepository
import vn.streamix.tv.core.domain.repository.PlayerRepository
import vn.streamix.tv.core.network.envelope.apiCall
import vn.streamix.tv.core.network.service.SessionApi
import vn.streamix.tv.core.network.service.SessionDownloadRequestDto
import javax.inject.Inject
import javax.inject.Named
import javax.inject.Singleton

@Singleton
class PlayerRepositoryImpl @Inject constructor(
    private val sessionApi: SessionApi,
    private val authStore: AuthStore,
    private val moshi: Moshi,
    @Named("shareReferralId") private val shareReferralId: String,
) : PlayerRepository {

    override suspend fun fetchStreamUrl(linkcode: String, password: String?): ApiResult<StreamUrl> {
        val token = authStore.token() ?: return ApiResult.AuthExpired
        val baseUrl = "https://www.fshare.vn/file/$linkcode"
        val urlWithReferral = appendReferral(baseUrl, shareReferralId)

        val result = apiCall(moshi) {
            sessionApi.createDownloadSession(
                SessionDownloadRequestDto(
                    zipflag = 0,
                    url = urlWithReferral,
                    password = password.orEmpty(),
                    token = token,
                ),
            )
        }
        return when (result) {
            is ApiResult.Success -> {
                val location = result.data.location ?: result.data.url
                if (location.isNullOrBlank()) {
                    ApiResult.Error(
                        errorCode = ErrorCodes.PARSE_ERROR,
                        message = "session/download response thiếu `location`",
                        httpStatus = 200,
                    )
                } else {
                    // Legacy KHÔNG trả expiresIn — dùng heuristic: stream URL Fshare typically
                    // valid ~12 giờ. App có thể refresh khi cần.
                    val ttlSec = DEFAULT_STREAM_TTL_SEC
                    ApiResult.Success(
                        data = StreamUrl(
                            url = location,
                            expiresInSeconds = ttlSec,
                            expiresAt = System.currentTimeMillis() + ttlSec * 1_000,
                        ),
                        meta = result.meta,
                        requestId = result.requestId,
                    )
                }
            }
            is ApiResult.Error -> result
            ApiResult.NetworkError -> ApiResult.NetworkError
            ApiResult.AuthExpired -> ApiResult.AuthExpired
            is ApiResult.RateLimited -> result
        }
    }

    private fun appendReferral(url: String, referralId: String): String {
        if (referralId.isBlank()) return url
        return if (url.contains('?')) "$url&share=$referralId" else "$url?share=$referralId"
    }

    companion object {
        /** Default TTL nếu server không trả expiry (legacy không có field này). */
        private const val DEFAULT_STREAM_TTL_SEC = 12L * 3600  // 12 giờ
    }
}

@Singleton
class PlaybackPositionRepositoryImpl @Inject constructor(
    private val dao: PlaybackPositionDao,
) : PlaybackPositionRepository {

    override fun observe(linkcode: String): Flow<PlaybackPosition?> =
        dao.observe(linkcode).map { it?.toDomain() }

    override fun observeContinueWatching(limit: Int): Flow<List<PlaybackPosition>> =
        dao.observeContinueWatching(limit).map { list -> list.map { it.toDomain() } }

    override suspend fun upsert(position: PlaybackPosition) {
        dao.upsert(position.toEntity())
    }

    override suspend fun delete(linkcode: String) {
        dao.delete(linkcode)
    }
}

private fun PlaybackPositionEntity.toDomain() = PlaybackPosition(
    linkcode = linkcode, positionMs = positionMs, durationMs = durationMs,
    updatedAt = updatedAt, fileName = fileName, thumbnail = thumbnail,
)

private fun PlaybackPosition.toEntity() = PlaybackPositionEntity(
    linkcode = linkcode, positionMs = positionMs, durationMs = durationMs,
    updatedAt = updatedAt, fileName = fileName, thumbnail = thumbnail,
)
