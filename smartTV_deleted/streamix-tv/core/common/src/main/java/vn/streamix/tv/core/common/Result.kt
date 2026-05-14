/*
 * StreamIX TV — ApiResult & domain Result
 * Reference: 13 §4.2 Response envelope
 */
package vn.streamix.tv.core.common

/**
 * Bọc kết quả gọi API gateway. Map từ HTTP response sau parse envelope.
 *
 * Response envelope theo Open API Gateway:
 *  - Success: { data, meta?, request_id }
 *  - Error:   { error: { code, message, details }, request_id }
 *
 * @see 13_implementation-spec-v1.md §4.2
 */
sealed interface ApiResult<out T> {
    data class Success<T>(
        val data: T,
        val meta: PaginationMeta? = null,
        val requestId: String? = null,
    ) : ApiResult<T>

    data class Error(
        val errorCode: String,
        val message: String,
        val httpStatus: Int,
        val requestId: String? = null,
        val details: Map<String, Any?>? = null,
    ) : ApiResult<Nothing>

    /** Mất kết nối, IOException, timeout */
    data object NetworkError : ApiResult<Nothing>

    /** Refresh token expire hoặc invalid → user phải re-login */
    data object AuthExpired : ApiResult<Nothing>

    /** API gateway trả 429 RATE_LIMIT_EXCEEDED */
    data class RateLimited(val retryAfterSeconds: Long? = null) : ApiResult<Nothing>
}

data class PaginationMeta(
    val page: Int,
    val perPage: Int,
    val total: Int,
)

inline fun <T, R> ApiResult<T>.map(transform: (T) -> R): ApiResult<R> = when (this) {
    is ApiResult.Success -> ApiResult.Success(transform(data), meta, requestId)
    is ApiResult.Error -> this
    ApiResult.NetworkError -> ApiResult.NetworkError
    ApiResult.AuthExpired -> ApiResult.AuthExpired
    is ApiResult.RateLimited -> this
}

inline fun <T> ApiResult<T>.onSuccess(action: (T) -> Unit): ApiResult<T> {
    if (this is ApiResult.Success) action(data)
    return this
}

inline fun <T> ApiResult<T>.onError(action: (errorCode: String, message: String) -> Unit): ApiResult<T> {
    if (this is ApiResult.Error) action(errorCode, message)
    return this
}

fun <T> ApiResult<T>.getOrNull(): T? = (this as? ApiResult.Success)?.data
