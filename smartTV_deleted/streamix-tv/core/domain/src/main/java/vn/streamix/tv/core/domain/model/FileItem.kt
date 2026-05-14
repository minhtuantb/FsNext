/*
 * StreamIX TV — File / Folder domain model
 * Reference: openapi.yaml §/api/v1/files; 13 §3.1
 */
package vn.streamix.tv.core.domain.model

data class FileItem(
    val linkcode: String,           // 12 chars, pattern ^[A-Z0-9]{12}$
    val name: String,
    val type: ItemType,             // file or folder
    val size: Long,                 // bytes; 0 cho folder
    val path: String,               // full path từ root, vd "/Movies/Action/movie.mkv"
    val isPublic: Boolean = false,
    val lastUpdatedAt: String? = null,    // ISO 8601
    val mimeType: String? = null,         // chỉ có khi gọi /metadata
    val durationMs: Long? = null,         // chỉ có với file video/audio
    val thumbnail: String? = null,        // URL thumbnail nếu backend cung cấp
    val description: String? = null,
    val isPasswordProtected: Boolean = false,
    val isFavorite: Boolean = false,      // V1 chỉ persist local Room
)

enum class ItemType { FILE, FOLDER }

/** Phân loại file theo extension để chọn icon + xác định có "playable" trong Player không */
enum class FileCategory {
    VIDEO, AUDIO, IMAGE, ARCHIVE, DOCUMENT, OTHER;

    companion object {
        fun fromExtension(name: String): FileCategory {
            val ext = name.substringAfterLast('.', "").lowercase()
            return when (ext) {
                "mp4", "mkv", "webm", "avi", "flv", "mov", "wmv", "m4v", "ts", "rmvb" -> VIDEO
                "mp3", "flac", "wav", "aac", "ogg", "m4a", "wma" -> AUDIO
                "jpg", "jpeg", "png", "webp", "gif", "bmp", "heic" -> IMAGE
                "zip", "rar", "7z", "tar", "gz" -> ARCHIVE
                "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "txt" -> DOCUMENT
                else -> OTHER
            }
        }

        fun isPlayable(category: FileCategory) = category == VIDEO || category == AUDIO
    }
}
