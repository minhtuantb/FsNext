# CLAUDE.md — FsNext

Hướng dẫn cho Claude Code khi làm việc trong repo này. Ngắn gọn, vận hành. Kiến trúc chi tiết:
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). Đánh giá & khuyến nghị: [docs/ASSESSMENT.md](docs/ASSESSMENT.md).

> Ngôn ngữ trao đổi mặc định: **tiếng Việt**.

## App là gì

FsNext = Fshare desktop client (Windows-first), C++17 + Qt 6 QML, MVVM + clean layering, DI tập trung qua
`AppContext`, **không global state**. Tính năng: download (multi-segment, resume), upload (chunked, resume),
file manager cache-first, auto-sync folder, OAuth loopback + silent refresh, transfer HUD/tray, Chrome extension.

## Cây thư mục (thực tế)

```
src/
  app/          AppContext (DI container), Application, main.cpp
  core/
    api/        FshareApi, HttpClient (libcurl), OAuthProvider, OAuthSecrets(.h, gitignored)
    services/   AuthService, OAuthService, RefreshTokenCoordinator, TransferService,
                FileService, FileCacheService, SettingsService, SyncService, FolderExpander, BatchFileResolver
    transfer/   TransferOrchestrator, BudgetManager, PriorityScheduler, DownloadEngine,
                UploadEngine, TransferWorker, TransferQueue, SpeedMeter
    cache/      FileCacheDB (SQLite+FTS5), FileSyncWorker
    repositories/ Settings/History/Sync/TransferHistory repos
    net/        LoopbackServer (OAuth redirect)
    models/     FileItem, TransferTask/State, User, SyncFolder, AppSettings, ApiResponse, AppError
    util/       SecureStore (DPAPI), Pkce, FileNameSanitizer, BadWordFilter, FormatUtil, FshareUrl, FileTypeHelper
  viewmodels/   19 VM (Auth/Download/Upload/FileManager/Sync/... + List/Tree models + HUD/Budget)
  platform/     SystemTray, TaskbarProgress, SingleInstance, PlatformUtils, nativehost/ (fsharenativeapp)
qml/
  Fshare/       UI hiện hành: Components, Dialogs, Pages (+Pages/FileManager), Utils
  FsAurora/     Theme đang chạy (AuroraTheme/AuroraColors) + Components/Pages/Windows (HomePage, LoginView, MiniHud)
tests/          8 unit test (CTest)
scripts/        build.bat (entry chuẩn), clear_cache.bat, register_native_host.bat, ...
docs/           tài liệu (xem docs/README.md)
```

## Build & Run

**Yêu cầu**: Qt **6.8.3 msvc2022_64**, VS 2022 (BuildTools/Community...), CMake ≥ 3.24 + Ninja, vcpkg
(`VCPKG_ROOT`), OpenSSL/libcurl qua vcpkg.

**Cách nhanh nhất (Windows)** — `scripts/build.bat` lo hết env (Qt + vcvars + configure + build Release):
```
cmd.exe /c "scripts\build.bat"      # → output/FsNext.exe
```

**Thủ công** (khi env MSVC đã load):
```
cmake --preset msvc2022             # configure → build/  (binaryDir)
cmake --build --preset release      # hoặc: cmake --build build --config Release -j 8
```
Preset khác: `msvc2022-debug` (build-debug/), `msvc2022-production` (build-production/), `mingw`.

**Run / smoke test**:
```
taskkill /F /IM FsNext.exe 2>NUL    # kill instance cũ (single-instance)
output\FsNext.exe
```
Có project skill `.claude/skills/fsnext-run` (build + launch + screenshot verify + qmllint).

**Lint QML** (qmllint từ Qt mingw, cần `-I qml`):
```
qmllint.exe -I qml qml/Fshare/Pages/<Page>.qml
```

**Tests**: `ctest --test-dir build` (sau khi build với `BUILD_TESTING`).

## Quy ước code

- Namespace `fsnext`. C++17. Header/impl cặp `.h/.cpp` trong cùng thư mục layer.
- **MVVM**: QML chỉ binding + display logic; business logic ở Service; ViewModel là cầu nối (Q_PROPERTY/Q_INVOKABLE).
- ViewModel mới phải được tạo + wire trong `AppContext::init()` và đăng ký trong `AppContext::registerQml()`.
- **Không hardcode màu/size trong QML** — dùng token `FsAurora.Theme` (`AuroraTheme`, `AuroraColors`).
- **Lambda async** (`QtConcurrent::run`, `SingleShotConnection`, `QTimer::singleShot`): capture `QPointer` cho
  QObject pointer + check `if (!guard) return;` đầu callback (bug-class số 1 theo crash-audit). Shutdown đã có
  `QThreadPool::globalInstance()->waitForDone(5000)` ở `main.cpp` để drain pool trước khi hủy service.
- Thêm file nguồn: cập nhật `CMakeLists.txt` (danh sách source) tương ứng. QML bundle qua GLOB (CONFIGURE_DEPENDS).
- Bí mật OAuth: `src/core/api/OAuthSecrets.h` **gitignored** — chỉ commit bản `.example` nếu có.

## Gotchas (đọc trước khi sửa)

- **Design-system QML**: atom tái sử dụng → `Fshare.Components` (chuẩn); `FsAurora.Theme` = token; `FsAurora` =
  shell/HUD/trang khung. Quy tắc import + bảng ánh xạ atom: [docs/design-system.md](docs/design-system.md). Đang
  hợp nhất dần (Stage 1 xong: FsIcon/FsTextField/FsCard; Stage 2: FsButton/FsBadge/FsSwitch/FsProgressBar). Trong
  file FsAurora cần atom Fshare → `import Fshare.Components 1.0 as Fsh` (tránh ambiguous type).
- **libcurl bỏ qua `QNetworkProxy`** → proxy phải set thủ công lên `HttpClient` và đọc lại trong engine
  (đã wire trong AppContext). Đừng kỳ vọng QNetworkProxy có tác dụng.
- **Silent refresh single-flight**: mọi API call đi qua `FshareApi::executeAuthed()`. Đừng tự gọi refresh song song —
  để `RefreshTokenCoordinator` serialize. HTTP 201 = session expired.
- **HttpClient cookie/session** đang bảo vệ bằng mutex (vá tạm). Nếu refactor session/cookie, thiết kế lại cho sạch.
- **SystemTray dùng Qt Widgets** (lý do: Qt chỉ hỗ trợ tray qua Widgets) → app link cả QtWidgets dù UI là QML.
- **SecureStore** chỉ mã hóa thật trên Windows (DPAPI); non-Windows trả plaintext (placeholder).
- Single-instance: instance thứ 2 chuyển lệnh cho instance đầu rồi thoát; nhớ `taskkill` trước khi smoke test.

## Tài liệu

Xem [docs/README.md](docs/README.md) để biết file nào dùng cho việc gì (kiến trúc / đánh giá / runbook / ADR /
test case / crash audit / spec).
