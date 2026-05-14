package vn.streamix.tv.core.data.local.room.dao

import androidx.room.Dao
import androidx.room.Query
import androidx.room.Upsert
import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.data.local.room.entity.PlaybackPositionEntity
import vn.streamix.tv.core.data.local.room.entity.RecentSearchEntity
import vn.streamix.tv.core.data.local.room.entity.UserCacheEntity

@Dao
interface UserCacheDao {
    @Query("SELECT * FROM user_cache LIMIT 1")
    fun observe(): Flow<UserCacheEntity?>

    @Query("SELECT * FROM user_cache LIMIT 1")
    suspend fun get(): UserCacheEntity?

    @Upsert
    suspend fun upsert(user: UserCacheEntity)

    @Query("DELETE FROM user_cache")
    suspend fun clear()
}

@Dao
interface PlaybackPositionDao {
    @Query("SELECT * FROM playback_position WHERE linkcode = :linkcode")
    fun observe(linkcode: String): Flow<PlaybackPositionEntity?>

    @Query("""
        SELECT * FROM playback_position
        WHERE positionMs > 30000 AND positionMs < (durationMs - 60000)
        ORDER BY updatedAt DESC LIMIT :limit
    """)
    fun observeContinueWatching(limit: Int): Flow<List<PlaybackPositionEntity>>

    @Upsert
    suspend fun upsert(position: PlaybackPositionEntity)

    @Query("DELETE FROM playback_position WHERE linkcode = :linkcode")
    suspend fun delete(linkcode: String)
}

@Dao
interface RecentSearchDao {
    @Query("SELECT * FROM recent_search ORDER BY searchedAt DESC LIMIT :limit")
    fun observe(limit: Int = 10): Flow<List<RecentSearchEntity>>

    @Upsert
    suspend fun upsert(item: RecentSearchEntity)

    @Query("DELETE FROM recent_search WHERE keyword = :keyword")
    suspend fun delete(keyword: String)

    @Query("DELETE FROM recent_search WHERE keyword NOT IN (SELECT keyword FROM recent_search ORDER BY searchedAt DESC LIMIT :keep)")
    suspend fun trim(keep: Int = 10)
}
