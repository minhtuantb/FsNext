/*
 * StreamIX TV — Settings DataStore wrapper
 */
package vn.streamix.tv.core.data.local

import android.content.Context
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.map
import vn.streamix.tv.core.domain.model.AppSettings
import vn.streamix.tv.core.domain.model.PlayerEngine
import vn.streamix.tv.core.domain.model.SubtitleColor
import vn.streamix.tv.core.domain.model.SubtitleSize
import javax.inject.Inject
import javax.inject.Singleton

private val Context.settingsDataStore by preferencesDataStore("streamix_settings")

@Singleton
class SettingsDataStore @Inject constructor(
    @ApplicationContext private val context: Context,
) {
    private val ds get() = context.settingsDataStore

    fun observe(): Flow<AppSettings> = ds.data.map { p -> p.toAppSettings() }

    /**
     * Read current settings, apply transform, persist.
     *
     * BUG FIX: Trước đây dùng `ds.data.collect { ... return@collect }` — `return@collect`
     * chỉ thoát lambda, KHÔNG terminate flow. `ds.data` là long-lived flow nên collect
     * hang forever → app stuck. Sửa: dùng `first()` lấy snapshot rồi unsubscribe.
     */
    suspend fun update(transform: (AppSettings) -> AppSettings) {
        val current = observe().first()
        val updated = transform(current)
        ds.edit { p ->
            p[Keys.PLAYER_ENGINE] = updated.playerEngine.name
            p[Keys.AUDIO_PASSTHROUGH] = updated.audioPassthroughHdmi
            p[Keys.AUTO_PLAY_NEXT] = updated.autoPlayNext
            p[Keys.SAVE_POSITION] = updated.savePlaybackPosition
            p[Keys.SUBTITLE_SIZE] = updated.subtitleSize.name
            p[Keys.SUBTITLE_COLOR] = updated.subtitleColor.name
            p[Keys.SUBTITLE_OUTLINE] = updated.subtitleOutline
            p[Keys.ONBOARDING_COMPLETED] = updated.onboardingCompleted
            p[Keys.LOCALE] = updated.locale
        }
    }

    private fun Preferences.toAppSettings(): AppSettings = AppSettings(
        playerEngine = PlayerEngine.entries.firstOrNull { it.name == this[Keys.PLAYER_ENGINE] }
            ?: PlayerEngine.EXOPLAYER,
        audioPassthroughHdmi = this[Keys.AUDIO_PASSTHROUGH] ?: false,
        autoPlayNext = this[Keys.AUTO_PLAY_NEXT] ?: false,
        savePlaybackPosition = this[Keys.SAVE_POSITION] ?: true,
        subtitleSize = SubtitleSize.entries.firstOrNull { it.name == this[Keys.SUBTITLE_SIZE] }
            ?: SubtitleSize.MEDIUM,
        subtitleColor = SubtitleColor.entries.firstOrNull { it.name == this[Keys.SUBTITLE_COLOR] }
            ?: SubtitleColor.WHITE,
        subtitleOutline = this[Keys.SUBTITLE_OUTLINE] ?: true,
        onboardingCompleted = this[Keys.ONBOARDING_COMPLETED] ?: false,
        locale = this[Keys.LOCALE] ?: "vi",
    )

    private object Keys {
        val PLAYER_ENGINE = stringPreferencesKey("player_engine")
        val AUDIO_PASSTHROUGH = booleanPreferencesKey("audio_passthrough")
        val AUTO_PLAY_NEXT = booleanPreferencesKey("auto_play_next")
        val SAVE_POSITION = booleanPreferencesKey("save_position")
        val SUBTITLE_SIZE = stringPreferencesKey("subtitle_size")
        val SUBTITLE_COLOR = stringPreferencesKey("subtitle_color")
        val SUBTITLE_OUTLINE = booleanPreferencesKey("subtitle_outline")
        val ONBOARDING_COMPLETED = booleanPreferencesKey("onboarding_completed")
        val LOCALE = stringPreferencesKey("locale")
    }
}
