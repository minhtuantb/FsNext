/*
 * StreamIX TV — C8 Dialog template (Standard / Destructive / Loading / Info)
 * Reference: 09 §8 (C8) + 14 §5 (D1-D7) + 13 §21
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

enum class FsDialogVariant { Standard, Destructive, Loading, Info }

/**
 * Dialog template chuẩn — component C8.
 *
 * @param visible Có hiển thị dialog không
 * @param title Tiêu đề (display ở title-lg style)
 * @param body Nội dung mô tả (body-lg)
 * @param confirmLabel Label nút phải; nếu null = dialog Info chỉ có Close
 * @param cancelLabel Label nút trái; nếu null = chỉ 1 nút
 * @param variant Standard (focus mặc định "Cancel") / Destructive (nút phải đỏ) / Loading (button loading + back disabled) / Info
 * @param defaultFocusOnConfirm true = focus vào Confirm; false = focus Cancel (Standard)
 * @param onConfirm Callback OK
 * @param onDismiss Callback Cancel hoặc Back
 * @param extraContent Optional content giữa body và footer (vd. password TextField cho D4)
 */
@Composable
fun FsDialog(
    visible: Boolean,
    title: String,
    body: String? = null,
    confirmLabel: String? = null,
    cancelLabel: String? = null,
    variant: FsDialogVariant = FsDialogVariant.Standard,
    defaultFocusOnConfirm: Boolean = false,
    onConfirm: () -> Unit = {},
    onDismiss: () -> Unit = {},
    extraContent: @Composable (() -> Unit)? = null,
) {
    AnimatedVisibility(
        visible = visible,
        enter = fadeIn() + scaleIn(initialScale = 0.95f),
        exit = fadeOut() + scaleOut(targetScale = 0.95f),
    ) {
        // Backdrop scrim — click ngoài = dismiss (trừ Loading)
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(FsColors.BgScrim)
                .clickable(
                    interactionSource = remember { MutableInteractionSource() },
                    indication = null,
                    enabled = variant != FsDialogVariant.Loading,
                    onClick = onDismiss,
                ),
            contentAlignment = Alignment.Center,
        ) {
            // Dialog container
            Column(
                modifier = Modifier
                    .widthIn(max = 600.dp)
                    .clip(RoundedCornerShape(FsRadius.Lg))
                    .background(FsColors.BgSurface)
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null,
                        onClick = {},  // tránh click xuyên xuống backdrop
                    )
                    .padding(FsSpacing.S6),
            ) {
                Text(text = title, style = FsType.TitleLg, color = FsColors.TextPrimary)

                if (!body.isNullOrBlank()) {
                    Spacer(Modifier.height(FsSpacing.S3))
                    Text(text = body, style = FsType.BodyLg, color = FsColors.TextSecondary)
                }

                if (extraContent != null) {
                    Spacer(Modifier.height(FsSpacing.S5))
                    extraContent()
                }

                Spacer(Modifier.height(FsSpacing.S6))

                // Footer buttons
                val cancelFocus = remember { FocusRequester() }
                val confirmFocus = remember { FocusRequester() }
                LaunchedEffect(visible) {
                    if (visible) {
                        if (defaultFocusOnConfirm || cancelLabel == null) confirmFocus.requestFocus()
                        else cancelFocus.requestFocus()
                    }
                }

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(FsSpacing.S3, Alignment.End),
                ) {
                    if (cancelLabel != null) {
                        FsButton(
                            text = cancelLabel,
                            onClick = onDismiss,
                            variant = FsButtonVariant.Secondary,
                            modifier = Modifier.focusRequester(cancelFocus),
                        )
                    }
                    if (confirmLabel != null) {
                        FsButton(
                            text = confirmLabel,
                            onClick = onConfirm,
                            variant = when (variant) {
                                FsDialogVariant.Destructive -> FsButtonVariant.Destructive
                                FsDialogVariant.Loading -> FsButtonVariant.Primary
                                else -> FsButtonVariant.Primary
                            },
                            loading = variant == FsDialogVariant.Loading,
                            modifier = Modifier.focusRequester(confirmFocus),
                        )
                    }
                }
            }
        }
    }
}
