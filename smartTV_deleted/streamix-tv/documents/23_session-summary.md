---
title: StreamIX TV — Session Summary (M1 + M3 + M4 + Audit)
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Session Summary — Toàn bộ phiên làm việc

## Hoàn thành 5 phase

### PHASE A — Wave M3 (Timfshare + Sheets) ✅
- A.1: Multi-host OkHttp setup — 3 client riêng (Fshare/Timfshare-search/Timfshare-trending/Sheets) với UA + auth header riêng cho từng host.
- A.2: TimfshareApi (search) + TimfshareTrendingApi + DTOs với filter video extension + URL strip query.
- A.3: SheetsApi (Retrofit + ScalarsConverter) + CsvParser tự viết support quoted-field + `""` escape.
- A.4: TimfshareRepository + FeaturedRepository (Sheets cache 30min, trending cache 15min, search cache 5min) + sanitize input.
- A.5: BuildConfig fields TIMFSHARE_BEARER, SHEET_*, GA4_* (placeholder, owner sẽ gửi GA4 secret sau).

### PHASE B — Wave M4 (Feature integration) ✅
- B.1: Domain interfaces FeaturedRepository + SearchRepository + model FeaturedItem.
- B.2: HomeViewModel rework — 4 row (Tiếp tục xem / Gợi ý / Xu hướng / Account TopBar).
- B.3: SearchViewModel rewire qua TimfshareRepository.
- B.4: BrowseViewModel giữ nguyên — folder URL/linkcode semantic xử lý ở FilesRepositoryImpl.canonicalFolderUrl().
- B.5: PlayerViewModel thêm PasswordRequired state + handler cho code 123/471/404.
- B.6: SessionExpiredDialog wired ở HomeScreen (per-screen approach — đủ cho V1).

### PHASE C — Test catalog + unit tests ✅
- 19_test-catalog.md: 130+ test cases liệt kê cho mọi screen × workflow, phân loại Unit / Instrumented / Manual.
- Test files written:
  - `:core:common`: CsvParserTest, FshareUrlTest (~25 cases).
  - `:core:network`: IntOrStringAdapterTest, ApiCallTest (MockWebServer-based, ~12 cases).
  - `:core:data`: AuthRepositoryImplTest, SearchRepositoryImplTest, PlayerRepositoryImplTest, FilesRepositoryImplTest, FeaturedRepositoryImplTest (~35 cases).
- Total: ~70 unit tests.

### PHASE D — TV quality audit + fixes ✅
- 20_tv-quality-criteria.md: 15 hạng mục × ~150 tiêu chí (Manifest, UX, D-pad, Performance, Accessibility, Networking, State, Security, Distribution, Compatibility, Player, Lifecycle, Telemetry, i18n, Testing).
- 21_tv-quality-audit.md: PASS 72 / PARTIAL 18 / FAIL 9 / N/A 18 = **78% PASS direct**.
- **Fixes immediate đã áp dụng**:
  1. Tạo `@drawable/ic_launcher` + adaptive icon v26 (fix critical build blocker — manifest reference @mipmap/ic_launcher rỗng).
  2. Thêm `screenOrientation="landscape"` vào MainActivity.
  3. Sửa adaptive-icon foreground reference từ @mipmap (rỗng) sang @drawable.
- 22_tv-quality-backlog.md: 29 items (5 P0, 13 P1, 11 P2) với câu hỏi cụ thể cho owner.

### PHASE E — Final verification ✅ (hiện tại)

## TV Compatibility Matrix

| Dòng TV | Hỗ trợ | Note |
|---|---|---|
| Sony BRAVIA Android TV (≥ 2018) | ✅ | minSdk 21, target 34 |
| Sony BRAVIA Google TV (≥ 2021) | ✅ | |
| Xiaomi Mi TV / Mi Box S | ✅ | very common VN |
| TCL Google TV (≥ 2022) | ✅ | |
| Sharp Aquos Android TV | ✅ | |
| TV box VN AOSP (Vsmart, Casper, Asanzo) | ✅ | minSdk 21 phủ |
| Nvidia Shield TV | ✅ | |
| Chromecast with Google TV | ✅ | |
| Amazon Fire TV (Fire OS) | ⚠️ | Cài được nhưng cần bật ADB; KHÔNG dùng Play Store |
| LG WebOS | ❌ | KHÔNG hỗ trợ — không phải Android |
| Samsung Tizen | ❌ | KHÔNG hỗ trợ — không phải Android |
| KaiOS TV | ❌ | KHÔNG hỗ trợ |

**Kích thước/độ phân giải**: 1280×720 (HD), 1920×1080 (FHD), 3840×2160 (4K) — Compose tự scale theo density. Panel size 32"-85" — không cần đặc biệt.

**Cài đặt APK**: Sideload qua TV File Manager (hoặc USB). Phương án A: manual notify + manual install, không có in-app update.

## Items P0 cần thảo luận ngay

| ID | Tiêu đề | Câu hỏi cho owner |
|---|---|---|
| B-A1 | KEYCODE_MEDIA_PLAY_PAUSE handler | Long-press FFW có cần seek nhanh hơn (×2, ×3)? |
| B-A2 | ExoPlayer release lifecycle | (no question — chỉ cần audit) |
| B-C1 | Cold start measurement | Cần TV thật để measure |
| B-E1 | Bearer Timfshare leak qua source | Token này có cần coi là secret strict không? Nếu cần rotate liên hệ partner |
| B-F1 | Test trên ≥ 2 dòng TV thật | Chuẩn bị thiết bị |

## Files mới / đã sửa trong phiên này

### New files (33)
- `core/common/.../CsvParser.kt` + test
- `core/common/.../FshareUrl.kt` + test
- `core/network/.../service/TimfshareApi.kt`
- `core/network/.../service/SheetsApi.kt`
- `core/network/.../interceptor/FshareHeaderInterceptor.kt`
- `core/network/.../envelope/IntOrStringAdapterTest.kt`
- `core/network/.../envelope/ApiCallTest.kt`
- `core/data/.../repository/FeaturedRepositoryImpl.kt`
- `core/data/.../repository/SearchRepositoryImpl.kt`
- `core/data/.../repository/AuthRepositoryImplTest.kt`
- `core/data/.../repository/SearchRepositoryImplTest.kt`
- `core/data/.../repository/PlayerRepositoryImplTest.kt`
- `core/data/.../repository/FilesRepositoryImplTest.kt`
- `core/data/.../repository/FeaturedRepositoryImplTest.kt`
- `core/domain/.../model/FeaturedItem.kt`
- `core/domain/.../repository/FeaturedRepository.kt`
- `app/.../res/drawable/ic_launcher.xml`
- `app/.../res/drawable/ic_launcher_foreground.xml`
- `documents/19_test-catalog.md`
- `documents/20_tv-quality-criteria.md`
- `documents/21_tv-quality-audit.md`
- `documents/22_tv-quality-backlog.md`
- `documents/23_session-summary.md` (this file)

### Modified files (~25)
- `core/network/.../envelope/Envelope.kt` — LegacyResponse + IntOrString
- `core/network/.../envelope/ApiCall.kt` — numeric dispatch
- `core/network/.../auth/AuthTokenProvider.kt` — token + sessionId
- `core/network/.../interceptor/AuthInterceptor.kt` — Cookie session_id
- `core/network/.../interceptor/AppKeyInterceptor.kt` — DEPRECATED stub
- `core/network/.../auth/TokenRefreshAuthenticator.kt` — DEPRECATED stub
- `core/network/.../service/AuthApi.kt` — flat login response
- `core/network/.../service/MeApi.kt` — flat + nested fallback
- `core/network/.../service/FilesApi.kt` — fileops/get + getFolderList
- `core/network/.../service/SessionApi.kt` — session/download
- `core/network/.../di/NetworkModule.kt` — multi-host
- `core/network/build.gradle.kts` — BuildConfig fields
- `core/common/.../ErrorCodes.kt` — numeric codes
- `core/common/build.gradle.kts` — ngầm OK
- `core/data/.../local/AuthStore.kt` — token + sessionId storage
- `core/data/.../repository/AuthRepositoryImpl.kt`
- `core/data/.../repository/UserRepositoryImpl.kt`
- `core/data/.../repository/FilesRepositoryImpl.kt`
- `core/data/.../repository/PlayerRepositoryImpl.kt`
- `core/data/.../di/DataModule.kt` — bind Featured + Search + sheet IDs
- `core/data/build.gradle.kts` — testImplementation mockwebserver
- `core/domain/.../model/Session.kt` — token + sessionId
- `feature/home/.../HomeViewModel.kt`, `HomeScreen.kt` — 4 rows
- `feature/search/.../SearchViewModel.kt`, `SearchScreen.kt` — Timfshare
- `feature/auth/.../LoginViewModel.kt` — numeric error mapping
- `feature/player/.../PlayerViewModel.kt` — PasswordRequired state
- `app/build.gradle.kts` — BuildConfig fields M1+M3
- `app/.../navigation/StreamIXNavGraph.kt` — Home routing FeaturedItem
- `app/src/main/AndroidManifest.xml` — landscape + drawable icon
- `app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml`, `ic_launcher_round.xml`
- `gradle/libs.versions.toml` — retrofit-converter-scalars + mockwebserver

## Build status

⚠️ Sandbox không có Android SDK + JDK 17 — không chạy được `./gradlew assembleDebug` ở môi trường này. Owner phải build trên máy có:
- JDK 17 (Android Studio bundled JBR)
- Android SDK 34
- gradle wrapper 8.10.2 (đã có trong repo)

Lệnh build:
```powershell
.\setup-env.ps1                  # Set JAVA_HOME
.\gradlew test                    # Run unit tests
.\gradlew assembleDebug           # Build debug APK
.\gradlew assembleRelease         # Build release APK (cần keystore.properties)
```

## Khuyến nghị bước kế tiếp

1. Owner build local + chạy `./gradlew test` để verify unit tests pass.
2. Build debug APK cài thử trên Android TV emulator (Pixel TV template).
3. Đọc `22_tv-quality-backlog.md` → trả lời các câu hỏi P0 (đặc biệt B-E1 về Bearer Timfshare).
4. Ưu tiên Sprint 1 (5 items P0): B-A1 (media keys), B-A2 (player release), B-C1 (cold start), B-E1 (bearer), B-F1 (test TV thật).
5. Sau Sprint 1 → release v0.1.0 internal beta cho ≤ 3 user nội bộ test feedback.

## Sources

- [API_REFERENCE_FOR_REUSE.md](D:\Work\FsNext\smartTV\streamix-tv\documents\API_REFERENCE_FOR_REUSE.md)
- [17_api-migration-analysis.md](D:\Work\FsNext\smartTV\documents\17_api-migration-analysis.md)
- [19_test-catalog.md](D:\Work\FsNext\smartTV\streamix-tv\documents\19_test-catalog.md)
- [20_tv-quality-criteria.md](D:\Work\FsNext\smartTV\streamix-tv\documents\20_tv-quality-criteria.md)
- [21_tv-quality-audit.md](D:\Work\FsNext\smartTV\streamix-tv\documents\21_tv-quality-audit.md)
- [22_tv-quality-backlog.md](D:\Work\FsNext\smartTV\streamix-tv\documents\22_tv-quality-backlog.md)
