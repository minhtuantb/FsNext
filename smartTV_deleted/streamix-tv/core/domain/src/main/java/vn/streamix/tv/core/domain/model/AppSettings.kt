/*
 * StreamIX TV — App settings (persisted in DataStore)
 * Reference: 13 §17 Settings Playback
 */
package vn.streamix.tv.core.domain.model

data class AppSettings(
    val playerEngine: PlayerEngine = PlayerEngine.EXOPLAYER,
    val audioPassthroughHdmi: Boolean = false,
    val autoPlayNext: Boolean = false,
    val savePlaybackPosition: Boolean = true,
    val subtitleSize: SubtitleSize = SubtitleSize.MEDIUM,
    val subtitleColor: SubtitleColor = SubtitleColor.WHITE,
    val subtitleOutline: Boolean = true,
    val onboardingCompleted: Boolean = false,
    val locale: String = "vi",     // "vi" or "en"
)

enum class PlayerEngine { EXOPLAYER, LIBVLC }

enum class SubtitleSize(val sp: Int) {
    SMALL(20), MEDIUM(28), LARGE(36), XLARGE(48)
}

enum class SubtitleColor(val hex: String) {
    WHITE("#FFFFFF"),
    YELLOW("#FFEB3B"),
    ORANGE("#FFB12C"),
}
