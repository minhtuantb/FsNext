/*
 * StreamIX TV — Featured / Suggested / Trending item.
 *
 * Thay thế cho `FileItem` ở những context content được biên tập từ Sheets/Timfshare:
 *  - Sheets: tên hiển thị + URL (file hoặc folder); cờ playable.
 *  - Timfshare search/trending: name + linkcode/url + size.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §4 + §5.
 */
package vn.streamix.tv.core.domain.model

data class FeaturedItem(
    /** Tên hiển thị (label sheet hoặc name từ Timfshare). */
    val title: String,
    /** Full Fshare URL. Có thể là file hoặc folder. */
    val url: String,
    /** Linkcode trích từ URL (nếu có). */
    val linkcode: String?,
    /** Có thể play trong app không? `false` = folder hoặc extension không hỗ trợ. */
    val isPlayable: Boolean,
    /** Bytes — null nếu không biết. */
    val size: Long? = null,
    /** Thumbnail URL — null nếu source không cung cấp. */
    val thumbnail: String? = null,
    /** Loại nguồn (để analytics + UI biết hiển thị). */
    val source: FeaturedSource,
)

enum class FeaturedSource {
    /** Sheet "Gợi ý" — biên tập thủ công. */
    SUGGESTED,
    /** Sheet "Xu hướng" / Timfshare /api/key/data-top. */
    TRENDING,
    /** Timfshare /v1/string-query-search. */
    SEARCH,
}
