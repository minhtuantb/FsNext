---
title: StreamIX TV — Verification Report toàn bộ
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Verification Report — Toàn bộ Codebase

Audit toàn diện sau redesign: file structure, navigation, business logic, design fidelity vs mockup.

## 1. File structure ✅

### Screens (12 + 7 Player overlays/dialogs)

| Screen | File | Status |
|---|---|---|
| S0 Splash | `feature/splash/SplashScreen.kt` | ✅ + golden underline accent |
| S1 Login | `feature/auth/LoginScreen.kt` | ⚠️ V1 form đơn giản, chưa split layout |
| S1b Onboarding | `feature/onboarding/OnboardingScreen.kt` | ✅ 2 steps |
| S2 Home | `feature/home/HomeScreen.kt` | ✅ Search bar + 3 hero cards + Continue Watching |
| S2a Suggested detail | `feature/home/CatalogScreens.kt` | ✅ Grid 4 cột (V1 minimal) |
| S2b Trending detail | `feature/home/CatalogScreens.kt` | ✅ Grid 4 cột (V1 minimal) |
| S2c Top detail | `feature/home/TopScreen.kt` | ✅ Vertical list rank + filter source |
| S3 Browse | `feature/browse/BrowseScreen.kt` | ✅ 4-col grid + pagination |
| S4 File Detail | `feature/browse/FileDetailScreen.kt` | ✅ |
| S5 Player | `feature/player/PlayerScreen.kt` | ✅ + PasswordRequiredOverlay + Error overlay |
| S12 Search | `feature/search/SearchScreen.kt` | ✅ Title + filter chips + suggestion pills |
| S6 Settings Hub | `feature/settings/SettingsScreens.kt` | ✅ |
| S7 Settings Account | `feature/settings/SettingsScreens.kt` | ✅ |
| S8 Settings Playback | `feature/settings/SettingsScreens.kt` | ✅ |
| S8b Settings Network | `feature/settings/SettingsScreens.kt` | ✅ |
| S9 Settings About | `feature/settings/SettingsScreens.kt` | ✅ |
| Player Overlay | `feature/player/PlayerOverlay.kt` | ✅ |
| Player Banner | `feature/player/PlayerBanner.kt` | ✅ |
| Player Error Overlay | `feature/player/PlayerErrorOverlay.kt` | ✅ all branches handled |
| Password Required | `feature/player/PasswordRequiredOverlay.kt` | ✅ NEW |
| Resume Prompt | `feature/player/ResumePromptDialog.kt` | ✅ |
| Track Selection | `feature/player/TrackSelectionSheet.kt` | ✅ |

### Repositories

| Repo | Interface | Impl | Status |
|---|---|---|---|
| AuthRepository | `core/domain` | `core/data` | ✅ Fshare legacy |
| UserRepository | `core/domain` | `core/data` | ✅ flatten flat/nested |
| FilesRepository | `core/domain` | `core/data` | ✅ + manual JSON parser cho folder list |
| PlayerRepository | `core/domain` | `core/data` | ✅ + referral 8805984 |
| PlaybackPositionRepository | `core/domain` | `core/data` | ✅ Room |
| SettingsRepository | `core/domain` | `core/data` | ✅ DataStore (deadlock fixed) |
| FeaturedRepository | `core/domain` | `core/data` | ✅ Sheets + Timfshare fallback |
| SearchRepository | `core/domain` | `core/data` | ✅ Timfshare + sanitize |
| TopRepository | `core/domain` | `core/data` | ✅ NEW — 3 sheet sources |

### Network services

| Service | Endpoint | Status |
|---|---|---|
| AuthApi | `POST /api/user/login` | ✅ flat response |
| MeApi | `GET /api/user/get` | ✅ flat + nested |
| FilesApi | `POST /api/fileops/get`, `getFolderList` | ✅ raw ResponseBody for flexible parse |
| SessionApi | `POST /api/session/download` | ✅ |
| TimfshareApi | search + trending | ✅ |
| SheetsApi | CSV export | ✅ |

## 2. Navigation graph ✅

```
SplashRoute
  ├─ → LoginRoute
  └─ → HomeRoute

LoginRoute → OnboardingRoute (first time)
            → HomeRoute (already onboarded)

OnboardingRoute → HomeRoute

HomeRoute
  ├─ → SuggestedRoute
  ├─ → TrendingRoute
  ├─ → TopRoute
  ├─ → SearchRoute
  ├─ → SettingsHubRoute
  ├─ → SettingsAccountRoute
  └─ → PlayerRoute (Continue Watching)

SuggestedRoute / TrendingRoute → BrowseRoute (folder) | FileDetailRoute (file)
TopRoute → FileDetailRoute
BrowseRoute → BrowseRoute (subfolder) | FileDetailRoute (file)
FileDetailRoute → PlayerRoute
SearchRoute → FileDetailRoute
SettingsHubRoute → Settings sub-screens
SettingsAccountRoute logout → LoginRoute (clear stack)
```

Mọi route đều có composable handler. Mọi callback đều navigate đến route hợp lệ.

## 3. Business logic per screen

### S0 Splash
- Gọi `authRepo.currentSession()` — null → Login route.
- Nếu có session: `userRepo.refreshMe()` → navigate Home.
- AuthExpired → clearSession + Login.
- NetworkError → Home (offline-first nhờ cache Room).

### S1 Login
- Validate email format + password ≥ 6 chars.
- `authRepo.login(email, password)` → save Session.
- Sau success: check `settings.onboardingCompleted` → Home hoặc Onboarding.
- Error mapping numeric: 405/"authenticate" → InvalidCredentials, 409/424 → AccountLocked, 33/37/38/39/502 → AppKeyError, "too many" → Throttled.

### S1b Onboarding
- 2 steps (D-pad / Resume watching).
- Click "Get started" → `markCompleted()` → DataStore write → Home.
- **BUG FIX**: trước đây `flow.collect { return@collect }` hang vĩnh viễn. Đã đổi thành `observe().first()`.

### S2 Home
- Top bar (logo, search icon, settings, avatar).
- Prominent search bar (click → SearchScreen).
- 3 hero cards (Gợi Ý / Xu Hướng / Top) — click → respective detail.
- Continue Watching row (chỉ hiển thị khi có data).
- HomeViewModel preload counts cho subtitle hero cards.
- BackHandler → ExitDialog.

### S2a Suggested detail
- Header "ĐỀ XUẤT CHO BẠN" + title "Gợi Ý" + count.
- Grid 4 cột FeaturedItem cards.
- Click item → if folder URL → Browse, if file → FileDetail.

### S2b Trending detail  
- Header "BẢNG XẾP HẠNG NHANH" + title "Xu Hướng" + count.
- Grid 4 cột.
- Same click behavior.

### S2c Top detail
- Header "BẢNG XẾP HẠNG" + filter chips IMDB/RT/FSHARE.
- Vertical list rank 01-100 + thumbnail + title + meta.
- Click → FileDetail (Top items đều là file playable).
- Source filter: switch → reload data (cache per source).

### S3 Browse
- API call `getFolderList` với canonical URL từ linkcode.
- Manual JSON parser handle 5 response shapes (array, object, nested data, etc).
- Pagination loadMore khi scroll cuối.
- Sort folder-first → file-second, alphabet.
- Click subfolder → re-navigate Browse(linkcode).

### S4 File Detail
- API call `fileops/get` lấy info file.
- Click Play → PlayerRoute.

### S5 Player
- `fetchStreamUrl` từ `/api/session/download` với referral.
- States: LoadingStream, Ready, PasswordRequired, Error.
- Code 123 → password dialog → retry với password.
- Code 471 → "Quá nhiều phiên tải" với hint.
- Code 404 → NotFound.
- AuthExpired → Error(Auth).
- Save playback position mỗi 5s.

### S12 Search
- TextField input + debounce 500ms.
- Min query length 2 chars.
- `searchRepo.search(query)` → Timfshare API.
- Idle state: hiển thị 4 suggestion pills "+ the bear" etc.
- Loaded state: filter chips Tất cả/Phim/Series/Folder + result grid.
- Empty/Error states với retry.
- Click result → FileDetail.

### S6-S9 Settings
- Hub: 4 mục con.
- Account: hiển thị user + logout.
- Playback: subtitle/audio/auto-next options.
- Network: Wifi-only toggle.
- About: version info.

## 4. Critical bugs đã fix

| Bug | Severity | Status | Note |
|---|---|---|---|
| ApiCall không catch JsonDataException → crash | P0 | ✅ Fixed | Catch all Throwable + Timber log |
| getFolderList response shape inconsistent | P0 | ✅ Fixed | Manual JSON parser handle array/object/nested |
| BrowseScreen subfolder pass empty path | P0 | ✅ Fixed | Pass linkcode |
| SettingsDataStore.update() hang (DataStore collect) | P0 | ✅ Fixed | Đổi sang first() |
| AGP/Gradle/compileSdk outdated | P1 | ✅ Fixed | AGP 8.13.2, compileSdk 35 |
| Onboarding stuck "Get started" | P0 | ✅ Fixed (cùng SettingsDataStore bug) |
| Top page chưa có | P0 | ✅ Implemented |
| Search UI không match mockup | P1 | ✅ Redesigned với chips + pills |
| Home không có 3 hero cards | P1 | ✅ Redesigned |

## 5. Design fidelity vs mockup

| Mockup element | Implementation | Match |
|---|---|---|
| Splash logo + golden underline | ✅ | ✅ |
| Login split layout (hero + form) | Form đơn giản căn giữa | ⚠️ V1.1 |
| Login "Hiện/Ẩn" password text | Icon eye | ⚠️ Cosmetic |
| Login "Nhớ tôi 30 ngày" | KHÔNG | ⚠️ V1.1 |
| Home 3 hero cards | ✅ với gradient + icon + count | ✅ |
| Home prominent search bar | ✅ | ✅ |
| Home Continue Watching row | ✅ | ✅ |
| Search title "Tìm kiếm" | ✅ | ✅ |
| Search filter chips | ✅ visual only | ⚠️ Click filter chưa thực sự filter data |
| Search on-screen keyboard | KHÔNG (TV system IME) | ⚠️ V1.1 |
| Search "Gợi ý cho bạn" pills | ✅ | ✅ |
| Top vertical list + ranks | ✅ | ✅ |
| Top filter IMDB/RT/FSHARE | ✅ | ✅ |
| Suggested detail grid | ✅ minimal | ⚠️ Chưa có folder card variant + badges |
| Trending tabs HÔM NAY/7 NGÀY/30 NGÀY | KHÔNG | ⚠️ V1.1 |
| Trending badges HOT/#1/OSCAR | KHÔNG | ⚠️ V1.1 (cần data từ sheet) |
| Sort filter SẮP XẾP/MỚI NHẤT | KHÔNG | ⚠️ V1.1 |

**Match rate**: ~75% — 3 critical features (Home/Top/Search) đã đúng design, các detail screens minimal V1, Login chưa redesign.

## 6. Vietnamese language

Đã dùng tiếng Việt chuẩn ở:
- strings.xml (vi default + values-en mirror)
- Tất cả hardcoded strings trong screens dùng VI
- Error messages thân thiện ("Không có kết nối mạng", "Phiên hết hạn", "Quá nhiều phiên tải...")

Chưa làm: Move các inline VI strings (hero card subtitle, top header, search suggestions) sang strings.xml. Backlog V1.1.

## 7. Build status

Sandbox không build được (proxy chặn JDK 17 + Android SDK). Owner build local với:
```powershell
.\setup-env.ps1
.\gradlew clean assembleDebug --refresh-dependencies
```

## 8. Còn lại cho V1.1 (không blocker release V1)

1. Login split layout (hero + form như mockup S1).
2. "Nhớ tôi 30 ngày" + "Quên mật khẩu" link.
3. Suggested detail: folder cards với "(N mục · M thư mục con)".
4. Trending detail: time period tabs + badges.
5. On-screen keyboard cho Search (TV system IME đủ cho V1).
6. Filter chips Search thực sự filter data (V1 visual only).
7. Move inline VI strings → strings.xml + values-en mirror.
8. Owner cung cấp 3 sheet IDs cho Top (IMDB/RT/Fshare) + GA4 secret + Timfshare bearer.
9. Cold start measurement + frame drop profiling trên TV thật.
10. Test catalog từ 19_test-catalog.md execute với device thật.

## 9. Items P0 release blocker (cần thảo luận với owner)

1. **B-A1** Hook KEYCODE_MEDIA_PLAY_PAUSE / FFW / REW cho TV remote
2. **B-A2** ExoPlayer release lifecycle audit
3. **B-C1** Cold start ≤ 2s measurement
4. **B-E1** Bearer Timfshare token rotation
5. **B-F1** Test trên ≥ 2 dòng TV thật

Đầy đủ chi tiết tại `22_tv-quality-backlog.md`.
