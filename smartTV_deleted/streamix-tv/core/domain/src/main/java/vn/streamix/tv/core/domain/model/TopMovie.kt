/*
 * StreamIX TV — Top movie ranking item.
 *
 * Mỗi entry trong Top 100 có rank, tên, năm, size, source ranking (IMDB/RT/Fshare).
 * Source data: sheet thứ 3 do team biên tập (sheet "Top") hoặc hardcoded fallback V1.
 */
package vn.streamix.tv.core.domain.model

data class TopMovie(
    /** Vị trí 1..100. */
    val rank: Int,
    val title: String,
    val year: Int? = null,
    /** Size bytes — null nếu không biết. */
    val size: Long? = null,
    /** Container/codec hint: "MKV", "MP4"... */
    val format: String? = null,
    /** Full Fshare URL (file). */
    val url: String,
    val linkcode: String?,
    val thumbnail: String? = null,
    val source: TopSource,
)

enum class TopSource {
    /** Top theo IMDB rating (manual edit). */
    IMDB,
    /** Top theo Rotten Tomatoes. */
    RT,
    /** Top theo Fshare (lượt download). */
    FSHARE,
}
