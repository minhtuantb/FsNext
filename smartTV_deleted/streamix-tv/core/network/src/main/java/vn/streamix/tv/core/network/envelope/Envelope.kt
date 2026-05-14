/*
 * StreamIX TV — Legacy Fshare API response contract.
 *
 * Khác Open API Gateway (envelope kiểu {data, meta, error, request_id}),
 * Fshare legacy trả flat: {code, msg, ...payload}.
 *  - code: Int hoặc String ("authenticate", "too many"...)
 *  - msg: human-readable message
 *  - payload: các field ngang hàng (token, session_id, location, dataFile, ...)
 *
 * Reference: D:\Work\FsNext\smartTV\streamix-tv\documents\API_REFERENCE_FOR_REUSE.md §3
 */
package vn.streamix.tv.core.network.envelope

import com.squareup.moshi.FromJson
import com.squareup.moshi.JsonQualifier
import com.squareup.moshi.JsonReader
import com.squareup.moshi.JsonWriter
import com.squareup.moshi.ToJson

/**
 * Mọi response DTO từ Fshare legacy implement interface này.
 * `apiCall` đọc `code`/`msg` để dispatch ApiResult.
 */
interface LegacyResponse {
    /** Code chuẩn hoá string. Ví dụ: "200", "201", "405", "authenticate", "too many". */
    val code: String?
    /** Human-readable message từ server. */
    val msg: String?
}

/**
 * Annotation đánh dấu field JSON có thể là Int hoặc String.
 * Fshare trả `code` ở 1 endpoint là 200 (number), endpoint khác là "authenticate" (string).
 *
 * Sử dụng:
 * ```
 * @Json(name = "code") @IntOrString val code: String? = null
 * ```
 *
 * Adapter [IntOrStringAdapter] phải được register vào Moshi (xem NetworkModule).
 */
@Retention(AnnotationRetention.RUNTIME)
@JsonQualifier
annotation class IntOrString

/**
 * Moshi adapter coerce Int|String → String? để parse `code` linh hoạt.
 * Boolean/null đều skip an toàn.
 */
class IntOrStringAdapter {

    @FromJson
    @IntOrString
    fun fromJson(reader: JsonReader): String? = when (reader.peek()) {
        JsonReader.Token.NUMBER -> reader.nextLong().toString()
        JsonReader.Token.STRING -> reader.nextString()
        JsonReader.Token.BOOLEAN -> reader.nextBoolean().toString()
        JsonReader.Token.NULL -> {
            reader.nextNull<Any?>()
            null
        }
        else -> {
            reader.skipValue()
            null
        }
    }

    @ToJson
    fun toJson(writer: JsonWriter, @IntOrString value: String?) {
        if (value == null) writer.nullValue() else writer.value(value)
    }
}

/**
 * DTO error fallback dùng khi parse errorBody của response 4xx/5xx.
 * Cũng phục vụ generic error parsing nếu DTO endpoint không capture.
 */
@com.squareup.moshi.JsonClass(generateAdapter = true)
data class GenericLegacyError(
    @com.squareup.moshi.Json(name = "code") @IntOrString override val code: String? = null,
    @com.squareup.moshi.Json(name = "msg") override val msg: String? = null,
) : LegacyResponse
