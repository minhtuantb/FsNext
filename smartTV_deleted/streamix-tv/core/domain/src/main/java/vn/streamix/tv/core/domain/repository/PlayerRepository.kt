/*
 * StreamIX TV — Player & playback persistence
 * Reference: 13 §10.0.4 + §14
 */
package vn.streamix.tv.core.domain.repository

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.PlaybackPosition
import vn.streamix.tv.core.domain.model.StreamUrl

interface PlayerRepository {
    /**
     * API-10: POST /api/v1/sessions/download
     *
     * ⚠️ Endpoint chưa được document trong API gateway (tag "Session" có nhưng path TODO).
     * Backend phải ship `v1/session.md` trước Phase 3. Tham chiếu 15 §B1.
     *
     * Implementation hiện tại trong PlayerRepositoryImpl trả mock cho dev environment.
     */
    suspend fun fetchStreamUrl(linkcode: String, password: String? = null): ApiResult<StreamUrl>
}

interface PlaybackPositionRepository {
    fun observe(linkcode: String): Flow<PlaybackPosition?>
    fun observeContinueWatching(limit: Int = 20): Flow<List<PlaybackPosition>>
    suspend fun upsert(position: PlaybackPosition)
    suspend fun delete(linkcode: String)
}
