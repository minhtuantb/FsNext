package vn.streamix.tv.core.data.repository

import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.mockk
import kotlinx.coroutines.test.runTest
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.ResponseBody.Companion.toResponseBody
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import retrofit2.Response
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.domain.model.FeaturedSource
import vn.streamix.tv.core.network.service.SheetsApi
import vn.streamix.tv.core.network.service.TimfshareTrendingApi
import vn.streamix.tv.core.network.service.TimfshareTrendingItemDto
import vn.streamix.tv.core.network.service.TimfshareTrendingResponseDto

class FeaturedRepositoryImplTest {

    private val sheets: SheetsApi = mockk()
    private val timTrending: TimfshareTrendingApi = mockk()

    private fun newRepo() = FeaturedRepositoryImpl(
        sheetsApi = sheets,
        timfshareTrendingApi = timTrending,
        goiyFileId = "FID1",
        goiyGid = "1",
        xuhuongFileId = "FID2",
        xuhuongGid = "2",
    )

    // RP-016
    @Test
    fun `suggested parses CSV rows and skips header`() = runTest {
        val csv = """tên,link
"Phim hot","https://www.fshare.vn/file/ABC123"
"Folder kinh điển","https://www.fshare.vn/folder/XYZ456""""
        coEvery { sheets.exportCsv("FID1", "csv", "1") } returns Response.success(csv)

        val r = newRepo().suggested()
        assertTrue(r is ApiResult.Success)
        val items = (r as ApiResult.Success).data
        assertEquals(2, items.size)
        assertEquals("Phim hot", items[0].title)
        assertTrue(items[0].isPlayable)
        assertEquals("Folder kinh điển", items[1].title)
        assertTrue(!items[1].isPlayable)
    }

    // RP-017
    @Test
    fun `suggested cache hit avoids second network call`() = runTest {
        val csv = """tên,link
"X","https://www.fshare.vn/file/AAA""""
        coEvery { sheets.exportCsv("FID1", "csv", "1") } returns Response.success(csv)

        val repo = newRepo()
        repo.suggested()
        repo.suggested()
        coVerify(exactly = 1) { sheets.exportCsv("FID1", "csv", "1") }
    }

    // RP-018
    @Test
    fun `trending sheet empty falls back to Timfshare`() = runTest {
        coEvery { sheets.exportCsv("FID2", "csv", "2") } returns Response.success("")
        coEvery { timTrending.trending() } returns Response.success(
            TimfshareTrendingResponseDto(
                dataFile = listOf(
                    TimfshareTrendingItemDto(
                        id = 1, name = "Top movie", linkcode = "TOP1",
                        fileExtension = "mp4", size = 999L,
                    ),
                ),
            ),
        )

        val r = newRepo().trending()
        assertTrue(r is ApiResult.Success)
        val items = (r as ApiResult.Success).data
        assertEquals(1, items.size)
        assertEquals(FeaturedSource.TRENDING, items[0].source)
        assertEquals("TOP1", items[0].linkcode)
    }

    // RP-019
    @Test
    fun `trending sheet empty and Timfshare error returns empty list`() = runTest {
        coEvery { sheets.exportCsv("FID2", "csv", "2") } returns Response.success("")
        coEvery { timTrending.trending() } returns Response.error(
            500,
            "".toResponseBody("application/json".toMediaType()),
        )

        val r = newRepo().trending()
        assertTrue(r is ApiResult.Success)
        assertEquals(emptyList<Any>(), (r as ApiResult.Success).data)
    }

    @Test
    fun `suggested filters out non-fshare URLs`() = runTest {
        val csv = """tên,link
"Good","https://www.fshare.vn/file/AAA"
"Bad","https://random.com/file/BAD""""
        coEvery { sheets.exportCsv("FID1", "csv", "1") } returns Response.success(csv)

        val r = newRepo().suggested()
        assertTrue(r is ApiResult.Success)
        val items = (r as ApiResult.Success).data
        assertEquals(1, items.size)
        assertEquals("Good", items[0].title)
    }

    @Test
    fun `suggested normalizes http to https`() = runTest {
        val csv = """tên,link
"X","http://fshare.vn/file/AAA""""
        coEvery { sheets.exportCsv("FID1", "csv", "1") } returns Response.success(csv)

        val r = newRepo().suggested()
        assertTrue(r is ApiResult.Success)
        val item = (r as ApiResult.Success).data.first()
        assertTrue(item.url.startsWith("https://www.fshare.vn"))
    }
}
