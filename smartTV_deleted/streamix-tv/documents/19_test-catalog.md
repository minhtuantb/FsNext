---
title: StreamIX TV — Test Catalog
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# StreamIX TV — Test Catalog

Liệt kê **tất cả** test case theo screen × workflow. Đánh dấu mức độ tự động hoá:
- 🟢 **Unit/JVM** — chạy bằng `./gradlew test` (no device).
- 🟡 **Instrumented** — yêu cầu Android emulator/device, defer nếu không có CI device.
- 🔴 **Manual** — kiểm tra tay trên TV thật khi QA.

## Cấu trúc

| Screen ID | Tên | ViewModel | Repository | UI test surface |
|---|---|---|---|---|
| S0 | Splash | SplashViewModel | AuthRepo, UserRepo | low |
| S1 | Login | LoginViewModel | AuthRepo | medium |
| S1b | Onboarding | OnboardingViewModel | SettingsRepo | low |
| S2 | Home | HomeViewModel | FeaturedRepo, UserRepo, PlaybackRepo | high |
| S3 | Browse | BrowseViewModel | FilesRepo | medium |
| S4 | File Detail | FileDetailViewModel | FilesRepo | low |
| S5 | Player | PlayerViewModel | PlayerRepo, PlaybackRepo | high |
| S12 | Search | SearchViewModel | SearchRepo | medium |
| S6-S9 | Settings (Hub/Account/Playback/Network/About) | SettingsViewModels | SettingsRepo | low |

---

## S0 — Splash

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S0-001 | Launch lần đầu, chưa có session | 🟢 | Routing → Login |
| S0-002 | Launch với session valid | 🟢 | refreshMe Success → Routing → Home |
| S0-003 | Launch với session — refreshMe trả AuthExpired | 🟢 | clearSession + Routing → Login |
| S0-004 | Launch với session — refreshMe NetworkError | 🟢 | Offline-first → Routing → Home (cache có user) |
| S0-005 | Launch trong khi Splash đang verify, app bị stop | 🟡 | Resume không crash |
| S0-006 | installSplashScreen() được gọi trước super.onCreate() | 🔴 | Splash hiển thị mượt trên Android 12+ và backport |

## S1 — Login

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S1-001 | Email rỗng → Submit | 🟢 | errorKey = InvalidEmail, không call API |
| S1-002 | Email sai format ("abc@") → Submit | 🟢 | errorKey = InvalidEmail |
| S1-003 | Password < 6 ký tự → Submit | 🟢 | errorKey = PasswordTooShort |
| S1-004 | Body code = 405 / "authenticate" | 🟢 | errorKey = InvalidCredentials |
| S1-005 | Body code = "too many" | 🟢 | errorKey = Throttled |
| S1-006 | Body code = 409 (account locked) | 🟢 | errorKey = AccountLocked |
| S1-007 | Body code = 424 (temp lock) | 🟢 | errorKey = AccountLocked |
| S1-008 | Body code = 33 / 37 / 38 / 39 / 502 (app key/device) | 🟢 | errorKey = AppKeyError |
| S1-009 | NetworkError | 🟢 | errorKey = Network |
| S1-010 | Body code = 200 + token + session_id | 🟢 | success = true, AuthStore lưu Session |
| S1-011 | Body code = 200 nhưng token = null | 🟢 | errorCode = PARSE_ERROR |
| S1-012 | Toggle password visibility | 🟢 | passwordVisible flip |
| S1-013 | TextField focus ring trên TV | 🔴 | Border + scale 1.05 |
| S1-014 | D-pad navigation Email → Password → Show/Hide → Submit | 🔴 | Focus order đúng |

## S1b — Onboarding

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S1b-001 | First time after login | 🟢 | Onboarding showing |
| S1b-002 | Skip → settingsRepo.markOnboarded(true) | 🟢 | onCompleted callback |
| S1b-003 | Already onboarded → skip route | 🟢 | Login navigates to Home |

## S2 — Home

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S2-001 | Initial load — suggested + trending success | 🟢 | 2 row Loaded |
| S2-002 | Suggested empty list (cache TTL hit, 0 items) | 🟢 | Row state Empty |
| S2-003 | Suggested NetworkError | 🟢 | Row state Error("Không có kết nối mạng") |
| S2-004 | Trending sheet trống → fallback Timfshare success | 🟢 | Trending Loaded từ Timfshare data |
| S2-005 | Trending sheet trống + Timfshare lỗi | 🟢 | Trending Empty (không Error) |
| S2-006 | refreshMe() → AuthExpired | 🟢 | sessionExpired = true, dialog visible |
| S2-007 | Click Continue Watching | 🔴 | Navigate Player với resumeMs |
| S2-008 | Click Featured (file URL) | 🔴 | Navigate FileDetail(linkcode) |
| S2-009 | Click Featured (folder URL) | 🔴 | Navigate Browse(linkcode, title) |
| S2-010 | Back press | 🔴 | ExitAppDialog visible |
| S2-011 | Back press + Confirm exit | 🔴 | Activity finish |
| S2-012 | Top bar: search button | 🔴 | Navigate Search |
| S2-013 | Top bar: settings button | 🔴 | Navigate SettingsHub |
| S2-014 | Top bar: avatar button | 🔴 | Navigate SettingsAccount |
| S2-015 | Cache hit suggested (gọi 2 lần trong TTL) | 🟢 | Lần 2 không call network |
| S2-016 | Cache invalidate sau TTL 30min | 🟢 | Call network mới |

## S3 — Browse

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S3-001 | Initial load — folder URL valid | 🟢 | items Loaded, sorted folder-first |
| S3-002 | folder URL không trích được linkcode | 🟢 | error = "Invalid folder path/URL" |
| S3-003 | listFiles → AuthExpired | 🟢 | error message ("Phiên hết hạn"), KHÔNG dialog (do xử lý ở Home flow) |
| S3-004 | listFiles → NetworkError | 🟢 | error = "Mất kết nối" |
| S3-005 | Pagination: loadMore khi hasMore = true | 🟢 | currentPage++, items append |
| S3-006 | Pagination: loadMore khi loadingMore = true | 🟢 | No-op |
| S3-007 | Pagination: loadMore khi hasMore = false | 🟢 | No-op |
| S3-008 | Sort: folders trước file, name lower-case | 🟢 | sorted correct |
| S3-009 | Click folder → onOpenFolder | 🔴 | Navigate Browse với folder.path = linkcode |
| S3-010 | Click file → onOpenFile | 🔴 | Navigate FileDetail |
| S3-011 | Back → navigateUp | 🔴 | Pop stack |
| S3-012 | D-pad navigation grid | 🔴 | Focus grid item by item |

## S4 — File Detail

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S4-001 | getFile success — file video | 🟢 | Loaded(playable=true) |
| S4-002 | getFile success — file ZIP | 🟢 | Loaded(playable=false) |
| S4-003 | getFile NetworkError | 🟢 | Error("Mất kết nối") |
| S4-004 | getFile AuthExpired | 🟢 | Error generic |
| S4-005 | Click Play button (playable) | 🔴 | Navigate Player |
| S4-006 | Click Play button (non-playable) | 🔴 | Disabled hoặc tooltip |
| S4-007 | Back | 🔴 | navigateUp |

## S5 — Player

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S5-001 | fetchStreamUrl Success — không có saved position | 🟢 | Ready(initialPosition=0, needsResumePrompt=false) |
| S5-002 | fetchStreamUrl Success — có saved position resumable | 🟢 | Ready(needsResumePrompt=true) |
| S5-003 | fetchStreamUrl Success — resumeMs param > 0 | 🟢 | Ready(initialPosition=resumeMs) |
| S5-004 | Body code = 123 (password required) | 🟢 | PasswordRequired state |
| S5-005 | retryWithPassword với password đúng | 🟢 | Ready |
| S5-006 | retryWithPassword với password sai | 🟢 | PasswordRequired (lần 2) |
| S5-007 | Body code = 404 | 🟢 | Error(NotFound) |
| S5-008 | Body code = 471 (too many sessions) | 🟢 | Error(TooManySessions) với hint |
| S5-009 | NetworkError | 🟢 | Error(Network, "Mất kết nối") |
| S5-010 | AuthExpired | 🟢 | Error(Auth) |
| S5-011 | savePosition khi positionMs > 0 | 🟢 | Room upsert |
| S5-012 | savePosition khi positionMs = 0 | 🟢 | No-op |
| S5-013 | onPlayerError(Codec) | 🟢 | Error(Codec) |
| S5-014 | URL Fshare có ?share=8805984 referral | 🟢 | Repository tự append |
| S5-015 | URL có sẵn query — append &share | 🟢 | URL đúng pattern |
| S5-016 | Default TTL 12h khi server không trả expiresIn | 🟢 | StreamUrl.expiresAt ≈ now + 12h |
| S5-017 | Bind ExoPlayer với URL → playback | 🔴 | Video play đúng |
| S5-018 | Pause / Resume / Seek bằng D-pad | 🔴 | Controller hiển thị + behaviour đúng |

## S12 — Search

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S12-001 | Query rỗng | 🟢 | Idle, không call |
| S12-002 | Query 1 ký tự | 🟢 | Idle |
| S12-003 | Query "abc" debounce 500ms | 🟢 | Sau 500ms call API 1 lần |
| S12-004 | Query "abc" rồi "abcd" trong 200ms | 🟢 | Cancel call cũ, call mới |
| S12-005 | Search 200 → 5 video items | 🟢 | Loaded, filter chỉ giữ video extension |
| S12-006 | Search 200 → empty | 🟢 | Empty |
| S12-007 | Search 401 (bearer revoke) | 🟢 | Error(FORBIDDEN) |
| S12-008 | NetworkError | 🟢 | Error("Không có kết nối mạng") |
| S12-009 | Sanitize: cắt 256 ký tự | 🟢 | Pass query truncated |
| S12-010 | Sanitize: chứa "<script" → reject | 🟢 | Idle (không call) |
| S12-011 | Sanitize: chứa control char | 🟢 | Strip control |
| S12-012 | Replace "." bằng " " trước call API | 🟢 | Pass keyword đã replace |
| S12-013 | Cache: search lại same query trong 5min | 🟢 | Hit cache, không call API |
| S12-014 | Click result → Navigate FileDetail | 🔴 | linkcode được extract |

## S6-S9 — Settings

| ID | Workflow | Type | Expected |
|---|---|---|---|
| S6-001 | SettingsHub: open Account | 🔴 | Navigate |
| S6-002 | SettingsHub: open Playback | 🔴 | Navigate |
| S7-001 | Account: refreshMe + display | 🟢 | UI hiển thị level VIP |
| S7-002 | Account: Logout button | 🟢 | authStore.clear + onLoggedOut |
| S8-001 | Playback: toggle subtitle setting | 🟢 | DataStore persist |
| S8-002 | Network: toggle wifi-only | 🟢 | DataStore persist |
| S9-001 | About: hiển thị version | 🟢 | versionName từ BuildConfig |

---

## Network Layer

| ID | Workflow | Type | Expected |
|---|---|---|---|
| NW-001 | apiCall HTTP 201 | 🟢 | AuthExpired |
| NW-002 | apiCall HTTP 200 + body code 201 | 🟢 | AuthExpired |
| NW-003 | apiCall HTTP 200 + body code 200 | 🟢 | Success |
| NW-004 | apiCall HTTP 200 + body code 405 | 🟢 | Error("405", msg) |
| NW-005 | apiCall HTTP 200 + body code "authenticate" | 🟢 | Error("authenticate", msg) |
| NW-006 | apiCall HTTP 200 + body null | 🟢 | Error(PARSE_ERROR) |
| NW-007 | apiCall HTTP 429 | 🟢 | RateLimited(retryAfter) |
| NW-008 | apiCall HTTP 500 errorBody parse được | 🟢 | Error(parsedCode, parsedMsg) |
| NW-009 | apiCall HTTP 500 errorBody parse fail | 🟢 | Error("500", "HTTP 500") |
| NW-010 | apiCall IOException | 🟢 | NetworkError |
| NW-011 | apiCall HttpException | 🟢 | Error(UNKNOWN) |
| NW-012 | IntOrStringAdapter: NUMBER 200 | 🟢 | "200" |
| NW-013 | IntOrStringAdapter: STRING "authenticate" | 🟢 | "authenticate" |
| NW-014 | IntOrStringAdapter: NULL | 🟢 | null |
| NW-015 | IntOrStringAdapter: BOOLEAN true | 🟢 | "true" |
| NW-016 | FshareHeaderInterceptor sets UA + cache-control | 🟢 | Headers present |
| NW-017 | FshareHeaderInterceptor không override UA nếu request đã set | 🟢 | UA gốc giữ nguyên |
| NW-018 | AuthInterceptor inject Cookie session_id cho Fshare host | 🟢 | Cookie header |
| NW-019 | AuthInterceptor không inject cho Timfshare host | 🟢 | No Cookie |
| NW-020 | AuthInterceptor không inject cho /api/user/login | 🟢 | No Cookie |

## Repository Layer

| ID | Workflow | Type | Expected |
|---|---|---|---|
| RP-001 | AuthRepository.login Success | 🟢 | Session saved, returns LoginResult |
| RP-002 | AuthRepository.login token thiếu | 🟢 | Error(PARSE_ERROR) |
| RP-003 | AuthRepository.logout | 🟢 | clear local |
| RP-004 | UserRepository.refreshMe flat success | 🟢 | UserCache upserted |
| RP-005 | UserRepository.refreshMe nested data success | 🟢 | flatten merges + upsert |
| RP-006 | UserRepository.refreshMe email priority email > user_email > login | 🟢 | Đúng |
| RP-007 | UserRepository.refreshMe level priority level > user_level > account_level | 🟢 | Đúng |
| RP-008 | FilesRepository.listFiles input là URL đầy đủ | 🟢 | canonicalFolderUrl OK |
| RP-009 | FilesRepository.listFiles input là linkcode trần | 🟢 | Build URL OK |
| RP-010 | FilesRepository.listFiles input không hợp lệ | 🟢 | Error(PARSE_ERROR) |
| RP-011 | FilesRepository.listFiles ext filter | 🟢 | Chỉ giữ folder + file matching ext |
| RP-012 | FilesRepository.searchFiles | 🟢 | Error(UNKNOWN, "M3 stub") |
| RP-013 | FilesRepository.listFolderTree | 🟢 | Success(emptyList) |
| RP-014 | PlayerRepository.fetchStreamUrl Success | 🟢 | StreamUrl với expiresAt = now + 12h |
| RP-015 | PlayerRepository.fetchStreamUrl referral append | 🟢 | URL chứa share=8805984 |
| RP-016 | FeaturedRepository.suggested Success | 🟢 | Items từ Sheet |
| RP-017 | FeaturedRepository.suggested cache hit | 🟢 | Lần 2 không call API |
| RP-018 | FeaturedRepository.trending sheet empty + Timfshare success | 🟢 | Items từ Timfshare |
| RP-019 | FeaturedRepository.trending sheet empty + Timfshare error | 🟢 | Empty list |
| RP-020 | SearchRepository.search "" | 🟢 | Success(emptyList) |
| RP-021 | SearchRepository.search "abc" cache hit | 🟢 | Lần 2 không call API |
| RP-022 | SearchRepository.search "<script>alert" | 🟢 | Success(emptyList) (sanitize blocked) |

## Helpers

| ID | Workflow | Type | Expected |
|---|---|---|---|
| HE-001 | CsvParser parse CSV chuẩn | 🟢 | Rows correct |
| HE-002 | CsvParser parse quoted field | 🟢 | Quotes preserved |
| HE-003 | CsvParser parse "" escape | 🟢 | Single " in cell |
| HE-004 | CsvParser parse CRLF line ending | 🟢 | Same as LF |
| HE-005 | CsvParser parse comma trong quoted | 🟢 | Cell có comma |
| HE-006 | CsvParser parse trailing newline | 🟢 | Last row included |
| HE-007 | FshareUrl.normalize http→https | 🟢 | "https://..." |
| HE-008 | FshareUrl.normalize fshare.vn → www.fshare.vn | 🟢 | Có www |
| HE-009 | FshareUrl.normalize URL non-fshare | 🟢 | null |
| HE-010 | FshareUrl.extractLinkcode file URL | 🟢 | "ABC123" |
| HE-011 | FshareUrl.extractLinkcode folder URL | 🟢 | "ABC123" |
| HE-012 | FshareUrl.isFolderUrl | 🟢 | True/false |

## Global UI

| ID | Workflow | Type | Expected |
|---|---|---|---|
| GU-001 | Network lost → NoNetworkOverlay | 🟡 | Overlay visible |
| GU-002 | Network restored → Overlay dismiss | 🟡 | Overlay hidden |
| GU-003 | Snackbar T1-T11 | 🟡 | Hiển thị 3-4s rồi dismiss |
| GU-004 | SessionExpiredDialog không cho dismiss bằng back | 🔴 | Force user click "Đăng nhập lại" |

---

## Test stack

- **Unit**: JUnit 4 + MockK + Turbine + kotlinx-coroutines-test
- **Instrumented**: AndroidX Test, Espresso, Compose UI test (defer nếu thiếu device)
- **CI**: `./gradlew test` chạy mọi unit; `./gradlew connectedAndroidTest` cho instrumented (cần emulator)

## Coverage target

- Network primitives (apiCall, IntOrStringAdapter): 100%
- Repository impls: 80%+
- ViewModels: 70%+ (focus state transitions)
- Helpers (CsvParser, FshareUrl): 100%
