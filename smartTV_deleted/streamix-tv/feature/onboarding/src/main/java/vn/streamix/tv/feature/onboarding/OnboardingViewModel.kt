package vn.streamix.tv.feature.onboarding

import androidx.lifecycle.ViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import vn.streamix.tv.core.domain.repository.SettingsRepository
import javax.inject.Inject

@HiltViewModel
class OnboardingViewModel @Inject constructor(
    private val settingsRepo: SettingsRepository,
) : ViewModel() {
    suspend fun markCompleted() {
        settingsRepo.setOnboardingCompleted(true)
    }
}
