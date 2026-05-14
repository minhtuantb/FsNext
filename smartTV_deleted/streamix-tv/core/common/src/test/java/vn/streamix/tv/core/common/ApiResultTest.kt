package vn.streamix.tv.core.common

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ApiResultTest {

    @Test
    fun mapSuccess() {
        val r: ApiResult<Int> = ApiResult.Success(42)
        val mapped = r.map { it.toString() }
        assertEquals(ApiResult.Success("42"), mapped)
    }

    @Test
    fun mapErrorPassthrough() {
        val r: ApiResult<Int> = ApiResult.Error("E", "msg", 500)
        val mapped = r.map { it.toString() }
        assertEquals(ApiResult.Error("E", "msg", 500), mapped)
    }

    @Test
    fun mapNetworkErrorPassthrough() {
        val r: ApiResult<Int> = ApiResult.NetworkError
        assertEquals(ApiResult.NetworkError, r.map { it.toString() })
    }

    @Test
    fun getOrNullSuccess() {
        assertEquals(42, ApiResult.Success(42).getOrNull())
    }

    @Test
    fun getOrNullError() {
        assertNull(ApiResult.Error("E", "m", 500).getOrNull())
        assertNull(ApiResult.NetworkError.getOrNull())
        assertNull(ApiResult.AuthExpired.getOrNull())
    }
}
