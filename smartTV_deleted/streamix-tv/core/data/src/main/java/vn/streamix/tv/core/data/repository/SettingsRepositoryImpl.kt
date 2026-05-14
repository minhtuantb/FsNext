package vn.streamix.tv.core.data.repository

import kotlinx.coroutines.flow.Flow
import vn.streamix.tv.core.data.local.SettingsDataStore
import vn.streamix.tv.core.domain.model.AppSettings
import vn.streamix.tv.core.domain.repository.SettingsRepository
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class SettingsRepositoryImpl @Inject constructor(
    private val ds: SettingsDataStore,
) : SettingsRepository {

    override fun observe(): Flow<AppSettings> = ds.observe()

    override suspend fun update(transform: (AppSettings) -> AppSettings) {
        ds.update(transform)
    }

    override suspend fun setOnboardingCompleted(completed: Boolean) {
        ds.update { it.copy(onboardingCompleted = completed) }
    }
}
