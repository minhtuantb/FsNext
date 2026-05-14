# Đánh giá codebase FsNext & Kế hoạch triển khai

**Ngày**: 2026-04-28
**Phạm vi**: Toàn bộ repo `D:\Work\FsNext` — Qt 6.8.3 / C++17 / QML, MVVM + Clean Architecture
**Phiên bản tham chiếu**: v6.0.0.176 (theo `REPORT.md`), 10 phase ban đầu đã hoàn thành; sau đó đã mở rộng thêm Sync, FileCacheService, TransferOrchestrator/BudgetManager, Aurora design system.

---

## 1. Tóm tắt điều hành

FsNext là bản viết lại từ đầu của desktop client Fshare, hiện tại là một codebase **trưởng thành về kiến trúc và đã chạy được**. Layering rất sạch (Models → Repositories → Services → ViewModels → QML), DI qua `AppContext`, không global state, async qua `QtConcurrent` + `QMetaObject::invokeMethod`, error handling typed (`AppError` + `ErrorCategory`).

Tuy nhiên project đang ở **trạng thái chuyển tiếp về UI**: hai design system cùng tồn tại (Fshare cũ + FsAurora mới), pages đã được port sang `AuroraTheme` nhưng các component bên dưới vẫn import `FshareTheme` (giá trị token đã được alias sang Aurora). Một số Phase 9 deliverable vẫn ở dạng stub (SystemTray, NativeMessaging cho Chrome extension), test coverage còn hẹp (chỉ FileCacheDB và FileCacheService), translation chỉ có file English. Build gần nhất fail vì exe đang chạy (không phải lỗi code).

Đề xuất hành động ngắn gọn: (1) đóng cứng design system về Aurora, (2) hoàn tất Phase 9 (tray + Chrome extension), (3) bù test cho transfer/auth, (4) bổ sung tiếng Việt làm source-of-truth, (5) các tính năng chiều sâu trong BACKLOG (multi-segment download, chunked upload).

---

## 2. Cấu trúc & quy mô

| Khu vực | Files | LOC ước tính |
|---|---|---|
| `src/core/` (models, api, services, repositories, transfer, cache, util) | 67 .cpp/.h | ~10,500 |
| `src/viewmodels/` | 26 .cpp/.h | ~5,449 |
| `src/app/` + `src/platform/` + `src/main.cpp` | 8 | ~620 |
| `qml/Fshare/Pages/` (9 pages) | 9 | ~8,962 |
| `qml/Fshare/Components/` (33 components) | 33 | ~4,351 |
| `qml/FsAurora/` (Theme + 10 Components + 3 Pages + design source) | 16 | ~3,200 |
| `qml/Main.qml` | 1 | 389 |
| Tests (`tests/`) | 2 | ~600 |
| Docs (`docs/`) | 13 | n/a |

Tổng: **113 file C++** + **64 file QML**, ~25k LOC C++/QML. Tài liệu kiến trúc đầy đủ trong `docs/` (00–07 + decisions + auto-upload-ui-design + test cases).

Build system: CMake 3.24+ / Ninja / MSVC 2022 / vcpkg static-md. Phụ thuộc: Qt 6.8.3, libcurl, OpenSSL, jsoncpp, cppcodec, zlib, nghttp2.

---

## 3. Đánh giá kiến trúc

### 3.1 Điểm mạnh

- **Layering nghiêm ngặt**. ViewModel chỉ phụ thuộc Service; Service chỉ phụ thuộc Repository/API; Models thuần data — đúng đồ thị phụ thuộc trong `docs/05_dependency_graph.md`.
- **Dependency injection rõ ràng**. `AppContext` là composition root duy nhất (`src/app/AppContext.cpp`), tất cả service/VM dùng `std::unique_ptr`, life-time được khai báo theo thứ tự (OAuthService → AuthService; TransferOrchestrator → TransferService) để tránh dangling reference. Không có `extern` global hay singleton ẩn.
- **Transfer subsystem rất chỉn chu**. `TransferOrchestrator` + `BudgetManager` + `PriorityScheduler` tách biệt "khi nào start" (orchestrator) khỏi "start cái gì" (TransferService/SyncService/FileSyncWorker). Có per-class slot caps, global cap, floor quota cho background tasks chống starvation, multi-thread (orchestrator chạy trên thread riêng), config thay đổi runtime qua `setConfig()` mà không kill task đang chạy.
- **Error handling typed**. `ApiResponse<T>` template và `AppError` với `ErrorCategory` (Network/Auth/Server/Storage/Transfer/Validation) cho phép caller pattern-match thay vì so sánh chuỗi. Mỗi error mang sẵn `retryable` để UI biết có nên cho retry.
- **Resilience tốt**. Crash handlers cho cả `std::terminate` và Windows SEH (`src/main.cpp:104-158`), log rotation 5 MiB × 4 file (`src/main.cpp:33`), single-instance qua QLocalServer/Socket, secure credential storage qua DPAPI (`SecureStore.cpp`) thay vì base64 cũ.
- **Cache layer SQLite + FTS5**. `FileCacheDB` (705 LOC) cung cấp full-text search, schema migration tự động, isolation theo `user_id`. `FileSyncWorker` chạy nền, ưu tiên thấp qua orchestrator để không tranh slot với download/upload.
- **i18n hooks**. `LanguageViewModel::translationReloadNeeded` → `engine.retranslate()` trong `main.cpp` cho phép đổi ngôn ngữ runtime; mọi UI string đều dùng `qsTr()` (tổng hơn 470 site qua các page).
- **Session-expired flow gọn**. `TransferService::sessionExpired` → `AuthService::handleSessionExpired` → emit `sessionExpiredNotice` cho QML (`AppContext.cpp:79`). Không cần state machine ngoài đó.

### 3.2 Điểm yếu / rủi ro

- **Hai design system song song**. `qml/Fshare/Components/` (33 file) vẫn import `Fshare.Theme`, trong khi `qml/Fshare/Pages/` đã port hoàn toàn sang `FsAurora.Theme`. Hiện workaround là `FshareColors.qml` được aliased — token name cũ (`red`, `bg0`...) trỏ sang giá trị Aurora. Hoạt động được nhưng:
  - Tăng cognitive load khi đọc code: cùng một màu có 2 tên (`FshareTheme.red` ↔ `AuroraTheme.accent`).
  - Hai trường `isDark` (FshareTheme.isDark + AuroraTheme.isDark) phải mirror tay trong `Main.qml:112-124`.
  - Comment trong `FshareColors.qml` đã ghi rõ "When pages are properly ported to FsAurora.* components, this file and FshareTheme.qml go away" — kế hoạch xoá có sẵn nhưng chưa thực thi.
- **Phase 9 chưa hoàn tất**. `SystemTray.cpp` còn `// TODO: Phase 9 - implement system tray with QSystemTrayIcon` (3 chỗ trong `src/platform/SystemTray.{h,cpp}`). `NativeMessaging` (Chrome extension host) hoàn toàn không có file — `CHECKLIST.md` yêu cầu `Platform > Chrome extension native messaging host` nhưng không tồn tại trong `src/platform/`.
- **Test coverage hẹp**. Chỉ 2 test executable: `test_file_cache_db` (21 test, đều PASS) và `test_file_cache_service` (15 test, đều PASS). Không có test cho `AuthService`, `TransferService`, `DownloadEngine`, `UploadEngine`, `FshareApi`, `BudgetManager`, `PriorityScheduler`, hay bất kỳ ViewModel nào. Vì kiến trúc DI sạch nên việc viết test ở các tầng đó hoàn toàn khả thi.
- **Translation file mới có English**. `src/i18n/fshare_en.ts` (495 dòng), nhưng UI hiện tại đều là tiếng Việt — về mặt logic Vietnamese là source language, English là target. Cần (a) tạo file `fshare_vi.ts` để dịch ngược, hoặc (b) định nghĩa rõ source là English và port toàn bộ `qsTr("Tải xuống")` → `qsTr("Download")` rồi dịch sang vi. Không xử lý → user thay đổi ngôn ngữ sang English thấy phần lớn UI vẫn hiển thị tiếng Việt vì chưa có translation cho strings đó.
- **Backend API blocker (đã ghi nhận)**. `POST /api/user/login` trả HTTP 400 (BACKLOG.md). Ghi chú: `dev bypass` đề cập trong `STATUS.md` đã không còn trong `AuthService.cpp` (đã xoá), nên hiện tại không có fallback — login real account là **bắt buộc** mà API hỏng → app không qua được màn login. Cần đẩy backend hoặc bật lại dev bypass có cờ.
- **Multi-segment download / chunked upload chưa implement**. BACKLOG nhận diện rõ. `DownloadEngine.cpp` (622 LOC) chỉ single-segment; `UploadEngine.cpp` (407 LOC) là single request — không retry per-chunk, không adaptive sizing. File >100MB trên mạng không ổn định sẽ hỏng.
- **Disk space pre-check thiếu**. `PlatformUtils::freeDiskSpace()` đã có nhưng chưa được gọi trước khi enqueue download. Edge case `06_edge_cases.md §2.4` chưa đóng.
- **File-name conflict UX không nhất quán**. Một số path overwrite, một số append số. Cần policy thống nhất + UI prompt (Skip/Overwrite/Rename).
- **Token refresh không có**. Session token chỉ giữ trong memory; expire mid-operation → user thấy `sessionExpired` toast và phải login lại. Có thể chấp nhận v6.0 nhưng nên ghi vào BACKLOG.
- **Connection pooling thiếu**. `HttpClient` tạo CURL handle mới mỗi call (`docs/BACKLOG.md`). Với app gọi list/get/info dày đặc khi browse, chi phí TLS handshake tích luỹ nhanh. Nên dùng `curl_easy_handle` reuse hoặc CURL multi-handle pool.
- **OAuth secret hardcoded**. Theo BACKLOG, `client_id/client_secret` của Google/Facebook/FPT-ID nằm trong `OAuthProvider.h` — desktop app không có "secret" thật sự, nhưng vẫn nên build-flag riêng cho dev/prod để không leak prod credentials qua repo dev.

---

## 4. Kiểm tra từng module / page / function

### 4.1 Authentication

| Component | File | Trạng thái | Ghi chú |
|---|---|---|---|
| `AuthService` | `src/core/services/AuthService.{h,cpp}` (75+395) | ✅ Login/logout/autoLogin/fetchUserInfo + sessionExpired | DPAPI thay base64 OK |
| `OAuthService` | `src/core/services/OAuthService.{h,cpp}` (82+385) | ✅ Loopback HTTP server + PKCE | RFC 8252 đúng chuẩn |
| `OAuthProvider` | `src/core/api/OAuthProvider.h` (113) | ⚠️ Secrets hardcoded | Tách build flag |
| `AuthViewModel` | `src/viewmodels/AuthViewModel.{h,cpp}` (78+157) | ✅ Đầy đủ Q_PROPERTY + sessionExpiredNotice |  |
| `LoginView.qml` | `qml/FsAurora/Pages/LoginView.qml` (545) | ✅ Aurora layout split | OAuth buttons có sẵn |
| **Backend integration** | — | ❌ HTTP 400 | Blocker, cần backend |

### 4.2 Download

| Component | File | Trạng thái |
|---|---|---|
| `DownloadEngine` | `src/core/transfer/DownloadEngine.{h,cpp}` (88+622) | ⚠️ Single-segment, có pause/resume/cancel/retry |
| `TransferService` (DL path) | `src/core/services/TransferService.{h,cpp}` (179+1019) | ✅ Queue + dispatch qua orchestrator |
| `FolderExpander` | `src/core/services/FolderExpander.{h,cpp}` (88+212) | ✅ Folder URL → tree → enqueue files, cancel-able |
| `DownloadViewModel` | `src/viewmodels/DownloadViewModel.{h,cpp}` (156+457) | ✅ Aggregate speed, scan banner state, infinite-scroll history |
| `TransferListModel` | `src/viewmodels/TransferListModel.{h,cpp}` (88+194) | ✅ Role-based, đúng QML idiom |
| `DownloadPage.qml` | `qml/Fshare/Pages/DownloadPage.qml` (702) | ✅ Editorial header, segmented Active/History, drop zone, scan banner, system-folder block toast |
| **Multi-segment** | — | ❌ Chưa có |
| **Disk space check** | — | ❌ Chưa gọi `PlatformUtils::freeDiskSpace` |

### 4.3 Upload

| Component | File | Trạng thái |
|---|---|---|
| `UploadEngine` | `src/core/transfer/UploadEngine.{h,cpp}` (63+407) | ⚠️ Single request, có pause/resume/cancel |
| `UploadViewModel` | `src/viewmodels/UploadViewModel.{h,cpp}` (182+483) | ✅ Local-file-delete confirm flow, signal `onLocalFileDeleted/onLocalFileDeleteFailed` |
| `UploadPage.qml` | `qml/Fshare/Pages/UploadPage.qml` (595) | ✅ Drop zone gradient, queue/history segmented, dialog có folder picker + privacy + password |
| `FsUploadDialog.qml` | `qml/Fshare/Components/FsUploadDialog.qml` (349) | ✅ Đầy đủ UI |
| **Chunked + per-chunk retry** | — | ❌ Chưa có |
| **Quota pre-check** | — | ⚠️ Chỉ phát hiện sau khi createUploadSession fail |

### 4.4 File Manager

| Component | File | Trạng thái |
|---|---|---|
| `FileService` | `src/core/services/FileService.{h,cpp}` (60+223) | ✅ Tất cả 12 file ops async |
| `FileCacheService` | `src/core/services/FileCacheService.{h,cpp}` (174+534) | ✅ State-machine WriteOp với pendingOp, có 15 unit test |
| `FileCacheDB` | `src/core/cache/FileCacheDB.{h,cpp}` (98+705) | ✅ FTS5, isolation user_id, 21 unit test |
| `BatchFileResolver` | `src/core/services/BatchFileResolver.{h,cpp}` (78+133) | ✅ Bounded-concurrency URL→FileItem |
| `FileManagerViewModel` | `src/viewmodels/FileManagerViewModel.{h,cpp}` (245+601) | ✅ Sort/filter/selection/search/breadcrumb/back-forward |
| `FileListModel` + `FolderTreeModel` + `FolderPickerModel` | `src/viewmodels/...` | ✅ |
| `FileManagerPage.qml` | `qml/Fshare/Pages/FileManagerPage.qml` (2595) | ⚠️ File rất lớn — context menu, rename dialog, share dialog, info panel, bulk action toolbar đã đủ; cần tách subcomponent |
| Stream / direct play | `PlatformUtils::playStreamUrl` | ✅ Trick m3u8 thay vì duyệt browser |

### 4.5 Sync (Auto Upload / one-way local→cloud)

| Component | File | Trạng thái |
|---|---|---|
| `SyncService` | `src/core/services/SyncService.{h,cpp}` (147+636) | ✅ File watcher, debounce, scan, max 5 folders, delete-after-upload |
| `SyncRepository` | `src/core/repositories/SyncRepository.{h,cpp}` (44+166) | ✅ Persist folder list & file states |
| `SyncViewModel` | `src/viewmodels/SyncViewModel.{h,cpp}` (153+454) | ✅ 2 model con: SyncFoldersModel, SyncFilesModel |
| `SyncPage.qml` | `qml/Fshare/Pages/SyncPage.qml` (830) | ✅ Editorial header, folder pair list, file list, toolbar, dialog Add/Remove/Settings |
| `AutoUploadPage.qml` | `qml/Fshare/Pages/AutoUploadPage.qml` (561) | ✅ Trang riêng cho monitoring (theo `auto-upload-ui-design.md`) |
| `FsWatchCard.qml` | `qml/Fshare/Components/FsWatchCard.qml` (427) | ✅ |
| Watch dialogs | `qml/Fshare/Dialogs/{AddWatchFolderDialog,RemoveWatchDialog,WatchFolderSettingsDialog}.qml` | ✅ |

### 4.6 Settings

| Component | File | Trạng thái |
|---|---|---|
| `SettingsService` | `src/core/services/SettingsService.{h,cpp}` (96+213) | ✅ 17 fields, persist QSettings |
| `SettingsViewModel` | `src/viewmodels/SettingsViewModel.{h,cpp}` (97+191) | ✅ |
| `SettingsRepository` | `src/core/repositories/SettingsRepository.{h,cpp}` (31+131) | ✅ |
| `SettingsPage.qml` | `qml/Fshare/Pages/SettingsPage.qml` (603) | ✅ Card-based, có General/Connection/Download/Upload/About sections |
| Live re-config slot caps | `AppContext.cpp:96-107` | ✅ settingsChanged → orchestrator.setConfig |

### 4.7 User info / Favorites / Home

| Component | File | Trạng thái |
|---|---|---|
| `UserInfoViewModel` | `src/viewmodels/UserInfoViewModel.{h,cpp}` (96+176) | ✅ |
| `UserInfoPage.qml` | `qml/Fshare/Pages/UserInfoPage.qml` (917) | ✅ |
| `FavoritesViewModel` | `src/viewmodels/FavoritesViewModel.{h,cpp}` (141+346) | ✅ |
| `FavoritesPage.qml` | `qml/Fshare/Pages/FavoritesPage.qml` (1373) | ⚠️ File rất lớn, nên tách collections ribbon + grid |
| `HomePage.qml` (Aurora) | `qml/FsAurora/Pages/HomePage.qml` (959) | ✅ Greeting + quick action + recent files |

### 4.8 Platform / Shell

| Component | File | Trạng thái |
|---|---|---|
| `SingleInstance` | `src/platform/SingleInstance.{h,cpp}` | ✅ |
| `SystemTray` | `src/platform/SystemTray.{h,cpp}` | ❌ Stub |
| `PlatformUtils` | `src/platform/PlatformUtils.{h,cpp}` | ✅ defaultDownloadFolder, autoStart, freeDiskSpace, proxyFromSystem, isSystemFolder, openInExplorer, openFile, playStreamUrl |
| Drag&drop global router | `qml/Main.qml:42-106` | ✅ Local file → Upload, Fshare URL → Download |
| Ctrl+V paste hook | `qml/Main.qml:144-152` | ✅ |
| Crash handlers | `src/main.cpp:104-158` | ✅ |
| Log rotation | `src/main.cpp:33-84` | ✅ |
| Native messaging (Chrome) | — | ❌ Không có file |
| Window minimum size | `qml/Main.qml:21-22` | ✅ 800×560 |

---

## 5. Đánh giá UI / UX so với Design System

### 5.1 Token coverage

`qml/FsAurora/Theme/AuroraTheme.qml` đã expose đầy đủ token theo handoff:
- Brand: accent (FF5B2E orange), accent2 (FFAF1D yellow), accent3 (FF3D7F pink), gradient stops, accentSoft/Tint10/Tint15.
- Semantic: success/warn/danger/info + soft variant.
- Surface adaptive (light/dark): bg, panel, sidebar (dark in both modes intentionally), border/borderStrong/divider, ink1–4.
- Typography: fontSans (Geist + Be Vietnam Pro), fontMono (Geist Mono), fontSerif (Instrument Serif). Type scale từ hero (48px) tới label/caption đúng spec.
- Spacing 4-px grid: sp1..sp16.
- Radius: sm (6) / md (10) / lg (14) / xl (20) / pill (999).
- Motion: durFast 140 / durBase 220 / durSlow 360, easing OutCubic + bezier `[0.2, 0, 0, 1]`.
- Heights: input 40 / button sm/md/lg / list row 44 / titlebar / toolbar.

Pages dùng đúng token (DownloadPage / UploadPage / SyncPage / SettingsPage / UserInfoPage / FavoritesPage / FileManagerPage / AutoUploadPage). Header pattern lặp lại nhất quán: kicker mono uppercase + serif mega-number + mini stats + actions — match đúng "editorial header" trong handoff.

### 5.2 Vấn đề UI/UX cần xử lý

1. **Component vẫn dùng `FshareTheme.*`** (33 file). Token có alias nên render đúng màu, nhưng:
   - Vài size token khác nhau giữa hai theme: ví dụ `FshareTheme.heightInput = 32` vs `AuroraTheme.heightInput = 40`. Component như `FsTextField` đang dùng 32px → không ăn khớp với pattern Aurora 40px input. Kiểm tra `qml/Fshare/Components/FsTextField.qml:24` để xác nhận.
   - `FshareTheme` có `bg0..bg3` (4 mức), `AuroraTheme` chỉ có `bg/panel`. Component vẫn dùng `bg2` cho "hover surface" mà Aurora không có → lệch palette.
2. **Reduce motion**. Cả hai theme có `reduceMotion` flag nhưng chưa wire vào `QAccessible::isActive()` từ C++. Hiện tại chỉ là property tĩnh — không tự đổi theo OS setting. Cần Application::initAccessibility() để query rồi push vào theme.
3. **Focus ring & a11y**. Handoff yêu cầu focus ring 2px accent + offset 2px. Một số component (FsButton, FsTextField) có outline khi focus, nhưng chưa kiểm tra a11y label trên icon-only buttons. Audit nhanh: tất cả `Aurora.FsIcon` trong toolbar phải có `accessibleName` hoặc tooltip.
4. **Empty / loading / error state**. Có `FsEmptyState`, `FsLoadingState`, `FsSkeletonLoader` nhưng skeleton chưa wire vào FileManagerPage khi đang load folder lớn — user thấy bảng trống. Tương tự DownloadPage history khi `loadMoreHistory()` chạy.
5. **Dialog sizing**. `FsDialog` mặc định không scroll khi nội dung dài (FsUploadDialog 349 LOC lúc 5+ files trong queue có thể vượt viewport). Cần thêm ScrollView khi `dialogHeight > parent.height * 0.8`.
6. **File rất lớn**:
   - `FileManagerPage.qml` (2595 LOC): nên tách context menu builder, rename dialog, share dialog, detail panel ra subcomponent. Hiện tại refactor một section sẽ chạm cả file.
   - `FavoritesPage.qml` (1373 LOC): tương tự.
   - `HomePage.qml` (959 LOC): có thể giữ vì chỉ dashboard.
7. **Showcase page chỉ bật ở dev build** (`Main.qml:282`, `currentPage === 7`). Tốt — nhưng cần đảm bảo `FSNEXT_DEV_BUILD` define được tắt mặc định trong `CMakePresets.json` cho preset production.
8. **HomePage dashboard cần tích hợp transfer-budget hint**. Hiện sidebar đã hiện "DL 2/8 · UL 1/4" nhưng HomePage hero chưa nhắc. Khi user vào app lần đầu sau sync chạy nền, họ không biết.
9. **i18n**. Pages có ~470 site `qsTr()`, gần 100% là tiếng Việt. Cần quyết định nguồn:
   - **Option A** (đơn giản nhất): khẳng định Vietnamese là source language, tạo `fshare_vi.ts` với fallback `<source>` = `<translation>`, dịch `fshare_en.ts` đầy đủ. App boot mặc định vi, đổi sang en chỉ override key có translation.
   - **Option B**: chuyển source string sang English, dịch cả vi và en. Cost cao (450+ string × 2 lần review).
10. **Drag&drop UX**. `Main.qml` route đúng nhưng không có hover feedback toàn cửa sổ — handoff yêu cầu "zone highlight gradient khi dragover". `UploadPage` có drop zone visual riêng, nhưng khi user drop ở page khác (Download, Files…) thì không có feedback. Cần overlay toàn cửa sổ với gradient soft khi `Drag` enter.

---

## 6. Kế hoạch triển khai (4 đợt)

Lộ trình đề xuất 4 đợt × ~2 tuần. Mỗi đợt khép kín, có thể release internal.

### Đợt 1 — Đóng cứng nền tảng (P0)

| # | Task | Output | File ảnh hưởng |
|---|---|---|---|
| 1.1 | Backend đẩy fix `/api/user/login` HTTP 400, hoặc bật lại dev bypass có cờ `FSNEXT_DEV_BYPASS` riêng (không bật mặc định) | Login real account thông | `AuthService.cpp`, `BACKLOG.md` |
| 1.2 | Tạo `fshare_vi.ts` (source-of-truth tiếng Việt), generate `fshare_en.ts` đầy đủ — dịch full bảng strings | 2 file `.ts` + lupdate cron | `src/i18n/`, CMake `qt_add_translations` |
| 1.3 | Wire `FshareTheme.reduceMotion` & `AuroraTheme.reduceMotion` từ `QAccessible::isActive()` lúc init | Khi user bật "Reduce motion" trên Windows, mọi `Behavior` tắt | `Application.cpp`, `Main.qml` |
| 1.4 | Migrate batch component dùng `FshareTheme` → `AuroraTheme` (33 file). Bắt đầu từ atoms: FsButton, FsTextField, FsCheckbox, FsRadio, FsSwitch, FsBadge, FsSpinner, FsToast | Component reference 100% Aurora | `qml/Fshare/Components/*.qml` |
| 1.5 | Sau khi component port xong → xoá `qml/Fshare/Theme/{FshareTheme,FshareColors}.qml`, gỡ cờ mirror trong `Main.qml` | 1 design system duy nhất | `Main.qml`, `qmldir` |
| 1.6 | Bù test: `AuthService` (login success/failure/expired), `BudgetManager` (acquire/release/floor), `PriorityScheduler`, `SpeedMeter`, `FshareUrl` parser, `FileNameSanitizer` | ≥ 60% line coverage core | `tests/test_*.cpp` |

**Định nghĩa hoàn thành**: build pass + 1 design system + ≥ 80 test PASS + i18n đủ vi/en.

### Đợt 2 — Hoàn tất Phase 9 + Robustness (P0)

| # | Task | File / Module |
|---|---|---|
| 2.1 | Triển khai `SystemTray` thật: QSystemTrayIcon (cần thêm Qt6::Widgets vào CMake), context menu (Hiện cửa sổ / Tạm dừng tất cả / Thoát), notification cho download complete + transfer error | `src/platform/SystemTray.{h,cpp}` |
| 2.2 | Minimize-to-tray: hook `closeEvent` với option settings.minimizeToTray | `Main.qml`, `AppSettings.h`, `SettingsService.cpp` |
| 2.3 | Native messaging host cho Chrome extension: `fsharenativeapp.exe` riêng (4-byte length prefix + JSON), registry key `HKCU\Software\Google\Chrome\NativeMessagingHosts\com.fshare.tool` | `src/platform/NativeMessaging.{h,cpp}`, `src/platform/native_main.cpp`, deploy script |
| 2.4 | Chrome extension MV3 mẫu (manifest + popup → SendMessage → host → IPC tới single-instance running) | `extension/` (thư mục mới) |
| 2.5 | Disk space pre-check trong `TransferService::addDownload`, block khi `freeDiskSpace < expectedSize × 1.1`; UI toast | `TransferService.cpp`, `DownloadPage.qml` |
| 2.6 | File-name conflict policy nhất quán: prompt Skip/Overwrite/Rename, lưu choice trong settings ("apply to all in queue") | `DownloadEngine.cpp`, `FsConfirmDialog.qml` (mới: FsConflictDialog) |
| 2.7 | Connection pooling: chuyển `HttpClient` sang reuse `CURL*` per host (curl share + cookie jar), giữ keep-alive | `src/core/api/HttpClient.{h,cpp}` |
| 2.8 | Token refresh: nếu API trả 401/403 mid-operation → AuthService retry login với credential lưu, transparent với UI; thất bại mới `sessionExpired` | `AuthService.cpp`, `TransferService.cpp`, `FshareApi.cpp` |

**Định nghĩa hoàn thành**: tray hoạt động, Chrome extension push được URL, edge case disk-full + name-conflict có UI, không còn 401 silent fail.

### Đợt 3 — Transfer chiều sâu (P1)

| # | Task | File / Module |
|---|---|---|
| 3.1 | Multi-segment download: probe HTTP 206, split N segments (settings 1–16), CURL multi-handle, merge khi xong, fallback single nếu server không hỗ trợ Range | `DownloadEngine.{h,cpp}`, `SettingsViewModel.h` (đã có `downloadSegments`) |
| 3.2 | Per-segment retry + real-link refresh khi segment fail | `DownloadEngine.cpp` |
| 3.3 | Chunked upload 20 MiB chunks + Content-Range, per-chunk retry (tới 100), session renewal khi 401, adaptive chunk size theo RTT | `UploadEngine.{h,cpp}` |
| 3.4 | Progress persistence cho download (state.json per task) → resume sau crash | `TransferService.cpp`, `HistoryRepository.cpp` |
| 3.5 | Speed limit (CURLOPT_MAX_RECV_SPEED_LARGE / MAX_SEND_SPEED_LARGE) UI: input "MB/s" trong SettingsPage Download | `SettingsPage.qml`, `DownloadEngine.cpp`, `UploadEngine.cpp` |
| 3.6 | Concurrent-session detection: server gửi error code → toast cảnh báo "đăng nhập từ thiết bị khác" + force logout sau 10s | `FshareApi.cpp`, `AuthService.cpp` |

### Đợt 4 — UX polish + refactor (P1/P2)

| # | Task | File / Module |
|---|---|---|
| 4.1 | Tách `FileManagerPage.qml` (2595 → ~600/file): `FmContextMenu.qml`, `FmRenameDialog.qml`, `FmShareDialog.qml`, `FmDetailPanel.qml`, `FmToolbar.qml` | `qml/Fshare/Pages/FileManager/` |
| 4.2 | Tương tự `FavoritesPage.qml` (1373) → `FavCollectionsRibbon.qml` + `FavGrid.qml` | `qml/Fshare/Pages/Favorites/` |
| 4.3 | Drag&drop overlay toàn cửa sổ với gradient soft + CTA "Thả vào Tải xuống / Tải lên" | `Main.qml` (DropArea wrapper Rectangle + Behavior on opacity) |
| 4.4 | Skeleton loader cho FileManager khi `isLoading || isSyncing` | `FileManagerPage.qml` + `FsSkeletonLoader.qml` |
| 4.5 | Command palette ⌘/Ctrl+K (handoff yêu cầu): danh sách action toàn app, fuzzy filter | `qml/Fshare/Components/FsCommandPalette.qml`, hook trong `Main.qml` |
| 4.6 | A11y audit: mỗi icon-only button + Aurora.FsIcon có `accessibleName`; toast `Accessible.role: Toast`; focus ring đúng spec | All components |
| 4.7 | Telemetry tự host (không Google Analytics): event log local + opt-in upload | `src/core/services/AnalyticsService.{h,cpp}` (mới) |
| 4.8 | Deploy script production: windeployqt + installer NSIS hoặc Inno Setup, code signing hook (skip nếu chưa có cert) | `scripts/deploy.bat`, `scripts/installer.iss` |

---

## 7. Bảng ưu tiên gộp

| Mức | Nhóm | Item |
|---|---|---|
| **P0 blocker** | Backend | Login HTTP 400 |
| **P0** | Architecture | Hoàn tất migration sang Aurora, xoá FshareTheme |
| **P0** | Platform | SystemTray thật + Chrome native messaging |
| **P0** | Quality | Bù test core (Auth, Transfer, BudgetManager, Engines) |
| **P0** | i18n | File `fshare_vi.ts` + en đầy đủ |
| **P1** | Transfer | Multi-segment download + chunked upload retry |
| **P1** | UX | Disk-full check, conflict dialog, drag overlay |
| **P1** | Network | Connection pooling, token refresh |
| **P2** | Refactor | Tách FileManagerPage / FavoritesPage |
| **P2** | UX | Command palette, skeleton, a11y audit |
| **P2** | Ops | Telemetry self-host, deploy installer |

---

## 8. Kết luận

Codebase ở mức **production-grade về kiến trúc** — DI sạch, layering chặt, transfer subsystem trưởng thành, có cơ chế resilience (crash handler, log rotation, secure store, single instance, file cache + FTS5, sync với watcher). Phần thiếu chủ yếu là **đóng cứng các quyết định đang treo**:

- Hai design system → một (Aurora).
- Phase 9 stub → tray + Chrome extension thật.
- Test coverage hẹp → mở rộng ra core service.
- BACKLOG về reliability (multi-segment, chunked, disk check, conflict, connection pool) → triển khai có thứ tự.

Triển khai theo 4 đợt ở trên, với Đợt 1+2 chiếm khoảng 4 tuần là đủ đưa app sang trạng thái RC; Đợt 3+4 đẩy sang RTM/GA. Backend login 400 là điểm chặn duy nhất nằm ngoài tay team frontend — cần kéo backend vào kế hoạch sớm.
