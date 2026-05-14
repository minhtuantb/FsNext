/*
 * StreamIX TV — SearchRepository (Timfshare partner).
 *
 * Sanitize input theo desktop:
 *  - Trim, max 256 ký tự.
 *  - Bỏ control char (0x00-0x1F trừ \t \n \r) + zero-width.
 *  - Chặn pattern XSS-like: <script, javascript:, vbscript:, <iframe, data:text/html, on<event>=
 *  - Replace `.` thành ` ` trước khi gọi API (giống StreamIX desktop).
 *
 * Cache: 5 phút theo keyword (case-insensitive).
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §4.1 + §8 (Sanitize input search).
 */
package vn.streamix.tv.core.data.repository

import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.core.domain.model.FeaturedItem
import vn.streamix.tv.core.domain.model.FeaturedSource
import vn.streamix.tv.core.domain.repository.SearchRepository
import vn.streamix.tv.core.network.service.TimfshareApi
import vn.streamix.tv.core.network.service.isVideoFileName
import vn.streamix.tv.core.network.service.stripUrlQuery
import java.io.IOException
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class SearchRepositoryImpl @Inject constructor(
    private val timfshareApi: TimfshareApi,
) : SearchRepository {

    private data class Entry(val value: List<FeaturedItem>, val expireAt: Long)

    private val mutex = Mutex()
    private val cache = LinkedHashMap<String, Entry>(MAX_CACHE_SIZE, 0.75f, true)

    override suspend fun search(query: String): ApiResult<List<FeaturedItem>> {
        val sanitized = sanitize(query) ?: return ApiResult.Success(emptyList())
        val key = sanitized.lowercase()

        // Cache hit?
        synchronized(cache) {
            val hit = cache[key]?.takeIf { it.expireAt > System.currentTimeMillis() }
            if (hit != null) return ApiResult.Success(hit.value)
        }

        return mutex.withLock {
            // Double-check
            synchronized(cache) {
                cache[key]?.takeIf { it.expireAt > System.currentTimeMillis() }
                    ?.let { return@withLock ApiResult.Success(it.value) }
            }

            try {
                val keywordForApi = sanitized.replace('.', ' ')
                val response = timfshareApi.search(keywordForApi)
                if (!response.isSuccessful) {
                    if (response.code() == 401) {
                        // Bearer token bị revoke → backend partner thay đổi.
                        return@withLock ApiResult.Error(
                            errorCode = ErrorCodes.FORBIDDEN,
                            message = "Search partner authorization failed",
                            httpStatus = 401,
                        )
                    }
                    return@withLock ApiResult.Error(
                        errorCode = response.code().toString(),
                        message = "Search failed: ${response.message()}",
                        httpStatus = response.code(),
                    )
                }
                val items = response.body()?.data.orEmpty()
                    .mapNotNull { dto ->
                        val name = dto.name ?: return@mapNotNull null
                        if (!isVideoFileName(name)) return@mapNotNull null
                        val url = stripUrlQuery(dto.url) ?: return@mapNotNull null
                        val normalized = FshareUrl.normalize(url) ?: return@mapNotNull null
                        if (!FshareUrl.isFileUrl(normalized)) return@mapNotNull null
                        val isPlayable = (dto.fileType?.let { it != "0" } ?: true)
                        val size = when (val s = dto.size) {
                            is Number -> s.toLong()
                            is String -> s.toLongOrNull()
                            else -> null
                        }
                        FeaturedItem(
                            title = name,
                            url = normalized,
                            linkcode = FshareUrl.extractLinkcode(normalized),
                            isPlayable = isPlayable,
                            size = size,
                            thumbnail = null,
                            source = FeaturedSource.SEARCH,
                        )
                    }

                synchronized(cache) {
                    if (cache.size >= MAX_CACHE_SIZE) {
                        // Evict eldest (LRU vì access-order=true)
                        val it = cache.entries.iterator()
                        if (it.hasNext()) {
                            it.next()
                            it.remove()
                        }
                    }
                    cache[key] = Entry(items, System.currentTimeMillis() + SEARCH_TTL_MS)
                }
                ApiResult.Success(items)
            } catch (_: IOException) {
                ApiResult.NetworkError
            } catch (t: Throwable) {
                ApiResult.Error(
                    errorCode = ErrorCodes.PARSE_ERROR,
                    message = "Search parse error: ${t.message}",
                    httpStatus = 0,
                )
            }
        }
    }

    /**
     * Sanitize keyword theo desktop StreamIX. Trả null nếu input rỗng / toàn whitespace
     * sau khi clean.
     */
    private fun sanitize(input: String): String? {
        var s = input.trim()
        if (s.isEmpty()) return null
        if (s.length > MAX_LEN) s = s.substring(0, MAX_LEN)

        // Remove control + zero-width
        s = s.filter { c ->
            val code = c.code
            !(code in 0x00..0x1F && c != '\t' && c != '\n' && c != '\r') &&
                code != 0x7F &&
                code != 0x200B && code != 0x200C && code != 0x200D && code != 0xFEFF
        }

        // Block dangerous patterns
        val lower = s.lowercase()
        for (bad in DANGEROUS_PATTERNS) {
            if (bad in lower) return null
        }

        return s.trim().takeIf { it.isNotEmpty() }
    }

    companion object {
        private const val MAX_LEN = 256
        private const val SEARCH_TTL_MS = 5L * 60 * 1000
        private const val MAX_CACHE_SIZE = 50

        private val DANGEROUS_PATTERNS = listOf(
            "<script",
            "javascript:",
            "vbscript:",
            "<iframe",
            "data:text/html",
            "onload=",
            "onerror=",
            "onclick=",
            "onmouseover=",
        )
    }
}
