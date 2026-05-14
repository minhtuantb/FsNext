/*
 * StreamIX TV — Room database
 * Reference: 13 §4.7
 */
package vn.streamix.tv.core.data.local.room

import androidx.room.Database
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import vn.streamix.tv.core.data.local.room.dao.PlaybackPositionDao
import vn.streamix.tv.core.data.local.room.dao.RecentSearchDao
import vn.streamix.tv.core.data.local.room.dao.UserCacheDao
import vn.streamix.tv.core.data.local.room.entity.PlaybackPositionEntity
import vn.streamix.tv.core.data.local.room.entity.RecentSearchEntity
import vn.streamix.tv.core.data.local.room.entity.UserCacheEntity

@Database(
    entities = [
        UserCacheEntity::class,
        PlaybackPositionEntity::class,
        RecentSearchEntity::class,
    ],
    version = 1,
    exportSchema = false,   // V1 dev: tắt export schema để tránh warning. V2 enable khi có migration.
)
@TypeConverters
abstract class StreamIXDatabase : RoomDatabase() {
    abstract fun userCacheDao(): UserCacheDao
    abstract fun playbackPositionDao(): PlaybackPositionDao
    abstract fun recentSearchDao(): RecentSearchDao

    companion object {
        const val DB_NAME = "streamix-tv.db"
    }
}
