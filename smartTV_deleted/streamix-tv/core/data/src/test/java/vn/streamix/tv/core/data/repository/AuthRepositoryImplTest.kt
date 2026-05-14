package vn.streamix.tv.core.data.repository

import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
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
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.domain.model.Session
import vn.streamix.tv.core.network.envelope.IntOrStringAdapter
import vn.streamix.tv.core.network.service.AuthApi
import vn.streamix.tv.core.network.service.LoginResponseDto

class AuthRepositoryImplTest {

    private val authApi: AuthApi = mockk()
    private val authStore: AuthStore = mockk(relaxed = true)
    private val moshi: Moshi = Moshi.Builder()
        .add(IntOrStringAdapter())
        .add(KotlinJsonAdapterFactory())
        .build()
    private val repo = AuthRepositoryImpl(
        authApi = authApi,
        authStore = authStore,
        moshi = moshi,
        appKey = "test_app_key",
    )

    // RP-001
    @Test
    fun `login success saves Session and returns LoginResult`() = runTest {
        coEvery { authApi.login(any()) } returns Response.success(
            LoginResponseDto(code = "200", msg = "ok", token = "TOK", sessionId = "SID"),
        )
        val r = repo.login("u@x.vn", "secret")
        assertTrue(r is ApiResult.Success)
        coVerify {
            authStore.save(match<Session> {
                it.token == "TOK" && it.sessionId == "SID" && it.email == "u@x.vn"
            })
        }
    }

    // RP-002
    @Test
    fun `login response without token returns PARSE_ERROR`() = runTest {
        coEvery { authApi.login(any()) } returns Response.success(
            LoginResponseDto(code = "200", msg = "ok", token = null, sessionId = "SID"),
        )
        val r = repo.login("u@x.vn", "secret")
        assertTrue(r is ApiResult.Error)
        assertEquals(ErrorCodes.PARSE_ERROR, (r as ApiResult.Error).errorCode)
    }

    // S1-004
    @Test
    fun `login server returns code 405 returns Error 405`() = runTest {
        coEvery { authApi.login(any()) } returns Response.success(
            LoginResponseDto(code = "405", msg = "wrong"),
        )
        val r = repo.login("u@x.vn", "secret")
        assertTrue(r is ApiResult.Error)
        assertEquals("405", (r as ApiResult.Error).errorCode)
    }

    // RP-003
    @Test
    fun `logout clears local store and returns Success`() = runTest {
        coEvery { authStore.clear() } returns Unit
        val r = repo.logout()
        assertTrue(r is ApiResult.Success)
        coVerify { authStore.clear() }
    }

    // S1-005
    @Test
    fun `login server returns code too many returns Error too many`() = runTest {
        coEvery { authApi.login(any()) } returns Response.success(
            LoginResponseDto(code = "too many", msg = "blocked"),
        )
        val r = repo.login("u@x.vn", "secret")
        assertTrue(r is ApiResult.Error)
        assertEquals("too many", (r as ApiResult.Error).errorCode)
    }

    @Test
    fun `login HTTP 405 errorBody parsed`() = runTest {
        coEvery { authApi.login(any()) } returns Response.error(
            405,
            """{"code": 405, "msg": "auth fail"}""".toResponseBody("application/json".toMediaType()),
        )
        val r = repo.login("u@x.vn", "secret")
        assertTrue(r is ApiResult.Error)
        assertEquals("405", (r as ApiResult.Error).errorCode)
    }
}
