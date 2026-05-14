/*
 * StreamIX TV — Playback position (Resume Watching)
 * Reference: 13 §10 Player; 14 §11 Continue Watching
 */
package vn.streamix.tv.core.domain.model

data class PlaybackPosition(
    val linkcode: String,
    val positionMs: Long,
    val durationMs: Long,
    val updatedAt: Long,            // epoch millis
    /** Cache cơ bản để hiển thị Continue Watching không cần re-fetch /me + file detail */
    val fileName: String,
    val thumbnail: String? = null,
) {
    val percent: Int
        get() = if (durationMs > 0) ((positionMs * 100) / durationMs).toInt().coerceIn(0, 100)
        else 0

    /** Có hiện trong row "Tiếp tục xem" không? */
    val isResumable: Boolean
        get() = positionMs > 30_000 && positionMs < (durationMs - 60_000)
}
