/*
 * StreamIX TV — FeaturedRepository implementation.
 *
 * Source ưu tiên (cho mỗi row Home):
 *  - Suggested: Sheet "Gợi ý" (Sheets API).
 *  - Trending: Sheet "Xu hướng" → fallback Timfshare /data-top nếu sheet trống.
 *
 * In-memory cache: TTL 30min cho suggested, 15min cho trending (theo desktop StreamIX
 * `kSuggestCacheTtlMs` ~30min). Cache invalidate khi user kéo refresh hoặc app restart.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §4.2 + §5.
 */
package vn.streamix.tv.core.data.repository

import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.CsvParser
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.core.domain.model.FeaturedItem
import vn.streamix.tv.core.domain.model.FeaturedSource
import vn.streamix.tv.core.domain.repository.FeaturedRepository
import vn.streamix.tv.core.network.service.SheetsApi
import vn.streamix.tv.core.network.service.TimfshareTrendingApi
import vn.streamix.tv.core.network.service.isVideoFileName
import java.io.IOException
import javax.inject.Inject
import javax.inject.Named
import javax.inject.Singleton

@Singleton
class FeaturedRepositoryImpl @Inject constructor(
    private val sheetsApi: SheetsApi,
    private val timfshareTrendingApi: TimfshareTrendingApi,
    @Named("sheetGoiyId") private val goiyFileId: String,
    @Named("sheetGoiyGid") private val goiyGid: String,
    @Named("sheetXuhuongId") private val xuhuongFileId: String,
    @Named("sheetXuhuongGid") private val xuhuongGid: String,
) : FeaturedRepository {

    private data class Cache<T>(val value: T, val expireAt: Long)

    private val mutex = Mutex()

    @Volatile
    private var suggestedCache: Cache<List<FeaturedItem>>? = null

    @Volatile
    private var trendingCache: Cache<List<FeaturedItem>>? = null

    override suspend fun suggested(): ApiResult<List<FeaturedItem>> {
        suggestedCache?.takeIf { it.expireAt > System.currentTimeMillis() }
            ?.let { return ApiResult.Success(it.value) }
        return mutex.withLock {
            // Double-check sau khi lấy lock
            suggestedCache?.takeIf { it.expireAt > System.currentTimeMillis() }
                ?.let { return@withLock ApiResult.Success(it.value) }

            val result = fetchSheet(goiyFileId, goiyGid, FeaturedSource.SUGGESTED)
            if (result is ApiResult.Success) {
                suggestedCache = Cache(result.data, System.currentTimeMillis() + SUGGESTED_TTL_MS)
            }
            result
        }
    }

    override suspend fun trending(): ApiResult<List<FeaturedItem>> {
        trendingCache?.takeIf { it.expireAt > System.currentTimeMillis() }
            ?.let { return ApiResult.Success(it.value) }
        return mutex.withLock {
            trendingCache?.takeIf { it.expireAt > System.currentTimeMillis() }
                ?.let { return@withLock ApiResult.Success(it.value) }

            // Ưu tiên sheet, fallback Timfshare nếu sheet trống/lỗi.
            val sheetResult = fetchSheet(xuhuongFileId, xuhuongGid, FeaturedSource.TRENDING)
            val finalResult = when {
                sheetResult is ApiResult.Success && sheetResult.data.isNotEmpty() -> sheetResult
                else -> fetchTimfshareTrending().also {
                    if (it !is ApiResult.Success && sheetResult is ApiResult.Success) {
                        // Sheet trả empty nhưng Timfshare lỗi → trả empty list (không error).
                        return@withLock ApiResult.Success(emptyList())
                    }
                }
            }
            if (finalResult is ApiResult.Success) {
                trendingCache = Cache(finalResult.data, System.currentTimeMillis() + TRENDING_TTL_MS)
            }
            finalResult
        }
    }

    // ─── Internal ─────────────────────────────────────────────────────

    private suspend fun fetchSheet(
        fileId: String,
        gid: String,
        source: FeaturedSource,
    ): ApiResult<List<FeaturedItem>> = try {
        val response = sheetsApi.exportCsv(fileId = fileId, gid = gid)
        if (!response.isSuccessful) {
            ApiResult.Error(
                errorCode = response.code().toString(),
                message = "Sheet fetch failed: ${response.message()}",
                httpStatus = response.code(),
            )
        } else {
            val csv = response.body() ?: ""
            ApiResult.Success(parseSheetCsv(csv, source))
        }
    } catch (_: IOException) {
        ApiResult.NetworkError
    } catch (t: Throwable) {
        ApiResult.Error(
            errorCode = ErrorCodes.PARSE_ERROR,
            message = "Sheet parse error: ${t.message}",
            httpStatus = 0,
        )
    }

    private suspend fun fetchTimfshareTrending(): ApiResult<List<FeaturedItem>> = try {
        val response = timfshareTrendingApi.trending()
        if (!response.isSuccessful) {
            ApiResult.Error(
                errorCode = response.code().toString(),
                message = "Timfshare trending failed: ${response.message()}",
                httpStatus = response.code(),
            )
        } else {
            val items = response.body()?.dataFile.orEmpty()
                .mapNotNull { dto ->
                    val lc = dto.linkcode ?: return@mapNotNull null
                    val nm = dto.name ?: return@mapNotNull null
                    val ext = dto.fileExtension?.let { ".$it" } ?: ""
                    val fullName = if (nm.endsWith(ext, ignoreCase = true)) nm else "$nm$ext"
                    if (!isVideoFileName(fullName)) return@mapNotNull null
                    FeaturedItem(
                        title = nm,
                        url = FshareUrl.fileUrl(lc),
                        linkcode = lc,
                        isPlayable = true,
                        size = dto.size,
                        thumbnail = null,
                        source = FeaturedSource.TRENDING,
                    )
                }
            ApiResult.Success(items)
        }
    } catch (_: IOException) {
        ApiResult.NetworkError
    } catch (t: Throwable) {
        ApiResult.Error(
            errorCode = ErrorCodes.PARSE_ERROR,
            message = "Timfshare trending parse error: ${t.message}",
            httpStatus = 0,
        )
    }

    private fun parseSheetCsv(csv: String, source: FeaturedSource): List<FeaturedItem> {
        val rows = CsvParser.parse(csv)
        if (rows.isEmpty()) return emptyList()

        val items = mutableListOf<FeaturedItem>()
        var skippedHeader = false
        for (row in rows) {
            if (row.size < 2) continue
            if (!skippedHeader) {
                skippedHeader = true
                val firstCell = row.getOrNull(0)?.lowercase().orEmpty()
                val secondCell = row.getOrNull(1)?.lowercase().orEmpty()
                if ("tên" in firstCell || "link" in secondCell || "name" in firstCell || "url" in secondCell) {
                    continue
                }
            }
            val label = row[0].trim()
            val rawUrl = row[1].trim()
            val normalized = FshareUrl.normalize(rawUrl) ?: continue
            if (!FshareUrl.isFileUrl(normalized) && !FshareUrl.isFolderUrl(normalized)) continue
            items += FeaturedItem(
                title = label,
                url = normalized,
                linkcode = FshareUrl.extractLinkcode(normalized),
                isPlayable = !FshareUrl.isFolderUrl(normalized),
                size = null,
                thumbnail = null,
                source = source,
            )
            if (items.size >= MAX_SHEET_ROWS) break
        }
        return items
    }

    companion object {
        private const val SUGGESTED_TTL_MS = 30L * 60 * 1000     // 30 phút
        private const val TRENDING_TTL_MS = 15L * 60 * 1000      // 15 phút
        private const val MAX_SHEET_ROWS = 100                    // = kSuggestMaxRows desktop
    }
}
