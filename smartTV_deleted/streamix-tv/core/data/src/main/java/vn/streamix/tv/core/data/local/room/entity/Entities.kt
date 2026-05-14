/*
 * StreamIX TV — Room entities
 */
package vn.streamix.tv.core.data.local.room.entity

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "user_cache")
data class UserCacheEntity(
    @PrimaryKey val id: String,
    val email: String,
    val fullName: String,
    val phone: String?,
    val isVip: Boolean,
    val vipLevel: Int,
    val vipExpiresAt: String?,
    val storageUsed: Long,
    val storageQuota: Long,
    val createdAt: String?,
    val cachedAt: Long,
)

@Entity(tableName = "playback_position")
data class PlaybackPositionEntity(
    @PrimaryKey val linkcode: String,
    val positionMs: Long,
    val durationMs: Long,
    val updatedAt: Long,
    val fileName: String,
    val thumbnail: String?,
)

@Entity(tableName = "recent_search")
data class RecentSearchEntity(
    @PrimaryKey val keyword: String,
    val searchedAt: Long,
)
