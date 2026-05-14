package vn.streamix.tv.core.network.envelope

import com.squareup.moshi.JsonClass
import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class IntOrStringAdapterTest {

    @JsonClass(generateAdapter = false)
    data class Envelope(
        @com.squareup.moshi.Json(name = "code") @IntOrString val code: String? = null,
    )

    private val moshi: Moshi = Moshi.Builder()
        .add(IntOrStringAdapter())
        .add(KotlinJsonAdapterFactory())
        .build()

    private val adapter = moshi.adapter(Envelope::class.java)

    @Test
    fun `parse numeric code as String`() {
        val result = adapter.fromJson("""{"code": 200}""")
        assertEquals("200", result?.code)
    }

    @Test
    fun `parse large numeric code as String`() {
        val result = adapter.fromJson("""{"code": 9999999}""")
        assertEquals("9999999", result?.code)
    }

    @Test
    fun `parse string code as String`() {
        val result = adapter.fromJson("""{"code": "authenticate"}""")
        assertEquals("authenticate", result?.code)
    }

    @Test
    fun `parse null code as null`() {
        val result = adapter.fromJson("""{"code": null}""")
        assertNull(result?.code)
    }

    @Test
    fun `parse missing code as null`() {
        val result = adapter.fromJson("""{}""")
        assertNull(result?.code)
    }

    @Test
    fun `parse boolean code as String`() {
        val result = adapter.fromJson("""{"code": true}""")
        assertEquals("true", result?.code)
    }

    @Test
    fun `parse object code skipped`() {
        // Object value at code field — adapter should skip and return null
        val result = adapter.fromJson("""{"code": {"nested": 1}}""")
        assertNull(result?.code)
    }
}
