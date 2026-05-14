/*
 * StreamIX TV — TopRepository.
 *
 * Cung cấp danh sách Top 100 phim theo source ranking (IMDB/RT/Fshare).
 */
package vn.streamix.tv.core.domain.repository

import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.TopMovie
import vn.streamix.tv.core.domain.model.TopSource

interface TopRepository {
    /**
     * Trả Top 100 theo source. Cache 1h.
     */
    suspend fun top(source: TopSource = TopSource.IMDB): ApiResult<List<TopMovie>>
}
