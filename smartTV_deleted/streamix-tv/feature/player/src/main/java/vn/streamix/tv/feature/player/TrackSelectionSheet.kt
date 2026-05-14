/*
 * StreamIX TV — S5b Track Selection (subtitle / audio chooser)
 * Reference: 13 §12 + 14 §7 (S5b 6 states)
 *
 * Hai variant: Subtitle (có "Tắt phụ đề" option), Audio (luôn có ít nhất 1 track).
 */
package vn.streamix.tv.feature.player

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import vn.streamix.tv.core.ui.components.FsBottomSheet
import vn.streamix.tv.core.ui.components.FsRadioItem

enum class TrackKind { Subtitle, Audio }

data class PlayerTrack(
    /** Group index (ExoPlayer track group) */
    val groupIndex: Int,
    /** Track index trong group */
    val trackIndex: Int,
    val language: String,
    val codec: String? = null,
    val isSelected: Boolean,
)

@Composable
fun TrackSelectionSheet(
    visible: Boolean,
    kind: TrackKind,
    tracks: List<PlayerTrack>,
    onSelect: (PlayerTrack?) -> Unit,    // null = "Tắt phụ đề" (chỉ kind=Subtitle)
    onDismiss: () -> Unit,
) {
    val title = stringResource(
        if (kind == TrackKind.Subtitle)
            vn.streamix.tv.core.ui.R.string.track_select_subtitle_title
        else
            vn.streamix.tv.core.ui.R.string.track_select_audio_title
    )

    FsBottomSheet(visible = visible, title = title, onDismiss = onDismiss) {
        LazyColumn(modifier = Modifier.fillMaxWidth()) {
            // "Tắt phụ đề" option chỉ cho subtitle
            if (kind == TrackKind.Subtitle) {
                item {
                    FsRadioItem(
                        label = stringResource(vn.streamix.tv.core.ui.R.string.track_select_subtitle_off),
                        selected = tracks.none { it.isSelected },
                        onSelect = { onSelect(null); onDismiss() },
                    )
                }
            }
            items(tracks, key = { "${it.groupIndex}-${it.trackIndex}" }) { track ->
                FsRadioItem(
                    label = formatTrackLabel(track),
                    selected = track.isSelected,
                    onSelect = { onSelect(track); onDismiss() },
                )
            }
        }
    }
}

private fun formatTrackLabel(t: PlayerTrack): String {
    val lang = t.language.takeIf { it.isNotBlank() } ?: "(không rõ)"
    val codec = t.codec?.takeIf { it.isNotBlank() }?.let { " ($it)" } ?: ""
    return "$lang$codec"
}
