package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import io.mockk.coEvery
import io.mockk.mockk
import io.mockk.slot
import kotlinx.coroutines.test.runTest
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.ResponseBody
import okhttp3.ResponseBody.Companion.toResponseBody
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import retrofit2.Response
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.domain.model.ItemType
import vn.streamix.tv.core.network.envelope.IntOrStringAdapter
import vn.streamix.tv.core.network.service.FilesApi
import vn.streamix.tv.core.network.service.GetFileResponseDto
import vn.streamix.tv.core.network.service.GetFolderListRequestDto

class FilesRepositoryImplTest {

    private val api: FilesApi = mockk()
    private val authStore: AuthStore = mockk()
    private val moshi: Moshi = Moshi.Builder()
        .add(IntOrStringAdapter())
        .add(KotlinJsonAdapterFactory())
        .build()
    private val repo = FilesRepositoryImpl(api, authStore, moshi)

    private fun jsonBody(json: String): ResponseBody =
        json.toResponseBody("application/json".toMediaType())

    // RP-008
    @Test
    fun `listFiles accepts full folder URL`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val captured = slot<GetFolderListRequestDto>()
        coEvery { api.getFolderList(capture(captured)) } returns Response.success(
            jsonBody("""{"code":200,"items":[]}""")
        )
        repo.listFiles(path = "https://www.fshare.vn/folder/ABC123?token=x", page = 1, perPage = 50)
        assertEquals("https://www.fshare.vn/folder/ABC123", captured.captured.url)
    }

    // RP-009
    @Test
    fun `listFiles accepts raw linkcode`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val captured = slot<GetFolderListRequestDto>()
        coEvery { api.getFolderList(capture(captured)) } returns Response.success(
            jsonBody("""{"code":200,"items":[]}""")
        )
        repo.listFiles(path = "ABC123", page = 1, perPage = 50)
        assertEquals("https://www.fshare.vn/folder/ABC123", captured.captured.url)
    }

    // RP-010
    @Test
    fun `listFiles invalid path returns PARSE_ERROR`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val r = repo.listFiles(path = "/", page = 1, perPage = 50)
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.PARSE_ERROR, (r as ApiResult.Error).errorCode)
    }

    @Test
    fun `listFiles no token returns AuthExpired`() = runTest {
        coEvery { authStore.token() } returns null
        val r = repo.listFiles(path = "ABC123", page = 1, perPage = 50)
        assertTrue(r is ApiResult.AuthExpired)
    }

    @Test
    fun `listFiles parses array response (no envelope)`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody(
                """[
                {"name":"Folder A","linkcode":"F01","type":"0","size":"0","id":1},
                {"name":"video.mp4","linkcode":"FILE1","type":"1","size":"12345","id":2}
                ]""".trimIndent()
            )
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.Success)
        val items = (r as ApiResult.Success).data
        assertEquals(2, items.size)
        assertEquals(ItemType.FOLDER, items[0].type)
        assertEquals(ItemType.FILE, items[1].type)
        assertEquals(12345L, items[1].size)
    }

    @Test
    fun `listFiles parses object response with items field`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody(
                """{"code":200,"msg":"ok","items":[
                {"name":"Folder","linkcode":"F","type":"0"},
                {"name":"movie.mp4","linkcode":"M","type":"1","size":"100"}
                ]}""".trimIndent()
            )
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.Success)
        assertEquals(2, (r as ApiResult.Success).data.size)
    }

    @Test
    fun `listFiles parses nested data object`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody(
                """{"code":200,"data":{"items":[
                {"name":"X","linkcode":"X1","type":"1","size":"1"}
                ]}}""".trimIndent()
            )
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.Success)
        assertEquals(1, (r as ApiResult.Success).data.size)
    }

    @Test
    fun `listFiles HTTP 201 maps to AuthExpired`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.error(
            201,
            jsonBody("""{"code":201}"""),
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.AuthExpired)
    }

    @Test
    fun `listFiles body code 201 maps to AuthExpired`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody("""{"code":201,"msg":"expired"}""")
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.AuthExpired)
    }

    @Test
    fun `listFiles empty array returns empty success`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(jsonBody("[]"))
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.Success)
        assertEquals(0, (r as ApiResult.Success).data.size)
    }

    @Test
    fun `listFiles malformed JSON returns Error PARSE_ERROR`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody("not a json")
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.PARSE_ERROR, (r as ApiResult.Error).errorCode)
    }

    @Test
    fun `listFiles ext filter keeps folders and matching files`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFolderList(any()) } returns Response.success(
            jsonBody(
                """[
                {"name":"Folder","linkcode":"F","type":"0"},
                {"name":"movie.mp4","linkcode":"M","type":"1","size":"1"},
                {"name":"doc.pdf","linkcode":"D","type":"1","size":"1"}
                ]""".trimIndent()
            )
        )
        val r = repo.listFiles(path = "ABC", page = 1, perPage = 10, ext = "mp4")
        assertTrue(r is ApiResult.Success)
        assertEquals(2, (r as ApiResult.Success).data.size)
    }

    @Test
    fun `searchFiles returns Error stub`() = runTest {
        val r = repo.searchFiles("query")
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.UNKNOWN, (r as ApiResult.Error).errorCode)
    }

    @Test
    fun `listFolderTree returns empty success`() = runTest {
        val r = repo.listFolderTree()
        assertTrue(r is ApiResult.Success)
        assertEquals(emptyList<Any>(), (r as ApiResult.Success).data)
    }

    @Test
    fun `getFile flattens nested data`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { api.getFile(any()) } returns Response.success(
            GetFileResponseDto(
                code = "200",
                data = GetFileResponseDto(
                    name = "movie.mkv",
                    size = 999L,
                    linkcode = "ABC",
                ),
            ),
        )
        val r = repo.getFile("ABC")
        assertTrue(r is ApiResult.Success)
        val item = (r as ApiResult.Success).data
        assertEquals("movie.mkv", item.name)
        assertEquals(999L, item.size)
    }

    @Test
    fun `listFiles converts page 1 to pageIndex 0`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val captured = slot<GetFolderListRequestDto>()
        coEvery { api.getFolderList(capture(captured)) } returns Response.success(
            jsonBody("""{"code":200,"items":[]}""")
        )
        repo.listFiles(path = "ABC", page = 1, perPage = 10)
        assertEquals(0, captured.captured.pageIndex)
    }
}
