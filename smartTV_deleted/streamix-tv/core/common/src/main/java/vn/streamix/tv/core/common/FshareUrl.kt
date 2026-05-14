/*
 * StreamIX TV — Helper chuẩn hoá URL Fshare.
 *
 * Logic copy từ StreamIX desktop (`normalizeFshareUrl`) — Phải tương đương để URL
 * gửi vào /api/session/download và /api/fileops/get khớp với canonical form server expect.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §3.6 (Top page parser) + best-practice §8.
 */
package vn.streamix.tv.core.common

object FshareUrl {

    /**
     * Chuẩn hoá URL Fshare:
     *  - Đổi http → https.
     *  - Thêm `www.` nếu thiếu.
     *  - Bỏ phần fragment (`#...`).
     *  - Giữ nguyên query (caller có thể chọn cắt nếu cần).
     *
     * Trả null nếu input không phải URL Fshare.
     */
    fun normalize(input: String?): String? {
        if (input.isNullOrBlank()) return null
        var url = input.trim()

        // Cắt fragment
        url = url.substringBefore('#')

        // http → https
        if (url.startsWith("http://", ignoreCase = true)) {
            url = "https://" + url.removePrefix("http://").removePrefix("HTTP://")
        }

        // Bổ sung www. nếu host là "fshare.vn" trần
        if (url.startsWith("https://fshare.vn/", ignoreCase = true)) {
            url = "https://www." + url.removePrefix("https://")
        }

        if (!url.contains("fshare.vn", ignoreCase = true)) return null
        return url
    }

    /** Cắt query string khỏi URL. */
    fun stripQuery(url: String): String {
        val q = url.indexOf('?')
        return if (q < 0) url else url.substring(0, q)
    }

    /** Trích linkcode từ URL `https://www.fshare.vn/{file|folder}/<LINKCODE>?...`. */
    fun extractLinkcode(url: String?): String? {
        if (url.isNullOrBlank()) return null
        val regex = Regex("""fshare\.vn/(?:file|folder)/([A-Za-z0-9]+)""")
        return regex.find(url)?.groupValues?.getOrNull(1)
    }

    /** Có phải URL folder không (vs file)? Folder = không playable. */
    fun isFolderUrl(url: String?): Boolean {
        if (url.isNullOrBlank()) return false
        return url.contains("fshare.vn/folder/", ignoreCase = true)
    }

    /** Có phải URL file không? */
    fun isFileUrl(url: String?): Boolean {
        if (url.isNullOrBlank()) return false
        return url.contains("fshare.vn/file/", ignoreCase = true)
    }

    /** Build canonical folder URL từ linkcode. */
    fun folderUrl(linkcode: String): String = "https://www.fshare.vn/folder/$linkcode"

    /** Build canonical file URL từ linkcode. */
    fun fileUrl(linkcode: String): String = "https://www.fshare.vn/file/$linkcode"
}
