/*
 * StreamIX TV — C16 TextField (login email/password)
 * Reference: 09 §8 (C16) + 13 §6 S1 Login
 */
package vn.streamix.tv.core.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import vn.streamix.tv.core.ui.theme.FsColors
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing
import vn.streamix.tv.core.ui.theme.FsType

@Composable
fun FsTextField(
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    placeholder: String = "",
    isError: Boolean = false,
    isPassword: Boolean = false,
    keyboardType: KeyboardType = KeyboardType.Text,
    imeAction: ImeAction = ImeAction.Next,
    leadingIcon: (@Composable () -> Unit)? = null,
    trailingIcon: (@Composable () -> Unit)? = null,
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isFocused by interactionSource.collectIsFocusedAsState()

    val borderColor = when {
        isError -> FsColors.Danger
        isFocused -> FsColors.AccentPrimary
        else -> FsColors.BorderDefault
    }
    val borderWidth = if (isFocused || isError) 4.dp else 1.dp

    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(72.dp)
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(FsColors.BgSurface)
            .border(borderWidth, borderColor, RoundedCornerShape(FsRadius.Md))
            .padding(horizontal = FsSpacing.S4),
        contentAlignment = Alignment.CenterStart,
    ) {
        Box(modifier = Modifier.padding(start = if (leadingIcon != null) 40.dp else 0.dp,
                                        end = if (trailingIcon != null) 40.dp else 0.dp)) {
            BasicTextField(
                value = value,
                onValueChange = onValueChange,
                singleLine = true,
                textStyle = FsType.BodyLg.copy(color = FsColors.TextPrimary),
                cursorBrush = SolidColor(FsColors.AccentPrimary),
                keyboardOptions = KeyboardOptions(keyboardType = keyboardType, imeAction = imeAction),
                visualTransformation = if (isPassword) PasswordVisualTransformation() else VisualTransformation.None,
                interactionSource = interactionSource,
                modifier = Modifier.fillMaxWidth(),
                decorationBox = { inner ->
                    if (value.isEmpty()) {
                        Text(placeholder, style = FsType.BodyLg, color = FsColors.TextTertiary)
                    }
                    inner()
                },
            )
        }
        leadingIcon?.let { Box(modifier = Modifier.align(Alignment.CenterStart)) { it() } }
        trailingIcon?.let { Box(modifier = Modifier.align(Alignment.CenterEnd)) { it() } }
    }
}
