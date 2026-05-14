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
import vn.streamix.tv.core.network.service.TimfshareApi
import vn.streamix.tv.core.network.service.TimfshareSearchItemDto
import vn.streamix.tv.core.network.service.TimfshareSearchResponseDto

class SearchRepositoryImplTest {

    private val api: TimfshareApi = mockk()
    private val repo = SearchRepositoryImpl(api)

    // RP-020 / S12-001
    @Test
    fun `empty query returns empty success without calling API`() = runTest {
        val r = repo.search("")
        assertTrue(r is ApiResult.Success)
        assertEquals(emptyList<Any>(), (r as ApiResult.Success).data)
        coVerify(exactly = 0) { api.search(any()) }
    }

    // S12-002
    @Test
    fun `whitespace-only query returns empty success`() = runTest {
        val r = repo.search("   ")
        assertTrue(r is ApiResult.Success)
        assertEquals(emptyList<Any>(), (r as ApiResult.Success).data)
        coVerify(exactly = 0) { api.search(any()) }
    }

    // S12-010
    @Test
    fun `dangerous pattern returns empty without calling API`() = runTest {
        val r = repo.search("<script>alert(1)</script>")
        assertTrue(r is ApiResult.Success)
        assertEquals(emptyList<Any>(), (r as ApiResult.Success).data)
        coVerify(exactly = 0) { api.search(any()) }
    }

    // S12-011 — Control char stripped (SOH = 0x01)
    @Test
    fun `query with control char gets stripped before sending`() = runTest {
        coEvery { api.search(any()) } returns Response.success(TimfshareSearchResponseDto(data = emptyList()))
        val withControl = "abc" + 0x01.toChar() + "def"
        repo.search(withControl)
        coVerify { api.search("abcdef") }
    }

    // S12-012 / RP-022
    @Test
    fun `dot is replaced by space before API call`() = runTest {
        coEvery { api.search(any()) } returns Response.success(TimfshareSearchResponseDto(data = emptyList()))
        repo.search("phim.hay.2024")
        coVerify { api.search("phim hay 2024") }
    }

    // S12-009
    @Test
    fun `query longer than 256 chars truncated`() = runTest {
        coEvery { api.search(any()) } returns Response.success(TimfshareSearchResponseDto(data = emptyList()))
        val long = "a".repeat(300)
        repo.search(long)
        coVerify {
            api.search(match { it.length == 256 })
        }
    }

    // S12-005 + non-video filter
    @Test
    fun `result filters non-video extensions`() = runTest {
        coEvery { api.search(any()) } returns Response.success(
            TimfshareSearchResponseDto(
                data = listOf(
                    TimfshareSearchItemDto(name = "phim.mp4", url = "https://www.fshare.vn/file/AAAAAAAAA1?token=x"),
                    TimfshareSearchItemDto(name = "doc.pdf", url = "https://www.fshare.vn/file/AAAAAAAAA2"),
                    TimfshareSearchItemDto(name = "movie.mkv", url = "https://www.fshare.vn/file/AAAAAAAAA3"),
                ),
            )
        )
        val r = repo.search("phim")
        assertTrue(r is ApiResult.Success)
        val items = (r as ApiResult.Success).data
        assertEquals(2, items.size)  // pdf bị loại
        assertTrue(items.all { it.url.contains("/file/") })
    }

    // S12-013 (cache hit case-insensitive)
    @Test
    fun `cache hit returns same data without second call`() = runTest {
        coEvery { api.search(any()) } returns Response.success(
            TimfshareSearchResponseDto(
                data = listOf(
                    TimfshareSearchItemDto(name = "x.mp4", url = "https://www.fshare.vn/file/CCCC"),
                ),
            )
        )
        repo.search("Phim")
        repo.search("phim")
        coVerify(exactly = 1) { api.search(any()) }
    }

    // S12-007
    @Test
    fun `HTTP 401 returns Error FORBIDDEN`() = runTest {
        coEvery { api.search(any()) } returns Response.error(
            401,
            "".toResponseBody("application/json".toMediaType()),
        )
        val r = repo.search("phim")
        assertTrue(r is ApiResult.Error)
        assertEquals("403", (r as ApiResult.Error).errorCode)
    }
}
