/*
 * StreamIX TV — Data layer Hilt module.
 *
 * Wave M3: thêm FeaturedRepository (Sheets + Timfshare trending) + SearchRepository (Timfshare).
 */
package vn.streamix.tv.core.data.di

import android.content.Context
import androidx.room.Room
import dagger.Binds
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import vn.streamix.tv.core.data.local.AuthStore
import vn.streamix.tv.core.data.local.room.StreamIXDatabase
import vn.streamix.tv.core.data.local.room.dao.PlaybackPositionDao
import vn.streamix.tv.core.data.local.room.dao.RecentSearchDao
import vn.streamix.tv.core.data.local.room.dao.UserCacheDao
import vn.streamix.tv.core.data.repository.AuthRepositoryImpl
import vn.streamix.tv.core.data.repository.FeaturedRepositoryImpl
import vn.streamix.tv.core.data.repository.FilesRepositoryImpl
import vn.streamix.tv.core.data.repository.PlaybackPositionRepositoryImpl
import vn.streamix.tv.core.data.repository.PlayerRepositoryImpl
import vn.streamix.tv.core.data.repository.SearchRepositoryImpl
import vn.streamix.tv.core.data.repository.SettingsRepositoryImpl
import vn.streamix.tv.core.data.repository.TopRepositoryImpl
import vn.streamix.tv.core.data.repository.UserRepositoryImpl
import vn.streamix.tv.core.domain.repository.AuthRepository
import vn.streamix.tv.core.domain.repository.FeaturedRepository
import vn.streamix.tv.core.domain.repository.FilesRepository
import vn.streamix.tv.core.domain.repository.PlaybackPositionRepository
import vn.streamix.tv.core.domain.repository.PlayerRepository
import vn.streamix.tv.core.domain.repository.SearchRepository
import vn.streamix.tv.core.domain.repository.SettingsRepository
import vn.streamix.tv.core.domain.repository.TopRepository
import vn.streamix.tv.core.domain.repository.UserRepository
import vn.streamix.tv.core.network.BuildConfig
import vn.streamix.tv.core.network.auth.AuthTokenProvider
import javax.inject.Named
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object DataModule {

    @Provides @Singleton
    fun provideDatabase(@ApplicationContext context: Context): StreamIXDatabase =
        Room.databaseBuilder(context, StreamIXDatabase::class.java, StreamIXDatabase.DB_NAME)
            .build()

    @Provides fun provideUserCacheDao(db: StreamIXDatabase): UserCacheDao = db.userCacheDao()
    @Provides fun providePlaybackPositionDao(db: StreamIXDatabase): PlaybackPositionDao = db.playbackPositionDao()
    @Provides fun provideRecentSearchDao(db: StreamIXDatabase): RecentSearchDao = db.recentSearchDao()

    // ─── Sheet IDs (Wave M3) ──────────────────────────────────────────

    @Provides @Singleton @Named("sheetGoiyId")
    fun provideSheetGoiyId(): String = BuildConfig.SHEET_GOIY_FILE_ID

    @Provides @Singleton @Named("sheetGoiyGid")
    fun provideSheetGoiyGid(): String = BuildConfig.SHEET_GOIY_GID

    @Provides @Singleton @Named("sheetXuhuongId")
    fun provideSheetXuhuongId(): String = BuildConfig.SHEET_XUHUONG_FILE_ID

    @Provides @Singleton @Named("sheetXuhuongGid")
    fun provideSheetXuhuongGid(): String = BuildConfig.SHEET_XUHUONG_GID

    // ─── Top sheets (3 source IMDB/RT/Fshare) ─────────────────────────

    @Provides @Singleton @Named("sheetTopImdbId")
    fun provideSheetTopImdbId(): String = BuildConfig.SHEET_TOP_IMDB_FILE_ID

    @Provides @Singleton @Named("sheetTopImdbGid")
    fun provideSheetTopImdbGid(): String = BuildConfig.SHEET_TOP_IMDB_GID

    @Provides @Singleton @Named("sheetTopRtId")
    fun provideSheetTopRtId(): String = BuildConfig.SHEET_TOP_RT_FILE_ID

    @Provides @Singleton @Named("sheetTopRtGid")
    fun provideSheetTopRtGid(): String = BuildConfig.SHEET_TOP_RT_GID

    @Provides @Singleton @Named("sheetTopFshareId")
    fun provideSheetTopFshareId(): String = BuildConfig.SHEET_TOP_FSHARE_FILE_ID

    @Provides @Singleton @Named("sheetTopFshareGid")
    fun provideSheetTopFshareGid(): String = BuildConfig.SHEET_TOP_FSHARE_GID
}

@Module
@InstallIn(SingletonComponent::class)
abstract class DataBindModule {
    @Binds @Singleton abstract fun bindAuthRepository(impl: AuthRepositoryImpl): AuthRepository
    @Binds @Singleton abstract fun bindUserRepository(impl: UserRepositoryImpl): UserRepository
    @Binds @Singleton abstract fun bindFilesRepository(impl: FilesRepositoryImpl): FilesRepository
    @Binds @Singleton abstract fun bindPlayerRepository(impl: PlayerRepositoryImpl): PlayerRepository
    @Binds @Singleton abstract fun bindPlaybackRepository(impl: PlaybackPositionRepositoryImpl): PlaybackPositionRepository
    @Binds @Singleton abstract fun bindSettingsRepository(impl: SettingsRepositoryImpl): SettingsRepository
    @Binds @Singleton abstract fun bindFeaturedRepository(impl: FeaturedRepositoryImpl): FeaturedRepository
    @Binds @Singleton abstract fun bindSearchRepository(impl: SearchRepositoryImpl): SearchRepository
    @Binds @Singleton abstract fun bindTopRepository(impl: TopRepositoryImpl): TopRepository

    /** AuthStore implements AuthTokenProvider — bind cho network layer dùng */
    @Binds @Singleton abstract fun bindAuthTokenProvider(authStore: AuthStore): AuthTokenProvider
}
