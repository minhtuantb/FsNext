/*
 * StreamIX TV — Navigation graph
 * Reference: 13 §4.4 + 14 §11 navigation diagram
 */
package vn.streamix.tv.navigation

import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.toRoute
import vn.streamix.tv.core.common.FshareUrl
import vn.streamix.tv.feature.auth.LoginScreen
import vn.streamix.tv.feature.browse.BrowseScreen
import vn.streamix.tv.feature.browse.FileDetailScreen
import vn.streamix.tv.feature.home.HomeScreen
import vn.streamix.tv.feature.home.SuggestedScreen
import vn.streamix.tv.feature.home.TopScreen
import vn.streamix.tv.feature.home.TrendingScreen
import vn.streamix.tv.feature.onboarding.OnboardingScreen
import vn.streamix.tv.feature.player.PlayerScreen
import vn.streamix.tv.feature.search.SearchScreen
import vn.streamix.tv.feature.settings.SettingsAboutScreen
import vn.streamix.tv.feature.settings.SettingsAccountScreen
import vn.streamix.tv.feature.settings.SettingsHubScreen
import vn.streamix.tv.feature.settings.SettingsNetworkScreen
import vn.streamix.tv.feature.settings.SettingsPlaybackScreen
import vn.streamix.tv.feature.splash.SplashScreen

@Composable
fun StreamIXNavGraph(navController: NavHostController) {
    NavHost(
        navController = navController,
        startDestination = SplashRoute,
    ) {
        composable<SplashRoute> {
            SplashScreen(
                onNavigateLogin = {
                    navController.navigate(LoginRoute) {
                        popUpTo(SplashRoute) { inclusive = true }
                    }
                },
                onNavigateHome = {
                    navController.navigate(HomeRoute) {
                        popUpTo(SplashRoute) { inclusive = true }
                    }
                },
            )
        }

        composable<LoginRoute> {
            LoginScreen(
                onLoginSuccess = { onboarded ->
                    val target = if (onboarded) HomeRoute else OnboardingRoute
                    navController.navigate(target) {
                        popUpTo(LoginRoute) { inclusive = true }
                    }
                },
            )
        }

        composable<OnboardingRoute> {
            OnboardingScreen(
                onCompleted = {
                    navController.navigate(HomeRoute) {
                        popUpTo(OnboardingRoute) { inclusive = true }
                    }
                },
            )
        }

        composable<HomeRoute> {
            HomeScreen(
                onOpenSuggested = { navController.navigate(SuggestedRoute) },
                onOpenTrending = { navController.navigate(TrendingRoute) },
                onOpenTop = { navController.navigate(TopRoute) },
                onOpenContinue = { linkcode, resumeMs ->
                    navController.navigate(PlayerRoute(linkcode, resumeMs))
                },
                onOpenSearch = { navController.navigate(SearchRoute) },
                onOpenSettings = { navController.navigate(SettingsHubRoute) },
                onOpenAccount = { navController.navigate(SettingsAccountRoute) },
                onSessionExpiredSignIn = {
                    navController.navigate(LoginRoute) { popUpTo(0) }
                },
            )
        }

        composable<SuggestedRoute> {
            SuggestedScreen(
                onOpenItem = { item ->
                    val lc = item.linkcode ?: FshareUrl.extractLinkcode(item.url)
                    if (lc == null) return@SuggestedScreen
                    if (FshareUrl.isFolderUrl(item.url)) {
                        navController.navigate(BrowseRoute(path = lc, name = item.title))
                    } else {
                        navController.navigate(FileDetailRoute(lc))
                    }
                },
                onBack = { navController.navigateUp() },
            )
        }

        composable<TrendingRoute> {
            TrendingScreen(
                onOpenItem = { item ->
                    val lc = item.linkcode ?: FshareUrl.extractLinkcode(item.url)
                    if (lc == null) return@TrendingScreen
                    if (FshareUrl.isFolderUrl(item.url)) {
                        navController.navigate(BrowseRoute(path = lc, name = item.title))
                    } else {
                        navController.navigate(FileDetailRoute(lc))
                    }
                },
                onBack = { navController.navigateUp() },
            )
        }

        composable<TopRoute> {
            TopScreen(
                onOpenFile = { lc -> navController.navigate(FileDetailRoute(lc)) },
                onBack = { navController.navigateUp() },
            )
        }

        composable<BrowseRoute> { entry ->
            val args = entry.toRoute<BrowseRoute>()
            BrowseScreen(
                folderPath = args.path,
                folderName = args.name,
                onOpenFolder = { folder ->
                    // Truyền linkcode (FilesRepositoryImpl.canonicalFolderUrl xử lý cả linkcode trần
                    // và full URL — path field giờ chứa linkcode để consistent).
                    navController.navigate(BrowseRoute(folder.linkcode, folder.name))
                },
                onOpenFile = { linkcode -> navController.navigate(FileDetailRoute(linkcode)) },
                onBack = { navController.navigateUp() },
            )
        }

        composable<FileDetailRoute> { entry ->
            val args = entry.toRoute<FileDetailRoute>()
            FileDetailScreen(
                linkcode = args.linkcode,
                onPlay = { linkcode, fileName ->
                    navController.navigate(PlayerRoute(linkcode = linkcode, fileName = fileName))
                },
                onBack = { navController.navigateUp() },
            )
        }

        composable<PlayerRoute> { entry ->
            val args = entry.toRoute<PlayerRoute>()
            PlayerScreen(
                linkcode = args.linkcode,
                resumeMs = args.resumeMs,
                fileName = args.fileName,
                onExit = { navController.navigateUp() },
            )
        }

        composable<SearchRoute> {
            SearchScreen(
                onOpenFile = { linkcode -> navController.navigate(FileDetailRoute(linkcode)) },
                onBack = { navController.navigateUp() },
            )
        }

        composable<SettingsHubRoute> {
            SettingsHubScreen(
                onOpenAccount = { navController.navigate(SettingsAccountRoute) },
                onOpenPlayback = { navController.navigate(SettingsPlaybackRoute) },
                onOpenNetwork = { navController.navigate(SettingsNetworkRoute) },
                onOpenAbout = { navController.navigate(SettingsAboutRoute) },
                onBack = { navController.navigateUp() },
            )
        }

        composable<SettingsAccountRoute> {
            SettingsAccountScreen(
                onLoggedOut = {
                    navController.navigate(LoginRoute) {
                        popUpTo(0)   // clear toàn bộ stack
                    }
                },
                onBack = { navController.navigateUp() },
            )
        }

        composable<SettingsPlaybackRoute> {
            SettingsPlaybackScreen(onBack = { navController.navigateUp() })
        }
        composable<SettingsNetworkRoute> {
            SettingsNetworkScreen(onBack = { navController.navigateUp() })
        }
        composable<SettingsAboutRoute> {
            SettingsAboutScreen(onBack = { navController.navigateUp() })
        }
    }
}
