#include "AppContext.h"

#include "core/api/HttpClient.h"
#include "core/api/FshareApi.h"
#include "core/services/AuthService.h"
#include "core/services/OAuthService.h"
#include "core/services/TransferService.h"
#include "core/transfer/TransferOrchestrator.h"
#include "core/transfer/BudgetManager.h"
#include "core/services/FileService.h"
#include "core/services/FileCacheService.h"
#include "core/services/SettingsService.h"
#include "core/services/BatchFileResolver.h"
#include "core/repositories/SettingsRepository.h"
#include "core/repositories/HistoryRepository.h"
#include "core/repositories/SyncRepository.h"
#include "core/services/SyncService.h"
#include "viewmodels/AuthViewModel.h"
#include "viewmodels/DownloadViewModel.h"
#include "viewmodels/UploadViewModel.h"
#include "viewmodels/FileManagerViewModel.h"
#include "viewmodels/SettingsViewModel.h"
#include "viewmodels/UserInfoViewModel.h"
#include "viewmodels/LanguageViewModel.h"
#include "viewmodels/FavoritesViewModel.h"
#include "viewmodels/SyncViewModel.h"
#include "viewmodels/TransferBudgetViewModel.h"
#include "viewmodels/HomeSearchViewModel.h"
#include "viewmodels/RemoteShareViewModel.h"
#include "core/util/BadWordFilter.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

namespace fsnext {

AppContext::AppContext(QObject *parent)
    : QObject(parent)
{
}

AppContext::~AppContext() = default;

void AppContext::init()
{
    qDebug() << "[FsNext] Initializing AppContext...";

    // Data layer
    m_httpClient = std::make_unique<HttpClient>();
    m_settingsRepo = std::make_unique<SettingsRepository>();
    m_historyRepo = std::make_unique<HistoryRepository>();
    m_syncRepo = std::make_unique<SyncRepository>();

    // API client
    m_api = std::make_unique<FshareApi>(m_httpClient.get());

    // Bad-word filter — loads the bundled dictionary once at startup.
    // Fail-open: a load failure logs a warning and leaves every keyword
    // classified as clean, never a hard error.
    m_badWordFilter = std::make_unique<BadWordFilter>();
    m_badWordFilter->loadFromResource();

    // Services — OAuth first so AuthService can reference it
    m_oauthService = std::make_unique<OAuthService>(m_httpClient.get());
    m_authService = std::make_unique<AuthService>(m_api.get(), m_settingsRepo.get(),
                                                   m_oauthService.get());
    // Orchestrator owns the shared slot/priority budget for every transfer
    // producer (TransferService user DL/UL, SyncService auto-UL).  Created
    // before TransferService because the service holds a non-owning pointer.
    m_transferOrchestrator = std::make_unique<TransferOrchestrator>();

    m_transferService = std::make_unique<TransferService>(m_api.get(), m_settingsRepo.get(),
                                                           m_transferOrchestrator.get(),
                                                           m_historyRepo.get());

    // Wire: when user logs in, load their history into TransferService
    QObject::connect(m_authService.get(), &AuthService::userChanged, this, [this]() {
        if (m_authService->isLoggedIn()) {
            const auto user = m_authService->currentUser();
            if (!user.id.isEmpty())
                m_transferService->loadHistory(user.id);
        }
    });

    // Wire: when a transfer hits HTTP 201 (session expired on the Fshare API),
    // force the user back to the login screen. AuthService clears state and
    // emits sessionExpiredNotice which QML picks up to show a toast.
    QObject::connect(m_transferService.get(), &TransferService::sessionExpired,
                     m_authService.get(),     &AuthService::handleSessionExpired);
    m_fileService = std::make_unique<FileService>(m_api.get());
    // FileCacheService routes folder-crawl metadata work through the shared
    // TransferOrchestrator so BudgetManager's global cap + metadataFloorGlobal
    // naturally throttles prefetch when DL/UL saturate the slot budget.
    m_fileCacheService = std::make_unique<FileCacheService>(m_api.get(),
                                                             m_fileService.get(),
                                                             m_transferOrchestrator.get());
    m_settingsService = std::make_unique<SettingsService>(m_settingsRepo.get());

    // Live-update the orchestrator's slot caps when the user changes
    // Download/Upload thread counts in Settings.  Without this the caps
    // only took effect on next app launch, and users' sliders felt broken.
    // SettingsService fires a single coarse settingsChanged() on every
    // setter, so we just re-read both values and push a fresh Config —
    // cheap, idempotent, and handles future settings we might add.
    QObject::connect(m_settingsService.get(), &SettingsService::settingsChanged,
                     this, [this]() {
        if (!m_transferOrchestrator) return;
        BudgetManager::Config cfg = m_transferOrchestrator->config();
        cfg.maxDownloadSlots = qBound(1, m_settingsService->downloadThreads(), 16);
        cfg.maxUploadSlots   = qBound(1, m_settingsService->uploadThreads(),   16);
        // maxGlobalSlots is the machine-wide ceiling.  0 disables the cap
        // (per-class caps still apply).  SettingsService has already clamped
        // to [0, 32] before we read it here.
        cfg.maxGlobalSlots   = m_settingsService->maxGlobalSlots();
        m_transferOrchestrator->setConfig(cfg);
    });

    // Wire: when a transfer completes, record the linkcode → local file mapping
    QObject::connect(m_transferService.get(), &TransferService::transferRecordReady,
                     this, [this](const QString &linkcode, const QString &localPath,
                                  const QString &fileName, int64_t fileSize,
                                  const QString &transferType) {
        m_fileCacheService->recordLocalFile(linkcode, localPath, fileName, fileSize, transferType);
    });

    // Wire: when an upload finishes, invalidate the destination folder in the
    // file-manager cache so the new file appears without the user having to
    // hit refresh. FileCacheService maps the upload PATH ("/", "/Sub") back to
    // the cached folderId used by the file manager.
    QObject::connect(m_transferService.get(), &TransferService::uploadCompleted,
                     this, [this](const QString &folderPath) {
        m_fileCacheService->refreshUploadFolder(folderPath);
    });

    // Wire: file cache follows the authenticated user
    QObject::connect(m_authService.get(), &AuthService::userChanged, this, [this]() {
        if (m_authService->isLoggedIn()) {
            const auto user = m_authService->currentUser();
            if (!user.id.isEmpty()) {
                m_fileCacheService->setCurrentUser(user.id);
                m_fileCacheService->loadFolderTree();  // kick background tree sync
            }
        } else {
            m_fileCacheService->onUserLoggedOut();
        }
    });

    // Batch file resolver — bounded-concurrency URL→FileItem resolver
    m_batchResolver = std::make_unique<BatchFileResolver>(m_api.get());

    // Sync service — orchestrates up to kMaxFolders local folders → Fshare.
    // Follows the authenticated user: cleared on logout, loaded on login.
    m_syncService = std::make_unique<SyncService>(m_transferService.get(),
                                                   m_api.get(),
                                                   m_syncRepo.get());
    QObject::connect(m_authService.get(), &AuthService::userChanged, this, [this]() {
        if (m_authService->isLoggedIn()) {
            const auto user = m_authService->currentUser();
            m_syncService->setUserId(user.id);
        } else {
            m_syncService->setUserId(QString{});
        }
    });

    // ViewModels
    m_authVM = std::make_unique<AuthViewModel>(m_authService.get());
    m_downloadVM = std::make_unique<DownloadViewModel>(m_transferService.get(),
                                                       m_settingsService.get(),
                                                       m_authService.get());
    m_uploadVM = std::make_unique<UploadViewModel>(m_transferService.get(), m_authService.get());
    m_fileManagerVM = std::make_unique<FileManagerViewModel>(m_fileCacheService.get(),
                                                              m_batchResolver.get());
    m_favoritesVM = std::make_unique<FavoritesViewModel>(m_api.get(),
                                                          m_fileCacheService.get());
    m_settingsVM = std::make_unique<SettingsViewModel>(m_settingsService.get());
    m_userInfoVM = std::make_unique<UserInfoViewModel>(m_authService.get());
    // LanguageViewModel must be created after QCoreApplication (already done in main.cpp)
    // so QSettings and QTranslator work correctly.
    m_languageVM = std::make_unique<LanguageViewModel>();

    m_syncVM = std::make_unique<SyncViewModel>(m_syncService.get());

    // Lightweight budget-status VM — surfaces orchestrator slot/pending
    // counters to QML so the UI can show a live "DL 2/8 · UL 1/4" indicator.
    m_budgetVM = std::make_unique<TransferBudgetViewModel>(m_transferOrchestrator.get());

    // Homepage search router — gates the input through the bad-word filter
    // and classifies it as URL/keyword for the QML layer. Phase 3 added the
    // FshareApi dependency so the VM can drive its own keyword-search
    // request without going through FileCacheService (whose
    // searchResultsLoaded signal is shared with FileManager).
    m_homeSearchVM = std::make_unique<HomeSearchViewModel>(m_badWordFilter.get(),
                                                            m_downloadVM.get(),
                                                            m_api.get());

    // Remote-share browser — backs the FolderBrowserDialog / FileDetailSheet
    // surfaces opened from the homepage when the user pastes a share URL.
    // Reuses FshareApi for getFileInfo / listFiles / createDownloadSession
    // and DownloadViewModel for the actual download / save-folder lookup.
    m_remoteShareVM = std::make_unique<RemoteShareViewModel>(m_api.get(),
                                                              m_downloadVM.get());

    qDebug() << "[FsNext] AppContext initialized successfully";
}

void AppContext::registerQml(QQmlApplicationEngine *engine)
{
    QQmlContext *ctx = engine->rootContext();

    // Register ViewModels as context properties
    ctx->setContextProperty(QStringLiteral("authViewModel"), m_authVM.get());
    ctx->setContextProperty(QStringLiteral("downloadViewModel"), m_downloadVM.get());
    ctx->setContextProperty(QStringLiteral("uploadViewModel"), m_uploadVM.get());
    ctx->setContextProperty(QStringLiteral("fileManagerViewModel"), m_fileManagerVM.get());
    ctx->setContextProperty(QStringLiteral("settingsViewModel"), m_settingsVM.get());
    ctx->setContextProperty(QStringLiteral("userInfoViewModel"), m_userInfoVM.get());
    ctx->setContextProperty(QStringLiteral("favoritesViewModel"), m_favoritesVM.get());
    ctx->setContextProperty(QStringLiteral("languageViewModel"), m_languageVM.get());
    ctx->setContextProperty(QStringLiteral("syncViewModel"), m_syncVM.get());
    ctx->setContextProperty(QStringLiteral("transferBudgetViewModel"), m_budgetVM.get());
    ctx->setContextProperty(QStringLiteral("homeSearchViewModel"), m_homeSearchVM.get());
    ctx->setContextProperty(QStringLiteral("remoteShareViewModel"), m_remoteShareVM.get());

    qDebug() << "[FsNext] QML context properties registered";
}

} // namespace fsnext
