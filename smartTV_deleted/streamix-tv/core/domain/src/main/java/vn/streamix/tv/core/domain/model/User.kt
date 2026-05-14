/*
 * StreamIX TV — User domain model
 * Reference: D:\Work\fshare-api-gateway\docs\api\v1\me.md
 */
package vn.streamix.tv.core.domain.model

data class User(
    val id: String,
    val email: String,
    val fullName: String,
    val phone: String?,
    val isVip: Boolean,
    val vipLevel: Int,
    val vipExpiresAt: String?,    // ISO 8601 từ API
    val storageUsed: Long,        // bytes
    val storageQuota: Long,       // bytes (-1 = unlimited)
    val createdAt: String?,        // ISO 8601
)
