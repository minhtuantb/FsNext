/*
 * StreamIX TV — FeaturedRepository (Sheets + Timfshare trending) + SearchRepository.
 *
 * - FeaturedRepository: cung cấp danh sách "Gợi ý" + "Xu hướng" cho Home rows.
 *   Source: Google Sheets CSV. Trending có thể fallback sang Timfshare /data-top
 *   nếu sheet rỗng.
 *
 * - SearchRepository: tìm kiếm theo từ khoá qua Timfshare partner.
 */
package vn.streamix.tv.core.domain.repository

import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FeaturedItem

interface FeaturedRepository {
    /** "Gợi ý" — sheet biên tập thủ công, ổn định. */
    suspend fun suggested(): ApiResult<List<FeaturedItem>>

    /** "Xu hướng" — sheet biên tập + fallback Timfshare trending. */
    suspend fun trending(): ApiResult<List<FeaturedItem>>
}

interface SearchRepository {
    /**
     * Tìm video qua Timfshare. Empty query trả empty list (không call network).
     * Sanitize input (trim, max 256 ký tự, bỏ control char) trước khi gọi.
     */
    suspend fun search(query: String): ApiResult<List<FeaturedItem>>
}
