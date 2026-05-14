/*
 * StreamIX TV — QR code generator dùng zxing-core (component C22)
 * Reference: 09 §8 (C22 QRCard) + 13 §19 (S7f Update guide)
 */
package vn.streamix.tv.core.ui.dialogs

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.google.zxing.BarcodeFormat
import com.google.zxing.EncodeHintType
import com.google.zxing.qrcode.QRCodeWriter
import com.google.zxing.qrcode.decoder.ErrorCorrectionLevel
import vn.streamix.tv.core.ui.theme.FsRadius
import vn.streamix.tv.core.ui.theme.FsSpacing

@Composable
fun QrCodeView(
    payload: String,
    sizeDp: Int = 240,
    backgroundColor: Color = Color.White,
    foregroundColor: Color = Color.Black,
) {
    val matrix = remember(payload) {
        runCatching {
            QRCodeWriter().encode(
                payload,
                BarcodeFormat.QR_CODE,
                512, 512,
                mapOf(EncodeHintType.ERROR_CORRECTION to ErrorCorrectionLevel.M),
            )
        }.getOrNull()
    }

    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(FsRadius.Md))
            .background(backgroundColor)
            .padding(FsSpacing.S3)
            .size(sizeDp.dp),
    ) {
        if (matrix != null) {
            Canvas(modifier = Modifier.size(sizeDp.dp)) {
                val cellSize = size.width / matrix.width
                for (x in 0 until matrix.width) {
                    for (y in 0 until matrix.height) {
                        if (matrix[x, y]) {
                            drawRect(
                                color = foregroundColor,
                                topLeft = Offset(x * cellSize, y * cellSize),
                                size = Size(cellSize, cellSize),
                            )
                        }
                    }
                }
            }
        }
    }
}
