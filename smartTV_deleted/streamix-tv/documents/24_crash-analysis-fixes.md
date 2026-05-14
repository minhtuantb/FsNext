---
title: StreamIX TV — Crash Analysis & TV-specific Fixes
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Crash Analysis — Folder Access & TV Compatibility

## Bug đã fix trong session này

### BUG 1 — Onboarding stuck (CRITICAL — đã fix trước)

**Symptom**: User click "Get started" trong Onboarding → app stuck.

**Root cause**: `SettingsDataStore.update()` dùng `flow.collect { ... return@collect }`. `return@collect` chỉ thoát lambda, KHÔNG terminate flow. `ds.data` là long-lived flow → collect hang vĩnh viễn → coroutine không bao giờ tiến đến `onCompleted()`.

**Fix**: Dùng `observe().first()` thay `collect`. `first()` tự terminate sau khi nhận value đầu tiên.

**File**: `core/data/src/main/.../SettingsDataStore.kt`

---

### BUG 2 — Folder access crash (CRITICAL — đang fix)

**Symptom**: Click vào folder trong Home/Browse → app crash.

**Root cause**: 3 lỗi cùng lúc:

#### 2.1 — `apiCall` không catch `JsonDataException`
`apiCall` chỉ catch `IOException` + `HttpException`. Moshi `JsonDataException` (RuntimeException subclass) propagate uncaught → kill coroutine → crash app.

#### 2.2 — getFolderList response shape không stable
Theo spec `API_REFERENCE_FOR_REUSE.md §3.5`:
```
Response: có thể là array, hoặc object có data/items/folders/files (array) hoặc data.items/data.folders/data.files.
```
DTO `GetFolderListResponseDto` chỉ accept object → khi server trả `[{...}]` thuần → JsonDataException("Expected BEGIN_OBJECT but was BEGIN_ARRAY") → crash via 2.1.

#### 2.3 — Browse subfolder navigation truyền field sai
`onOpenFolder = { folder -> BrowseRoute(folder.path, folder.name) }`. Trước fix, `folder.path = ""` (empty) → `canonicalFolderUrl("")` returns null → Error.

**Fix**:
1. `apiCall` catch `JsonDataException`, `JsonEncodingException`, và **mọi `Throwable`** — không bao giờ propagate.
2. `getFolderList` đổi return type sang `Response<ResponseBody>` — Repository tự parse manually bằng Moshi `JsonReader`. Hỗ trợ tất cả 5 shape có thể của server.
3. `FolderListItem.toDomainOrNull()` đặt `path = linkcode` để khi navigate subfolder, nav luôn pass linkcode. Đồng thời sửa NavGraph: `BrowseRoute(folder.linkcode, folder.name)`.

**Files**:
- `core/network/src/main/.../envelope/ApiCall.kt` — catch-all + Timber.
- `core/network/src/main/.../service/FilesApi.kt` — Response<ResponseBody>.
- `core/data/src/main/.../repository/FilesRepositoryImpl.kt` — manual JSON parser handle array OR object OR nested data.
- `app/src/main/.../navigation/StreamIXNavGraph.kt` — pass linkcode.

---

## Phân tích nghiệp vụ — Predict failure modes (theo flow)

### Flow A: Splash → Login → Onboarding → Home

| Step | Failure mode | Mitigation hiện tại |
|---|---|---|
| Splash | `refreshMe()` 401/timeout | Offline-first: nếu cache có user → vào Home; nếu không → Login |
| Splash | Hilt injection fail | Hilt validate compile-time, fail at startup → ANR. Chưa có recovery |
| Login | Network fail | `ApiResult.NetworkError` → snackbar "Mất kết nối" |
| Login | Server returns malformed JSON | `apiCall` catch JsonDataException → Error(PARSE_ERROR) |
| Login | App key blocked (code 39) | LoginErrorKey.AppKeyError mapping |
| Login | session_id rỗng nhưng code=200 | Check + return Error(PARSE_ERROR) |
| Onboarding | DataStore write fail | Đã fix BUG 1 |
| Onboarding | User press Back rapidly | BackHandler chưa wire — back về Login default |
| Home | observeMe() Room flow fail | Room exception caught by Compose lifecycle, state vẫn render |
| Home | Sheets fetch timeout | NetworkError → Row state Error |
| Home | Sheets returns HTML (not public) | CSV parser returns 0 rows → Row state Empty |
| Home | Timfshare bearer rỗng | Trending fallback → 401 → Error message |
| Home | Recompose loop khi Flow emit nhiều | Compose tự handle distinctUntilChanged ở stateIn |

### Flow B: Home → Click Featured → Browse / FileDetail / Player

| Step | Failure mode | Mitigation |
|---|---|---|
| Click Featured (folder URL) | linkcode parse fail | NavGraph check null → no-op (silent) |
| Click Featured (file URL) | Same | Same |
| Browse load | API returns array | Đã fix BUG 2 |
| Browse load | API returns object with `code: 471` | Body code dispatch → Error("Quá nhiều phiên") |
| Browse load | API returns 404 | HTTP non-2xx → Error("HTTP 404") — UI shows retry |
| Browse load | Cookie session_id expired (HTTP 201) | AuthExpired → ViewModel state error → user re-login flow |
| Browse pagination | loadMore khi offline | NetworkError → silent fail (loadingMore = false) |
| Browse subfolder click | folder.linkcode null/empty | navigate ignores → no-op |
| FileDetail load | Server trả flat hoặc nested | DTO accept both via flatten() |
| FileDetail Play | onPlay với linkcode rỗng | PlayerScreen handles empty linkcode → Error |

### Flow C: Player playback

| Step | Failure mode | Mitigation |
|---|---|---|
| fetchStreamUrl | Body code 123 (password) | PasswordRequired state → dialog |
| fetchStreamUrl | Body code 471 (too many sessions) | Error(TooManySessions) với hint |
| fetchStreamUrl | Body code 404 | Error(NotFound) |
| fetchStreamUrl | Cookie expired | AuthExpired |
| fetchStreamUrl | location field null | Error(PARSE_ERROR) |
| ExoPlayer prepare | URL invalid (HTTP 403) | onPlayerError(BAD_HTTP_STATUS) → ErrorCause.UrlExpired |
| ExoPlayer prepare | Codec không hỗ trợ (vd. HEVC trên TV cũ) | onPlayerError(DECODER_INIT_FAILED) → ErrorCause.Codec |
| ExoPlayer prepare | Network timeout | onPlayerError(NETWORK_CONNECTION_FAILED) → ErrorCause.Network |
| ExoPlayer prepare | OOM khi load video lớn | Crash native — không catch được. TV box ≥2GB RAM ổn |
| Playback | User press BACK | StopPlaybackDialog hỏi confirm |
| Playback | App background → resume | onPause/onResume managed bởi Compose lifecycle |
| Playback | savedPosition.observe() flow stuck | first() — chỉ lấy 1 value rồi unsubscribe |

### Flow D: TV remote D-pad navigation

| Concern | Status |
|---|---|
| FsButton DPAD_CENTER firing onClick | OK (Modifier.clickable supports DPAD_CENTER) |
| Default focus mỗi screen | ⚠️ Một số screen thiếu — backlog B-B1 |
| Focus restoration sau back | ⚠️ Compose chưa native restore — backlog B-B2 |
| KEYCODE_MEDIA_PLAY_PAUSE handler | ❌ Chưa implement — backlog B-A1 |
| Long-press FFW seek nhanh | ❌ Chưa — backlog |
| LazyVerticalGrid spatial nav | OK (Compose default) |

### Flow E: TV box / device-specific

| Concern | Status |
|---|---|
| compileSdk 35, minSdk 21 | ✅ Phủ TV box VN 2015+ |
| Landscape orientation | ✅ Manifest declared |
| Banner image cho ATV launcher | ✅ ic_banner.xml |
| LEANBACK_LAUNCHER intent filter | ✅ |
| Touch screen not required | ✅ touchscreen=false |
| HDCP requirement | ⏭️ V2 (Widevine DRM) |
| Memory < 192MB target | 🔴 Cần measure trên TV thật |
| Cold start < 2s | 🔴 Cần measure |

---

## TV-specific gotchas đã được account for

### G1: `runBlocking` trong AuthInterceptor
`AuthInterceptor.intercept` cần Cookie session_id (sync API của OkHttp). Solution: `runBlocking { tokenProvider.sessionId() }`. AuthStore.sessionId() đọc từ in-memory `_sessionFlow.value` — instant, không suspend thật. Không block thread đáng kể.

### G2: ExoPlayer release lifecycle
`DisposableEffect(exoPlayer) { onDispose { exoPlayer.release() } }` — ExoPlayer giải phóng khi Composable rời composition. Tránh leak.

### G3: Coil image cache trên TV box thiếu RAM
Coil mặc định 1/8 max heap cho memory cache, 2% disk cho disk cache. Trên TV box 1GB RAM (max heap ~256MB), memory cache ~32MB — đủ cho Featured rows.

### G4: Network detect trên TV WiFi
`GlobalUiViewModel.networkCallback` dùng ConnectivityManager.NetworkCallback — phát hiện mất WiFi dưới 2 giây. Hiển thị G1 NoNetworkOverlay.

### G5: Multi-host OkHttp clients
3 client riêng (Fshare/Timfshare/Sheets) với UA + auth khác nhau → tránh leak Cookie session_id sang Timfshare/Sheets.

### G6: TLS validation
Mặc định OkHttp validate cert chain. Nếu TV box cũ thiếu CA root → request fail. Workaround (nếu cần): bundle `fshare_chain.pem` và load qua TrustManager — chưa implement, backlog nếu phát hiện vấn đề.

---

## Test cases cho folder access (bổ sung 19_test-catalog.md)

| ID | Test | Expected |
|---|---|---|
| FA-001 | listFiles với path = "ABC123" (linkcode trần) | URL build = "https://www.fshare.vn/folder/ABC123" |
| FA-002 | listFiles với path = full URL có query | Query bị strip khỏi linkcode |
| FA-003 | listFiles với path = "" | Error PARSE_ERROR (không crash) |
| FA-004 | listFiles với path = "abc" (< 6 chars) | Error PARSE_ERROR |
| FA-005 | Server trả `[{...}]` array | Parsed thành list items |
| FA-006 | Server trả `{code:200, items:[...]}` object | Parsed thành list items |
| FA-007 | Server trả `{code:200, data:{items:[...]}}` nested | Parsed thành list items |
| FA-008 | Server trả `[]` rỗng | Success(emptyList) |
| FA-009 | Server trả `{code:201}` | AuthExpired |
| FA-010 | Server trả HTTP 201 | AuthExpired |
| FA-011 | Server trả malformed JSON `"not json"` | Error(PARSE_ERROR) — KHÔNG crash |
| FA-012 | Server trả empty body | Success(emptyList) |
| FA-013 | size là String "509.78 MB" | toLongOrNull() returns 0 (safe) |
| FA-014 | size là Long 509000000 | Parse thành 509000000L |
| FA-015 | type là Int 1 | Map về "1" string → ItemType.FILE |
| FA-016 | type là String "0" | ItemType.FOLDER |
| FA-017 | linkcode null trong item | mapNotNull skip item |
| FA-018 | name null trong item | mapNotNull skip item |
| FA-019 | ext filter "mp4" | Giữ folder + file .mp4, bỏ .pdf |
| FA-020 | Pagination page=2 | pageIndex=1 trong API call |

Tests đã viết sẵn trong `core/data/src/test/.../FilesRepositoryImplTest.kt`.

---

## Khuyến nghị test trên TV thật

Sau khi build APK debug:
1. Cài qua USB hoặc TV File Manager.
2. Login với account có folder + file thật.
3. Click vào folder Home → Browse render.
4. Click subfolder → Browse render lại.
5. Click file → FileDetail → Play → Player render.
6. Test các edge case: folder rỗng, folder lỗi 404, video password-protected.
7. Test với network ngắt — xem G1 NoNetworkOverlay hiển thị đúng không.
8. Test với session expired (chờ token Fshare hết hạn) — xem AuthExpired flow.
