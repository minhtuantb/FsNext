/*
 * StreamIX TV — Google Sheets CSV export endpoint.
 *
 * URL pattern:
 *   https://docs.google.com/spreadsheets/d/{file_id}/export?format=csv&gid={gid}
 *
 * Sheet phải share "Anyone with the link can view" (theo team biên tập), không cần OAuth.
 *
 * Response: text/csv, text-encoding UTF-8.
 *
 * Reference: API_REFERENCE_FOR_REUSE.md §5.
 */
package vn.streamix.tv.core.network.service

import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Path
import retrofit2.http.Query

interface SheetsApi {

    /**
     * Tải nội dung sheet dạng CSV. Trả raw String (đã được ScalarsConverterFactory parse).
     */
    @GET("spreadsheets/d/{fileId}/export")
    suspend fun exportCsv(
        @Path("fileId") fileId: String,
        @Query("format") format: String = "csv",
        @Query("gid") gid: String,
    ): Response<String>
}
