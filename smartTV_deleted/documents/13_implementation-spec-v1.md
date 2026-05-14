---
title: 13 — Implementation Specification V1 (StreamIX TV)
date: 2026-05-04
version: 1.1
revision_note: |
  v1.1 (2026-05-05) — Migrate sang Fshare Open API Gateway (oapi.fshare.vn /api/v1/*).
  Khác biệt nền tảng với v1.0: JWT Bearer + X-App-Key header thay PHPSESSID cookie;
  response envelope { data, meta, request_id } thay { code, msg }; error code string thay numeric.
  Đóng 6/8 open items theo quyết định user 2026-05-05. Xem §3.0 và §29.
status: BẮT BUỘC đọc trước khi engineering implement code
audience: Senior Android Engineers, Tech Lead, QA
owner: Tech Lead Android
inputs: 02 (architecture), 04 rev3 (distribution), 09 (UX), 10 (handoff), 11 (audit), 12 (scope), `D:\Work\fshare-api-gateway\docs\api`, design package v1.0
---

# 13. Implementation Specification V1

## 0. Mục đích & cách dùng tài liệu

Tài liệu này là **single source of truth cho engineer khi viết code**. Tổng hợp từ các tài liệu đã chốt:

- **Scope**: theo `12_scope-decisions-v1.md` (V1 sau cuts).
- **UX**: theo `09_design-spec.md` + design package `streamix-handoff/` v1.0.
- **Architecture**: theo `02_khuyen-nghi-kien-truc.md`.
- **APIs**: từ `D:\Work\FsNext\docs\02_api_contracts.md` (desktop reference).
- **Distribution**: theo `04_apk-va-update.md` rev 3 (Phương án A).

Mỗi screen được spec theo cấu trúc 10 mục. Engineer đọc một screen là có thể bắt đầu code mà không phải đoán.

**Khi có mâu thuẫn**: tài liệu này thắng tài liệu cũ; nếu mâu thuẫn với design package → flag với PM trước khi tiếp tục.

---

# Phần A — Tổng quan

## 1. Scope V1 (chốt)

### 1.1 18 screens trong V1

| ID | Tên | Module Gradle | Ưu tiên implement |
|----|-----|---------------|-------------------|
| S0 | Splash | `:app` | P1 (Phase 0) |
| S1 | Login (email/password) | `:feature:auth` | P1 (Phase 2) |
| S2 | Home | `:feature:home` | P1 (Phase 2) |
| S3 | Browse Folder | `:feature:browse` | P1 (Phase 2) |
| S4 | File Detail | `:feature:browse` | P1 (Phase 2) |
| S5 | Player Full-screen | `:feature:player` | P1 (Phase 3) |
| S5a | Player Overlay | `:feature:player` | P1 (Phase 3) |
| S5b | Track Selection (subtitle/audio) | `:feature:player` | P2 (Phase 3) |
| S5c | Player Error | `:feature:player` | P1 (Phase 3) |
| S5d | Resume Prompt | `:feature:player` | P1 (Phase 3) |
| S7 | Settings Hub | `:feature:settings` | P2 (Phase 4) |
| S7a | Settings Account | `:feature:settings` | P2 (Phase 4) |
| S7b | Settings Playback | `:feature:settings` | P2 (Phase 4) |
| S7d | Settings Network (info-only) | `:feature:settings` | P3 (Phase 4) |
| S7f | Settings About + Send Log | `:feature:settings` | P2 (Phase 4) |
| S9 | Onboarding (first-run) | `:feature:onboarding` | P2 (Phase 2) |
| S10 | Confirm Dialog (template) | `:core:ui` | P1 (component) |
| S11 | Global Error / No Network | `:core:ui` | P1 (component) |
| S12 | Search | `:feature:search` | P2 (Phase 3) |

### 1.2 KHÔNG implement V1

Theo `12` D-2: S1a (QR), S1c (OAuth), S6 (Downloads), S7c (Settings Download), S7e (Settings Update), S8 (Update prompts), S5e (Post-play — designer cut). Tương ứng các module Gradle: `:feature:download`, `:feature:update`, các sub-screen Settings tương ứng — KHÔNG tạo.

## 2. Architecture quick reference

```
UI Compose (TV) → ViewModel (AAC) → UseCase → Repository → DataSource (API + Room + DataStore)
                                                              ↓
                                                        FshareApiService (Retrofit)
                                                        Room DB (PlaybackPosition, etc.)
                                                        DataStore Prefs (Settings, Auth)
                                                        EncryptedSharedPreferences (Token)
```

DI: **Hilt**. Async: **Coroutines + Flow**. Navigation: **Navigation Compose** trong single Activity.

## 3. API contracts dùng trong V1

### 3.0 Source of truth — Fshare Open API Gateway

V1 dùng **Fshare Open API Gateway** thay vì legacy desktop API. Source of truth duy nhất:

```
D:\Work\fshare-api-gateway\docs\api\
├── README.md           ← quy ước chung (envelope, headers, rate limit)
├── openapi.yaml        ← OpenAPI 3.0 source of truth
├── v1\auth.md          ← login, refresh, logout, register, oauth
├── v1\me.md            ← GET /me
├── v1\system.md        ← health
├── v1\session.md       ← TODO (Stream URL endpoint — chưa document)
├── v1\files.md         ← TODO
└── examples\           ← mock JSON request+response cho mỗi endpoint
```

Khác biệt với desktop API:

| Mặt | Desktop legacy | **Open API Gateway (V1)** |
|-----|----------------|---------------------------|
| Base URL | `https://api.fshare.vn` | `https://oapi.fshare.vn` |
| Versioning | Không | Bắt buộc `/api/v1/...` |
| Auth | Cookie `PHPSESSID=xxx; key=appkey` | Header `Authorization: Bearer <jwt>` + `X-App-Key: <key>` |
| Token model | Single session token, không refresh | JWT 1h + refresh_token 7d (rotate per family) |
| Success envelope | `{code: 200, msg, ...payload}` | `{data, meta?, request_id}` |
| Error envelope | `{code: <int>, msg}` | `{error: {code: <STRING>, message, details?}, request_id}` |
| Error codes | Numeric (200/401/502...) | String (`AUTH_INVALID_CREDENTIALS`, `RATE_LIMIT_EXCEEDED`...) |
| Rate limit hint | Không | Headers `X-RateLimit-Limit/Remaining/Reset` |
| Request ID | Không | Header `X-Request-Id` (ULID, server gen nếu thiếu) |

**App Key cho TV**: theo D-4 dùng cùng desktop key (`dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` 2026 plaintext, XOR-encoded). Tuy nhiên header format mới là `X-App-Key: <key>` chứ không còn cookie. Đặt giá trị tạm `tv_v1_<key>` nếu backend yêu cầu prefix platform; default fallback dùng raw desktop key — confirm với backend ở Phase 0 spike.

### 3.1 Endpoints dùng trong V1

| ID | Method + Path | Auth | Dùng ở screen | Spec file | Phase |
|----|---------------|------|---------------|-----------|-------|
| API-1 | `POST /api/v1/auth/login` | None | S1 | `v1/auth.md` | 2 |
| API-2 | `POST /api/v1/auth/refresh` | refresh_token | Auto (interceptor) | `v1/auth.md` (TODO) | 2 |
| API-3 | `POST /api/v1/auth/logout` | Bearer | S7a | `v1/auth.md` (TODO) | 4 |
| API-4 | `GET /api/v1/me` | Bearer + AppKey | S0 (verify), S2, S7a | `v1/me.md` | 2 |
| API-5 | `GET /api/v1/files?path=&page=&per_page=&dir_only=&ext=` | Bearer + AppKey | S2 (folders), S3 (browse) | `openapi.yaml` §`/api/v1/files` | 2 |
| API-6 | `GET /api/v1/files/{linkcode}` | Bearer + AppKey | S4 | `openapi.yaml` §`/api/v1/files/{linkcode}` | 2 |
| API-7 | `GET /api/v1/files/{linkcode}/metadata` | Bearer + AppKey | S4 (extended info) | `openapi.yaml` | 2 |
| API-8 | `GET /api/v1/files/search?q=&path=&ext=&page=&per_page=` | Bearer + AppKey | S12 | `openapi.yaml` §`/api/v1/files/search` | 3 |
| API-9 | `GET /api/v1/folders` | Bearer + AppKey | S2 (folder tree root), S3 | `openapi.yaml` §`/api/v1/folders` | 2 |
| **API-10** | **Stream URL endpoint** ⏳ | Bearer + AppKey | S5 Player | `v1/session.md` (chưa document — backend cần build) | **3 — BLOCKER** |
| API-11 | `GET /api/v1/health` | None | Debug only | `v1/system.md` | — |

### 3.2 Endpoints KHÔNG gọi trong V1

Đã có trong gateway nhưng V1 không dùng (theo scope cuts ở `12`):
- `/api/v1/auth/oauth/{provider}` — cắt OAuth
- `/api/v1/auth/register`, `/auth/password/*`, `/me/delete*` — TV không quản lý account lifecycle
- `/api/v1/files/{linkcode}/rename`, `/copy`, `/move`, `/delete`, `/recycle*` — cắt file ops
- `/api/v1/files/{linkcode}/visibility`, `/password*`, `/check-duplicate` — cắt security ops
- `/api/v1/files/favorites`, `/files/{linkcode}/favorite` — V1 chỉ persist local Room (V2 sync server)
- `/api/v1/folders` (POST), `/folders/{linkcode}/follow*`, `/me/follows*` — cắt social/follow
- `/api/v1/shares/*` — cắt sharing
- `/api/v1/photos`, `/notifications`, `/app/config` — TODO ở backend, V1 không dùng

### 3.3 ⚠️ Stream URL endpoint — gap cần resolve trước Phase 3

Theo `D:\Work\fshare-api-gateway\docs\api\README.md` line 116, group "Session" có file `v1/session.md` được liệt kê nhưng **chưa được implement**. Tag mô tả "Download / upload session, generate direct link" nhưng không có path nào trong `openapi.yaml`.

Implication cho Phase 3 Player:
- Trước khi engineering bắt đầu module `:feature:player`, backend phải define + ship endpoint trong `v1/session.md`.
- Estimate cần backend: shape giống legacy `POST /api/session/download` với input `{linkcode, password?, zip?}` → output `{data: {url, expires_in, ...}}`.
- Action: PM đặt ticket backend, ETA trước Tuần 7 (start Phase 3).

API-10 trong bảng §3.1 sẽ được cập nhật khi `v1/session.md` ship; engineer mock interface trước, swap implementation khi backend ready.

## 4. Common patterns — bắt buộc tuân thủ

### 4.1 Network layer (`:core:network`) — Open API Gateway

Base URL: `https://oapi.fshare.vn` (production) / `https://oapi.dev.fshare.vn` (staging).

Chain interceptors (theo thứ tự):

1. `AppKeyInterceptor` — chèn `X-App-Key` cố định mọi request.
2. `UserAgentInterceptor` — chèn `User-Agent: Fshare/androidtv/{versionName}`.
3. `AuthInterceptor` — chèn `Authorization: Bearer <jwt>` cho endpoint cần auth.
4. `RequestIdInterceptor` — gen ULID `X-Request-Id` nếu chưa có.
5. `TokenRefreshInterceptor` (Authenticator) — auto refresh khi 401 `AUTH_TOKEN_EXPIRED`.
6. `LoggingInterceptor` (debug only) — log request/response.

```kotlin
@Singleton
class AppKeyInterceptor @Inject constructor() : Interceptor {
    override fun intercept(chain: Interceptor.Chain): Response =
        chain.proceed(
            chain.request().newBuilder()
                .header("X-App-Key", BuildConfig.APP_KEY)
                .build()
        )
}

@Singleton
class AuthInterceptor @Inject constructor(
    private val authStore: AuthStore,
) : Interceptor {
    override fun intercept(chain: Interceptor.Chain): Response {
        val original = chain.request()
        // Bỏ qua endpoint public (login, register, health, refresh)
        if (original.tag(NoAuth::class.java) != null) return chain.proceed(original)
        val session = runBlocking { authStore.session() }
        val req = if (session != null) {
            original.newBuilder()
                .header("Authorization", "Bearer ${session.accessToken}")
                .build()
        } else original
        return chain.proceed(req)
    }
}

@Singleton
class TokenRefreshAuthenticator @Inject constructor(
    private val authStore: AuthStore,
    private val authApi: AuthApi,
) : Authenticator {
    private val mutex = Mutex()

    override fun authenticate(route: Route?, response: Response): Request? = runBlocking {
        if (response.priorResponse?.priorResponse != null) return@runBlocking null  // already retried twice
        val current = authStore.session() ?: return@runBlocking null
        // Single-flight: chỉ refresh 1 lần dù nhiều request 401 đồng thời
        mutex.withLock {
            val latest = authStore.session()!!
            if (latest.accessToken != current.accessToken) {
                // Đã có token mới do request khác refresh — dùng luôn
                return@withLock response.request.newBuilder()
                    .header("Authorization", "Bearer ${latest.accessToken}")
                    .build()
            }
            when (val r = authApi.refresh(RefreshRequest(latest.refreshToken))) {
                is ApiResult.Success -> {
                    val (access, refresh, expiresIn, user) = r.data
                    authStore.save(Session(access, refresh, user.email), ttl = expiresIn.seconds)
                    response.request.newBuilder()
                        .header("Authorization", "Bearer $access")
                        .build()
                }
                else -> {
                    authStore.clear()
                    null  // OkHttp sẽ trả 401 lên app, navigate Login
                }
            }
        }
    }
}
```

Đặc biệt:
- `BuildConfig.APP_KEY` build-time inject. Theo D-4: cùng desktop key (`dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` 2026 plaintext, **XOR-encoded** trong source). Khi backend confirm format prefix theo platform (vd `tv_v1_xxxx`), update `BuildConfig.APP_KEY` tương ứng.
- `User-Agent` format: `Fshare/androidtv/{versionName}` (theo convention API gateway README).
- Mọi POST/PUT/PATCH gửi `Content-Type: application/json`.
- Endpoint không cần auth (login, refresh, register, health) đánh dấu Retrofit annotation hoặc tag `NoAuth` để skip `AuthInterceptor`.

### 4.1.1 AuthStore — JWT model mới

```kotlin
data class Session(
    val accessToken: String,        // JWT, TTL 1h
    val refreshToken: String,       // rt_<ulid>_<random>, TTL 7d
    val email: String,
    val accessExpiresAt: Long = 0,  // epoch millis, computed from expires_in
)

@Singleton
class AuthStore @Inject constructor(
    @ApplicationContext context: Context,
) {
    private val prefs = EncryptedSharedPreferences.create(
        context, "streamix_auth",
        MasterKey.Builder(context).setKeyScheme(AES256_GCM).build(),
        AES256_SIV, AES256_GCM
    )

    suspend fun save(session: Session, ttl: Duration) = withContext(Dispatchers.IO) {
        prefs.edit()
            .putString("access", session.accessToken)
            .putString("refresh", session.refreshToken)
            .putString("email", session.email)
            .putLong("access_exp", System.currentTimeMillis() + ttl.inWholeMilliseconds)
            .apply()
    }

    suspend fun session(): Session? = withContext(Dispatchers.IO) {
        val access = prefs.getString("access", null) ?: return@withContext null
        val refresh = prefs.getString("refresh", null) ?: return@withContext null
        Session(
            accessToken = access,
            refreshToken = refresh,
            email = prefs.getString("email", "")!!,
            accessExpiresAt = prefs.getLong("access_exp", 0)
        )
    }

    suspend fun clear() = withContext(Dispatchers.IO) {
        prefs.edit().clear().apply()
    }
}
```

Lưu ý:
- Refresh token rotate per family — mỗi lần refresh trả `refresh_token` mới, lưu đè cái cũ.
- TTL refresh_token 7 ngày. Sau 7 ngày user phải re-login.
- Không cần "Nhớ tôi 30 ngày" extend — hết 7d là phải login. Update S1 spec tương ứng (xem §6.0.4).

### 4.2 Response envelope (Open API Gateway)

**Success (HTTP 2xx)**:
```json
{
  "data": { ... },
  "meta": { "page": 1, "per_page": 50, "total": 1234 },
  "request_id": "01HX8F2K3P9R5Z7Y6Q4M2N1V8B"
}
```

**Error (HTTP 4xx/5xx)**:
```json
{
  "error": {
    "code": "AUTH_INVALID_CREDENTIALS",
    "message": "Sai email hoặc mật khẩu",
    "details": { "field": "user_email" }
  },
  "request_id": "01HX8F2K3P9R5Z7Y6Q4M2N1V8B"
}
```

Engineer dùng Retrofit `Call<T>` với generic envelope wrapper:

```kotlin
@JsonClass(generateAdapter = true)
data class ApiEnvelope<T>(
    val data: T?,
    val meta: PaginationMeta? = null,
    val error: ApiError? = null,
    @Json(name = "request_id") val requestId: String? = null,
)

@JsonClass(generateAdapter = true)
data class ApiError(
    val code: String,                         // String, không phải numeric
    val message: String,
    val details: Map<String, Any>? = null,
)

@JsonClass(generateAdapter = true)
data class PaginationMeta(
    val page: Int,
    @Json(name = "per_page") val perPage: Int,
    val total: Int,
)
```

Wrap thành sealed class trong app:

```kotlin
sealed interface ApiResult<out T> {
    data class Success<T>(val data: T, val meta: PaginationMeta? = null): ApiResult<T>
    data class Error(val errorCode: String, val message: String, val httpStatus: Int): ApiResult<Nothing>
    data object NetworkError: ApiResult<Nothing>
    data object AuthExpired: ApiResult<Nothing>
    data class RateLimited(val retryAfterSeconds: Long? = null): ApiResult<Nothing>
}
```

#### 4.2.1 Error code mapping (string codes từ API gateway)

| API error.code | HTTP | App ApiResult | UI hành vi |
|----------------|------|---------------|------------|
| (no error) | 2xx | Success | proceed |
| `AUTH_INVALID_CREDENTIALS` | 401 | Error | S1: hiển thị "Email hoặc mật khẩu không đúng" |
| `AUTH_TOKEN_EXPIRED` | 401 | (tự refresh qua Authenticator) | retry tự động; nếu refresh fail → AuthExpired |
| `AUTH_TOKEN_MISSING` | 401 | AuthExpired | clear session, navigate S1 |
| `AUTH_TOKEN_INVALID` | 401 | AuthExpired | clear session, navigate S1 |
| `AUTH_USER_LOCKED` | 410 | Error | S1: "Tài khoản tạm khoá, liên hệ hỗ trợ" |
| `APP_KEY_INVALID` | 403 | Error (fatal) | S11 fatal: "App config sai, liên hệ IT" |
| `APP_KEY_MISSING` | 403 | Error (fatal) | S11 fatal |
| `VALIDATION_FAILED` | 422 | Error | hiển thị `details.field` cụ thể |
| `RESOURCE_NOT_FOUND` | 404 | Error | "File/folder không tồn tại" + back |
| `RESOURCE_FORBIDDEN` | 403 | Error | "Không có quyền truy cập" |
| `RESOURCE_CONFLICT` | 409 | Error | hiển thị `message` |
| `RATE_LIMIT_EXCEEDED` | 429 | RateLimited | đợi `Retry-After` rồi retry; nếu user-facing → toast "Vui lòng thử lại sau" |
| `INTERNAL_ERROR` | 500 | Error retryable | retry exp backoff 3 lần, sau cùng hiển thị toast |
| `SERVICE_UNAVAILABLE` | 503 | Error retryable | retry exp backoff |
| (network IOException) | — | NetworkError | hiển thị S11 |

Helper extension chuyển Retrofit Response → ApiResult:

```kotlin
suspend fun <T> Call<ApiEnvelope<T>>.toApiResult(): ApiResult<T> = try {
    val response = awaitResponse()
    val body = response.body() ?: response.errorBody()?.let {
        moshi.adapter(ApiEnvelope::class.java).fromJson(it.string())
    }
    when {
        response.isSuccessful && body?.data != null ->
            ApiResult.Success(body.data!! as T, body.meta)
        body?.error?.code in listOf("AUTH_TOKEN_MISSING", "AUTH_TOKEN_INVALID") ->
            ApiResult.AuthExpired
        body?.error?.code == "RATE_LIMIT_EXCEEDED" ->
            ApiResult.RateLimited(response.header("Retry-After")?.toLongOrNull())
        body?.error != null ->
            ApiResult.Error(body.error.code, body.error.message, response.code())
        else ->
            ApiResult.Error("UNKNOWN", "Lỗi không xác định", response.code())
    }
} catch (e: IOException) {
    ApiResult.NetworkError
}
```

### 4.3 ViewModel pattern

Mỗi screen có **một** ViewModel với:

```kotlin
@HiltViewModel
class SomeViewModel @Inject constructor(
    private val useCase: SomeUseCase,
) : ViewModel() {
    private val _uiState = MutableStateFlow<UiState>(UiState.Loading)
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private val _events = Channel<UiEvent>(Channel.BUFFERED)
    val events = _events.receiveAsFlow()

    sealed interface UiState {
        data object Loading : UiState
        data class Loaded(val data: ScreenData) : UiState
        data class Error(val message: String, val retry: () -> Unit) : UiState
        data object Empty : UiState
    }

    sealed interface UiEvent {
        data class NavigateTo(val route: String) : UiEvent
        data class ShowSnackbar(val text: String, val type: SnackType) : UiEvent
    }
}
```

UI thu `uiState` qua `collectAsStateWithLifecycle()`, thu `events` qua `LaunchedEffect` + `collect`.

### 4.4 Navigation (Navigation Compose)

Single Activity. Navigation graph khai báo route với type-safe args:

```kotlin
sealed interface Routes {
    @Serializable data object Splash : Routes
    @Serializable data object Login : Routes
    @Serializable data object Home : Routes
    @Serializable data class Browse(val folderCode: String, val folderName: String) : Routes
    @Serializable data class FileDetail(val fileCode: String) : Routes
    @Serializable data class Player(val fileCode: String, val resumePos: Long = 0L) : Routes
    @Serializable data object Settings : Routes
    @Serializable data class SettingsSub(val section: SettingsSection) : Routes
    @Serializable data object Onboarding : Routes
    @Serializable data class Search(val initialQuery: String = "") : Routes
}
```

Default `popUpTo` strategy: từ Login → Home thì pop Login (user không back về Login).

### 4.5 Error display

3 mức:
1. **Snackbar (C10)** — lỗi non-fatal, có thể tự dismiss sau 4s. Ví dụ: "Tên file đã được sao chép".
2. **ErrorState fullscreen (C15)** — lỗi load màn hình thất bại, có nút "Thử lại". Ví dụ: S3 không load được.
3. **Dialog (C8)** — lỗi cần user xác nhận. Ví dụ: file bị xoá khi đang xem.

Không bao giờ hiển thị stack trace cho user. Log nội bộ qua Timber.

### 4.6 Logging

Dùng `Timber` với planted tree:
- Debug: `Timber.DebugTree()`
- Release: `CrashlyticsTree` (silent log Crashlytics) hoặc `FileTree` (ghi `/data/data/<pkg>/files/log/streamix-tv-{date}.log` rotating max 5 MB).

User có thể "Gửi log" từ S7f → upload file log mới nhất tới `tv-update.fshare.local/log/feedback`.

### 4.7 Persistence

| Data | Store | Reason |
|------|-------|--------|
| Session token + PHPSESSID + email | EncryptedSharedPreferences | Bảo mật |
| User profile cache (TTL 1h) | Room (`user_cache`) | Offline |
| File listing cache (TTL 5 phút) | Room (`file_cache`) | Offline browse vừa xem |
| PlaybackPosition | Room (`playback_position`) | Resume |
| App settings (theme, playback, …) | DataStore Preferences | Type-safe |
| Onboarding completed flag | DataStore Preferences | Once |

---

# Phần B — Per-screen specifications

Mỗi screen theo template 10 mục:

1. ID, name, purpose, entry/exit
2. States
3. UI components dùng
4. Data flow + APIs
5. User behaviors + D-pad
6. Design tokens applied
7. Icons
8. Localization keys
9. ViewModel signature
10. Acceptance criteria

---

## 5. S0 — Splash

### 5.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Init app (Hilt, DataStore, Room), check session, route tiếp |
| Entry | Cold start app từ launcher |
| Exit | Có session → S2; không session → S1; init lỗi → S11 |
| Block back | Có (Back nuốt) |
| Time budget | < 1.5s nominal, ≤ 2.5s p95 |

### 5.0.2 States

- `Initializing`: đang init Hilt, DataStore.
- `Verifying`: có saved session, đang gọi `/api/user/get` để verify.
- `Routing`: đã quyết định destination, đang navigate.
- `InitFailed`: lỗi quá lâu (> 15s) — chuyển S11.

### 5.0.3 Components dùng

- Vector logo `logo_streamix_tv.svg` (cần designer giao v1.1)
- `ProgressBar` (C11) indeterminate, mảnh

### 5.0.4 Data flow + APIs

```
[Cold start]
  → Application.onCreate (Hilt Application)
  → SplashViewModel.init()
    → AuthStore.session() — đọc EncryptedSharedPreferences
    → Nếu session.accessExpiresAt < now (token hết hạn):
        → Authenticator sẽ tự refresh khi gọi API-4 dưới
    → Nếu refresh_token expire (>7 ngày): clear → Routing(Login)
    → Nếu session != null:
        → API-4: GET /api/v1/me
          Headers: Authorization: Bearer <jwt>, X-App-Key: <key>
          Response 200: { data: { id, email, full_name, is_vip, vip_level, vip_expires_at, storage_used, storage_quota, ... }, request_id }
        → 200 → emit Routing(Home), update Room user_cache
        → AUTH_TOKEN_INVALID/MISSING (sau khi refresh fail) → clear → Routing(Login)
        → NetworkError → cache fallback: nếu user_cache TTL còn → Routing(Home)
                                          không → emit Routing(Login)
    → Nếu session == null:
        → emit Routing(Login)
  → After Routing emitted: navigate, popUpTo<Splash>{ inclusive=true }
```

### 5.0.5 Behaviors

- Phím **bất kỳ**: ignored (block input).
- Hiển thị > 5s vẫn ở Initializing → render dòng caption "Đang khởi động..." (key `splash_loading`).
- > 15s → chuyển sang S11 với "Khởi động thất bại" + retry.

### 5.0.6 Tokens

| Token | Giá trị |
|-------|---------|
| Background | `color/bg/base` (#0F1218) |
| Logo size | 192 × 192 dp center |
| Caption typography | `body-md` |
| Progress bar | linear indeterminate, `color/accent/primary`, height 2 dp |
| Motion | exit fade + scale 1.0→1.1, `motion/quick` 200ms |

### 5.0.7 Icons

(không có — chỉ logo)

### 5.0.8 Localization keys

`app_name`, `splash_loading`, `error_init_title`, `error_init_action_retry`.

### 5.0.9 ViewModel skeleton

```kotlin
@HiltViewModel
class SplashViewModel @Inject constructor(
    private val authStore: AuthStore,
    private val getUserInfoUseCase: GetUserInfoUseCase,
) : ViewModel() {
    private val _state = MutableStateFlow<SplashState>(SplashState.Initializing)
    val state = _state.asStateFlow()

    init { viewModelScope.launch { route() } }

    private suspend fun route() {
        val session = authStore.session()
        if (session == null) {
            _state.value = SplashState.Routing(Routes.Login); return
        }
        when (val r = getUserInfoUseCase()) {
            is ApiResult.Success -> _state.value = SplashState.Routing(Routes.Home)
            ApiResult.AuthExpired -> {
                authStore.clear(); _state.value = SplashState.Routing(Routes.Login)
            }
            ApiResult.NetworkError -> _state.value = SplashState.Routing(Routes.Home) // cache fallback
            is ApiResult.Error -> _state.value = SplashState.InitFailed(r.msg)
        }
    }
}
```

### 5.0.10 Acceptance

- [ ] Cold start → S2 hiển thị < 2.5s trên Mi Box S khi session valid.
- [ ] Session expire → tự xoá + về S1.
- [ ] Mất mạng + có cache → vẫn vào S2 (offline-first).
- [ ] Phím bất kỳ trong splash bị ignore.

---

## 6. S1 — Login (email/password)

### 6.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Cho user đăng nhập tài khoản Fshare |
| Entry | S0 (không session) hoặc S7a (sau logout) |
| Exit | Login OK → S9 (lần đầu) hoặc S2; back ở S1 → exit app confirm |
| Auth method V1 | **CHỈ email/password** (D-2 cắt QR + OAuth) |

### 6.0.2 States

- `Idle`: form trống hoặc auto-fill từ EncryptedSharedPreferences.
- `Submitting`: đang gọi `/api/user/login`.
- `Error`: lỗi (sai mật khẩu, lock, network).
- `Success`: OK, đợi navigate (transient < 500ms).

### 6.0.3 Components

- `TextField` (C16) ×2 — email, password
- `Button` (C2) primary — "Đăng nhập"
- ~~`Switch` (C17) — "Nhớ tôi 30 ngày"~~ → BỎ (gateway có refresh_token TTL 7d cố định, không config được). Thay bằng caption "Phiên giữ trong 7 ngày" dưới nút Submit.
- `KeyboardOnScreen` (C21) — bật khi focus TextField, không có keyboard rời
- Logo nhỏ trên cùng
- Helper text "Quên mật khẩu? → fshare.vn trên điện thoại"

### 6.0.4 Data flow + APIs

```
User gõ email + password → Submit
  → API-1: POST /api/v1/auth/login
    Headers: Content-Type: application/json, X-App-Key: <key>, User-Agent: Fshare/androidtv/<ver>
    Body: { user_email, password }
    Response 200: {
      data: {
        access_token: "<jwt>",
        refresh_token: "rt_<ulid>_<rand>",
        token_type: "Bearer",
        expires_in: 3600,
        user: { id, email, full_name, is_vip, vip_level, vip_expires_at, storage_used, storage_quota, ... }
      },
      request_id
    }
  → Lưu vào EncryptedSharedPreferences:
      AuthStore.save(Session(accessToken, refreshToken, email), ttl = expires_in.seconds)
      Note: refresh_token TTL backend là 7 ngày — không cần TTL bên client; khi refresh fail (7d sau) thì AuthExpired tự logout
  → Lưu user info vào Room user_cache (TTL 1h, refetch via API-4 sau)
  → Check DataStore "onboarding_completed" flag:
      false → navigate S9 Onboarding
      true → navigate S2 Home
```

Error mapping (string codes):
- `AUTH_INVALID_CREDENTIALS` (401) → "Email hoặc mật khẩu không đúng"
- `AUTH_USER_LOCKED` (410) → "Tài khoản tạm khoá, liên hệ hỗ trợ"
- `VALIDATION_FAILED` (422) → highlight field bị lỗi, hiển thị `details.field` rồi `error.message`
- `RATE_LIMIT_EXCEEDED` (429) → "Quá nhiều lần thử, vui lòng đợi 15 phút" (5 fail/15min/IP, 10 fail/15min/email)
- `APP_KEY_INVALID/MISSING` (403) → fatal error S11 "App config sai, liên hệ IT"
- NetworkError → "Không có kết nối, kiểm tra mạng"

**Note về "Nhớ tôi"**: API gateway có refresh_token TTL 7 ngày cố định (không tuỳ chỉnh client). Switch "Nhớ tôi 30 ngày" trong UI cũ KHÔNG còn ý nghĩa — bỏ. Thay bằng caption "Phiên đăng nhập giữ trong 7 ngày" dưới nút Submit.

### 6.0.5 Behaviors + D-pad

| Element | OK | D-pad |
|---------|-----|-------|
| TextField email | mở on-screen keyboard, focus row đầu | trong keyboard: chuyển ký tự |
| TextField password | mở keyboard, masked input | tương tự |
| Eye icon (show/hide) password | toggle | — |
| Switch "Nhớ tôi" | toggle | — |
| Button "Đăng nhập" | submit | — |
| Helper text | (non-interactive) | — |

D-pad mặc định trong form: Up/Down chuyển field; Left/Right trong on-screen keyboard chuyển ký tự.

Long-press OK trên Eye icon: toggle. Submit chỉ khi:
- Email match regex `.+@.+\..+`
- Password ≥ 6 ký tự

### 6.0.6 Tokens

| Token | Giá trị |
|-------|---------|
| Form max width | 720 dp center |
| Field height | 64 dp |
| Field padding | `space/4` (16 dp) inner |
| Field border focus | 4 dp `color/accent/primary` |
| Field bg | `color/bg/surface` |
| Button height | 64 dp full-width |
| Spacing fields | `space/5` (24 dp) |

### 6.0.7 Icons

- `ic_email.svg` (leading icon TextField email)
- `ic_visibility.svg` / `ic_visibility_off.svg` — **DESIGNER CẦN BỔ SUNG** (chưa có trong v1.0)

### 6.0.8 Localization keys

`login_hub_title`, `login_hub_subtitle`, `login_method_email`, `login_method_email_hint`, `login_email_field`, `login_password_field`, `login_remember_me`, `login_button_submit`, `login_help_link`, `login_error_invalid_credentials`, `login_error_locked`, `login_error_network`, `login_show_password`, `login_hide_password`.

### 6.0.9 ViewModel

```kotlin
@HiltViewModel
class LoginViewModel @Inject constructor(
    private val loginUseCase: LoginUseCase,
    private val authStore: AuthStore,
    private val onboardingFlag: OnboardingFlagStore,
) : ViewModel() {
    private val _state = MutableStateFlow(LoginUiState())
    val state = _state.asStateFlow()

    fun submit(email: String, password: String, rememberMe: Boolean) = viewModelScope.launch {
        _state.update { it.copy(submitting = true, error = null) }
        when (val r = loginUseCase(email, password)) {
            is ApiResult.Success -> {
                authStore.save(r.data, ttl = if (rememberMe) 30.days else 1.days)
                val onboarded = onboardingFlag.isCompleted()
                _state.update { it.copy(submitting = false, success = true) }
                _events.send(if (onboarded) NavigateToHome else NavigateToOnboarding)
            }
            is ApiResult.Error -> _state.update {
                it.copy(submitting = false, error = mapErrorToString(r))
            }
            ApiResult.NetworkError -> _state.update {
                it.copy(submitting = false, error = stringResource(R.string.login_error_network))
            }
            ApiResult.AuthExpired -> _state.update {
                it.copy(submitting = false, error = stringResource(R.string.login_error_invalid_credentials))
            }
        }
    }
}
```

### 6.0.10 Acceptance

- [ ] Email + password đúng → vào Home (lần đầu qua Onboarding).
- [ ] Sai password → error hiển thị, không navigate.
- [ ] "Nhớ tôi" ON → reopen app trong 30 ngày không phải login lại.
- [ ] Network error → message rõ + nút retry.
- [ ] On-screen keyboard tự xuất hiện khi focus TextField.

---

## 7. S2 — Home

### 7.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Trung tâm browse — entry chính sau login |
| Entry | Login OK / Onboarding done; Splash với valid session |
| Exit | Card → S4 hoặc S3; Search → S12; Settings → S7; Avatar → S7a; Back → confirm exit |
| Rows V1 | 3 (Continue Watching / Recent / Folders) — cắt "Đã tải xuống" |

### 7.0.2 States

- `Loading`: skeleton 3 row.
- `Loaded`: full data.
- `Partial`: một row fail load (giữ row khác hoạt động).
- `Empty`: tài khoản trống — hiển thị empty state đơn giản.

### 7.0.3 Components

- `TopBar` (C1) — logo, search icon, avatar, settings
- `RowSection` (C6) ×3
- `FileCard` (C4) với progress overlay (Continue Watching)
- `FileCard` (C4) plain (Recent)
- `FolderCard` (C5) (Folders)
- `Skeleton` (C13)
- `EmptyState` (C14)

### 7.0.4 Data flow + APIs

```
On enter:
  Parallel:
    A. Continue Watching: Room query `playback_position`
       SELECT * WHERE positionMs > 30s AND positionMs < durationMs - 60s
       ORDER BY updatedAt DESC LIMIT 20
       → join với file metadata cache (Room) hoặc lazy fetch API-6 nếu thiếu
    B. Recent files: API-5 GET /api/v1/files
       Query: ?path=/&page=1&per_page=20&dir_only=0
       Sort client-side by last_updated_at desc, filter type=file
    C. Root folders: API-9 GET /api/v1/folders
       (trả flat tree, lọc parent_linkcode == null cho root, max 30)
       HOẶC fallback: API-5 GET /api/v1/files?path=/&dir_only=1&per_page=30

Render khi đủ:
  - Continue Watching ẩn nếu rỗng
  - Recent ẩn nếu rỗng
  - Folders luôn hiển thị (root folders user có)

Refresh:
  - Pull-down refresh (D-pad up từ row đầu khi đã ở top): refetch B + C
  - Continue Watching update tự động khi quay lại từ S5 (Flow từ Room)
```

### 7.0.5 Behaviors + D-pad

| Element | OK | Behavior |
|---------|-----|---------|
| Card Continue Watching | OK = Resume → S5 với resumePos | progress overlay 4dp dưới ảnh |
| Card Recent | OK = S4 File Detail | — |
| FolderCard | OK = S3 Browse | — |
| TopBar Search icon | OK = S12 | — |
| TopBar Settings | OK = S7 | — |
| TopBar Avatar | OK = S7a | — |
| Back | confirm "Thoát StreamIX TV?" | default focus "Không" |

D-pad map:
- Up từ TopBar = không di chuyển (top boundary).
- Down từ TopBar = vào row 1.
- Up/Down giữa các row.
- Left/Right scroll trong row.
- Cuối row Right: stop, không loop.
- TopBar focus order: logo (non-focusable) → search → settings → avatar.

Focus restoration:
- Vào Home lần đầu: focus item đầu tiên row 1 (Continue Watching nếu có; nếu không, row 2; nếu rỗng cả 2, row 3).
- Quay lại Home từ S3/S4/S5: restore focus về row + item cũ (dùng `rememberSaveable`).

### 7.0.6 Tokens

| Element | Token |
|---------|-------|
| Screen padding ngoài | `space/7` (48 dp) horizontal, `space/5` (24 dp) top |
| Row gap | `space/6` (32 dp) between rows |
| Card gap trong row | `space/4` (16 dp) |
| Row title typography | `title-md` (24sp/32) |
| TopBar height | 80 dp |
| Avatar size | 56 dp tròn |

### 7.0.7 Icons

`ic_search`, `ic_settings`, `ic_user` (avatar fallback), `ic_video` (default thumbnail), `ic_folder`.

### 7.0.8 Localization keys

`home_greeting_morning/afternoon/evening`, `home_section_continue`, `home_section_recent`, `home_section_folders`, `home_view_all`, `home_topbar_search_hint`, `home_topbar_account`, `home_topbar_settings`, `home_empty_title`, `home_empty_body`, `home_progress_label`, `home_back_confirm_title`, `home_back_confirm_body`.

### 7.0.9 ViewModel

```kotlin
@HiltViewModel
class HomeViewModel @Inject constructor(
    private val getContinueWatching: GetContinueWatchingUseCase,
    private val getRecentFiles: GetRecentFilesUseCase,
    private val getRootFolders: GetRootFoldersUseCase,
) : ViewModel() {
    val state: StateFlow<HomeUiState> = combine(
        getContinueWatching(),     // Flow<List<ContinueWatchingItem>>
        getRecentFiles().asResultFlow(),
        getRootFolders().asResultFlow(),
    ) { cw, recent, folders ->
        HomeUiState(
            continueWatching = cw,
            recentFiles = recent,
            folders = folders,
            isLoading = recent is Loading && folders is Loading,
        )
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), HomeUiState.Empty)

    fun refresh() = viewModelScope.launch {
        getRecentFiles.refresh()
        getRootFolders.refresh()
    }
}
```

### 7.0.10 Acceptance

- [ ] Mở Home < 2s sau login (data từ cache + bg fetch).
- [ ] Continue Watching update real-time khi xem xong S5.
- [ ] Mất mạng + có cache: hiển thị bình thường.
- [ ] Mất mạng + không cache: empty + nút "Thử lại".
- [ ] Focus restore đúng khi back từ S3/S4/S5.

---

## 8. S3 — Browse Folder

### 8.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Browse content trong 1 folder cụ thể |
| Entry | S2 (chọn folder), S3 (subfolder) |
| Exit | folder con → recurse S3; file → S4; Back → cha hoặc S2 |

### 8.0.2 States

- `Loading`: skeleton grid.
- `Loaded`: data full hoặc partial paginated.
- `Empty`: folder rỗng.
- `Error`: API fail.
- `Paginating`: đang load thêm trang.

### 8.0.3 Components

- `TopBar` (C1) variant breadcrumb
- Grid 4 cột × N hàng
- `FolderCard` + `FileCard`
- `Skeleton`, `EmptyState`, `ErrorState`
- Footer paginate spinner

### 8.0.4 Data flow + APIs

```
Args: folderPath (e.g. /Movies/Action), folderName

On enter:
  → API-5 GET /api/v1/files
    Query: ?path=<folderPath>&page=1&per_page=50&dir_only=0
    Headers: Authorization: Bearer <jwt>, X-App-Key: <key>
    Response 200: {
      data: [
        { linkcode, name, type: "file"|"folder", size, path, is_public, last_updated_at }, ...
      ],
      meta: { page, per_page, total },
      request_id
    }

Render khi response về: grid card.
Khi user scroll xuống cuối:
  - Nếu data.length < meta.total: page++, append
  - Nếu loaded >= total: stop

Pre-cache: lưu Room `file_cache` với key = folderPath + page, TTL 5 phút.
```

Sort: folder lên trước, file sau (sort `type` desc — folder<file alphabetically). Trong nhóm: theo `name` asc (default; có thể đổi qua filter chip V2).

Lưu ý: API gateway dùng `path` (string) chứ KHÔNG còn `linkcode` cho folder browsing. Engineering đổi data model `FileItem.folderPath: String` thay vì `folderCode`. Khi navigate vào subfolder: append child name vào path: `"/Movies"` + `"/Action"` = `"/Movies/Action"`.

### 8.0.5 Behaviors + D-pad

| Element | OK | Behavior |
|---------|-----|---------|
| FolderCard | OK = S3 với folderCode mới | — |
| FileCard | OK = S4 | — |
| TopBar back arrow | OK = back | — |
| Breadcrumb segment | (non-interactive trong V1) | — |

D-pad: 4 cột grid; Up/Down/Left/Right giữa cards. Up từ row 1 = lên TopBar back arrow.

Edge:
- Folder > 2000 item: hiển thị 2000 đầu + banner "Hiển thị 2000 file đầu".
- File bị deleted ở backend giữa lúc browse → vào S4 sẽ lỗi.

### 8.0.6 Tokens

| Element | Token |
|---------|-------|
| Grid columns | 4 |
| Card size | 320 × 220 dp |
| Card gap | `space/4` (16 dp) |
| Grid padding | `space/7` (48 dp) horizontal |
| Breadcrumb typography | `body-md` |

### 8.0.7 Icons

`ic_back`, `ic_folder`, `ic_file`, `ic_video`, `ic_audio`, `ic_image`, `ic_archive` (depending on file type).

### 8.0.8 Localization keys

`browse_loading`, `browse_empty_title`, `browse_empty_body`, `browse_error_title`, `browse_error_action_retry`, `browse_pagination_loading`, `browse_warning_2000_limit`.

### 8.0.9 ViewModel

```kotlin
@HiltViewModel
class BrowseViewModel @Inject constructor(
    private val handle: SavedStateHandle,
    private val listFiles: ListFilesPagedUseCase,
) : ViewModel() {
    private val args = handle.toRoute<Routes.Browse>()
    val pagingFlow: Flow<PagingData<FileItem>> = listFiles(args.folderCode)
        .cachedIn(viewModelScope)
}
```

Dùng Paging 3 với `RemoteMediator` để mix Room cache + network.

### 8.0.10 Acceptance

- [ ] Folder lớn (1000+ item) browse mượt 60fps.
- [ ] Pagination tự load khi scroll cuối.
- [ ] Cache 5 phút: vào lại folder vừa xem → instant.
- [ ] Mất mạng giữa pagination: hiển thị "Lỗi mạng" cuối list + retry.

---

## 9. S4 — File Detail

### 9.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Hiển thị metadata file + nút "Phát ngay" / "Yêu thích" |
| Entry | S2 (file card), S3 (file card) |
| Exit | "Phát ngay" → S5; "Yêu thích" toggle (V2 có thể có); Back → screen trước |

### 9.0.2 States

- `Loading`: skeleton metadata.
- `Loaded`: hiển thị đầy đủ.
- `Error`: lỗi load.
- `FileProtected`: cần password (hiển thị dialog nhập password trước khi play).
- `FileNotPlayable`: file là image/zip — disable nút Phát.

### 9.0.3 Components

- Thumbnail lớn 720 × 405 dp (cột trái)
- Metadata cột phải: title, chips (size/duration/format), description
- `Button` (C2) primary "Phát ngay"
- `IconButton` (C3) "Yêu thích" (toggle visual, V1 không persist server-side)
- Caption Audio/Subtitle list

### 9.0.4 Data flow + APIs

```
Args: linkcode (12 chars uppercase A-Z 0-9 — pattern ^[A-Z0-9]{12}$)

On enter:
  → API-6: GET /api/v1/files/{linkcode}
    Headers: Authorization: Bearer <jwt>, X-App-Key: <key>
    Response 200: { data: file_object, request_id }
  
  Optional → API-7: GET /api/v1/files/{linkcode}/metadata (cho extended info: codec, duration)
  
  → Render metadata
  → Nếu file marked password-protected (field từ response) → state = FileProtected, show dialog nhập password
  → Determine "playable":
      Extension/MIME type startsWith video/audio
      → playable = true
    Khác → playable = false (image/zip)
```

Không gọi API-10 (Stream URL) ở S4 — chỉ gọi khi user nhấn Phát (chuyển sang S5).

Error specific:
- `RESOURCE_NOT_FOUND` (404): file đã bị xoá → toast + auto back về screen trước.
- `RESOURCE_FORBIDDEN` (403): user không có quyền → "Không có quyền xem file này".

### 9.0.5 Behaviors

| Element | OK | Behavior |
|---------|-----|---------|
| Phát ngay | OK = navigate S5 | default focus |
| Yêu thích | toggle, persist Room (V1) | — |
| Back | screen trước, focus restore | — |

Edge:
- File hỏng (size 0): button Phát disable, tooltip "File không xem được".
- File yêu cầu password: dialog nhập password trước khi navigate S5; password lưu trong Player navigation arg.

### 9.0.6 Tokens

| Element | Token |
|---------|-------|
| Layout | 2 column 40/60 split |
| Padding | `space/7` (48 dp) ngoài |
| Title typography | `display-md` (48sp/56) |
| Meta caption | `body-md` (18sp/26) |
| Button height | 56 dp |
| Button gap | `space/3` (12 dp) |

### 9.0.7 Icons

`ic_play_circle` (button play), `ic_favorite` / `ic_favorite_filled`, `ic_download` (V2), `ic_back`.

### 9.0.8 Localization keys

`detail_loading`, `detail_meta_size`, `detail_meta_duration`, `detail_meta_format`, `detail_button_play`, `detail_button_favorite`, `detail_audio_tracks`, `detail_subtitle_tracks`, `detail_password_required_title`, `detail_password_required_body`, `detail_password_field`, `detail_error_file_corrupted`, `detail_error_not_playable`.

### 9.0.9 ViewModel

```kotlin
@HiltViewModel
class FileDetailViewModel @Inject constructor(
    handle: SavedStateHandle,
    private val getFileInfo: GetFileInfoUseCase,
) : ViewModel() {
    private val args = handle.toRoute<Routes.FileDetail>()
    private val _state = MutableStateFlow<FileDetailUiState>(FileDetailUiState.Loading)
    val state = _state.asStateFlow()

    init { viewModelScope.launch { load() } }

    private suspend fun load() {
        when (val r = getFileInfo(args.fileCode)) {
            is ApiResult.Success -> _state.value = FileDetailUiState.Loaded(r.data)
            is ApiResult.Error -> _state.value = FileDetailUiState.Error(r.msg)
            ApiResult.NetworkError -> _state.value = FileDetailUiState.Error("Network")
            ApiResult.AuthExpired -> _events.send(NavigateToLogin)
        }
    }
}
```

### 9.0.10 Acceptance

- [ ] File metadata load < 1s.
- [ ] Button Phát default focus.
- [ ] File password: dialog nhập đúng → play; sai → error message.
- [ ] File không play được: nút Phát disable.

---

## 10. S5 — Player Full-screen

### 10.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Phát video toàn màn hình |
| Entry | S4 "Phát ngay"; S2 Continue Watching (với resumePos) |
| Exit | Back → S4 / S2 (confirm); Ended → S4 |
| Engine | Media3 ExoPlayer 1.4+; libVLC fallback (V2) |

### 10.0.2 States

- `LoadingStream`: đang gọi API-9 lấy URL.
- `Buffering`: ExoPlayer buffering ban đầu.
- `Playing`: đang phát.
- `Paused`: paused.
- `Buffering-mid`: buffer giữa chừng.
- `Ended`: stream kết thúc.
- `Error`: lỗi (chuyển S5c).

### 10.0.3 Components

- AndroidView wrap `PlayerView` (Media3) với `useController = false`
- `PlayerOverlay` (C23) Compose phủ trên (xem S5a)
- Top notch transient hiển thị title file

### 10.0.4 Data flow + APIs

⚠️ **Endpoint API-10 chưa định nghĩa trong API gateway** (xem §3.3). Spec dưới đây dựa trên giả định backend sẽ ship trong `v1/session.md`. Engineering tạo interface mock trong Phase 1, swap implementation khi backend ready.

```
Args: linkcode, resumePos (default 0)

On enter:
  → API-10 (giả định): POST /api/v1/sessions/download   ← chưa chốt path
    Headers: Authorization: Bearer <jwt>, X-App-Key: <key>
    Body: { linkcode, password?: "..." }   ← chưa chốt shape
    Response (giả định): {
      data: {
        url: "https://download7.fshare.vn/...",
        expires_in: 21600,             ← TTL seconds (giả định 6h)
        expires_at: "2026-05-04T16:00:00+07:00"
      },
      request_id
    }
  → ExoPlayer setup:
      - DefaultHttpDataSource.Factory với:
          User-Agent: Fshare/androidtv/<ver>
          Authorization: Bearer <jwt>   ← Open API gateway uses Bearer, không còn cookie
      - MediaItem.fromUri(stream_url)
      - Nếu resumePos > 0:
          → Check Room PlaybackPosition WHERE linkcode = args.linkcode
          → Nếu có position > 30s và < duration−60s:
              → state = ResumePromptShow (S5d)
              → wait user response
          → User OK "Tiếp tục": exoPlayer.seekTo(savedPos)
          → User OK "Bắt đầu lại": exoPlayer.seekTo(0)
      - exoPlayer.prepare()
      - exoPlayer.playWhenReady = true
  → Lưu (linkcode, expires_at) vào memory để biết khi nào URL hết hạn
    → Khi `now > expires_at - 5min`: pre-emptive refresh API-10 lấy URL mới

Trong khi playing:
  Tick mỗi 5s:
    Room.upsert PlaybackPosition(linkcode, currentPos, duration, now)
  
  Listener ExoPlayer:
    - onPlayerError(PlaybackException):
        TYPE_RENDERER → state = Error(codec) → S5c
        TYPE_SOURCE HTTP 403 → URL hết hạn → re-fetch API-10, setMediaItem(newUrl) + seekTo(currentPos)
        TYPE_SOURCE HTTP 401 → token expired → Authenticator tự refresh → re-fetch API-10
        TYPE_NETWORK → retry 3 lần với exponential backoff (1s, 2s, 4s)
    - onPlaybackStateChanged(STATE_ENDED):
        → state = Ended
        → Room.upsert position = duration (đánh dấu xem hết)
        → navigate back S4 sau 2s hoặc nếu user OK
```

**Action item cho backend (block Phase 3)**: confirm + ship `POST /api/v1/sessions/download`:
- Input shape: `{linkcode: string, password?: string}` (HTTP body)
- Output shape: `{data: {url, expires_in, expires_at}, request_id}`
- TTL khuyến nghị: ≥ 6h để cover phim dài + tránh refresh giữa chừng
- Error: `RESOURCE_FORBIDDEN` nếu file private; `VALIDATION_FAILED` nếu thiếu password

### 10.0.5 Behaviors + D-pad (override)

| Phím | Hành vi |
|------|---------|
| OK / Center / Play-Pause | toggle play/pause; hiện overlay 3s |
| Left | tua -10s; long-press = -30s; hiện overlay 3s |
| Right | tua +10s; long-press = +30s; hiện overlay 3s |
| Up | hiện overlay (S5a) — focus trên track buttons |
| Down | hiện overlay — focus trên timeline thumb |
| Back | confirm "Tắt phim?" (S5d-style) — default "Không" |
| Number 0–9 | nhảy đến 0%, 10%, 20%... (nice-to-have, P2) |

Auto-hide overlay sau 5s không tương tác.

### 10.0.6 Tokens

| Element | Token |
|---------|-------|
| Background | `#000` (đen tuyền cho video) |
| Loading text | `body-lg` trên video |
| Buffer spinner color | `color/accent/primary` |

### 10.0.7 Icons (player overlay xem S5a)

(không icon trong base S5, chỉ video)

### 10.0.8 Localization keys

`player_loading`, `player_buffering`, `player_error_codec`, `player_error_network`, `player_back_confirm_title`, `player_back_confirm_body`.

### 10.0.9 ViewModel

```kotlin
@HiltViewModel
class PlayerViewModel @Inject constructor(
    handle: SavedStateHandle,
    private val getStreamUrl: GetStreamUrlUseCase,
    private val playbackRepo: PlaybackPositionRepository,
) : ViewModel() {
    private val args = handle.toRoute<Routes.Player>()
    private val _state = MutableStateFlow<PlayerState>(PlayerState.LoadingStream)
    val state = _state.asStateFlow()
    
    val resumePosFlow: Flow<Long> = playbackRepo.observe(args.fileCode)
        .map { it?.positionMs ?: 0L }

    suspend fun fetchStreamUrl(password: String? = null): Result<String> =
        getStreamUrl(args.fileCode, password)

    fun savePosition(positionMs: Long, durationMs: Long) = viewModelScope.launch {
        playbackRepo.upsert(PlaybackPosition(args.fileCode, positionMs, durationMs, System.currentTimeMillis()))
    }
}
```

### 10.0.10 Acceptance

- [ ] Time-to-first-frame < 3s sau API-9 trả URL.
- [ ] Tua ±10s mượt, không lag > 500ms.
- [ ] Resume từ Room đúng vị trí ±2s.
- [ ] URL TTL expire (403): tự refresh + seek về vị trí cũ.
- [ ] Back có confirm trước khi exit.

---

## 11. S5a — Player Overlay (controls)

### 11.0.1 Tổng quan

Compose UI phủ trên video, hiển thị on-demand 5s rồi ẩn.

### 11.0.2 Components

- Top zone: nút back (`IconButton` C3 + ic_back) trái + title file phải. Gradient `#000` 70% → transparent.
- Center zone: trống (giữ video clean) — overlay icon play/pause to tạm 0.5s khi user toggle (visual feedback).
- Bottom zone:
  - Time elapsed | progress bar (8 dp height, thumb 16 dp) | time remaining
  - Hàng buttons: [⏮ Prev (nếu có queue, V1 không có)] [⏯ Play/Pause] [⏭ Next] | spacer | [Phụ đề] [Audio] [⚙ More]

### 11.0.3 Behaviors + D-pad

| Focus on | OK | Up | Down | Left | Right |
|----------|-----|-----|------|------|-------|
| Play/Pause | toggle | up cluster trên (back button) | thumb timeline | — | nút Phụ đề |
| Phụ đề | mở S5b | up | thumb | Play/Pause | Audio |
| Audio | mở S5b | up | thumb | Phụ đề | More |
| Thumb timeline | pause/play | hàng nút | — | tua -10s | tua +10s |
| Back arrow | exit (confirm) | — | hàng nút | — | title (non-interactive) |

Default focus khi overlay hiện: Play/Pause.

### 11.0.4 Tokens

| Element | Token |
|---------|-------|
| Top gradient | linear `#000 alpha 0.7` → `transparent` |
| Bottom gradient | `transparent` → `#000 alpha 0.7` |
| Time mono | typography `caption-mono` (16sp/22), `tnum` enabled |
| Button size | 48 × 48 dp (cluster), 64 dp focus target |
| Progress thumb size | 16 dp default, 20 dp focus |
| Progress track height | 4 dp default, 8 dp focus on thumb |
| Overlay enter | `motion/quick` 100 ms fade-in |
| Overlay exit | `motion/standard` 200 ms fade-out |

### 11.0.5 Icons

`ic_back`, `ic_play`, `ic_pause`, `ic_skip_prev`, `ic_skip_next`, `ic_subtitles`, `ic_audio_track`, `ic_settings`.

### 11.0.6 Localization keys

`player_overlay_play`, `player_overlay_pause`, `player_overlay_subtitle`, `player_overlay_audio`, `player_overlay_more`, `player_overlay_back`.

### 11.0.7 Acceptance

- [ ] Overlay xuất hiện trong 100ms khi user nhấn phím bất kỳ.
- [ ] Auto-hide đúng sau 5s không tương tác.
- [ ] Tua ±10s từ thumb mượt, không jitter.
- [ ] Time format `HH:MM:SS` đúng cho video > 1h, `MM:SS` cho video < 1h.

---

## 12. S5b — Track Selection (BottomSheet)

### 12.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Chọn audio track hoặc subtitle track |
| Entry | S5a → click "Phụ đề" hoặc "Audio" |
| Exit | OK = select + close; Back = close không thay đổi |

**Status**: designer cut trong README v1.0 nhưng V1 NEN GIỮ — subtitle chọn được là essential UX cho phim có nhiều audio/sub track.

### 12.0.2 Components

- `BottomSheet` (C9) — chiều cao 40% screen
- `RadioGroup` (C18) item list

### 12.0.3 Data flow

```
On open:
  - tracks = exoPlayer.currentTracks
  - filter type C.TRACK_TYPE_TEXT cho subtitle tab
  - filter type C.TRACK_TYPE_AUDIO cho audio tab
  - render list với selected = current selection

On select:
  - Player.setTrackSelectionParameters(...)
  - close sheet
```

### 12.0.4 Behaviors

| Phím | Hành vi |
|------|---------|
| Up/Down | chuyển focus item |
| OK | select track + close |
| Back | close không apply |

### 12.0.5 Tokens

| Element | Token |
|---------|-------|
| Sheet height | 40% screen |
| Sheet bg | `color/bg/surface` |
| Item height | 56 dp |
| Item padding | `space/4` 16 dp |
| Title | `title-md` |
| Sheet enter | y +100% → 0, fade 0→1, `motion/standard` 200 ms |

### 12.0.6 Icons

`ic_subtitles`, `ic_audio_track`, `ic_check` (selected indicator).

### 12.0.7 Localization keys

`track_select_subtitle_title`, `track_select_audio_title`, `track_select_subtitle_off`, `track_format_template` (e.g., `{language} ({codec})`).

### 12.0.8 Acceptance

- [ ] File MKV với 2 sub track + 2 audio track: hiển thị đủ 2+2 + "Tắt phụ đề".
- [ ] Select sub track → subtitle thay đổi ngay.
- [ ] Back không apply → giữ track cũ.

---

## 13. S5c — Player Error

### 13.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Khi không play được (codec, mạng, URL hết hạn không refresh được) |
| Entry | S5 emit Error |
| Exit | Retry → S5; Đổi engine → S5 với libVLC; Back → S4 |

### 13.0.2 Components

- Overlay tối 80% phủ video
- Icon error 96 dp center
- Headline `headline` (36 sp/44)
- Body caption ngắn lỗi đã localize
- 3 nút dọc

### 13.0.3 Behaviors

| Element | OK | 
|---------|-----|
| Thử lại (default) | re-init S5 |
| Đổi engine VLC | re-init với libVLC backend (V2) |
| Quay lại | navigate S4 |
| Gửi log lỗi (link nhỏ) | upload log file |

### 13.0.4 Tokens

| Element | Token |
|---------|-------|
| Overlay alpha | `#000` 80% |
| Icon size | 96 dp |
| Icon color | `color/state/danger` |
| Headline | `headline` |
| Body | `body-lg` |
| Button gap | `space/3` 12 dp |

### 13.0.5 Icons

`ic_error`.

### 13.0.6 Localization keys

`player_error_title`, `player_error_codec_body`, `player_error_network_body`, `player_error_url_expired_body`, `player_error_action_retry`, `player_error_action_switch_engine`, `player_error_action_back`, `player_error_send_log`.

### 13.0.7 Acceptance

- [ ] Codec không hỗ trợ: hiển thị error rõ + 3 nút.
- [ ] Mạng yếu: error retry tự động 3 lần trước khi hiển thị S5c.
- [ ] Send log: tạo bundle log + upload thành công thì toast "Đã gửi log #ID".

---

## 14. S5d — Resume Prompt

### 14.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Hỏi user có muốn resume từ vị trí dừng |
| Entry | S5 trước khi play, có saved position 30s < pos < duration−60s |
| Exit | "Tiếp tục" → seek + play; "Bắt đầu lại" → seek 0 + play |

### 14.0.2 Components

- `Dialog` (C8) medium

### 14.0.3 Behaviors

Default focus: "Tiếp tục" (primary).

### 14.0.4 Tokens

Standard Dialog medium.

### 14.0.5 Localization keys

`resume_prompt_title` (placeholder `{time}`), `resume_prompt_body` (placeholder `{date}`), `resume_prompt_action_resume`, `resume_prompt_action_restart`.

### 14.0.6 Acceptance

- [ ] Pos > 30s và < duration−60s → hiện prompt; ngược lại không hiện.
- [ ] "Tiếp tục" → exoPlayer.seekTo(savedPos) trong 200ms.
- [ ] Pos < 30s: skip prompt, play từ đầu.
- [ ] Pos > duration−60s: skip prompt, play từ đầu (đã coi như xem hết).

---

## 15. S7 — Settings Hub

### 15.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Trung tâm setting, dẫn 4 sub-screen |
| Entry | S2 TopBar Settings icon |
| Exit | sub → S7a/b/d/f; Back → S2 |

### 15.0.2 Components

- 2 column layout: menu trái (~30%), preview phải (~70%)
- `ListItem` (C7) menu items
- Caption preview

### 15.0.3 Sub-screens V1 (4 items)

| Item | Sub-screen | Section dùng |
|------|-----------|--------------|
| Tài khoản | S7a | account info, logout |
| Phát video | S7b | engine, audio, subtitle |
| Mạng | S7d (info-only) | trạng thái mạng |
| Giới thiệu | S7f | version, send log |

**KHÔNG** có "Tải về" (S7c) và "Cập nhật" (S7e) — đã cắt.

### 15.0.4 Behaviors

D-pad: Up/Down menu; Right (hoặc OK) vào sub. Default focus item đầu "Tài khoản".

### 15.0.5 Tokens

| Element | Token |
|---------|-------|
| Menu width | 380 dp |
| Menu item height | 64 dp |
| Item icon size | 32 dp leading |
| Item label | `body-lg` |
| Selected item bg | `color/accent/primary-soft` |
| Selected item text | `color/accent/primary` |
| Preview body | `body-md` muted |

### 15.0.6 Icons

`ic_user`, `ic_play`, `ic_wifi`, `ic_info`, `ic_chevron_right`.

### 15.0.7 Localization keys

`settings_hub_title`, `settings_section_account`, `settings_section_playback`, `settings_section_network`, `settings_section_about`, `settings_preview_account`, `settings_preview_playback`, `settings_preview_network`, `settings_preview_about`.

### 15.0.8 Acceptance

- [ ] Mở Settings instant (< 200ms).
- [ ] D-pad Right vào sub đúng.
- [ ] Preview thay đổi khi focus item khác (không lag).

---

## 16. S7a — Settings Account

### 16.0.1 Tổng quan

Hiển thị account info + logout. **KHÔNG có "Switch user"** trong V1 (chỉ logout rồi login lại).

### 16.0.2 Data flow

```
On enter:
  → Đọc Room user_cache
  → Background: API-4 GET /api/v1/me → update cache
    Response: { data: { id, email, full_name, phone, is_vip, vip_level, vip_expires_at, storage_used, storage_quota, created_at }, request_id }
  
Render:
  - Avatar (fallback initials chữ đầu full_name)
  - full_name (display-md)
  - email (body-md muted)
  - Grid 2x2:
      - VIP status (chip: is_vip ? "VIP" : "Free")
      - VIP expiry date (vip_expires_at, format dd/MM/yyyy)
      - Webspace tổng (progressBar storage_used/storage_quota + "X/Y GB")
      - Traffic ⚠️ (API gateway hiện tại không trả traffic; bỏ ô này hoặc hiển thị "—")
  - Nút "Đăng xuất" (secondary)
```

### 16.0.3 APIs

- API-4 `GET /api/v1/me` (refresh background).
- API-3 `POST /api/v1/auth/logout` (khi logout) — invalidate refresh_token family server-side.

### 16.0.4 Behaviors

| Element | OK |
|---------|-----|
| Đăng xuất | confirm dialog → API-3 → clear AuthStore → navigate S1 |

### 16.0.5 Tokens

| Element | Token |
|---------|-------|
| Avatar size | 120 dp |
| Avatar border | 3 dp `color/accent/primary` |
| Section gap | `space/6` 32 dp |
| Grid gap | `space/4` 16 dp |
| Progress height | 8 dp |

### 16.0.6 Icons

`ic_user`, `ic_logout`, `ic_storage`, `ic_vip_badge`.

### 16.0.7 Localization keys

`account_title`, `account_label_email`, `account_label_vip`, `account_label_vip_expire`, `account_label_webspace`, `account_label_traffic`, `account_button_logout`, `account_logout_confirm_title`, `account_logout_confirm_body`, `account_logout_action_confirm`, `account_logout_action_cancel`, `account_vip_active`, `account_vip_inactive`, `account_traffic_used` (placeholder `{used}`, `{total}`).

### 16.0.8 Acceptance

- [ ] Logout → clear session + navigate S1 < 1s.
- [ ] Confirm dialog default focus "Hủy" (an toàn).
- [ ] Network fail khi load: vẫn hiển thị data cache + caption "Đang offline".

---

## 17. S7b — Settings Playback

### 17.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Cấu hình player |

### 17.0.2 Items

| Item | Type | Default | Persist |
|------|------|---------|---------|
| Engine phát | RadioGroup ExoPlayer / libVLC | ExoPlayer | DataStore |
| Audio passthrough HDMI | Switch | OFF | DataStore |
| Tự động phát file kế tiếp | Switch | OFF (V1; V2 có thể ON nếu queue có) | DataStore |
| Lưu vị trí xem dở | Switch | ON | DataStore |
| Cỡ phụ đề | RadioGroup Nhỏ/Vừa/Lớn/Rất lớn | Vừa | DataStore |
| Màu phụ đề | RadioGroup Trắng/Vàng/Cam | Trắng | DataStore |
| Viền phụ đề | Switch | ON | DataStore |

### 17.0.3 Components

`ListItem` (C7) variants: with-radio, with-switch.

### 17.0.4 Tokens

Standard ListItem.

### 17.0.5 Icons

(không icon, chỉ chevron right cho RadioGroup expand)

### 17.0.6 Localization keys

`playback_title`, `playback_engine_label`, `playback_engine_exo`, `playback_engine_vlc`, `playback_audio_passthrough`, `playback_auto_next`, `playback_save_position`, `playback_subtitle_size`, `playback_subtitle_size_small/medium/large/xlarge`, `playback_subtitle_color`, `playback_subtitle_color_white/yellow/orange`, `playback_subtitle_outline`.

### 17.0.7 Acceptance

- [ ] Đổi setting → apply ngay khi vào player tiếp theo.
- [ ] Cỡ phụ đề preview ngay trong Settings (small section "Preview" với placeholder text).

---

## 18. S7d — Settings Network (info-only)

### 18.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Hiển thị trạng thái mạng + link mở Settings hệ thống |

### 18.0.2 Items

- Section info: "Wi-Fi: {connected/disconnected}" — `{SSID}` — IP `{ip}` — Link speed `{Mbps}` (đọc từ WifiManager)
- Nút "Mở Cài đặt mạng hệ thống" → Intent `Settings.ACTION_WIRELESS_SETTINGS`

~~Speed test~~ — **CẮT** theo quyết định user 2026-05-05 (không cần speedtest endpoint riêng).

### 18.0.3 Localization keys

`network_title`, `network_status_wifi`, `network_status_ssid`, `network_status_ip`, `network_status_link_speed`, `network_action_open_system_settings`.

### 18.0.4 Acceptance

- [ ] Hiển thị đúng trạng thái mạng hiện tại (WifiManager.connectionInfo).
- [ ] Nút mở system settings hoạt động (Intent ACTION_WIRELESS_SETTINGS).

---

## 19. S7f — Settings About + Send Log

### 19.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Version info + cách cập nhật + gửi log lỗi |

### 19.0.2 Items

- Logo + tên app
- Version "StreamIX TV {versionName} ({versionCode})" (font mono)
- Section "Cách cập nhật" → button "Xem hướng dẫn" → mở dialog với QR + URL `tv.streamix.vn`
- Device info (read-only):
  - Model: `Build.MODEL`
  - Android: `Build.VERSION.RELEASE` (API `Build.VERSION.SDK_INT`)
  - Free space: `Environment.getDataDirectory().freeSpace`
  - App ID hash (last 8 chars)
- Caption "Liên hệ: tv-support@fshare.vn / Slack #streamix-tv-support"

~~Nút "Gửi nhật ký lỗi"~~ — **CẮT** theo quyết định user 2026-05-05 (endpoint /log/feedback không cần V1). Log file vẫn ghi local qua Timber FileTree (xem §26) để dev có thể `adb pull` debug khi cần.

### 19.0.3 APIs

(không gọi API riêng — chỉ đọc system info)

### 19.0.4 Localization keys

`about_title`, `about_version_label`, `about_section_update`, `about_update_show_qr`, `about_update_qr_body`, `about_section_device`, `about_label_model`, `about_label_android`, `about_label_freespace`, `about_label_appid`, `about_contact_caption`.

### 19.0.5 Acceptance

- [ ] Version hiển thị đúng.
- [ ] "Xem hướng dẫn" mở dialog với QR scan-able dẫn về `tv.streamix.vn`.
- [ ] Device info chính xác.

---

## 20. S9 — Onboarding

### 20.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Hướng dẫn user mới sau login đầu tiên |
| Entry | Sau login lần đầu (DataStore `onboarding_completed = false`) |
| Exit | "Bắt đầu" → S2 + set flag true; "Bỏ qua" → cùng |

### 20.0.2 Steps V1 (rút từ 3 → 2 steps)

1. **Điều khiển bằng remote** — icon `ic_dpad`, body "Dùng phím lên xuống trái phải để chọn, OK để xác nhận, Back để quay lại."
2. **Tiếp tục xem dở** — icon `ic_replay`, body "Mỗi video bạn xem được lưu vị trí — quay lại bất cứ lúc nào."

(Cắt step 3 "Cập nhật tự động" — không còn áp dụng với Phương án A)

### 20.0.3 Components

- Carousel ngang 2 step
- Vector icon lớn 96 dp
- Headline + body 1 dòng
- Page indicator 2 chấm
- Nút "Tiếp" (step 1) / "Bắt đầu" (step 2)
- Nút "Bỏ qua" góc dưới phải

### 20.0.4 Behaviors

D-pad Left/Right: chuyển step. OK trên Tiếp/Bắt đầu: tiến lên / hoàn tất.

### 20.0.5 Tokens

Standard.

### 20.0.6 Icons

`ic_dpad`, `ic_replay`.

### 20.0.7 Localization keys

`onboarding_step1_title`, `onboarding_step1_body`, `onboarding_step2_title`, `onboarding_step2_body`, `onboarding_action_next`, `onboarding_action_start`, `onboarding_action_skip`.

### 20.0.8 Acceptance

- [ ] Chỉ chạy đúng 1 lần / lifetime app.
- [ ] "Bỏ qua" cũng set flag completed.
- [ ] Set flag persistent across uninstall? KHÔNG (DataStore xoá khi uninstall).

---

## 21. S10 — Confirm Dialog (template component)

### 21.0.1 Tổng quan

Component dùng chung cho mọi confirm. Variant:

| Variant | Use case | Default focus | Style nút phải |
|---------|----------|---------------|---------------|
| Standard | Thoát app, thoát phim | "Không" | primary |
| Destructive | Đăng xuất, xoá local data | "Hủy" | danger (`color/state/danger` bg) |
| Loading | Đang xử lý lâu | (no focus, button loading) | — |

### 21.0.2 API

```kotlin
@Composable
fun ConfirmDialog(
    title: String,
    body: String? = null,
    confirmLabel: String,
    cancelLabel: String,
    variant: DialogVariant = DialogVariant.Standard,
    isLoading: Boolean = false,
    onConfirm: () -> Unit,
    onCancel: () -> Unit,
)
```

### 21.0.3 Tokens

| Element | Token |
|---------|-------|
| Dialog max-width | 600 dp |
| Padding | `space/6` 32 dp |
| Title | `title-lg` |
| Body | `body-lg` |
| Button gap | `space/3` 12 dp |
| Backdrop | `color/bg/scrim` 72% |
| Enter | scale 0.95→1.0, fade 0→1, `motion/standard` 200ms |

### 21.0.4 Acceptance

- [ ] Default focus đúng theo variant.
- [ ] Back = cancel (trừ Loading variant — Back disabled).
- [ ] Destructive variant nút phải có visual đỏ rõ.

---

## 22. S11 — Global Error / No Network

### 22.0.1 Tổng quan

Hiển thị khi:
- Mất mạng giữa chừng.
- Server không phản hồi quá 30s.
- Init splash > 15s.

### 22.0.2 Components

Full-screen overlay `color/bg/base` 100%.

- Icon lớn 128 dp (wifi-off / cloud-off)
- Headline `headline`
- Body `body-lg`
- 2 nút: "Cài đặt mạng" (mở Intent system) + "Thử lại"

### 22.0.3 Behaviors

- Auto-retry mỗi 10s (silent) — nếu reconnect thì auto-dismiss.
- Manual "Thử lại" → trigger reload.

### 22.0.4 Localization keys

`error_no_network_title`, `error_no_network_body`, `error_no_network_action_settings`, `error_no_network_action_retry`, `error_server_title`, `error_server_body`.

### 22.0.5 Icons

`ic_wifi_off`, `ic_error`.

### 22.0.6 Acceptance

- [ ] Mất mạng > 5s: overlay xuất hiện.
- [ ] Reconnect: auto-dismiss < 5s sau khi back online.
- [ ] Manual retry: trigger reload từ context cha.

---

## 23. S12 — Search

### 23.0.1 Tổng quan

| Mục | Giá trị |
|-----|---------|
| Mục đích | Tìm file theo tên (designer thêm vào V1) |
| Entry | S2 TopBar Search icon |
| Exit | Card → S4 hoặc S3; Back → S2 |

### 23.0.2 States

- `Idle`: chưa nhập keyword, hiển thị recent searches + suggestions (V2).
- `Searching`: skeleton.
- `Results`: list kết quả.
- `Empty`: keyword > 2 ký tự, kết quả rỗng.
- `Error`.

### 23.0.3 Components

- TopBar variant: TextField search ở giữa, back button trái
- KeyboardOnScreen (C21) full QWERTY VN/EN
- Result list grid 4 cột (giống S3)
- `Chip` (C19) recent searches

### 23.0.4 Data flow + APIs

```
User gõ keyword (debounce 500ms khi keyword.length >= 2):
  → API-8: GET /api/v1/files/search
    Query: ?q=<keyword>&page=1&per_page=50
    Headers: Authorization: Bearer <jwt>, X-App-Key: <key>
    Optional: &path=/Movies (filter trong subfolder), &ext=mp4 (filter extension)
    Response 200: {
      data: [<file objects>],
      meta: { page, per_page, total },
      request_id
    }
  → Render results
  → Lưu vào Room recent_searches (max 10 entry)

Pagination: scroll cuối → page++ với cùng query
```

Constraint: q `minLength: 1, maxLength: 255` per openapi.yaml.

Error specific:
- `VALIDATION_FAILED` (422) khi keyword > 255 chars hoặc invalid → trim hoặc hiển thị "Từ khoá quá dài"

### 23.0.5 Behaviors

- TextField default focus.
- Recent search chip → fill keyword + auto search.
- Result card → S4.

### 23.0.6 Tokens

| Element | Token |
|---------|-------|
| TextField height | 64 dp |
| TextField max-width | 800 dp center |
| Recent chip gap | `space/2` 8 dp |
| Grid result | giống S3 |

### 23.0.7 Icons

`ic_search`, `ic_back`, `ic_close` (clear).

### 23.0.8 Localization keys

`search_field_hint`, `search_recent_label`, `search_clear`, `search_no_results_title`, `search_no_results_body`, `search_min_chars` (placeholder `{n}`).

### 23.0.9 ViewModel

```kotlin
@HiltViewModel
class SearchViewModel @Inject constructor(
    private val searchUseCase: SearchUseCase,
    private val recentRepo: RecentSearchRepository,
) : ViewModel() {
    private val query = MutableStateFlow("")
    val recentSearches: StateFlow<List<String>> = recentRepo.observe()
    val results: Flow<PagingData<FileItem>> = query
        .debounce(500)
        .filter { it.length >= 2 }
        .flatMapLatest { searchUseCase(it) }
        .cachedIn(viewModelScope)

    fun setQuery(s: String) { query.value = s }
    fun clearQuery() { query.value = "" }
}
```

### 23.0.10 Acceptance

- [ ] Gõ ≥ 2 ký tự → search trong 500ms debounce.
- [ ] Kết quả load mượt với Paging.
- [ ] Recent searches lưu, max 10 mục.

---

# Phần C — Cross-cutting concerns

## 24. Auth & token management

```kotlin
@Singleton
class AuthStore @Inject constructor(
    @ApplicationContext private val context: Context,
) {
    private val prefs = EncryptedSharedPreferences.create(
        context, "streamix_auth",
        MasterKey.Builder(context).setKeyScheme(AES256_GCM).build(),
        AES256_SIV, AES256_GCM
    )

    suspend fun save(session: Session, ttl: Duration) = withContext(Dispatchers.IO) {
        prefs.edit()
            .putString("token", session.token)
            .putString("php", session.phpSessionId)
            .putString("email", session.email)
            .putLong("expire", System.currentTimeMillis() + ttl.inWholeMilliseconds)
            .apply()
    }

    suspend fun session(): Session? = withContext(Dispatchers.IO) {
        if (System.currentTimeMillis() > prefs.getLong("expire", 0)) {
            clear(); return@withContext null
        }
        val token = prefs.getString("token", null) ?: return@withContext null
        Session(token, prefs.getString("php", "")!!, prefs.getString("email", "")!!)
    }

    suspend fun clear() = withContext(Dispatchers.IO) {
        prefs.edit().clear().apply()
    }
}
```

## 25. Error handling chuẩn

```kotlin
suspend fun <T> Call<T>.toApiResult(): ApiResult<T> = try {
    val response = awaitResponse()
    when {
        response.isSuccessful -> ApiResult.Success(response.body()!!)
        response.code() in listOf(401, 498) -> ApiResult.AuthExpired
        response.code() in listOf(0, 502, 503, 504) -> ApiResult.NetworkError
        else -> ApiResult.Error(response.code(), response.errorBody()?.string() ?: "Unknown")
    }
} catch (e: IOException) {
    ApiResult.NetworkError
} catch (e: HttpException) {
    ApiResult.Error(e.code(), e.message())
}
```

## 26. Logging

```kotlin
@HiltAndroidApp
class StreamIXApp : Application() {
    override fun onCreate() {
        super.onCreate()
        if (BuildConfig.DEBUG) Timber.plant(Timber.DebugTree())
        else Timber.plant(FileTree(filesDir.resolve("log"), maxSize = 5L * 1024 * 1024))
    }
}
```

`FileTree` rotate log file mỗi ngày, giữ 7 ngày gần nhất.

## 27. Performance budgets

| Metric | Budget |
|--------|--------|
| Cold start → S2 | ≤ 2.5s Mi Box S |
| Frame budget | 60 fps, < 5% drop |
| API call | retry 3 lần exp backoff (1s, 2s, 4s) trước khi error |
| Image load | Coil 2.7, mem cache 50 MB, disk 100 MB |
| Memory peak | ≤ 250 MB |

---

# Phần D — Implementation order

## 28. Thứ tự build (theo phase trong `07`)

### Phase 0 — Setup (1 tuần)

1. Repo `streamix-tv/` package `vn.streamix.tv`.
2. Gradle Convention plugin với multi-module.
3. Hello APK chạy được Mi Box S + FPT Box.
4. CI Github Actions build debug.

### Phase 1 — Core (2 tuần)

5. `:core:domain` — Models 1-1 từ desktop.
6. `:core:network` — Retrofit + AuthInterceptor + FshareApiService với 10 API V1.
7. `:core:data` — Room schema + DataStore + Repositories.
8. `:core:common` — port SpeedMeter, FileNameSanitizer, BudgetManager.
9. `:core:ui` — Theme/tokens v1.0 + Components shell (15 components).
10. Convert 45 icon SVG → VectorDrawable.
11. Hilt modules.
12. Instrumented test login + getUserInfo + listFiles.

### Phase 2 — Auth + Browse + Home (2.5 tuần)

13. `:feature:auth` — S1 Login (chỉ email/password).
14. `:feature:home` — S2 Home với 3 row.
15. `:feature:browse` — S3, S4.
16. S0 Splash + routing.
17. S9 Onboarding (2 step).
18. Internal milestone: end-to-end demo từ Splash → Login → Home → Browse → File Detail.

### Phase 3 — Player + Search (3 tuần)

19. `:feature:player` — S5, S5a, S5b, S5c, S5d.
20. ExoPlayer integration với DefaultHttpDataSource cookie.
21. PlaybackPosition Room.
22. `:feature:search` — S12.
23. Internal milestone: end-to-end demo phát video.

### Phase 4 — Settings + Distribute (1 tuần)

24. `:feature:settings` — S7, S7a, S7b, S7d, S7f.
25. CI/CD release pipeline theo `04` rev 3.
26. nginx portal `tv.streamix.vn` với `install.html`.
27. Internal milestone: release v0.9 internal cho team test.

### Phase 5 — Hardening + Beta (3 tuần)

28. Test ma trận 5 thiết bị (Mi Box S, FPT Box, TCL, Sony, Chromecast).
29. Test 20 file mẫu codec.
30. Crashlytics integration.
31. Beta channel 50 user pilot.
32. Bug fix.

### Phase 6 — V1 Public (1 tuần)

33. Slack `#streamix-tv-releases` setup.
34. Email blast template chuẩn bị.
35. Release v1.0.
36. Monitor 30 ngày.

---

# Phần E — Open items

## 29. Open items — trạng thái sau quyết định 2026-05-05

| # | Open item | Trạng thái | Resolution |
|---|-----------|-----------|------------|
| 1 | Endpoint `/log/feedback` | ✅ **CLOSED** | KHÔNG CẦN V1 — bỏ "Gửi log" khỏi S7f. Log file vẫn ghi local via Timber FileTree, dev có thể `adb pull` để debug |
| 2 | Speed test endpoint | ✅ **CLOSED** | KHÔNG CẦN V1 — bỏ "Kiểm tra tốc độ" khỏi S7d. WifiManager.connectionInfo đủ thông tin link speed |
| 3 | App key cho TV | ✅ **CLOSED** | Dùng cùng desktop key `dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` (D-4 quyết định). Note: API gateway dùng header `X-App-Key`, không còn cookie. Nếu backend yêu cầu prefix platform (`tv_v1_*`), confirm trong Phase 0 spike |
| 4 | API rate limit handling | ✅ **CLOSED** | KHÔNG CẦN logic client phức tạp. Backend trả `429 RATE_LIMIT_EXCEEDED` với `Retry-After` header → ApiResult.RateLimited xử lý global qua Snackbar "Vui lòng thử lại sau" |
| 5 | Audio passthrough test thực tế | ⏳ **OPEN — Phase 5** | Test ma trận trên Mi Box S, Sony Bravia, Onkyo AVR. Tune `AudioCapabilities` per device |
| 6 | Stream URL TTL chính xác | ⚠️ **OPEN — BLOCKER Phase 3** | Backend cần ship `v1/session.md` với endpoint `POST /api/v1/sessions/download`. ETA trước Tuần 7. Engineering mock interface, swap khi ready. Xem §3.3 |
| 7 | `ic_visibility` / `ic_visibility_off` | ✅ **CLOSED** | Đã tự tạo SVG: `D:\Work\FsNext\smartTV\assets\icons\ic_visibility.svg` + `ic_visibility_off.svg`. Style match design system (viewBox 24, currentColor, stroke 2, round caps). 256 + 350 bytes |
| 8 | `ic_notification` | ✅ **CLOSED** | Đã tự tạo SVG: `D:\Work\FsNext\smartTV\assets\icons\ic_notification.svg`. 260 bytes |

### 29.1 Open items mới phát sinh từ migration API gateway

| # | Item | Severity | Owner |
|---|------|----------|-------|
| 9 | App key format prefix platform (`tv_v1_*` hay raw) | P1 | Backend confirm Phase 0 |
| 10 | Stream URL endpoint shape (path + body + response) | P0 — block Phase 3 | Backend define + ship |
| 11 | `/api/v1/me` không trả traffic field | P2 | Bỏ ô traffic ở S7a HOẶC backend extend response |
| 12 | Refresh token TTL 7d cố định (không config) | P2 | Cập nhật S1 UI bỏ "Nhớ tôi 30 ngày" — đã làm |
| 13 | Folder browse dùng `path` string thay `linkcode` | P1 — model change | Cập nhật `FileItem.folderPath`, đã ghi rõ ở S3 §8.0.4 |
| 14 | Pagination scheme `page/per_page` thay `page_index/page_size` | P2 | Cập nhật trong Repository — đã ghi |

---

## 30. Sign-off & next steps

Trước khi engineering bắt đầu code, các stakeholder phải sign-off tài liệu này:

- [ ] Tech Lead Android
- [ ] Product Manager
- [ ] QA Lead
- [ ] Designer (verify mapping tokens/components đúng)
- [ ] Backend (verify endpoints + rate limit)

Sau sign-off:
- Tech Lead tạo Jira/Linear ticket cho mỗi screen với link về §của tài liệu này.
- Engineer pickup ticket → đọc §tương ứng → implement.
- QA viết test cases dựa trên §"Acceptance criteria" của mỗi screen.
- PM track progress per phase.

— Hết Implementation Spec V1.0 —
