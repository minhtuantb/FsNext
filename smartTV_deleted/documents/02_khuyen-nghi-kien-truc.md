---
title: 02 — Kiến trúc target & layer mapping
date: 2026-05-04
---

# 02. Kiến trúc target cho FsNext TV

## 2.1 Nguyên tắc thiết kế

1. **Giữ nguyên tinh thần Clean Architecture** của bản desktop. Đổi UI và một phần data layer; layer nghiệp vụ (Service, Repository, Domain Model) phải gần như nguyên xi để dễ đối chiếu, dễ port lại fix bug.
2. **Kotlin-first**, không Java. Coroutines + Flow thay cho QThread + signals. ViewModel của Android Architecture Components thay cho ViewModel C++.
3. **Single-Activity**, navigate bằng Jetpack Navigation Compose. Không Fragment cũ, không Activity-per-screen — không khớp với D-pad focus model.
4. **Offline-first cho metadata** (file list, history, settings) bằng Room DB. Network là edge, không phải core.
5. **Player tách module riêng** — `:player` Gradle module, có thể test độc lập, dễ thay engine (ExoPlayer ↔ libVLC).
6. **Update là first-class citizen**, không phải afterthought — module `:update` riêng, có thể bật/tắt qua remote config.

## 2.2 Sơ đồ layer

```
┌─────────────────────────────────────────────────────────────┐
│            UI Layer — Jetpack Compose for TV                 │
│  Screens (Login, Home, Browse, Detail, Player, Settings)     │
│  TV-specific: TvLazyRow, ImmersiveList, Carousel, Card       │
│  Focus engine: tự lo bằng Compose-TV; không cần custom       │
├─────────────────────────────────────────────────────────────┤
│            ViewModel Layer (AAC ViewModel)                    │
│  AuthViewModel, BrowseViewModel, PlayerViewModel,            │
│  DownloadViewModel, SettingsViewModel, UpdateViewModel        │
│  StateFlow<UiState> — Compose collectAsStateWithLifecycle    │
├─────────────────────────────────────────────────────────────┤
│            Domain Layer (pure Kotlin, no Android)             │
│  UseCases: LoginUseCase, ListFilesUseCase, GetStreamUrl…      │
│  Models: User, FileItem, TransferTask, PlaybackPosition…      │
│  ↑ port 1-1 từ src/core/models của bản desktop                │
├─────────────────────────────────────────────────────────────┤
│            Data Layer                                         │
│  ApiRepository (Retrofit/OkHttp) ── FshareApiService          │
│  HistoryRepository (Room DB)                                  │
│  SettingsRepository (DataStore)                               │
│  TransferRepository (multi-segment download manager)          │
│  PlaybackPositionRepository (Room: position by fileCode)      │
├─────────────────────────────────────────────────────────────┤
│            Platform / Native                                  │
│  ExoPlayer (Media3) ── primary player                         │
│  libVLC-Android ── fallback player                            │
│  PackageInstaller ── self-update                              │
│  WorkManager ── background tasks (download tiếp khi tắt UI)   │
└─────────────────────────────────────────────────────────────┘
```

## 2.3 Mapping desktop ↔ TV

| Desktop concept | TV concept | Ghi chú |
|-----------------|-----------|---------|
| `AppContext` (DI container thủ công) | **Hilt** (Dagger) | Giảm boilerplate, chuẩn Android |
| `Application::init` (SSL, CURL, proxy) | `Application.onCreate` + Hilt modules | OkHttp tự lo SSL; proxy do Android system |
| `QThread` workers | Kotlin **coroutines** + `Dispatchers.IO` | An toàn hơn, ít memory hơn |
| `Q_PROPERTY` / `Q_INVOKABLE` | `StateFlow` / `suspend fun` | Compose collect tự rebuild UI |
| `QSettings` | **DataStore** (Preferences) | Async, type-safe |
| XML history file (`downloadhistory_{uid}.xml`) | **Room** entity `download_history` | Truy vấn SQL được; backup được |
| `QSystemTrayIcon` | **MediaSession** + Notification | Cho phép control player từ remote, từ launcher recents |
| `QtSingleApplication` | Android tự lo (singleTask launchMode) | Bỏ |
| `QWebEngineView` cho OAuth | **Custom Tabs** (cho phone) **/ device-flow** (cho TV) | Trên TV không có browser tử tế, dùng device-flow |
| `qsTr()` / .ts file | `strings.xml` + `values-en/` | Tiếng Việt là default, English là override |

## 2.4 Module Gradle đề xuất

```
fsnext-tv/
├── app/                       # Android Application module, manifest, MainActivity
├── feature/
│   ├── auth/                  # Login, OAuth, QR login
│   ├── browse/                # Folder tree, file list
│   ├── player/                # Embedded player UI + controls
│   ├── download/              # Tải về USB/internal
│   ├── settings/              # Setting screens
│   └── update/                # In-app update
├── core/
│   ├── domain/                # UseCases, Models — pure Kotlin/JVM
│   ├── data/                  # Repositories, Room, DataStore
│   ├── network/               # Retrofit, OkHttp, FshareApiService
│   ├── ui/                    # Theme, design system, common Composables
│   └── common/                # Utils: SpeedMeter, FileNameSanitizer, BudgetManager (port từ desktop)
├── player-engine/
│   ├── exoplayer-impl/        # ExoPlayer wrapper
│   └── vlc-impl/              # libVLC fallback wrapper
└── build-logic/               # Convention plugins cho Gradle
```

Lý do tách `core/network` và `core/data`: dễ swap implementation (mock cho test); dễ chia sẻ với companion app mobile sau này.

## 2.5 Lifecycle & threading model

Android TV kill app dễ hơn desktop. Phải thiết kế ngay từ đầu:

- **Player session**: chạy trong `MediaSessionService` (foreground service). Người dùng nhấn Home → app vẫn phát nhạc nền nếu cần.
- **Download trong background**: dùng `WorkManager` + `Foreground Service` khi đang chạy. Lý do: TV idle có thể vào doze; foreground service đảm bảo network không bị cắt.
- **Auth session**: lưu refresh token trong `EncryptedSharedPreferences`. Refresh tự động qua Authenticator interceptor của OkHttp.
- **Process death recovery**: tất cả ViewModel state survive được nhờ `SavedStateHandle` cho key state; còn lại reload từ Repository (Room cache + network).

## 2.6 Quyết định "C++ NDK hay viết lại Kotlin?"

Câu hỏi quan trọng: tái sử dụng `DownloadEngine.cpp` (multi-segment + libcurl) qua NDK, hay viết lại bằng OkHttp + coroutines?

| Tiêu chí | Reuse C++ NDK | Viết lại Kotlin |
|----------|---------------|----------------|
| Effort khởi tạo | Cao (build libcurl-android, JNI bridge, CMake cho từng ABI) | Trung bình |
| Maintenance hai nền tảng | Một code base, cùng fix | Hai code base, hai logic |
| Hiệu năng | Tương đương (cả hai dùng native socket) | Tương đương |
| Crash debug | Khó (NDK stack trace) | Dễ (Kotlin stacktrace) |
| Build size APK | +2–3 MB libcurl × 4 ABI = +12 MB | Không thêm |
| Phụ thuộc | libcurl, openssl native | OkHttp đã có, dùng cho REST nên khỏi thêm |

**Khuyến nghị**: **Viết lại bằng Kotlin coroutines + OkHttp Range request**. Lý do:
- Logic multi-segment không quá phức tạp (~500 dòng C++ → ~300 dòng Kotlin).
- Tránh JNI overhead cho mỗi segment progress callback.
- APK gọn hơn, hỗ trợ 4 ABI miễn phí, không cần build native.
- Bug "the previous build attempt failed at link time because the binary was running" sẽ không tồn tại nữa.

Trừ khi: testing thực tế cho thấy multi-segment download trên TV chậm hơn desktop đáng kể (Wi-Fi yếu hơn nên có thể không phải vấn đề), khi đó có thể NDK-port sau.

## 2.7 Error handling & observability

Bản desktop có `AppError` typed error + `runtime.log` 160 KB. Bản TV cần:

- **Sealed `Result<T>`** kiểu `Success | NetworkError | AuthError | ApiError(code, msg)`. Mỗi UseCase trả Result.
- **Crashlytics** (Firebase) — bắt buộc cho TV vì user không gửi log thủ công như desktop.
- **In-app log viewer** cho support: Settings → Advanced → "Gửi nhật ký lỗi" → đính kèm 200 dòng cuối + thiết bị info.
- **Sentry** hoặc **Bugsnag** thay Crashlytics nếu muốn self-hosted.

## 2.8 Kết luận chương 2

Kiến trúc 5 layer của bản desktop port sang Android tự nhiên: View→Compose, ViewModel→AAC ViewModel, Service→UseCase, Repository giữ nguyên tên, Data đổi thư viện. Hai khuyến nghị có ảnh hưởng lớn nhất:

1. **Viết lại transfer engine bằng Kotlin** thay vì NDK — đơn giản hoá build, giảm APK size.
2. **Tách player thành module độc lập với 2 implementation** — phòng khi codec lạ, đổi engine không phải sửa toàn app.

Stack cụ thể (Compose-TV vs alternatives) ở [03](03_lua-chon-cong-nghe.md).
