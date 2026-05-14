/*
 * StreamIX TV — Type-safe navigation routes
 * Reference: 13 §4.4
 */
package vn.streamix.tv.navigation

import kotlinx.serialization.Serializable

@Serializable data object SplashRoute
@Serializable data object LoginRoute
@Serializable data object OnboardingRoute
@Serializable data object HomeRoute
@Serializable data object SearchRoute
@Serializable data object SuggestedRoute
@Serializable data object TrendingRoute
@Serializable data object TopRoute
@Serializable data object SettingsHubRoute
@Serializable data object SettingsAccountRoute
@Serializable data object SettingsPlaybackRoute
@Serializable data object SettingsNetworkRoute
@Serializable data object SettingsAboutRoute

@Serializable data class BrowseRoute(val path: String, val name: String)
@Serializable data class FileDetailRoute(val linkcode: String)
@Serializable data class PlayerRoute(
    val linkcode: String,
    val resumeMs: Long = 0L,
    val fileName: String = "",
    val password: String? = null,
)
