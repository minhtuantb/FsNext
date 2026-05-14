/*
 * StreamIX TV — TopRepository implementation.
 *
 * Source: 3 Google Sheet riêng cho IMDB/RT/Fshare ranking. Mỗi sheet format:
 *   Cột 0: rank (1..100)
 *   Cột 1: tên phim
 *   Cột 2: năm
 *   Cột 3: URL Fshare
 *   Cột 4: format (MKV/MP4 — optional)
 *   Cột 5: size bytes (optional)
 *
 * Cache TTL 1 giờ per source.
 *
 * V1: nếu sheet rỗng/lỗi → trả empty list (UI hiện empty state). Không hardcode list.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §5 (Sheets) + design system mockup S2c.
 */
package vn.streamix.tv.core.data.repository

import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.CsvParser
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.core.domain.model.TopMovie
import vn.streamix.tv.core.domain.model.TopSource
import vn.streamix.tv.core.domain.repository.TopRepository
import vn.streamix.tv.core.network.service.SheetsApi
import java.io.IOException
import javax.inject.Inject
import javax.inject.Named
import javax.inject.Singleton

@Singleton
class TopRepositoryImpl @Inject constructor(
    private val sheetsApi: SheetsApi,
    @Named("sheetTopImdbId") private val imdbFileId: String,
    @Named("sheetTopImdbGid") private val imdbGid: String,
    @Named("sheetTopRtId") private val rtFileId: String,
    @Named("sheetTopRtGid") private val rtGid: String,
    @Named("sheetTopFshareId") private val fshareFileId: String,
    @Named("sheetTopFshareGid") private val fshareGid: String,
) : TopRepository {

    private data class Cache(val value: List<TopMovie>, val expireAt: Long)

    private val mutex = Mutex()

    @Volatile private var imdbCache: Cache? = null
    @Volatile private var rtCache: Cache? = null
    @Volatile private var fshareCache: Cache? = null

    override suspend fun top(source: TopSource): ApiResult<List<TopMovie>> {
        val cache = when (source) {
            TopSource.IMDB -> imdbCache
            TopSource.RT -> rtCache
            TopSource.FSHARE -> fshareCache
        }
        cache?.takeIf { it.expireAt > System.currentTimeMillis() }
            ?.let { return ApiResult.Success(it.value) }

        return mutex.withLock {
            val (fileId, gid) = when (source) {
                TopSource.IMDB -> imdbFileId to imdbGid
                TopSource.RT -> rtFileId to rtGid
                TopSource.FSHARE -> fshareFileId to fshareGid
            }
            // Sheet ID rỗng → trả empty (giai đoạn placeholder cho V1).
            if (fileId.isBlank() || gid.isBlank()) {
                return@withLock ApiResult.Success(emptyList())
            }
            val result = fetch(fileId, gid, source)
            if (result is ApiResult.Success) {
                val newCache = Cache(result.data, System.currentTimeMillis() + TTL_MS)
                when (source) {
                    TopSource.IMDB -> imdbCache = newCache
                    TopSource.RT -> rtCache = newCache
                    TopSource.FSHARE -> fshareCache = newCache
                }
            }
            result
        }
    }

    private suspend fun fetch(
        fileId: String,
        gid: String,
        source: TopSource,
    ): ApiResult<List<TopMovie>> = try {
        val response = sheetsApi.exportCsv(fileId = fileId, gid = gid)
        if (!response.isSuccessful) {
            ApiResult.Error(
                errorCode = response.code().toString(),
                message = "Top sheet fetch failed: ${response.message()}",
                httpStatus = response.code(),
            )
        } else {
            val csv = response.body() ?: ""
            ApiResult.Success(parseCsv(csv, source))
        }
    } catch (_: IOException) {
        ApiResult.NetworkError
    } catch (t: Throwable) {
        ApiResult.Error(
            errorCode = ErrorCodes.PARSE_ERROR,
            message = "Top sheet parse error: ${t.message}",
            httpStatus = 0,
        )
    }

    /**
     * CSV format expected:
     *   rank, title, year, url, format(optional), size_bytes(optional)
     * Header row được skip nếu cell 0 chứa "rank" hoặc "stt".
     */
    private fun parseCsv(csv: String, source: TopSource): List<TopMovie> {
        val rows = CsvParser.parse(csv)
        if (rows.isEmpty()) return emptyList()

        val items = mutableListOf<TopMovie>()
        var skippedHeader = false
        for (row in rows) {
            if (row.size < 4) continue
            if (!skippedHeader) {
                skippedHeader = true
                val first = row[0].lowercase().trim()
                if ("rank" in first || "stt" in first || "hạng" in first) continue
            }
            val rank = row[0].trim().toIntOrNull() ?: continue
            if (rank !in 1..200) continue
            val title = row[1].trim()
            if (title.isBlank()) continue
            val year = row.getOrNull(2)?.trim()?.toIntOrNull()
            val rawUrl = row[3].trim()
            val normalized = FshareUrl.normalize(rawUrl) ?: continue
            if (!FshareUrl.isFileUrl(normalized)) continue   // chỉ giữ file (xem được)

            val format = row.getOrNull(4)?.trim()?.takeIf { it.isNotBlank() }
            val size = row.getOrNull(5)?.trim()?.toLongOrNull()
            items += TopMovie(
                rank = rank,
                title = title,
                year = year,
                size = size,
                format = format,
                url = normalized,
                linkcode = FshareUrl.extractLinkcode(normalized),
                thumbnail = null,
                source = source,
            )
            if (items.size >= MAX_ROWS) break
        }
        // Sắp theo rank để UI render thứ tự.
        return items.sortedBy { it.rank }
    }

    companion object {
        private const val TTL_MS = 60L * 60 * 1000   // 1 giờ
        private const val MAX_ROWS = 200
    }
}
