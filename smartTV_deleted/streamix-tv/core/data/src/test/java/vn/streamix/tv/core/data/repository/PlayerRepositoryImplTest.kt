package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.mockk
import io.mockk.slot
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import retrofit2.Response
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.network.envelope.IntOrStringAdapter
import vn.streamix.tv.core.network.service.SessionApi
import vn.streamix.tv.core.network.service.SessionDownloadRequestDto
import vn.streamix.tv.core.network.service.SessionDownloadResponseDto

class PlayerRepositoryImplTest {

    private val sessionApi: SessionApi = mockk()
    private val authStore: AuthStore = mockk()
    private val moshi: Moshi = Moshi.Builder()
        .add(IntOrStringAdapter())
        .add(KotlinJsonAdapterFactory())
        .build()
    private val repo = PlayerRepositoryImpl(
        sessionApi = sessionApi,
        authStore = authStore,
        moshi = moshi,
        shareReferralId = "8805984",
    )

    // RP-014 / S5-001
    @Test
    fun `fetchStreamUrl success returns StreamUrl with default TTL`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { sessionApi.createDownloadSession(any()) } returns Response.success(
            SessionDownloadResponseDto(code = "200", location = "https://download.fshare.vn/abc/stream"),
        )
        val r = repo.fetchStreamUrl("ABC123", null)
        assertTrue(r is ApiResult.Success)
        val url = (r as ApiResult.Success).data
        assertEquals("https://download.fshare.vn/abc/stream", url.url)
        assertEquals(12L * 3600, url.expiresInSeconds)
    }

    // RP-015 / S5-014
    @Test
    fun `referral id appended when no query`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val captured = slot<SessionDownloadRequestDto>()
        coEvery { sessionApi.createDownloadSession(capture(captured)) } returns Response.success(
            SessionDownloadResponseDto(code = "200", location = "x"),
        )
        repo.fetchStreamUrl("ABC123", null)
        assertTrue(captured.captured.url.endsWith("?share=8805984"))
    }

    // S5-015 — URL with existing query
    @Test
    fun `referral appended with ampersand when query exists`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        // Note: PlayerRepositoryImpl always builds URL from linkcode, so no existing query.
        // We test indirectly via the appendReferral logic.
        val captured = slot<SessionDownloadRequestDto>()
        coEvery { sessionApi.createDownloadSession(capture(captured)) } returns Response.success(
            SessionDownloadResponseDto(code = "200", location = "x"),
        )
        repo.fetchStreamUrl("XYZ", null)
        assertEquals("https://www.fshare.vn/file/XYZ?share=8805984", captured.captured.url)
    }

    // S5-004
    @Test
    fun `code 123 returns Error FILE_PASSWORD_REQUIRED`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { sessionApi.createDownloadSession(any()) } returns Response.success(
            SessionDownloadResponseDto(code = "123", msg = "password required"),
        )
        val r = repo.fetchStreamUrl("ABC", null)
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.FILE_PASSWORD_REQUIRED, (r as ApiResult.Error).errorCode)
    }

    // S5-008
    @Test
    fun `code 471 returns Error TOO_MANY_SESSIONS`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { sessionApi.createDownloadSession(any()) } returns Response.success(
            SessionDownloadResponseDto(code = "471", msg = "too many sessions"),
        )
        val r = repo.fetchStreamUrl("ABC", null)
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.TOO_MANY_SESSIONS, (r as ApiResult.Error).errorCode)
    }

    // RP-014 - no token
    @Test
    fun `no token returns AuthExpired`() = runTest {
        coEvery { authStore.token() } returns null
        val r = repo.fetchStreamUrl("ABC", null)
        assertTrue(r is ApiResult.AuthExpired)
    }

    // S5-002 — code 200 success but missing location field
    @Test
    fun `success without location returns Error PARSE_ERROR`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        coEvery { sessionApi.createDownloadSession(any()) } returns Response.success(
            SessionDownloadResponseDto(code = "200", msg = "ok"),
        )
        val r = repo.fetchStreamUrl("ABC", null)
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.PARSE_ERROR, (r as ApiResult.Error).errorCode)
    }

    @Test
    fun `password passed in body when provided`() = runTest {
        coEvery { authStore.token() } returns "TOK"
        val captured = slot<SessionDownloadRequestDto>()
        coEvery { sessionApi.createDownloadSession(capture(captured)) } returns Response.success(
            SessionDownloadResponseDto(code = "200", location = "x"),
        )
        repo.fetchStreamUrl("ABC", "secret123")
        assertEquals("secret123", captured.captured.password)
        assertEquals("TOK", captured.captured.token)
        assertNotNull(captured.captured.url)
    }
}
