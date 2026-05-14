package vn.streamix.tv.core.domain.repository

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.domain.model.AppSettings

interface SettingsRepository {
    fun observe(): Flow<AppSettings>
    suspend fun update(transform: (AppSettings) -> AppSettings)
    suspend fun setOnboardingCompleted(completed: Boolean)
}
