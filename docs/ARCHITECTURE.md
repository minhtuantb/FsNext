# FsNext — Architecture Reference

> Nguồn sự thật phản ánh **code thực tế** (cập nhật 2026-05-29, v6.x). Mọi đường dẫn trong tài liệu này
> trỏ tới file đang tồn tại trong `src/`. Khi sửa kiến trúc, cập nhật file này — đừng để nó drift như
> các `docs/0x_*` cũ (đã xóa).

FsNext là Fshare desktop client (Windows-first), C++17 + Qt 6 QML, kiến trúc **MVVM + clean layering**,
dependency injection tập trung, **không có global state**.

---

## 1. Các tầng (layers)

```
┌──────────────────────────────────────────────────────────────┐
│ QML View            qml/Fshare/* (UI hiện hành) + qml/FsAurora/* (theme + 1 số trang/HUD) │
├──────────────────────────────────────────────────────────────┤
│ ViewModel (C++)     src/viewmodels/*  — Q_PROPERTY / Q_INVOKABLE, đăng ký làm context property │
├──────────────────────────────────────────────────────────────┤
│ Service (C++)       src/core/services/* — business logic, orchestration │
├──────────────────────────────────────────────────────────────┤
│ Repository (C++)    src/core/repositories/* + src/core/cache/* — data access (QSettings/SQLite/JSON) │
├──────────────────────────────────────────────────────────────┤
│ Engine / API / Net  src/core/transfer/*, src/core/api/*, src/core/net/* — libcurl, HTTP, OAuth loopback │
├──────────────────────────────────────────────────────────────┤
│ Platform / Util     src/platform/*, src/core/util/* — DPAPI, tray, taskbar, PKCE, sanitizer │
└──────────────────────────────────────────────────────────────┘
```

Quy tắc: View không chứa business logic; ViewModel chỉ gọi Service; Service không phụ thuộc QML; Engine/Net
là I/O thuần (libcurl) chạy ngoài main thread.

---

## 2. Bootstrap & Dependency Injection — `AppContext`

`src/main.cpp` tạo `QGuiApplication` + `QQmlApplicationEngine`, rồi `fsnext::AppContext::init()`
([src/app/AppContext.cpp](src/app/AppContext.cpp)) khởi tạo & wire **toàn bộ** đồ thị phụ thuộc theo thứ tự:

1. **Data**: `HttpClient`, `SettingsRepository`, `HistoryRepository`, `SyncRepository`.
2. **API**: `FshareApi(httpClient)`; `RefreshTokenCoordinator(httpClient, settingsRepo)` được tạo sớm (đọc
   `lastRefreshAt` cho cold-start 7-day check) rồi inject vào `FshareApi` qua `setRefreshCoordinator()`.
3. **Services**: `OAuthService` → `AuthService(api, settingsRepo, oauth)` (nhận coordinator trước khi autoLogin
   có thể chạy) → `TransferOrchestrator` → `TransferService(api, settings, orchestrator, history)` →
   `FileService` → `FileCacheService(api, fileService, orchestrator)` → `SettingsService` → `BatchFileResolver` →
   `SyncService(transferService, api, syncRepo)`.
4. **Signal wiring** (xem AppContext.cpp): login → load history + file cache tree + sync userId; settings đổi →
   đẩy lại `BudgetManager::Config` (download/upload/global slots) + proxy URL; transfer xong → record linkcode→local
   path + invalidate folder cache; HTTP 201 (session expired) → `AuthService::handleSessionExpired`.
5. **ViewModels** (19): tạo sau service tương ứng. `TransferHudViewModel` tạo **cuối** vì tham chiếu
   Upload/Download/Sync/Budget VM + TransferService.
6. `registerQml(engine)`: đăng ký mọi VM làm `contextProperty` (`authViewModel`, `downloadViewModel`, ...).

> Toàn bộ object sở hữu bằng `std::unique_ptr` trong AppContext; thứ tự khai báo = thứ tự hủy ngược.
> Các con trỏ non-owning (vd. `RefreshTokenCoordinator` được cả `FshareApi` và `AuthService` giữ) phải sống lâu hơn.

---

## 3. Transfer subsystem — `src/core/transfer/`, `src/core/services/TransferService.*`

```
TransferService / SyncService / FileCacheService  (producers)
        │ enqueue(id, class, priority)
        ▼
TransferOrchestrator  (own thread)  ── BudgetManager (slot accounting, non-QObject)
        │                            └── 3× PriorityScheduler: Download / Upload / Metadata
        │ dispatchReady(id, class, prio)   (Qt::QueuedConnection về producer thread)
        ▼
TransferService.dispatchDownload/Upload(id) → spawn DownloadEngine / UploadEngine (libcurl + QThread)
        │ progressChanged / completed / failed
        ▼
SpeedMeter (rolling-window rate + ETA)  →  TransferListModel (QML)  →  HistoryRepository / TransferHistoryDb
```

- **BudgetManager** ([BudgetManager.h](src/core/transfer/BudgetManager.h)): cap theo lớp (DL 8 / UL 4 / Metadata 2)
  + global cap (mặc định 10, 0 = tắt) + background-floor chống starvation. Là **non-QObject** (không event loop) để
  đọc trạng thái lock-free. Slot DL/UL chỉnh live từ Settings (qua `AppContext`).
- **DownloadEngine**: single + multi-segment HTTP Range (IDM-style), resume qua sidecar `.fsdownload`.
- **UploadEngine**: chunked, resumable (`queryResumeOffset`); sync upload throttle 5 MB/s.
- **FolderExpander**: crawl cây thư mục để folder-download enqueue từng file.
- Metadata prefetch (FileSyncWorker) **dùng chung** budget qua pool Metadata → tự throttle khi DL/UL bão hòa.

---

## 4. Auth & OAuth — `src/core/services/`, `src/core/net/`, `src/core/util/`

- **OAuth2 loopback (RFC 8252)**: `OAuthService` mở browser tới provider với `redirect_uri=http://127.0.0.1:<port>`;
  `LoopbackServer` ([net/LoopbackServer.cpp](src/core/net/LoopbackServer.cpp)) bắt 1 redirect rồi tự đóng;
  `Pkce` ([util/Pkce.cpp](src/core/util/Pkce.cpp)) sinh code_verifier + S256 challenge; `state` chống CSRF.
- **Silent refresh**: `RefreshTokenCoordinator` giữ canonical (token, sessionId, cookie). Single-flight: nhiều
  request gặp token hết hạn → 1 caller refresh, các caller khác chờ rồi replay. Outcome: Success / HardFail (re-login,
  refresh token > 7 ngày) / SoftFail (lỗi tạm, giữ token cũ). `FshareApi::executeAuthed()` bọc mọi call:
  `ensureFresh()` → gọi → nếu lỗi Auth thì refresh + replay 1 lần.
- **AuthService**: login email/password + `loginWithProvider`, `autoLogin` (refresh_token lúc khởi động),
  `handleSessionExpired`. Credentials lưu mã hóa bằng `SecureStore` (Windows DPAPI; non-Windows trả plaintext —
  placeholder Keychain/libsecret).

---

## 5. File manager cache-first — `src/core/cache/`, `src/core/services/FileCacheService.*`

- **FileCacheDB** ([cache/FileCacheDB.cpp](src/core/cache/FileCacheDB.cpp)): SQLite (`%APPDATA%/.../cache.db`),
  bảng `files` / `folder_sync` / `local_files`, FTS5 cho search local, tách theo user.
- **FileSyncWorker**: crawl từng page của folder từ API → ghi vào DB, gated qua pool Metadata của orchestrator.
- **FileCacheService**: read-path serve ngay từ DB rồi enqueue background refresh; write-path (rename/delete/move)
  optimistic update → gọi API → sửa lại cache nếu fail. `recordLocalFile` map linkcode→local path; `refreshUploadFolder`
  invalidate folder đích sau upload để file mới hiện ngay.

---

## 6. Sync / auto-upload — `src/core/services/SyncService.*`, `src/core/repositories/SyncRepository.*`

- Tối đa **5 folder**, walk đệ quy; skip-list cứng (`.tmp`, `Thumbs.db`, `.DS_Store`, `~$*`, `.git`, `node_modules`,
  `__pycache__`, `.venv`); bỏ file > 1 GB. Upload private (secured), throttle 5 MB/s, priority `Background`.
- `QFileSystemWatcher` + rescan định kỳ 5 phút. Cây local → mirror sang Fshare (tạo subfolder lazy). Tùy chọn xóa
  local sau khi upload thành công. State + activity log lưu trong `SyncRepository` (SQLite), bền qua restart.

---

## 7. ViewModels (`src/viewmodels/`)

Auth/Core: `AuthViewModel`, `LanguageViewModel`, `SettingsViewModel`, `UserInfoViewModel`.
Transfer: `DownloadViewModel`, `UploadViewModel`, `UploadStagingViewModel` (sống sót qua page-Loader teardown),
`TransferListModel` (QAbstractListModel), `TransferBudgetViewModel` (telemetry "DL 2/8 · UL 1/4"),
`TransferHudViewModel` (gộp cho tray/mini-HUD).
File: `FileManagerViewModel`, `FileListModel`, `FolderTreeModel`, `FolderPickerModel`, `FavoritesViewModel`.
Khác: `SyncViewModel`, `HomeSearchViewModel` (phân loại URL/keyword + bad-word filter), `RemoteShareViewModel`
(duyệt share URL dán từ homepage).

---

## 8. Platform & Util

- `src/platform/`: `SystemTray` (Qt Widgets — cách duy nhất Qt hỗ trợ tray), `TaskbarProgress` (Windows),
  `SingleInstance`, `PlatformUtils` (proxy URL resolve, folder dialog), `nativehost/` (Chrome extension IPC, build
  thành executable `fsharenativeapp` riêng).
- `src/core/util/`: `SecureStore` (DPAPI), `Pkce`, `FileNameSanitizer`, `BadWordFilter` (dictionary đóng gói +
  override `%APPDATA%/.../badwords-overrides.json`, fail-open), `FormatUtil`, `FshareUrl`, `FileTypeHelper`.

---

## 9. Threading model

- **Main thread**: QML engine, render, ViewModel property, signal/slot dispatch.
- **TransferOrchestrator**: own thread; emit `dispatchReady` qua `Qt::QueuedConnection`.
- **Transfer engines**: mỗi task spawn QThread riêng cho libcurl (blocking I/O cô lập).
- **FileSyncWorker**: background metadata crawl.
- Cross-thread đi qua signal/slot queued. `BudgetManager` lock-free đọc; `HttpClient` cookie/session bảo vệ bằng
  mutex (vá tạm — xem `docs/ASSESSMENT.md`).

---

## 10. QML — FsAurora vs Fshare (cùng tồn tại)

- **Theme đang chạy = `FsAurora.Theme`** (`AuroraTheme`, `AuroraColors`). `qml/Fshare/Theme/*` đã bị xóa.
- `Main.qml` import: `Fshare.Components/Dialogs/Pages/Utils` (UI chính) + `FsAurora.Theme` (theme) +
  `FsAurora.Components/Pages/Windows` (alias `Aurora*` — HomePage, LoginView, MiniHudWindow).
- `qml/Fshare/Pages/`: Download, Upload, FileManager (+ thư mục `FileManager/`), Settings, Sync, Favorites,
  UserInfo, Showcase. `qml/FsAurora/Pages/`: HomePage, LoginView, ShowcasePage. `qml/FsAurora/Windows/MiniHudWindow.qml`.
- Hệ quả: **2 design-system song song** là nợ kỹ thuật cần hợp nhất (xem ASSESSMENT). `qml/FsAurora/` còn chứa
  artifact thiết kế (`*.html`, `design-canvas.jsx`, `handoff/`) — không tham chiếu lúc runtime.

---

## 11. Build & test

- Build: `cmake --preset msvc2022` → `cmake --build --preset release`; output `output/FsNext.exe`. Chi tiết: `CLAUDE.md`.
- Tests (`tests/`, CTest): `test_file_cache_db`, `test_file_cache_service`, `test_budget_manager`, `test_speed_meter`,
  `test_fshare_url`, `test_filename_sanitizer`, `test_filename_resolver`, `test_resolve_proxy_url`.
