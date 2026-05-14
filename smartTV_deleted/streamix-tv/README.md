# StreamIX TV

Android TV client cho fshare.vn — Open API Gateway. Phân phối nội bộ qua APK.

> **Source of truth thiết kế / kế hoạch**: thư mục `D:\Work\FsNext\smartTV\documents\` — đặc biệt:
> - `13_implementation-spec-v1.md` — spec đầy đủ per screen + API
> - `14_screen-catalog.md` — catalog 80 frames
> - `15_readiness-checklist.md` — readiness + blockers

## Tương thích thiết bị

| Item | Giá trị |
|------|---------|
| `minSdk` | 21 (Android 5.0 — phủ TV box VN từ 2015) |
| `targetSdk` | 34 (Android 14) |
| Kích thước màn hình | 720p / 1080p / 4K (Compose-TV `dp`-based responsive) |
| TV brands hỗ trợ V1 | Sony Bravia, TCL, Xiaomi, Sharp, FPT Play Box, MyTV Box, Vinabox (Android-based) |
| Out of scope V1 | Samsung Tizen, LG WebOS, Hisense Vidaa |

## Build

```bash
# Hello APK debug
./gradlew assembleDebug

# Release universal APK (cần keystore.properties hoặc env vars)
export KEYSTORE_PATH=/path/to/release.jks
export KEYSTORE_PASSWORD=...
export KEY_ALIAS=streamix-tv
export KEY_PASSWORD=...
./gradlew assembleRelease

# Output: app/build/outputs/apk/release/app-universal-release.apk (~22 MB)
```

## Cài lên TV

```bash
# ADB
adb connect <ip-tv>:5555
adb install -r app-universal-release.apk

# Hoặc sideload qua portal nội bộ tv.streamix.vn (xem 04 rev3 §4.5)
```

## Cấu trúc module

```
streamix-tv/
├── app/                     # Application module — Manifest, MainActivity, Nav graph
├── build-logic/             # Gradle convention plugins (shared config)
├── core/
│   ├── common/              # Pure Kotlin: ApiResult, ErrorCodes, FileSize, TimeFormat
│   ├── domain/              # Models, Repository interfaces
│   ├── network/             # Retrofit + interceptors (X-App-Key, JWT) + APIs
│   ├── data/                # Room + DataStore + EncryptedSharedPreferences + Repository impls
│   └── ui/                  # Theme tokens, common Composables (FsButton/Card/TextField/...)
└── feature/
    ├── splash/              # S0
    ├── auth/                # S1 (email + password V1, không QR/OAuth)
    ├── home/                # S2 (3 row sau cuts)
    ├── browse/              # S3 + S4
    ├── player/              # S5 + Media3 ExoPlayer
    ├── search/              # S12
    ├── settings/            # S7 + S7a/b/d/f
    └── onboarding/          # S9 (2 step sau cuts)
```

## Trạng thái implement

| Module | Trạng thái |
|--------|-----------|
| Project skeleton + Gradle | ✅ Done |
| Core domain models | ✅ Done |
| Core network (Retrofit + interceptors + 4 API services) | ✅ Done |
| Core data (Room + DataStore + AuthStore + repos) | ✅ Done |
| Core UI (theme tokens + 5 components) | ✅ Done |
| Splash screen | ✅ Done |
| Login screen (email + password) | ✅ Done |
| Home screen (3 row) | ✅ Done |
| Browse + File Detail | ✅ Done |
| Search | ✅ Done |
| Player + ExoPlayer integration | ✅ Done (cần real Stream URL endpoint) |
| Settings (Hub + Account + Playback + Network + About) | ✅ Done |
| Onboarding (2 step) | ✅ Done |
| Resources (strings vi/en, drawables, themes) | ✅ Done |
| **Tests** | ❌ TODO Phase 1 |
| **Player Overlay custom (S5a)** | ❌ TODO V1.1 (V1 dùng PlayerView default controller) |
| **Track Selection sheet (S5b)** | ❌ TODO V1.1 |
| **Resume Prompt dialog (S5d)** | ❌ TODO V1.1 |
| **Confirm dialogs D1-D7** | ❌ TODO V1.1 |
| **TopBar component (C1)** | ❌ TODO chờ Designer v1.1 |
| **On-screen keyboard custom (C21)** | ❌ TODO chờ Designer v1.1 (V1 dùng IME default) |
| **Brand assets (logo + splash + banner)** | ❌ Chờ Designer v1.1 (placeholder) |

## Blockers (P0 từ `15_readiness-checklist.md`)

- **B1** — Stream URL endpoint chưa có trong API gateway → Player chỉ test được khi backend ship `v1/session.md`
- **B2** — Designer v1.1 (typography bump, tokens 3 lớp, brand assets, 80 frames đầy đủ)
- **B3** — Server `tv.streamix.vn` + nginx + SSL → cần DevOps Phase 4
- **B4** — Signing keystore → Tech Lead tạo trong Tuần 1

## Quick start dev

```bash
# 1. Clone repo
git clone <repo-url> streamix-tv && cd streamix-tv

# 2. Tạo local.properties
echo "sdk.dir=/path/to/Android/Sdk" > local.properties

# 3. Tạo keystore.properties (cho release build)
cat > keystore.properties << EOF
storeFile=../signing/release.jks
storePassword=<masked>
keyAlias=streamix-tv
keyPassword=<masked>
STREAMIX_APP_KEY=dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt
EOF

# 4. Build debug
./gradlew :app:assembleDebug

# 5. Cài lên Mi Box S
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

— Generated 2026-05-05 by automated implementation —
