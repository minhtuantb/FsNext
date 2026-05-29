#pragma once
#include <QObject>
#include <memory>

class QQmlApplicationEngine;

namespace fsnext {

class HttpClient;
class FshareApi;
class BadWordFilter;
class AuthService;
class OAuthService;
class RefreshTokenCoordinator;
class TransferOrchestrator;
class TransferService;
class FileService;
class FileCacheService;
class SettingsService;
class BatchFileResolver;
class SettingsRepository;
class HistoryRepository;
class SyncRepository;
class SyncService;
class AuthViewModel;
class DownloadViewModel;
class UploadViewModel;
class UploadStagingViewModel;
class FileManagerViewModel;
class SettingsViewModel;
class UserInfoViewModel;
class FavoritesViewModel;
class LanguageViewModel;
class SyncViewModel;
class TransferBudgetViewModel;
class HomeSearchViewModel;
class RemoteShareViewModel;
class TransferHudViewModel;

class AppContext : public QObject {
    Q_OBJECT
public:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext();

    // Initialize all services and viewmodels
    void init();

    // Register QML types and context properties
    void registerQml(QQmlApplicationEngine *engine);

    // Accessors
    HttpClient *httpClient() const { return m_httpClient.get(); }
    FshareApi *api() const { return m_api.get(); }
    AuthService *authService() const { return m_authService.get(); }
    TransferService *transferService() const { return m_transferService.get(); }
    FileService *fileService() const { return m_fileService.get(); }
    SettingsService *settingsService() const { return m_settingsService.get(); }
    LanguageViewModel *languageViewModel() const { return m_languageVM.get(); }
    // HUD aggregate VM — drives the tray-icon colour, balloon notifications
    // and (in P1+) the Mini Window / Tray Popup QML surfaces.  main.cpp
    // pulls this out to wire SystemTray and balloon gating.
    TransferHudViewModel *hudViewModel() const { return m_hudVM.get(); }

private:
    // Data layer
    std::unique_ptr<HttpClient> m_httpClient;
    std::unique_ptr<SettingsRepository> m_settingsRepo;
    std::unique_ptr<HistoryRepository> m_historyRepo;
    std::unique_ptr<SyncRepository>    m_syncRepo;

    // API
    std::unique_ptr<FshareApi> m_api;
    // Silent re-auth coordinator — depends on HttpClient + SettingsRepository,
    // injected into FshareApi and AuthService.  Must outlive both.
    std::unique_ptr<RefreshTokenCoordinator> m_refreshCoord;

    // Utilities (no Qt dependencies beyond QObject) — kept here so they
    // live for the application lifetime and can be referenced by ViewModels.
    std::unique_ptr<BadWordFilter> m_badWordFilter;

    // Services
    std::unique_ptr<OAuthService> m_oauthService;       // Must outlive m_authService (reference)
    std::unique_ptr<AuthService> m_authService;
    // Must outlive m_transferService (held by pointer inside the service).
    std::unique_ptr<TransferOrchestrator> m_transferOrchestrator;
    std::unique_ptr<TransferService> m_transferService;
    std::unique_ptr<FileService> m_fileService;
    std::unique_ptr<FileCacheService> m_fileCacheService;
    std::unique_ptr<SettingsService> m_settingsService;
    std::unique_ptr<BatchFileResolver> m_batchResolver;
    std::unique_ptr<SyncService>     m_syncService;

    // ViewModels
    std::unique_ptr<AuthViewModel> m_authVM;
    std::unique_ptr<DownloadViewModel> m_downloadVM;
    std::unique_ptr<UploadViewModel> m_uploadVM;
    // Session-and-restart-stable holder for the upload "staging" state — see
    // UploadStagingViewModel.h. Lives here (not under UploadPage) so that
    // navigating between pages doesn't destroy the user's in-progress batch.
    std::unique_ptr<UploadStagingViewModel> m_uploadStagingVM;
    std::unique_ptr<FileManagerViewModel> m_fileManagerVM;
    std::unique_ptr<FavoritesViewModel> m_favoritesVM;
    std::unique_ptr<SettingsViewModel> m_settingsVM;
    std::unique_ptr<UserInfoViewModel> m_userInfoVM;
    std::unique_ptr<LanguageViewModel> m_languageVM;
    std::unique_ptr<SyncViewModel>     m_syncVM;
    std::unique_ptr<TransferBudgetViewModel> m_budgetVM;
    std::unique_ptr<HomeSearchViewModel> m_homeSearchVM;
    std::unique_ptr<RemoteShareViewModel> m_remoteShareVM;
    // HUD aggregate VM — created last so it can reference every other VM.
    std::unique_ptr<TransferHudViewModel> m_hudVM;
};

} // namespace fsnext
