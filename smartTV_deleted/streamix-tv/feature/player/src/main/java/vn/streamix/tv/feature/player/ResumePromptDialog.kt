/*
 * StreamIX TV — S5d Resume Prompt
 * Reference: 13 §14
 *
 * Hiển thị khi vào Player và có saved position 30s < pos < duration−60s.
 * Dialog medium với 2 nút: "Bắt đầu lại" / "Tiếp tục" (default focus).
 */
package vn.streamix.tv.feature.player

import androidx.compose.runtime.Composable
import androidx.compose.ui.res.stringResource
import vn.streamix.tv.core.common.TimeFormat
import vn.streamix.tv.core.ui.components.FsDialog

@Composable
fun ResumePromptDialog(
    visible: Boolean,
    positionMs: Long,
    onResume: () -> Unit,
    onRestart: () -> Unit,
) {
    FsDialog(
        visible = visible,
        title = stringResource(vn.streamix.tv.core.ui.R.string.resume_prompt_title, TimeFormat.forPlayer(positionMs)),
        cancelLabel = stringResource(vn.streamix.tv.core.ui.R.string.resume_prompt_action_restart),
        confirmLabel = stringResource(vn.streamix.tv.core.ui.R.string.resume_prompt_action_resume),
        defaultFocusOnConfirm = true,
        onConfirm = onResume,
        onDismiss = onRestart,
    )
}
