/*
 * StreamIX TV — apiCall dispatcher cho Fshare LEGACY API.
 *
 * Chuyển Response<T : LegacyResponse> → ApiResult<T> theo quy tắc:
 *   1. HTTP 201 (response code, không phải body) ⇒ AuthExpired (toàn cục).
 *   2. HTTP 2xx + body.code in {null, "200"} ⇒ Success.
 *   3. HTTP 2xx + body.code = "201" ⇒ AuthExpired.
 *   4. HTTP 2xx + body.code khác ⇒ Error(code, msg).
 *   5. HTTP non-2xx ⇒ parse errorBody (nếu có) làm GenericLegacyError → Error.
 *      Nếu body code = "201" ⇒ AuthExpired.
 *   6. IOException ⇒ NetworkError.
 *   7. JsonDataException / RuntimeException khác ⇒ Error(PARSE_ERROR) — KHÔNG crash app.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3 + §8 (best practices).
 */
package vn.streamix.tv.core.network.envelope

import com.squareup.moshi.JsonDataException
import com.squareup.moshi.JsonEncodingException
import com.squareup.moshi.Moshi
import retrofit2.HttpException
import retrofit2.Response
import timber.log.Timber
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import java.io.IOException

/**
 * Bọc 1 Retrofit suspend call trả về typed DTO implement [LegacyResponse].
 *
 * @param moshi để parse generic error nếu non-2xx và DTO không capture được
 * @param call lambda gọi Retrofit API
 * @return ApiResult<T> — Success / Error / AuthExpired / NetworkError / RateLimited
 */
suspend inline fun <reified T : LegacyResponse> apiCall(
    moshi: Moshi,
    crossinline call: suspend () -> Response<T>,
): ApiResult<T> = try {
    val response = call()
    when {
        // Quy tắc 1: HTTP 201 toàn cục = session expired.
        response.code() == 201 -> ApiResult.AuthExpired

        response.isSuccessful -> {
            val body = response.body()
            if (body == null) {
                ApiResult.Error(
                    errorCode = ErrorCodes.PARSE_ERROR,
                    message = "Empty response body",
                    httpStatus = response.code(),
                )
            } else {
                val code = body.code
                when (code) {
                    null, ErrorCodes.OK -> ApiResult.Success(data = body)
                    ErrorCodes.SESSION_EXPIRED -> ApiResult.AuthExpired
                    else -> ApiResult.Error(
                        errorCode = code,
                        message = body.msg ?: "Lỗi $code",
                        httpStatus = response.code(),
                    )
                }
            }
        }

        else -> {
            // Non-2xx: parse errorBody nếu có thể.
            val errorBody = response.errorBody()?.string()
            val parsed = errorBody?.let { tryParseGenericError(moshi, it) }
            val parsedCode = parsed?.code
            when {
                response.code() == 429 -> {
                    val retryAfter = response.headers()["Retry-After"]?.toLongOrNull()
                    ApiResult.RateLimited(retryAfterSeconds = retryAfter)
                }
                parsedCode == ErrorCodes.SESSION_EXPIRED -> ApiResult.AuthExpired
                else -> ApiResult.Error(
                    errorCode = parsedCode ?: response.code().toString(),
                    message = parsed?.msg ?: response.message().ifBlank { "HTTP ${response.code()}" },
                    httpStatus = response.code(),
                )
            }
        }
    }
} catch (e: HttpException) {
    Timber.w(e, "apiCall HttpException code=${e.code()}")
    ApiResult.Error(
        errorCode = ErrorCodes.UNKNOWN,
        message = e.message() ?: "HttpException",
        httpStatus = e.code(),
    )
} catch (e: IOException) {
    Timber.w(e, "apiCall IOException")
    ApiResult.NetworkError
} catch (e: JsonDataException) {
    // Moshi parse error — server trả format khác DTO expect (vd. array thay object).
    Timber.e(e, "apiCall JsonDataException — server response shape mismatch")
    ApiResult.Error(
        errorCode = ErrorCodes.PARSE_ERROR,
        message = "Server response format không hợp lệ",
        httpStatus = 0,
    )
} catch (e: JsonEncodingException) {
    Timber.e(e, "apiCall JsonEncodingException — malformed JSON")
    ApiResult.Error(
        errorCode = ErrorCodes.PARSE_ERROR,
        message = "Server response không phải JSON hợp lệ",
        httpStatus = 0,
    )
} catch (t: Throwable) {
    // Catch-all — KHÔNG bao giờ để exception propagate ra ngoài.
    // Bao gồm: NPE, NumberFormatException, IllegalStateException...
    Timber.e(t, "apiCall unexpected throwable")
    ApiResult.Error(
        errorCode = ErrorCodes.UNKNOWN,
        message = t.message ?: "Lỗi không xác định",
        httpStatus = 0,
    )
}

/** Parse generic {code, msg} từ JSON error body. Trả null nếu fail. */
@PublishedApi
internal fun tryParseGenericError(moshi: Moshi, json: String): GenericLegacyError? = runCatching {
    moshi.adapter(GenericLegacyError::class.java).fromJson(json)
}.getOrNull()
