/*
 * StreamIX TV — FilesRepository (Fshare LEGACY).
 *
 * Mapping vs. domain interface:
 *  - listFiles(path, ...) → POST /api/fileops/getFolderList
 *      `path` ở đây là canonical folder URL `https://www.fshare.vn/folder/<linkcode>`
 *      hoặc chỉ linkcode → repository tự build URL.
 *      Response có thể là array thuần HOẶC object — parse linh hoạt.
 *  - getFile(linkcode) → POST /api/fileops/get với URL `https://www.fshare.vn/file/<linkcode>`.
 *  - getFileMetadata(linkcode) → giống getFile (legacy không tách metadata).
 *  - searchFiles(...) → DELEGATED → wave M3 đã rewire qua SearchRepository riêng.
 *      Tạm thời trả Error stub; UI Search dùng SearchRepository trực tiếp.
 *  - listFolderTree() → KHÔNG có endpoint. Trả empty list.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.4 + §3.5.
 */
package vn.streamix.tv.core.data.repository

import com.squareup.moshi.JsonReader
import com.squareup.moshi.Moshi
import okio.Buffer
import timber.log.Timber
import vn.streamix.tv.core.common.ApiResult
import vn.streamix.tv.core.common.ErrorCodes
import vn.streamix.tv.core.common.map
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.domain.model.FileItem
import vn.streamix.tv.core.domain.model.ItemType
import vn.streamix.tv.core.domain.repository.FilesRepository
import vn.streamix.tv.core.network.envelope.apiCall
import vn.streamix.tv.core.network.service.FilesApi
import vn.streamix.tv.core.network.service.FolderListItem
import vn.streamix.tv.core.network.service.FolderListParseResult
import vn.streamix.tv.core.network.service.GetFileRequestDto
import vn.streamix.tv.core.network.service.GetFileResponseDto
import vn.streamix.tv.core.network.service.GetFolderListRequestDto
import java.io.IOException
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class FilesRepositoryImpl @Inject constructor(
    private val filesApi: FilesApi,
    private val authStore: AuthStore,
    private val moshi: Moshi,
) : FilesRepository {

    override suspend fun listFiles(
        path: String,
        page: Int,
        perPage: Int,
        dirOnly: Boolean,
        ext: String?,
    ): ApiResult<List<FileItem>> {
        val token = authStore.token() ?: return ApiResult.AuthExpired
        val canonicalUrl = canonicalFolderUrl(path) ?: return ApiResult.Error(
            errorCode = ErrorCodes.PARSE_ERROR,
            message = "Đường dẫn thư mục không hợp lệ: $path",
            httpStatus = 400,
        )
        val pageIndex = (page - 1).coerceAtLeast(0)
        val request = GetFolderListRequestDto(
            token = token,
            url = canonicalUrl,
            dirOnly = if (dirOnly) 1 else 0,
            pageIndex = pageIndex,
            limit = perPage,
        )

        return try {
            val response = filesApi.getFolderList(request)

            // Quy tắc 1: HTTP 201 toàn cục = session expired.
            if (response.code() == 201) return ApiResult.AuthExpired

            if (!response.isSuccessful) {
                Timber.w("getFolderList HTTP %d %s", response.code(), response.message())
                return ApiResult.Error(
                    errorCode = response.code().toString(),
                    message = response.message().ifBlank { "HTTP ${response.code()}" },
                    httpStatus = response.code(),
                )
            }

            val rawJson = response.body()?.string().orEmpty()
            if (rawJson.isBlank()) return ApiResult.Success(emptyList())

            val parsed = parseFolderListResponse(rawJson)
            // Body code 201 (object response) → AuthExpired.
            val parsedCode = parsed.code
            if (parsedCode == ErrorCodes.SESSION_EXPIRED) return ApiResult.AuthExpired
            if (parsedCode != null && parsedCode != ErrorCodes.OK) {
                return ApiResult.Error(
                    errorCode = parsedCode,
                    message = parsed.msg ?: "Lỗi $parsedCode",
                    httpStatus = response.code(),
                )
            }

            val items = parsed.items
                .mapNotNull { it.toDomainOrNull() }
                .let { applyExtFilter(it, ext) }
            ApiResult.Success(items)
        } catch (e: IOException) {
            Timber.w(e, "getFolderList network error")
            ApiResult.NetworkError
        } catch (t: Throwable) {
            Timber.e(t, "getFolderList unexpected throwable")
            ApiResult.Error(
                errorCode = ErrorCodes.PARSE_ERROR,
                message = t.message ?: "Lỗi xử lý response",
                httpStatus = 0,
            )
        }
    }

    override suspend fun getFile(linkcode: String): ApiResult<FileItem> =
        getFileInternal(linkcode)

    override suspend fun getFileMetadata(linkcode: String): ApiResult<FileItem> =
        getFileInternal(linkcode)

    override suspend fun searchFiles(
        query: String,
        path: String?,
        ext: String?,
        page: Int,
        perPage: Int,
    ): ApiResult<List<FileItem>> {
        // Search rewire qua SearchRepository (Timfshare). FilesRepository.searchFiles
        // giữ stub để không break interface contract.
        return ApiResult.Error(
            errorCode = ErrorCodes.UNKNOWN,
            message = "Search dùng SearchRepository (Timfshare partner)",
            httpStatus = 501,
        )
    }

    override suspend fun listFolderTree(): ApiResult<List<FileItem>> {
        // Legacy không có endpoint cây thư mục root.
        return ApiResult.Success(emptyList())
    }

    // ─── Internal: getFile ─────────────────────────────────────────────

    private suspend fun getFileInternal(linkcode: String): ApiResult<FileItem> {
        val token = authStore.token() ?: return ApiResult.AuthExpired
        val url = "https://www.fshare.vn/file/$linkcode"
        val result = apiCall(moshi) {
            filesApi.getFile(GetFileRequestDto(token = token, url = url))
        }
        return result.map { it.flatten().toDomain(linkcode) }
    }

    // ─── Folder list manual parser ─────────────────────────────────────

    /**
     * Parse response của /api/fileops/getFolderList. Hỗ trợ 5 shape:
     *  1. Array thuần các item.
     *  2. Object với top-level array field tên items/folders/files.
     *  3. Object với nested data chứa array.
     *  4. Array rỗng.
     *  5. Object error chứa code 201/404 etc.
     *
     * @return FolderListParseResult; KHÔNG throw exception (catch trong caller).
     */
    private fun parseFolderListResponse(json: String): FolderListParseResult {
        val source = Buffer().writeUtf8(json)
        val reader = JsonReader.of(source)
        return when (reader.peek()) {
            JsonReader.Token.BEGIN_ARRAY -> {
                val items = readItemArray(reader)
                FolderListParseResult(code = null, msg = null, items = items)
            }
            JsonReader.Token.BEGIN_OBJECT -> readObject(reader)
            else -> FolderListParseResult(items = emptyList())
        }
    }

    /** Đọc JSON array các FolderListItem. */
    private fun readItemArray(reader: JsonReader): List<FolderListItem> {
        val items = mutableListOf<FolderListItem>()
        reader.beginArray()
        while (reader.hasNext()) {
            when (reader.peek()) {
                JsonReader.Token.BEGIN_OBJECT -> readSingleItem(reader)?.let { items += it }
                else -> reader.skipValue()
            }
        }
        reader.endArray()
        return items
    }

    /** Đọc object — extract code/msg/items/folders/files/data. */
    private fun readObject(reader: JsonReader): FolderListParseResult {
        var code: String? = null
        var msg: String? = null
        val collected = mutableListOf<FolderListItem>()

        reader.beginObject()
        while (reader.hasNext()) {
            when (reader.nextName()) {
                "code" -> code = readCode(reader)
                "msg", "message" -> msg = readNullableString(reader)
                "items", "folders", "files" -> when (reader.peek()) {
                    JsonReader.Token.BEGIN_ARRAY -> collected += readItemArray(reader)
                    else -> reader.skipValue()
                }
                "data" -> {
                    // Nested object/array
                    when (reader.peek()) {
                        JsonReader.Token.BEGIN_ARRAY -> collected += readItemArray(reader)
                        JsonReader.Token.BEGIN_OBJECT -> {
                            // data: {items/folders/files: [...]}
                            reader.beginObject()
                            while (reader.hasNext()) {
                                when (reader.nextName()) {
                                    "items", "folders", "files" -> when (reader.peek()) {
                                        JsonReader.Token.BEGIN_ARRAY -> collected += readItemArray(reader)
                                        else -> reader.skipValue()
                                    }
                                    else -> reader.skipValue()
                                }
                            }
                            reader.endObject()
                        }
                        else -> reader.skipValue()
                    }
                }
                else -> reader.skipValue()
            }
        }
        reader.endObject()
        return FolderListParseResult(code = code, msg = msg, items = collected)
    }

    private fun readSingleItem(reader: JsonReader): FolderListItem? {
        var name: String? = null
        var linkcode: String? = null
        var type: String? = null
        var size: String? = null
        var id: String? = null
        var thumbnail: String? = null
        var description: String? = null

        reader.beginObject()
        while (reader.hasNext()) {
            when (reader.nextName()) {
                "name" -> name = readNullableString(reader)
                "linkcode" -> linkcode = readNullableString(reader)
                "type" -> type = readIntOrString(reader)
                "size" -> size = readIntOrString(reader)
                "id" -> id = readIntOrString(reader)
                "thumbnail" -> thumbnail = readNullableString(reader)
                "description" -> description = readNullableString(reader)
                else -> reader.skipValue()
            }
        }
        reader.endObject()

        if (name == null && linkcode == null) return null
        return FolderListItem(
            name = name,
            linkcode = linkcode,
            type = type,
            size = size,
            id = id,
            thumbnail = thumbnail,
            description = description,
        )
    }

    /** Đọc code field — có thể là number, string hoặc null. */
    private fun readCode(reader: JsonReader): String? = when (reader.peek()) {
        JsonReader.Token.NUMBER -> reader.nextLong().toString()
        JsonReader.Token.STRING -> reader.nextString()
        JsonReader.Token.NULL -> {
            reader.nextNull<Any?>()
            null
        }
        else -> {
            reader.skipValue()
            null
        }
    }

    private fun readIntOrString(reader: JsonReader): String? = when (reader.peek()) {
        JsonReader.Token.NUMBER -> reader.nextLong().toString()
        JsonReader.Token.STRING -> reader.nextString()
        JsonReader.Token.NULL -> {
            reader.nextNull<Any?>()
            null
        }
        else -> {
            reader.skipValue()
            null
        }
    }

    private fun readNullableString(reader: JsonReader): String? = when (reader.peek()) {
        JsonReader.Token.STRING -> reader.nextString()
        JsonReader.Token.NUMBER -> reader.nextLong().toString()
        JsonReader.Token.NULL -> {
            reader.nextNull<Any?>()
            null
        }
        else -> {
            reader.skipValue()
            null
        }
    }

    // ─── Helpers ──────────────────────────────────────────────────────

    private fun applyExtFilter(items: List<FileItem>, ext: String?): List<FileItem> {
        if (ext.isNullOrBlank()) return items
        val needles = ext.split(',', ' ').map { it.trim().lowercase() }.filter { it.isNotEmpty() }
        if (needles.isEmpty()) return items
        return items.filter { item ->
            item.type == ItemType.FOLDER ||
                needles.any { item.name.endsWith(".$it", ignoreCase = true) }
        }
    }
}

// ─── DTO → Domain mapping ─────────────────────────────────────────────

private fun FolderListItem.toDomainOrNull(): FileItem? {
    val lc = linkcode?.takeIf { it.isNotBlank() } ?: return null
    val nm = name?.takeIf { it.isNotBlank() } ?: return null
    val itemType = if (type == "0") ItemType.FOLDER else ItemType.FILE
    val sz = size?.toLongOrNull() ?: 0L
    return FileItem(
        linkcode = lc,
        name = nm,
        type = itemType,
        size = sz,
        // Path = linkcode để khi click subfolder, navigation pass linkcode (canonicalFolderUrl
        // sẽ accept cả linkcode trần lẫn full URL).
        path = lc,
        isPublic = false,
        lastUpdatedAt = null,
        thumbnail = thumbnail,
        description = description,
    )
}

/** Merge top-level + nested data/file fields. */
private fun GetFileResponseDto.flatten(): GetFileResponseDto {
    val nested = (data ?: file)?.flatten()
    if (nested == null) return this
    return GetFileResponseDto(
        code = code ?: nested.code,
        msg = msg ?: nested.msg,
        name = name ?: nested.name,
        filename = filename ?: nested.filename,
        fileName = fileName ?: nested.fileName,
        size = size ?: nested.size,
        fileSize = fileSize ?: nested.fileSize,
        filesize = filesize ?: nested.filesize,
        humanSize = humanSize ?: nested.humanSize,
        linkcode = linkcode ?: nested.linkcode,
        url = url ?: nested.url,
        thumbnail = thumbnail ?: nested.thumbnail,
        description = description ?: nested.description,
        mimeType = mimeType ?: nested.mimeType,
        durationMs = durationMs ?: nested.durationMs,
        isPasswordProtected = isPasswordProtected ?: nested.isPasswordProtected,
        data = null,
        file = null,
        message = message ?: nested.message,
        error = error ?: nested.error,
        errorMsg = errorMsg ?: nested.errorMsg,
    )
}

private fun GetFileResponseDto.toDomain(fallbackLinkcode: String): FileItem {
    val effectiveName = name ?: filename ?: fileName ?: "Unknown"
    val effectiveSize = size ?: fileSize ?: filesize ?: 0L
    val effectiveLinkcode = linkcode ?: fallbackLinkcode
    return FileItem(
        linkcode = effectiveLinkcode,
        name = effectiveName,
        type = ItemType.FILE,
        size = effectiveSize,
        path = "",
        isPublic = false,
        lastUpdatedAt = null,
        mimeType = mimeType,
        durationMs = durationMs,
        thumbnail = thumbnail,
        description = description,
        isPasswordProtected = isPasswordProtected ?: false,
    )
}

/**
 * Chuẩn hoá path/URL → canonical folder URL. Trả null nếu không trích được linkcode.
 *
 * Chấp nhận:
 *  - Full URL: `https://www.fshare.vn/folder/ABC123?...`
 *  - Path tương đối: `/folder/ABC123`
 *  - Linkcode trần: `ABC123`
 */
private fun canonicalFolderUrl(input: String): String? {
    val trimmed = input.trim()
    if (trimmed.isEmpty()) return null
    val regex = Regex("""fshare\.vn/folder/([A-Za-z0-9]+)""")
    val match = regex.find(trimmed)?.groupValues?.getOrNull(1)
    val linkcode = match ?: run {
        if (trimmed.matches(Regex("""[A-Za-z0-9]{6,}"""))) trimmed else null
    }
    return linkcode?.let { "https://www.fshare.vn/folder/$it" }
}
