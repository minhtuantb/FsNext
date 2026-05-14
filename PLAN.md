# Development Plan — FsNext (Fshare Tool v6.0)

## Phase Overview

| Phase | Name | Goal | Key Deliverables |
|-------|------|------|-----------------|
| 1 | Foundation | Build system + core infrastructure | CMake, main.cpp, AppContext, models, HttpClient |
| 2 | API Client | Complete API layer | FshareApi, all endpoints, ApiResponse, error handling |
| 3 | Auth | Login/logout working end-to-end | AuthService, AuthViewModel, LoginPage QML |
| 4 | Design System | Complete QML component library | Theme, all Fs* components, Main window shell |
| 5 | Download | Full download functionality | TransferEngine, DownloadViewModel, DownloadPage |
| 6 | Upload | Full upload functionality | UploadEngine, UploadViewModel, UploadPage |
| 7 | File Manager | File browsing and operations | FileService, FileManagerViewModel, FileManagerPage |
| 8 | Settings & Info | Settings and user info | SettingsService, SettingsPage, UserInfoPage |
| 9 | Platform | System integration | Tray, single instance, Chrome extension, drag&drop |
| 10 | Polish & Test | Final integration, testing, bug fixes | All integration tests pass, deploy scripts |

---

## Phase 1 — Foundation

**Goal**: Application builds, launches, and shows a blank QML window.

**Deliverables**:
- [ ] CMakeLists.txt (root + src/)
- [ ] CMakePresets.json (msvc2022 preset)
- [ ] main.cpp (QML engine setup, basic window)
- [ ] AppContext.h/.cpp (skeleton DI container)
- [ ] Application.h/.cpp (init: SSL, CURL, proxy)
- [ ] All core models (User, FileItem, TransferTask, TransferState, AppSettings, ApiResponse, AppError)
- [ ] HttpClient.h/.cpp (CURL wrapper: GET, POST, headers, proxy, SSL)
- [ ] resources.qrc (empty structure)
- [ ] Main.qml (basic ApplicationWindow)

**Success criteria**: `cmake --build` succeeds, app launches and shows a window for 30+ seconds.

**Dependencies**: Qt 6.8+, vcpkg (curl, openssl), Ninja, MSVC 2022

---

## Phase 2 — API Client

**Goal**: All Fshare REST API endpoints callable from C++.

**Deliverables**:
- [ ] FshareApi.h/.cpp (all 20 endpoints from API contracts)
- [ ] OAuthProvider.h/.cpp (Google, Facebook, FPT ID config)
- [ ] ApiRepository.h/.cpp (wraps FshareApi)
- [ ] SettingsRepository.h/.cpp (wraps QSettings)
- [ ] HistoryRepository.h/.cpp (JSON read/write)

**Success criteria**: Unit test or manual test calling login + getUserInfo + listFiles against real API.

**Dependencies**: Phase 1 (HttpClient, models)

---

## Phase 3 — Authentication

**Goal**: User can log in with email/password and see main layout.

**Deliverables**:
- [ ] AuthService.h/.cpp (login, logout, session, auto-login)
- [ ] AuthViewModel.h/.cpp (QML bridge for login state)
- [ ] LoginPage.qml (email, password, remember me, error)
- [ ] Main.qml updated (StackView: LoginPage → placeholder main)

**Success criteria**: Login with real Fshare account → navigate to main layout. Wrong password → error shown.

**Dependencies**: Phase 2 (FshareApi login endpoint)

---

## Phase 4 — Design System & Shell

**Goal**: Complete QML component library and main application shell.

**Deliverables**:
- [ ] FshareTheme.qml (copy + adapt from existing)
- [ ] FshareColors.qml (copy + adapt from existing)
- [ ] All FsComponents: FsButton, FsCard, FsTextField, FsProgressBar, FsNavigation, FsBadge, FsToast, FsDialog, FsTransferItem, FsFileRow, FsFolderTree, FsEmptyState, FsLoadingState, FsErrorState, FsSearchBar, FsDropZone, FsContextMenu
- [ ] Main window with sidebar navigation
- [ ] Page routing (placeholder pages for each section)
- [ ] Toast notification system
- [ ] Dark mode toggle
- [ ] Window minimum size (800×560)

**Success criteria**: Sidebar navigation works, all pages show placeholder content, dark mode toggles correctly. Components render according to design system.

**Dependencies**: Phase 3 (login works, can see main layout)

---

## Phase 5 — Download

**Goal**: User can add, start, pause, resume, cancel downloads. History persists.

**Deliverables**:
- [ ] DownloadEngine.h/.cpp (single + multi-segment CURL)
- [ ] TransferWorker.h/.cpp (QThread wrapper)
- [ ] TransferQueue.h/.cpp (queue with slot management)
- [ ] SpeedMeter.h/.cpp (speed + ETA)
- [ ] TransferService.h/.cpp (download orchestration)
- [ ] TransferListModel.h/.cpp (QAbstractListModel for downloads)
- [ ] DownloadViewModel.h/.cpp
- [ ] DownloadPage.qml (active list, history, toolbar, empty state)
- [ ] AddDownloadDialog.qml (link input, folder, password)
- [ ] PasswordDialog.qml (password prompt for protected files)

**Success criteria**: Download a real file from Fshare → progress shows → file saved correctly. Pause and resume works. History shows completed items after restart.

**Dependencies**: Phase 4 (components), Phase 2 (API: createDownloadSession)

---

## Phase 6 — Upload

**Goal**: User can upload files to Fshare with full queue management.

**Deliverables**:
- [ ] UploadEngine.h/.cpp (chunked CURL upload)
- [ ] TransferService.h/.cpp (upload orchestration — extend Phase 5)
- [ ] UploadViewModel.h/.cpp
- [ ] UploadPage.qml (active list, history, toolbar)
- [ ] AddUploadDialog.qml (file picker, folder picker, properties)
- [ ] FolderPickerDialog.qml (Fshare folder tree selection)

**Success criteria**: Upload a file to Fshare → progress shows → file appears in Fshare. Pause and resume works. History persists.

**Dependencies**: Phase 5 (TransferService, TransferQueue), Phase 2 (API: createUploadSession)

---

## Phase 7 — File Manager

**Goal**: User can browse, search, and manage files/folders on Fshare.

**Deliverables**:
- [ ] FileService.h/.cpp (all file operations)
- [ ] FileManagerViewModel.h/.cpp (folder tree + file list + actions)
- [ ] FileManagerPage.qml (tree, list, toolbar, context menu, search)
- [ ] FileInfoDialog.qml (file properties)
- [ ] ConfirmDialog.qml (delete confirmation, etc.)

**Success criteria**: Browse folders → see files → create/rename/delete/move/copy works. Search finds files. Secure/password/direct-link toggles work.

**Dependencies**: Phase 4 (components: FsFolderTree, FsFileRow, FsContextMenu), Phase 2 (all file API endpoints)

---

## Phase 8 — Settings & User Info

**Goal**: Settings page functional, user info displayed.

**Deliverables**:
- [ ] SettingsService.h/.cpp
- [ ] SettingsViewModel.h/.cpp
- [ ] SettingsPage.qml (general, connection, download, upload, about)
- [ ] UserInfoViewModel.h/.cpp
- [ ] UserInfoPage.qml (account, storage, traffic, VIP)
- [ ] UpdateService.h/.cpp (version check)
- [ ] AboutDialog.qml

**Success criteria**: Change settings → persists after restart. User info shows correct data. Version check works.

**Dependencies**: Phase 4 (components), Phase 2 (API: getUserInfo, getLatestVersion)

---

## Phase 9 — Platform Integration

**Goal**: System-level features: tray, single instance, Chrome extension, drag & drop.

**Deliverables**:
- [ ] SingleInstance.h/.cpp (named pipe/socket)
- [ ] SystemTray.h/.cpp (icon, context menu, minimize to tray)
- [ ] NativeMessaging.h/.cpp (Chrome extension host)
- [ ] PlatformUtils.h/.cpp (auto-start, proxy detection)
- [ ] Drag & drop support in Main.qml
- [ ] Window always-on-top toggle

**Success criteria**: Double-click app → second instance sends link to first. Tray icon works. Chrome extension sends links to app.

**Dependencies**: Phase 5 (download, to test Chrome extension → download flow)

---

## Phase 10 — Polish & Integration Testing

**Goal**: All features working, stable, ready for release.

**Deliverables**:
- [ ] All integration tests pass (see CHECKLIST.md)
- [ ] OAuth login (Google, Facebook, FPT ID) working
- [ ] Edge cases handled (disk full, network error, token expiry)
- [ ] Debug scripts (debug_run, check_deps, clear_cache, health_check)
- [ ] Deploy script (package for distribution)
- [ ] REPORT.md (final summary)
- [ ] STATUS.md updated to "Complete"

**Success criteria**: App runs stable for extended period. All CHECKLIST items checked. No critical bugs.

**Dependencies**: All previous phases

---

## Phase Results

_Updated after each phase completion._

| Phase | Status | Start Date | End Date | Notes |
|-------|--------|------------|----------|-------|
| 1 | Not started | — | — | — |
| 2 | Not started | — | — | — |
| 3 | Not started | — | — | — |
| 4 | Not started | — | — | — |
| 5 | Not started | — | — | — |
| 6 | Not started | — | — | — |
| 7 | Not started | — | — | — |
| 8 | Not started | — | — | — |
| 9 | Not started | — | — | — |
| 10 | Not started | — | — | — |
