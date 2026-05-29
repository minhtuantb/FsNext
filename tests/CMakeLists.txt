# ── FsNext unit tests ─────────────────────────────────────
# Each test target is a small QtTest executable linking only what it needs.
# Keep sources narrow so test builds stay fast and don't drag in QML/Quick.

set(CMAKE_AUTOMOC ON)

# ── test_file_cache_db ────────────────────────────────────
# Exercises FileCacheDB against in-memory SQLite. Depends on FormatUtil
# (parseTimestamp) and FileItem header only — no QtQuick, no network.
qt_add_executable(test_file_cache_db
    test_file_cache_db.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.h
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
)

target_include_directories(test_file_cache_db PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_file_cache_db PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME FileCacheDB COMMAND test_file_cache_db)

# ── test_file_cache_service ───────────────────────────────
# Phase 2: state-machine tests for FileCacheService. Exercises the
# WriteOp pendingOp flow via a FakeFileService that synthesises
# operationComplete/Failed signals without a real API.
#
# Pulls in FileCacheService + FileCacheDB + FormatUtil. FshareApi is only
# referenced through a forward declaration for the nullptr sentinel — no
# network calls happen because the event loop never pumps processNext.
qt_add_executable(test_file_cache_service
    test_file_cache_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FileService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FileService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FileCacheService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FileCacheService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.h
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileSyncWorker.h
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileSyncWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.h
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
    # Pulled in transitively: FileSyncWorker now talks to the TransferOrchestrator
    # (enqueue/release/cancelQueued/dispatchReady), and FshareApi uses
    # RefreshTokenCoordinator for the auth-refresh helper around executeAuthed.
    # The test itself doesn't exercise transfer dispatch, but the linker still
    # needs the symbols.
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
)

target_include_directories(test_file_cache_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)

target_link_libraries(test_file_cache_service PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)

if(WIN32)
    target_link_libraries(test_file_cache_service PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_file_cache_service PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()

add_test(NAME FileCacheService COMMAND test_file_cache_service)

# ── test_sync_scan ────────────────────────────────────────
# M18: the pure filesystem-walk half of the sync scan (SyncScanner). Builds a
# real tree under QTemporaryDir and asserts the ScanResult. Deliberately links
# ONLY SyncScanner.cpp + Qt6::Core/Test — the walk has no FshareApi /
# TransferService / SQLite / QObject dependency, so no curl/moc is dragged in.
qt_add_executable(test_sync_scan
    test_sync_scan.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/SyncScanner.h
    ${CMAKE_SOURCE_DIR}/src/core/services/SyncScanner.cpp
)
target_include_directories(test_sync_scan PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_sync_scan PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SyncScan COMMAND test_sync_scan)

# ── test_budget_manager ───────────────────────────────────
# Pure logic — slot caps, floor quotas, global cap.  No event loop.
qt_add_executable(test_budget_manager
    test_budget_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferPriority.h
)
target_include_directories(test_budget_manager PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_budget_manager PRIVATE Qt6::Core Qt6::Test)
add_test(NAME BudgetManager COMMAND test_budget_manager)

# ── test_fshare_url ───────────────────────────────────────
# URL parser tolerance matrix.
qt_add_executable(test_fshare_url
    test_fshare_url.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.cpp
)
target_include_directories(test_fshare_url PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_fshare_url PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FshareUrl COMMAND test_fshare_url)

# ── test_filename_sanitizer ───────────────────────────────
qt_add_executable(test_filename_sanitizer
    test_filename_sanitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.cpp
)
target_include_directories(test_filename_sanitizer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_filename_sanitizer PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FileNameSanitizer COMMAND test_filename_sanitizer)

# ── test_filename_resolver ────────────────────────────────
# Exercises ConflictPolicy semantics (rename / overwrite / skip / ask) against
# a real QTemporaryDir.  No QtQuick / network / DB dependencies.
qt_add_executable(test_filename_resolver
    test_filename_resolver.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.cpp
)
target_include_directories(test_filename_resolver PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_filename_resolver PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FileNameResolver COMMAND test_filename_resolver)

# ── test_speed_meter ──────────────────────────────────────
# Uses QTest::qWait — must run as a Qt event-loop binary.
qt_add_executable(test_speed_meter
    test_speed_meter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.cpp
)
target_include_directories(test_speed_meter PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_speed_meter PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SpeedMeter COMMAND test_speed_meter)

# ── test_resolve_proxy_url ────────────────────────────────
# Pure-logic coverage of PlatformUtils::resolveProxyUrl — the single function
# every libcurl producer (HttpClient + Download/Upload engines) routes through
# to translate the persisted proxy mode/host/port into a CURLOPT_PROXY string.
# Pulls Qt6::Gui only because PlatformUtils.cpp includes QDesktopServices for
# unrelated helpers; resolveProxyUrl itself touches none of that.
qt_add_executable(test_resolve_proxy_url
    test_resolve_proxy_url.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.h
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.cpp
)
target_include_directories(test_resolve_proxy_url PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_resolve_proxy_url PRIVATE
    Qt6::Core Qt6::Gui Qt6::Test
)
if(WIN32)
    target_link_libraries(test_resolve_proxy_url PRIVATE advapi32 shell32 user32)
endif()
add_test(NAME ResolveProxyUrl COMMAND test_resolve_proxy_url)

# ── test_transfer_orchestrator ────────────────────────────
# Async dispatch loop: enqueue → dispatchReady (on the orchestrator's own
# thread) → release, plus cancel-before-dispatch and priority ordering.
# Pure logic + QThread — no network, no DB.
qt_add_executable(test_transfer_orchestrator
    test_transfer_orchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferPriority.h
)
target_include_directories(test_transfer_orchestrator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_transfer_orchestrator PRIVATE Qt6::Core Qt6::Test)
add_test(NAME TransferOrchestrator COMMAND test_transfer_orchestrator)

# ── test_refresh_coordinator ──────────────────────────────
# Non-network paths of the silent re-auth gate: session ownership, the no-op
# ensureFreshToken cases, and the 7-day persisted window. Constructed with
# http=nullptr so no HTTP runs — but RefreshTokenCoordinator.cpp references
# HttpClient symbols (setCookie/post) so the real HttpClient + libcurl/OpenSSL
# must be linked, same as test_file_cache_service.
qt_add_executable(test_refresh_coordinator
    test_refresh_coordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
)
target_include_directories(test_refresh_coordinator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_refresh_coordinator PRIVATE
    Qt6::Core
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_refresh_coordinator PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_refresh_coordinator PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME RefreshCoordinator COMMAND test_refresh_coordinator)

# ── test_auth_service ─────────────────────────────────────
# AuthService login/logout state machine against a FakeFshareApi (IFshareApi).
# AuthService ODR-references OAuthService + RefreshTokenCoordinator symbols
# (signal connects in the ctor) even though both are null here, so those .cpp
# files — and their transitive deps (HttpClient/curl, LoopbackServer/Network,
# Pkce, SecureStore) — must be linked. FshareApi.cpp is NOT needed: the fake
# provides the IFshareApi surface.
qt_add_executable(test_auth_service
    test_auth_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
)
target_include_directories(test_auth_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_auth_service PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_auth_service PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_auth_service PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME AuthService COMMAND test_auth_service)

# >>> Lô1: test_transfer_history_db (covers ATM-0271) risk=Low — pure SQLite in-memory repo with no network/threading; the only API used (open/upsertTask/saveProgressSnapshot/loadInFlight) is verified against the real header signatures, and progress_json is read back via loadInFlight's documented snapshot out-param.
# ── test_transfer_history_db ──────────────────────────────
# Exercises TransferHistoryDb against in-memory SQLite. Pins ATM-0271: the
# UPSERT path (INSERT ... ON CONFLICT DO UPDATE) must preserve progress_json
# on a state-change re-upsert. TransferTask.h / TransferState.h are
# header-only structs/enums (no .cpp), so the only source compiled besides the
# test is TransferHistoryDb.cpp — no QtQuick, no network, no curl.
qt_add_executable(test_transfer_history_db
    test_transfer_history_db.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferTask.h
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
)

target_include_directories(test_transfer_history_db PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_transfer_history_db PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME TransferHistoryDb COMMAND test_transfer_history_db)

# >>> Lô1: test_auth_viewmodel (covers ATM-0001, ATM-0002, ATM-0003, ATM-0004, ATM-0006, ATM-0007, ATM-0008, ATM-0010, ATM-0011) risk=Low — links the exact same target set as the proven test_auth_service plus AuthViewModel.cpp; the VM has no extra link deps (OAuthProvider.h is header-only OAuthConfig consumed via AuthService.cpp which is already linked). Async paths use the same QSignalSpy/QTRY pattern and the cleanup drains the pool before teardown.
# ── test_auth_viewmodel ───────────────────────────────────
# AuthViewModel (MVVM bridge) driven through a REAL AuthService wired to a
# FakeFshareApi (IFshareApi) + in-memory SettingsRepository — no network. Mirrors
# test_auth_service's link list: AuthService ODR-references OAuthService +
# RefreshTokenCoordinator symbols (signal connects in the ctor) even though both
# are null here, so those .cpp files — and their transitive deps (HttpClient/curl,
# LoopbackServer/Network, Pkce, SecureStore) — must be linked. PLUS the VM under
# test (AuthViewModel.cpp). FshareApi.cpp is NOT needed: the fake provides the
# IFshareApi surface.
qt_add_executable(test_auth_viewmodel
    test_auth_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/AuthViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/AuthViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
)
target_include_directories(test_auth_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_auth_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_auth_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_auth_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME AuthViewModel COMMAND test_auth_viewmodel)

# >>> Lô1: test_settings_viewmodel (covers ATM-0077, ATM-0086, ATM-0087, ATM-0094) risk=Low — pure synchronous VM/service/repo wiring (no network, no QtConcurrent); QSettings isolated under a throwaway org/app name and cleared in init/cleanup, so deterministic.
# ── test_settings_viewmodel ───────────────────────────────
# SettingsViewModel property getter/setter + NOTIFY contract + applySettings,
# driven against a REAL SettingsService + SettingsRepository (QSettings, no
# network, no QtConcurrent). Link list is narrow: VM + service + repo only.
# AppSettings is header-only. SettingsService.cpp uses QStandardPaths/QDir —
# both live in Qt6::Core, so no Qt6::Gui/Quick is needed.
qt_add_executable(test_settings_viewmodel
    test_settings_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/SettingsViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/SettingsViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/SettingsService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/SettingsService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
)
target_include_directories(test_settings_viewmodel PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_settings_viewmodel PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME SettingsViewModel COMMAND test_settings_viewmodel)

# >>> Lô1: test_favorites_viewmodel (covers ATM-0047, ATM-0048, ATM-0060, ATM-0061) risk=Med — file mới + cmake_snippet biên dịch sạch theo convention test_file_cache_service; chưa build thực tế (bị cấm), và service-graph nặng (FileCacheService → FshareApi/HttpClient/curl) nên rủi ro thiếu source/link symbol cao hơn test thuần.
# ── test_favorites_viewmodel ──────────────────────────────
# FavoritesViewModel(FshareApi*, FileCacheService*) against a real
# FileCacheService opened via its test-only dbPath ctor on a QTemporaryDir
# SQLite file. api == nullptr throughout: the navigation paths exercised here
# go through the cache (which only schedules a *background* sync) and never
# pump the worker pool, so no network runs.
#
# Pulls the FileCacheService service-graph (same set as test_file_cache_service:
# FileService + FileCacheDB + FileSyncWorker + FshareApi/HttpClient +
# TransferOrchestrator/BudgetManager/PriorityScheduler + RefreshTokenCoordinator
# + SettingsRepository + FormatUtil) PLUS the VM layer (FavoritesViewModel +
# FileListModel) and PlatformUtils (FavoritesViewModel.cpp calls it for
# openInExplorer/playStreamUrl). FileItem.h and FileTypeHelper.h are
# header-only. Qt6::Gui/Concurrent are needed because FavoritesViewModel.cpp
# uses QGuiApplication/QClipboard and QtConcurrent.
qt_add_executable(test_favorites_viewmodel
    test_favorites_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/FavoritesViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/FavoritesViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/FileListModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/FileListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FileService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FileService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FileCacheService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FileCacheService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.h
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileCacheDB.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileSyncWorker.h
    ${CMAKE_SOURCE_DIR}/src/core/cache/FileSyncWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.h
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.h
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.cpp
)

target_include_directories(test_favorites_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/viewmodels
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)

target_link_libraries(test_favorites_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)

if(WIN32)
    target_link_libraries(test_favorites_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_favorites_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()

add_test(NAME FavoritesViewModel COMMAND test_favorites_viewmodel)

# >>> Lô1: test_download_viewmodel (covers ATM-0016, ATM-0017, ATM-0021, ATM-0026, ATM-0029) risk=Med — link list is wide (full TransferService graph incl. DownloadEngine/UploadEngine/FolderExpander/FshareApi/HttpClient + libcurl/OpenSSL/jsoncpp); if curl/SQL symbols drift the linker will complain, but this mirrors the proven test_file_cache_service/test_auth_service recipe so the risk is build-config, not test-logic.
# ── test_download_viewmodel ───────────────────────────────
# DownloadViewModel against the REAL service graph wired with in-memory /
# null collaborators. FshareApi is passed as nullptr (the ctor never touches
# it, and no test takes a network code path) but TransferService.cpp ODR-
# references FshareApi symbols, so FshareApi.cpp + HttpClient.cpp + their
# transitive deps (RefreshTokenCoordinator, libcurl/OpenSSL/jsoncpp) must be
# linked — same shape as test_file_cache_service / test_auth_service. The
# download/upload engines (DownloadEngine/UploadEngine/SpeedMeter) and the
# folder crawler (FolderExpander) are referenced by TransferService.cpp so
# the linker needs them even though the non-network tests never spawn one.
# AuthService is NOT linked: DownloadViewModel takes auth=nullptr here.
# QTEST_MAIN (not GUILESS) + DownloadViewModel's QClipboard/QGuiApplication
# and PlatformUtils' QDesktopServices pull in Qt6::Gui.
qt_add_executable(test_download_viewmodel
    test_download_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/DownloadViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/DownloadViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/SettingsService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/SettingsService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    # AuthService graph: DownloadViewModel.cpp connects to
    # AuthService::isLoggedInChanged / calls isLoggedIn(), so the whole
    # auth closure must link even though the test passes auth=nullptr.
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.h
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.h
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.cpp
)

target_include_directories(test_download_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)

target_link_libraries(test_download_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)

if(WIN32)
    target_link_libraries(test_download_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_download_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()

add_test(NAME DownloadViewModel COMMAND test_download_viewmodel)

# >>> Lô1: test_upload_viewmodel (covers ATM-0122, ATM-0123, ATM-0126, ATM-0127, ATM-0128, ATM-0129, ATM-0130, ATM-0131, ATM-0132) risk=Med — file content (ctor signature, getters, signals, TransferListModel::count) verified against real headers, but the broad link list (TransferService + AuthService transitive closure) is hand-derived from #include scans; an unlisted internal .cpp dependency could surface as a linker error on first build.
# ── test_upload_viewmodel ─────────────────────────────────
# UploadViewModel's non-network surface, constructed with BOTH deps null
# (TransferService*=nullptr, AuthService*=nullptr) — the guarded "service-less"
# branch. We assert model()/historyModel() construction, the "idle" runState
# default, and addUpload's fail-fast no-op; everything that needs a live
# upload round-trip is QSKIP'd.
#
# Even with a null service the VM's ctor takes &TransferService::taskAdded /
# &AuthService::isLoggedInChanged member-function pointers, so those classes'
# metaobjects must link. We pull the full TransferService + AuthService graphs
# (same transitive closure as test_file_cache_service + test_auth_service):
# engines, orchestrator, FshareApi/HttpClient (curl/OpenSSL), history DB,
# OAuth/LoopbackServer, SecureStore, Pkce. Qt6::Gui is needed because
# UploadViewModel.cpp uses QGuiApplication/QClipboard/QDesktopServices.
qt_add_executable(test_upload_viewmodel
    test_upload_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
    # ── TransferService + transfer graph ──────────────────
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/BatchFileResolver.h
    ${CMAKE_SOURCE_DIR}/src/core/services/BatchFileResolver.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferWorker.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferQueue.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.cpp
    # ── AuthService graph ─────────────────────────────────
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    # ── shared lower layers ───────────────────────────────
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.h
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.h
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.cpp
)
target_include_directories(test_upload_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_upload_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_upload_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_upload_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME UploadViewModel COMMAND test_upload_viewmodel)

# >>> Lô1: test_upload_staging_viewmodel (covers ATM-0109, ATM-0110, ATM-0111, ATM-0112, ATM-0117, ATM-0118, ATM-0121) risk=Med — chỉ rủi ro link/build: target kéo nguyên closure transfer + auth + api (engines dùng libcurl/jsoncpp trực tiếp), nên danh sách source/lib phải đủ; logic test thuần không cần mạng nên chạy ổn định. Nếu UploadEngine/DownloadEngine ODR-tham chiếu thêm symbol (vd PlatformUtils đã thêm) thì cần bổ sung — đã rà include chính nhưng chưa build thật.
# ── test_upload_staging_viewmodel ─────────────────────────
# UploadStagingViewModel ctor is (UploadViewModel*, SettingsRepository*). We
# build a REAL UploadViewModel with a null TransferService (addUpload short-
# circuits on !m_service, so commit() runs without network) and a real
# SettingsRepository (QSettings-backed; the test wipes UploadStaging/* keys per
# init() so hydrate() starts clean).
#
# Taking the address of UploadViewModel's connected signals ODR-references
# TransferService + AuthService moc symbols, so the whole transfer subsystem
# (engines/orchestrator/worker/queue) + FshareApi/HttpClient + the Auth graph
# (OAuthService/RefreshTokenCoordinator/LoopbackServer/SecureStore/Pkce) +
# repositories must be linked — same closure as test_auth_service +
# test_file_cache_service plus the transfer engines. The engines call libcurl
# and jsoncpp directly, so CURL/OpenSSL/jsoncpp are required even though no
# network runs.
qt_add_executable(test_upload_staging_viewmodel
    test_upload_staging_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadStagingViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadStagingViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UploadViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/TransferListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
    # ── Services ──
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/TransferService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.h
    ${CMAKE_SOURCE_DIR}/src/core/services/FolderExpander.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    # ── Transfer subsystem ──
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/DownloadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/UploadEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferWorker.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferQueue.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/SpeedMeter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferOrchestrator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/BudgetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferPriority.h
    # ── API ──
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.h
    ${CMAKE_SOURCE_DIR}/src/core/api/FshareApi.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
    # ── Net / Auth helpers ──
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    # ── Repositories ──
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.cpp
    # ── Util ──
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FileNameSanitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FshareUrl.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
    # ── Platform ──
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.h
    ${CMAKE_SOURCE_DIR}/src/platform/PlatformUtils.cpp
)
target_include_directories(test_upload_staging_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_upload_staging_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Sql
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_upload_staging_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_upload_staging_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME UploadStagingViewModel COMMAND test_upload_staging_viewmodel)

# >>> Lô2: test_format_util risk=Low
# ── test_format_util ──────────────────────────────────────
qt_add_executable(test_format_util
    test_format_util.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
)
target_include_directories(test_format_util PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_format_util PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FormatUtil COMMAND test_format_util)

# >>> Lô2: test_file_type_helper risk=Low
qt_add_executable(test_file_type_helper
    test_file_type_helper.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FileTypeHelper.h
)
target_include_directories(test_file_type_helper PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_file_type_helper PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FileTypeHelper COMMAND test_file_type_helper)

# >>> Lô2: test_pkce risk=Low
qt_add_executable(test_pkce
    test_pkce.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
)
target_include_directories(test_pkce PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_pkce PRIVATE Qt6::Core Qt6::Test)
add_test(NAME Pkce COMMAND test_pkce)

# >>> Lô2: test_secure_store risk=Low
qt_add_executable(test_secure_store
    test_secure_store.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
)
target_include_directories(test_secure_store PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_secure_store PRIVATE Qt6::Core Qt6::Test)
if(WIN32)
    target_link_libraries(test_secure_store PRIVATE crypt32)
endif()
add_test(NAME SecureStore COMMAND test_secure_store)

# >>> Lô2: test_app_error risk=Low
qt_add_executable(test_app_error
    test_app_error.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/AppError.h
)
target_include_directories(test_app_error PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_app_error PRIVATE Qt6::Core Qt6::Test)
add_test(NAME AppError COMMAND test_app_error)

# >>> Lô2: test_api_response risk=Low
qt_add_executable(test_api_response
    test_api_response.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/ApiResponse.h
    ${CMAKE_SOURCE_DIR}/src/core/models/AppError.h
)
target_include_directories(test_api_response PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_api_response PRIVATE Qt6::Core Qt6::Test)
add_test(NAME ApiResponse COMMAND test_api_response)

# >>> Lô2: test_app_settings risk=Low
qt_add_executable(test_app_settings
    test_app_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/AppSettings.h
)
target_include_directories(test_app_settings PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_app_settings PRIVATE Qt6::Core Qt6::Test)
add_test(NAME AppSettings COMMAND test_app_settings)

# >>> Lô2: test_file_item risk=Low
qt_add_executable(test_file_item
    test_file_item.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/FileItem.h
)
target_include_directories(test_file_item PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_file_item PRIVATE Qt6::Core Qt6::Test)
add_test(NAME FileItem COMMAND test_file_item)

# >>> Lô2: test_transfer_task risk=Low
qt_add_executable(test_transfer_task
    test_transfer_task.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferTask.h
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
)
target_include_directories(test_transfer_task PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_transfer_task PRIVATE Qt6::Core Qt6::Test)
add_test(NAME TransferTask COMMAND test_transfer_task)

# >>> Lô2: test_user_model risk=Low
qt_add_executable(test_user_model
    test_user_model.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/User.h
)
target_include_directories(test_user_model PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_user_model PRIVATE Qt6::Core Qt6::Test)
add_test(NAME UserModel COMMAND test_user_model)

# >>> Lô2: test_sync_folder risk=Low
qt_add_executable(test_sync_folder
    test_sync_folder.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/SyncFolder.h
)
target_include_directories(test_sync_folder PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_sync_folder PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SyncFolder COMMAND test_sync_folder)

# >>> Lô2: test_history_repository risk=Low
qt_add_executable(test_history_repository
    test_history_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/HistoryRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/TransferHistoryDb.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferTask.h
    ${CMAKE_SOURCE_DIR}/src/core/models/TransferState.h
)
target_include_directories(test_history_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_history_repository PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)
add_test(NAME HistoryRepository COMMAND test_history_repository)

# >>> Lô2: test_sync_repository risk=Low
qt_add_executable(test_sync_repository
    test_sync_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SyncRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SyncRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/models/SyncFolder.h
)
target_include_directories(test_sync_repository PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_sync_repository PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME SyncRepository COMMAND test_sync_repository)

# >>> Lô2: test_priority_scheduler risk=Low
qt_add_executable(test_priority_scheduler
    test_priority_scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.h
    ${CMAKE_SOURCE_DIR}/src/core/transfer/PriorityScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/transfer/TransferPriority.h
)
target_include_directories(test_priority_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_priority_scheduler PRIVATE Qt6::Core Qt6::Test)
add_test(NAME PriorityScheduler COMMAND test_priority_scheduler)

# >>> Lô2: test_language_viewmodel risk=Low
qt_add_executable(test_language_viewmodel
    test_language_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/LanguageViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/LanguageViewModel.cpp
)
target_include_directories(test_language_viewmodel PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_language_viewmodel PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME LanguageViewModel COMMAND test_language_viewmodel)

# >>> Lô2: test_userinfo_viewmodel risk=Low
qt_add_executable(test_userinfo_viewmodel
    test_userinfo_viewmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UserInfoViewModel.h
    ${CMAKE_SOURCE_DIR}/src/viewmodels/UserInfoViewModel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/AuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.h
    ${CMAKE_SOURCE_DIR}/src/core/services/OAuthService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.h
    ${CMAKE_SOURCE_DIR}/src/core/services/RefreshTokenCoordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.h
    ${CMAKE_SOURCE_DIR}/src/core/repositories/SettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.h
    ${CMAKE_SOURCE_DIR}/src/core/util/SecureStore.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.h
    ${CMAKE_SOURCE_DIR}/src/core/util/Pkce.cpp
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.h
    ${CMAKE_SOURCE_DIR}/src/core/util/FormatUtil.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.h
    ${CMAKE_SOURCE_DIR}/src/core/net/LoopbackServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.h
    ${CMAKE_SOURCE_DIR}/src/core/api/HttpClient.cpp
)
target_include_directories(test_userinfo_viewmodel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/cppcodec
)
target_link_libraries(test_userinfo_viewmodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Test
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)
if(WIN32)
    target_link_libraries(test_userinfo_viewmodel PRIVATE
        ws2_32 crypt32 wldap32 normaliz secur32 advapi32
        user32 bcrypt iphlpapi shell32
    )
    target_compile_definitions(test_userinfo_viewmodel PRIVATE
        CURL_STATICLIB WIN32_LEAN_AND_MEAN
    )
endif()
add_test(NAME UserInfoViewModel COMMAND test_userinfo_viewmodel)
