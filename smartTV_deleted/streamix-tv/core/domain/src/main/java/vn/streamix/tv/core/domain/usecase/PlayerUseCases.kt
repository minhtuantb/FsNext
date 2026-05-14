/*
 * StreamIX TV — Player use cases
 */
package vn.streamix.tv.core.domain.usecase

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.PlaybackPosition
import vn.streamix.tv.core.domain.model.StreamUrl
import vn.streamix.tv.core.domain.repository.PlaybackPositionRepository
import vn.streamix.tv.core.domain.repository.PlayerRepository
import javax.inject.Inject

class GetStreamUrlUseCase @Inject constructor(
    private val playerRepo: PlayerRepository,
) {
    suspend operator fun invoke(linkcode: String, password: String? = null): ApiResult<StreamUrl> =
        playerRepo.fetchStreamUrl(linkcode, password)
}

class ObservePlaybackPositionUseCase @Inject constructor(
    private val repo: PlaybackPositionRepository,
) {
    operator fun invoke(linkcode: String): Flow<PlaybackPosition?> = repo.observe(linkcode)
}

class ObserveContinueWatchingUseCase @Inject constructor(
    private val repo: PlaybackPositionRepository,
) {
    operator fun invoke(limit: Int = 20): Flow<List<PlaybackPosition>> =
        repo.observeContinueWatching(limit)
}

class SavePlaybackPositionUseCase @Inject constructor(
    private val repo: PlaybackPositionRepository,
) {
    suspend operator fun invoke(position: PlaybackPosition) = repo.upsert(position)
}
