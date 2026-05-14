/*
 * StreamIX TV — CSV parser tối thiểu, hỗ trợ quoted-field + `""` escape.
 *
 * Không dùng thư viện vì chỉ cần parse 2 sheet "Gợi ý" / "Xu hướng" — schema đơn giản
 * (cột 0: label, cột 1: URL). StreamIX desktop dùng implementation tương tự (CSV chuẩn).
 *
 * Spec:
 *  - Phân tách dòng bằng `\n` hoặc `\r\n`.
 *  - Phân tách cell bằng `,`.
 *  - Cell bao bằng `"` để chứa `,` hoặc newline.
 *  - Trong quoted cell, `""` = literal `"`.
 *  - Lưu ý: trim cell sau parse — KHÔNG trim quoted content (giữ nguyên).
 */
package vn.streamix.tv.core.common

object CsvParser {

    /** Parse toàn bộ CSV thành list-of-rows, mỗi row là list-of-cells. */
    fun parse(input: String): List<List<String>> {
        val rows = mutableListOf<List<String>>()
        val currentRow = mutableListOf<String>()
        val cell = StringBuilder()
        var inQuotes = false
        var i = 0
        val n = input.length

        while (i < n) {
            val c = input[i]
            when {
                inQuotes -> {
                    when {
                        c == '"' && i + 1 < n && input[i + 1] == '"' -> {
                            cell.append('"')
                            i += 2
                        }
                        c == '"' -> {
                            inQuotes = false
                            i++
                        }
                        else -> {
                            cell.append(c)
                            i++
                        }
                    }
                }
                else -> {
                    when (c) {
                        '"' -> {
                            inQuotes = true
                            i++
                        }
                        ',' -> {
                            currentRow.add(cell.toString())
                            cell.setLength(0)
                            i++
                        }
                        '\r' -> {
                            // CRLF — bỏ qua \r, đợi \n
                            i++
                        }
                        '\n' -> {
                            currentRow.add(cell.toString())
                            cell.setLength(0)
                            rows.add(currentRow.toList())
                            currentRow.clear()
                            i++
                        }
                        else -> {
                            cell.append(c)
                            i++
                        }
                    }
                }
            }
        }

        // Flush cell + row cuối nếu file không kết thúc bằng newline
        if (cell.isNotEmpty() || currentRow.isNotEmpty()) {
            currentRow.add(cell.toString())
            rows.add(currentRow.toList())
        }

        return rows
    }
}
