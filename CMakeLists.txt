cmake_minimum_required(VERSION 3.24)
project(FsNext VERSION 6.0.0 LANGUAGES C CXX)

# ── C++ Standard ──────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt 6 ──────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    Network
    Qml
    Quick
    QuickControls2
    Sql
    Svg
    Test
    Widgets   # for QSystemTrayIcon (SystemTray.cpp).  Adds ~3 MB to the
              # bundle but is the only Qt-blessed way to put an icon in the
              # OS tray on Windows / macOS / Linux.
)
qt_standard_project_setup()

# ── vcpkg dependencies ────────────────────────────────────
# CURL built with HTTP/2 expects CURL::nghttp2 imported target. vcpkg's
# nghttp2 package doesn't ship a CMake config, so we create the target
# manually before find_package(CURL) resolves it.
find_library(_NGHTTP2_LIB NAMES nghttp2)
find_path(_NGHTTP2_INC NAMES nghttp2/nghttp2.h)
if(_NGHTTP2_LIB AND _NGHTTP2_INC)
    if(NOT TARGET CURL::nghttp2)
        add_library(CURL::nghttp2 STATIC IMPORTED)
        set_target_properties(CURL::nghttp2 PROPERTIES
            IMPORTED_LOCATION "${_NGHTTP2_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_NGHTTP2_INC}"
        )
    endif()
    message(STATUS "FsNext: nghttp2 found → HTTP/2 download multiplexing enabled")
else()
    message(STATUS "FsNext: nghttp2 not found → HTTP/2 disabled, falling back to HTTP/1.1")
endif()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# ── Version from git ──────────────────────────────────────
execute_process(
    COMMAND git rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_REVISION)
    set(GIT_REVISION "0")
endif()
message(STATUS "FsNext version: ${PROJECT_VERSION}.${GIT_REVISION}")

# ── Output directories ────────────────────────────────────
set(FSNEXT_OUT_DIR "${CMAKE_SOURCE_DIR}/output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FSNEXT_OUT_DIR}")
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${FSNEXT_OUT_DIR}")
endforeach()

# ── jsoncpp (vendored single-file) ────────────────────────
add_library(jsoncpp STATIC
    "${CMAKE_SOURCE_DIR}/lib/jsoncpp/jsoncpp.cpp"
)
target_include_directories(jsoncpp PUBLIC
    "${CMAKE_SOURCE_DIR}/lib/jsoncpp"
)

# ── Source files ──────────────────────────────────────────
set(FSNEXT_SOURCES
    # App
    src/main.cpp
    src/app/Application.cpp
    src/app/AppContext.cpp

    # Core - API
    src/core/api/HttpClient.cpp
    src/core/api/FshareApi.cpp

    # Core - Cache
    src/core/cache/FileCacheDB.cpp
    src/core/cache/FileSyncWorker.cpp

    # Core - Services
    src/core/services/AuthService.cpp
    src/core/services/OAuthService.cpp
    src/core/services/RefreshTokenCoordinator.cpp
    src/core/services/TransferService.cpp
    src/core/services/FolderExpander.cpp
    src/core/services/FileService.cpp
    src/core/services/FileCacheService.cpp
    src/core/services/BatchFileResolver.cpp
    src/core/services/SettingsService.cpp
    src/core/services/SyncService.cpp
    src/core/services/SyncScanner.cpp

    # Core - Net (OAuth loopback server for RFC 8252 flow)
    src/core/net/LoopbackServer.cpp

    # Core - Util
    src/core/util/Pkce.cpp
    src/core/util/SecureStore.cpp
    src/core/util/FileNameSanitizer.cpp
    src/core/util/FshareUrl.cpp
    src/core/util/FormatUtil.cpp
    src/core/util/BadWordFilter.cpp

    # Core - Repositories
    src/core/repositories/SettingsRepository.cpp
    src/core/repositories/HistoryRepository.cpp
    src/core/repositories/TransferHistoryDb.cpp
    src/core/repositories/SyncRepository.cpp

    # Core - Transfer
    src/core/transfer/DownloadEngine.cpp
    src/core/transfer/UploadEngine.cpp
    src/core/transfer/TransferWorker.cpp
    src/core/transfer/TransferQueue.cpp
    src/core/transfer/SpeedMeter.cpp
    src/core/transfer/BudgetManager.cpp
    src/core/transfer/PriorityScheduler.cpp
    src/core/transfer/TransferOrchestrator.cpp

    # ViewModels
    src/viewmodels/AuthViewModel.cpp
    src/viewmodels/DownloadViewModel.cpp
    src/viewmodels/UploadViewModel.cpp
    src/viewmodels/UploadStagingViewModel.cpp
    src/viewmodels/FileManagerViewModel.cpp
    src/viewmodels/FavoritesViewModel.cpp
    src/viewmodels/FileListModel.cpp
    src/viewmodels/FolderTreeModel.cpp
    src/viewmodels/FolderPickerModel.cpp
    src/viewmodels/SettingsViewModel.cpp
    src/viewmodels/UserInfoViewModel.cpp
    src/viewmodels/TransferListModel.cpp
    src/viewmodels/LanguageViewModel.cpp
    src/viewmodels/SyncViewModel.cpp
    src/viewmodels/TransferBudgetViewModel.cpp
    src/viewmodels/HomeSearchViewModel.cpp
    src/viewmodels/RemoteShareViewModel.cpp
    src/viewmodels/TransferHudViewModel.cpp

    # Platform
    src/platform/SingleInstance.cpp
    src/platform/SystemTray.cpp
    src/platform/TaskbarProgress.cpp
    src/platform/PlatformUtils.cpp
)

set(FSNEXT_HEADERS
    # App
    src/app/Application.h
    src/app/AppContext.h

    # Core - Models
    src/core/models/User.h
    src/core/models/FileItem.h
    src/core/models/TransferTask.h
    src/core/models/TransferState.h
    src/core/models/AppSettings.h
    src/core/models/ApiResponse.h
    src/core/models/AppError.h
    src/core/models/SyncFolder.h

    # Core - API
    src/core/api/HttpClient.h
    src/core/api/FshareApi.h
    src/core/api/OAuthProvider.h

    # Core - Cache
    src/core/cache/FileCacheDB.h
    src/core/cache/FileSyncWorker.h

    # Core - Services
    src/core/services/AuthService.h
    src/core/services/OAuthService.h
    src/core/services/RefreshTokenCoordinator.h
    src/core/services/TransferService.h
    src/core/services/FolderExpander.h
    src/core/services/FileService.h
    src/core/services/FileCacheService.h
    src/core/services/BatchFileResolver.h
    src/core/services/SettingsService.h
    src/core/services/SyncService.h
    src/core/services/SyncScanner.h

    # Core - Net
    src/core/net/LoopbackServer.h

    # Core - Util
    src/core/util/Pkce.h
    src/core/util/SecureStore.h
    src/core/util/FileTypeHelper.h
    src/core/util/FileNameSanitizer.h
    src/core/util/FshareUrl.h
    src/core/util/FormatUtil.h
    src/core/util/BadWordFilter.h

    # Core - Repositories
    src/core/repositories/SettingsRepository.h
    src/core/repositories/HistoryRepository.h
    src/core/repositories/TransferHistoryDb.h
    src/core/repositories/SyncRepository.h

    # Core - Transfer
    src/core/transfer/DownloadEngine.h
    src/core/transfer/UploadEngine.h
    src/core/transfer/TransferWorker.h
    src/core/transfer/TransferQueue.h
    src/core/transfer/SpeedMeter.h
    src/core/transfer/TransferPriority.h
    src/core/transfer/BudgetManager.h
    src/core/transfer/PriorityScheduler.h
    src/core/transfer/TransferOrchestrator.h

    # ViewModels
    src/viewmodels/AuthViewModel.h
    src/viewmodels/DownloadViewModel.h
    src/viewmodels/UploadViewModel.h
    src/viewmodels/FileManagerViewModel.h
    src/viewmodels/FavoritesViewModel.h
    src/viewmodels/FileListModel.h
    src/viewmodels/FolderTreeModel.h
    src/viewmodels/FolderPickerModel.h
    src/viewmodels/SettingsViewModel.h
    src/viewmodels/UserInfoViewModel.h
    src/viewmodels/TransferListModel.h
    src/viewmodels/LanguageViewModel.h
    src/viewmodels/SyncViewModel.h
    src/viewmodels/TransferBudgetViewModel.h
    src/viewmodels/HomeSearchViewModel.h
    src/viewmodels/RemoteShareViewModel.h
    src/viewmodels/TransferHudViewModel.h

    # Platform
    src/platform/SingleInstance.h
    src/platform/SystemTray.h
    src/platform/TaskbarProgress.h
    src/platform/PlatformUtils.h
)

# ── Executable ────────────────────────────────────────────
# resources/app.rc is Windows-only — it embeds the app icon (so File Explorer
# and the taskbar show our branding) and version metadata (so the Properties
# dialog shows Company/Version/Copyright).  Guard the include with WIN32 so
# Linux/macOS builds don't try to compile it.
set(FSNEXT_PLATFORM_RESOURCES "")
if(WIN32)
    list(APPEND FSNEXT_PLATFORM_RESOURCES resources/app.rc)
endif()

qt_add_executable(FsNext WIN32
    ${FSNEXT_SOURCES}
    ${FSNEXT_HEADERS}
    ${FSNEXT_PLATFORM_RESOURCES}
    resources/resources.qrc
)

# ── QML module ────────────────────────────────────────────
file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS "qml/*.qml")
file(GLOB_RECURSE QMLDIR_FILES CONFIGURE_DEPENDS "qml/**/qmldir")
file(GLOB_RECURSE QML_SVG_FILES CONFIGURE_DEPENDS "qml/*.svg")

qt_add_resources(FsNext "qml_resources"
    PREFIX "/qml"
    BASE "qml"
    FILES ${QML_FILES} ${QMLDIR_FILES} ${QML_SVG_FILES}
)

# ── Aurora fonts (optional — bundled into qrc if downloaded) ──────────────
# The Aurora design variant requires Geist / Geist Mono / Instrument Serif /
# Be Vietnam Pro. Fonts are not vendored in git; run
#   pwsh scripts/fetch-aurora-fonts.ps1   (or the .sh equivalent)
# once to populate resources/fonts/. CONFIGURE_DEPENDS re-globs on every
# build, so newly added font files are picked up without re-running cmake.
file(GLOB AURORA_FONT_FILES CONFIGURE_DEPENDS "resources/fonts/*.ttf")
if(AURORA_FONT_FILES)
    qt_add_resources(FsNext "aurora_fonts"
        PREFIX "/fonts"
        BASE "resources/fonts"
        FILES ${AURORA_FONT_FILES}
    )
    list(LENGTH AURORA_FONT_FILES _aurora_font_count)
    message(STATUS "FsNext: Aurora fonts bundled (${_aurora_font_count} file(s))")
else()
    message(STATUS "FsNext: Aurora fonts NOT bundled — run scripts/fetch-aurora-fonts.{ps1,sh} to enable")
endif()

# ── App icons (tray icon, window icon) ────────────────────────────────────
# resources/icons/app.png is loaded by SystemTray.cpp as ":/icons/app.png".
# Without it the tray icon is invisible — user cannot restore the window
# after minimize-to-tray. CONFIGURE_DEPENDS re-globs every build so new
# icons get picked up without re-running cmake.
file(GLOB APP_ICON_FILES CONFIGURE_DEPENDS
    "resources/icons/*.png"
    "resources/icons/*.svg"
    "resources/icons/*.ico"
)
if(APP_ICON_FILES)
    qt_add_resources(FsNext "app_icons"
        PREFIX "/icons"
        BASE "resources/icons"
        FILES ${APP_ICON_FILES}
    )
    list(LENGTH APP_ICON_FILES _app_icon_count)
    message(STATUS "FsNext: App icons bundled (${_app_icon_count} file(s))")
else()
    message(WARNING "FsNext: No app icons in resources/icons/ — tray icon will be invisible")
endif()

# ── Include directories ───────────────────────────────────
target_include_directories(FsNext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/cppcodec
)

# ── Link libraries ────────────────────────────────────────
target_link_libraries(FsNext PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Svg
    Qt6::Widgets   # QSystemTrayIcon — drives main.cpp to use QApplication
                   # instead of QGuiApplication (Widgets needs the wider event loop).
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp
)

# ── Platform libraries (Windows) ──────────────────────────
if(WIN32)
    target_link_libraries(FsNext PRIVATE
        ws2_32
        crypt32
        wldap32
        normaliz
        secur32
        advapi32
        user32
        bcrypt
        iphlpapi
        shell32
        ole32     # CoCreateInstance for ITaskbarList3 (TaskbarProgress.cpp)
    )
    target_compile_definitions(FsNext PRIVATE
        CURL_STATICLIB
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    if(MSVC)
        target_compile_options(FsNext PRIVATE
            # MSVC defaults source-encoding to system codepage (CP1252/CP1258),
            # mangling any non-ASCII Vietnamese string literals baked into the
            # binary (OAuth success page, error toasts, log messages). /utf-8
            # forces BOTH source charset and execution charset to UTF-8, which is
            # what QString/QByteArray::toUtf8 expects everywhere.
            /utf-8
            # Suppress C4858: QtConcurrent::run() future return value discarded (intentional)
            /wd4858
        )
    else()
        target_compile_options(FsNext PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
        )
    endif()
endif()

# ── Platform libraries (macOS) ────────────────────────────
# SecureStore uses Keychain Services (Security.framework) to persist
# secrets. CoreFoundation is needed for the CF*-family helpers.
if(APPLE)
    target_link_libraries(FsNext PRIVATE
        "-framework Security"
        "-framework CoreFoundation"
    )
endif()

# ── Platform libraries (Linux) ────────────────────────────
# SecureStore uses libsecret (Secret Service D-Bus API) when available.
# If the dev machine lacks libsecret-1 headers, the fallback branch in
# SecureStore.cpp refuses to persist rather than silently storing
# plaintext — so this dependency is OPTIONAL.
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)
        if(LIBSECRET_FOUND)
            target_link_libraries(FsNext PRIVATE PkgConfig::LIBSECRET)
            target_compile_definitions(FsNext PRIVATE FSNEXT_HAVE_LIBSECRET)
        else()
            message(WARNING
                "libsecret-1 not found — SecureStore will refuse to persist secrets on this build. "
                "Install libsecret-1-dev (Debian/Ubuntu) or libsecret-devel (Fedora) to enable secret persistence.")
        endif()
    endif()
endif()

# ── Compile definitions ───────────────────────────────────
target_compile_definitions(FsNext PRIVATE
    FSNEXT_VERSION="${PROJECT_VERSION}"
    FSNEXT_REVISION="${GIT_REVISION}"
    FSNEXT_VERSION_FULL="${PROJECT_VERSION}.${GIT_REVISION}"
)

# DEV build flag — enables dev-only features (auto-login with test account, dev bypass).
# Only enabled for Debug/RelWithDebInfo builds OR when explicitly requested with -DFSNEXT_ENABLE_DEV_BUILD=ON
# For production Release builds, this is OFF and dev code is compiled out.
option(FSNEXT_ENABLE_DEV_BUILD "Enable dev-only features (hardcoded test creds, API bypass)" OFF)

if(FSNEXT_ENABLE_DEV_BUILD OR CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    target_compile_definitions(FsNext PRIVATE FSNEXT_DEV_BUILD=1)
    message(STATUS "FsNext: DEV build enabled (dev credentials + bypass compiled in)")
else()
    message(STATUS "FsNext: Production build (dev features compiled OUT)")
endif()

# ── i18n: compile .ts → .qm (English translation) ───────────────────────
# Source language is Vietnamese (qsTr() source strings are Vietnamese).
# Only one .qm file is needed: fshare_en.qm  (English translation).
# fshare_vi.qm is NOT compiled — source strings ARE the Vietnamese display.
#
# To update translations after adding/changing qsTr() strings:
#   lupdate qml/ src/ -ts src/i18n/fshare_en.ts
# To recompile:
#   lrelease src/i18n/fshare_en.ts -qm output/translations/fshare_en.qm
#
file(MAKE_DIRECTORY "${FSNEXT_OUT_DIR}/translations")

find_program(LRELEASE_EXE
    NAMES lrelease6 lrelease
    HINTS "${Qt6_DIR}/../../../bin" "$ENV{QTDIR}/bin"
)
if(LRELEASE_EXE AND EXISTS "${CMAKE_SOURCE_DIR}/src/i18n/fshare_en.ts")
    add_custom_command(TARGET FsNext POST_BUILD
        COMMAND "${LRELEASE_EXE}"
                "${CMAKE_SOURCE_DIR}/src/i18n/fshare_en.ts"
                -qm "${FSNEXT_OUT_DIR}/translations/fshare_en.qm"
        COMMENT "Compiling fshare_en.ts → output/translations/fshare_en.qm"
        VERBATIM
    )
    message(STATUS "FsNext: lrelease wired → ${LRELEASE_EXE}")
else()
    message(STATUS "FsNext: lrelease not found or src/i18n/fshare_en.ts missing. "
                   "Run lrelease manually to compile translations.")
endif()

# ── qmllint target ────────────────────────────────────────
# Catches QML-side regressions that the C++ compiler can't see (e.g. typos in
# attached property capitalisation like the Overlay.modal/overlay.modal bug
# the upgrade pass fixed). Opt-in because qmllint is noisy on Qt 6.x — adopt
# fixes incrementally by running it manually for now.
#
#   cmake --build build --target qml_lint
#
option(FSNEXT_QML_LINT "Run qmllint over qml/ as part of the build" OFF)
find_program(QMLLINT_EXE
    NAMES qmllint qmllint6
    HINTS "${Qt6_DIR}/../../../bin" "$ENV{QTDIR}/bin"
)
if(QMLLINT_EXE)
    file(GLOB_RECURSE _qml_lint_files CONFIGURE_DEPENDS
        "qml/Main.qml"
        "qml/Fshare/Pages/*.qml"
        "qml/Fshare/Components/*.qml"
        "qml/Fshare/Dialogs/*.qml"
        "qml/Fshare/Utils/*.qml"
        "qml/FsAurora/Pages/*.qml"
        "qml/FsAurora/Components/*.qml"
        "qml/FsAurora/Theme/*.qml"
    )
    add_custom_target(qml_lint
        COMMAND "${QMLLINT_EXE}"
                -I "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                ${_qml_lint_files}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "qmllint over qml/ tree"
        VERBATIM
    )
    if(FSNEXT_QML_LINT)
        # When opt-in is set, hook into FsNext's PRE_BUILD so a green build
        # implies clean lint — useful in CI.  Local devs default to OFF.
        add_dependencies(FsNext qml_lint)
    endif()
    message(STATUS "FsNext: qmllint wired (target 'qml_lint'; auto-run = ${FSNEXT_QML_LINT})")
else()
    message(STATUS "FsNext: qmllint NOT found — install Qt's qmllint or set QTDIR env to enable target 'qml_lint'")
endif()

# ── Unit tests ────────────────────────────────────────────
# `tests/` is a sibling CMakeLists.  Gate it on FSNEXT_BUILD_TESTS so a
# dependency-light release build can opt out (default ON during dev/CI).
option(FSNEXT_BUILD_TESTS "Build FsNext unit tests" ON)
if(FSNEXT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── fsharenativeapp.exe — Chrome native-messaging host ───────────────────
# Tiny console binary spawned by Chrome.  Reads one length-prefixed JSON
# message from stdin, forwards the URL to the running FsNext.exe via the
# QLocalSocket single-instance channel, writes ACK back, exits.
qt_add_executable(fsharenativeapp
    src/platform/nativehost/nativehost_main.cpp
)
target_link_libraries(fsharenativeapp PRIVATE Qt6::Core Qt6::Network)
target_include_directories(fsharenativeapp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(fsharenativeapp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${FSNEXT_OUT_DIR}"
    WIN32_EXECUTABLE FALSE   # console subsystem — Chrome pipes stdin/stdout
)

# ── lupdate target: refresh fshare_en.ts from current qsTr() / tr() sites ──
# Use:  cmake --build build --target update_translations
# This is opt-in (NOT part of `all`) — translators rerun it before reviewing.
find_program(LUPDATE_EXE
    NAMES lupdate6 lupdate
    HINTS "${Qt6_DIR}/../../../bin" "$ENV{QTDIR}/bin"
)
if(LUPDATE_EXE)
    add_custom_target(update_translations
        COMMAND "${LUPDATE_EXE}"
                "${CMAKE_SOURCE_DIR}/qml"
                "${CMAKE_SOURCE_DIR}/src"
                -ts "${CMAKE_SOURCE_DIR}/src/i18n/fshare_en.ts"
                -no-obsolete
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "lupdate → src/i18n/fshare_en.ts (Vietnamese source)"
        VERBATIM
    )
endif()

# ── windeployqt (auto-deploy Qt DLLs + QML modules after link) ────────────
# Without this, the built .exe alone cannot run — Qt6*.dll, QtQuick plugins,
# QML module types, etc. must be side-loaded. Hook into POST_BUILD so every
# `cmake --build` leaves a runnable output/ tree.
if(WIN32)
    find_program(WINDEPLOYQT_EXE
        NAMES windeployqt6 windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "$ENV{QTDIR}/bin"
    )
    if(WINDEPLOYQT_EXE)
        set(_deploy_config_arg "--release")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_deploy_config_arg "--debug")
        endif()
        add_custom_command(TARGET FsNext POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}"
                    ${_deploy_config_arg}
                    --qmldir "${CMAKE_SOURCE_DIR}/qml"
                    --no-translations
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    "$<TARGET_FILE:FsNext>"
            COMMENT "Running windeployqt (Qt DLLs + QML modules → output/)"
            VERBATIM
        )
        message(STATUS "FsNext: windeployqt wired → ${WINDEPLOYQT_EXE}")
    else()
        message(WARNING "FsNext: windeployqt6 not found; output/ will be missing Qt DLLs. "
                        "Set Qt6_DIR or add Qt bin/ to PATH.")
    endif()
endif()

