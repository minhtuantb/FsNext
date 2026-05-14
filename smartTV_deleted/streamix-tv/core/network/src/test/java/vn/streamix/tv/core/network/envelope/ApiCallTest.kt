package vn.streamix.tv.core.network.envelope

import com.squareup.moshi.JsonClass
import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import kotlinx.coroutines.test.runTest
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import retrofit2.Response
import retrofit2.Retrofit
import retrofit2.converter.moshi.MoshiConverterFactory
import retrofit2.http.GET
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes

class ApiCallTest {

    @JsonClass(generateAdapter = false)
    data class TestEnvelope(
        @com.squareup.moshi.Json(name = "code") @IntOrString override val code: String? = null,
        @com.squareup.moshi.Json(name = "msg") override val msg: String? = null,
        @com.squareup.moshi.Json(name = "value") val value: String? = null,
    ) : LegacyResponse

    interface TestApi {
        @GET("/test")
        suspend fun get(): Response<TestEnvelope>
    }

    private lateinit var server: MockWebServer
    private lateinit var api: TestApi
    private lateinit var moshi: Moshi

    @Before
    fun setUp() {
        server = MockWebServer()
        server.start()
        moshi = Moshi.Builder()
            .add(IntOrStringAdapter())
            .add(KotlinJsonAdapterFactory())
            .build()
        api = Retrofit.Builder()
            .baseUrl(server.url("/"))
            .addConverterFactory(MoshiConverterFactory.create(moshi))
            .build()
            .create(TestApi::class.java)
    }

    @After
    fun tearDown() {
        server.shutdown()
    }

    // NW-001
    @Test
    fun `HTTP 201 globally maps to AuthExpired`() = runTest {
        server.enqueue(MockResponse().setResponseCode(201).setBody("""{"code":201,"msg":"expired"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.AuthExpired)
    }

    // NW-002
    @Test
    fun `HTTP 200 with body code 201 maps to AuthExpired`() = runTest {
        server.enqueue(MockResponse().setResponseCode(200).setBody("""{"code":201,"msg":"expired"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.AuthExpired)
    }

    // NW-003
    @Test
    fun `HTTP 200 body code 200 returns Success`() = runTest {
        server.enqueue(MockResponse().setResponseCode(200).setBody("""{"code":200,"msg":"ok","value":"hello"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Success)
        assertEquals("hello", (result as ApiResult.Success).data.value)
    }

    // NW-004
    @Test
    fun `HTTP 200 body code 405 returns Error 405`() = runTest {
        server.enqueue(MockResponse().setResponseCode(200).setBody("""{"code":405,"msg":"sai mật khẩu"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Error)
        result as ApiResult.Error
        assertEquals("405", result.errorCode)
        assertEquals("sai mật khẩu", result.message)
    }

    // NW-005
    @Test
    fun `HTTP 200 body code authenticate returns Error authenticate`() = runTest {
        server.enqueue(MockResponse().setResponseCode(200).setBody("""{"code":"authenticate","msg":"x"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Error)
        assertEquals("authenticate", (result as ApiResult.Error).errorCode)
    }

    // NW-006
    @Test
    fun `HTTP 200 empty body returns Error PARSE_ERROR`() = runTest {
        server.enqueue(MockResponse().setResponseCode(200).setBody(""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Error)
        assertEquals(ErrorCodes.PARSE_ERROR, (result as ApiResult.Error).errorCode)
    }

    // NW-007
    @Test
    fun `HTTP 429 returns RateLimited`() = runTest {
        server.enqueue(
            MockResponse()
                .setResponseCode(429)
                .setHeader("Retry-After", "30")
                .setBody("""{"code":429,"msg":"rate limited"}""")
        )
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.RateLimited)
        assertEquals(30L, (result as ApiResult.RateLimited).retryAfterSeconds)
    }

    // NW-008
    @Test
    fun `HTTP 500 errorBody parsed returns Error from body`() = runTest {
        server.enqueue(
            MockResponse().setResponseCode(500).setBody("""{"code":"internal","msg":"db down"}""")
        )
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Error)
        result as ApiResult.Error
        assertEquals("internal", result.errorCode)
        assertEquals("db down", result.message)
    }

    // NW-009
    @Test
    fun `HTTP 500 errorBody not JSON returns Error with HTTP code`() = runTest {
        server.enqueue(MockResponse().setResponseCode(500).setBody("internal server error"))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.Error)
        assertEquals("500", (result as ApiResult.Error).errorCode)
    }

    // NW-010 — IOException maps to NetworkError
    @Test
    fun `IOException maps to NetworkError`() = runTest {
        server.shutdown()  // simulate connection failure
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.NetworkError)
    }

    // Special: HTTP 4xx with body code 201 → AuthExpired
    @Test
    fun `HTTP 401 with body code 201 returns AuthExpired`() = runTest {
        server.enqueue(MockResponse().setResponseCode(401).setBody("""{"code":201,"msg":"expired"}"""))
        val result = apiCall(moshi) { api.get() }
        assertTrue(result is ApiResult.AuthExpired)
    }
}
