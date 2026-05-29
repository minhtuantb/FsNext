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
