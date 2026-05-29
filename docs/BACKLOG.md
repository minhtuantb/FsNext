# BACKLOG — FsNext

> Legend: ✅ done · 🔶 partial · ⬜ open. Cập nhật 2026-05-29.

## Active backlog (đợt review 2026-05-29 — xem `docs/ASSESSMENT.md`)

### P1
- ✅ **Hợp nhất 2 design-system QML** — `Fshare.Components` chuẩn (xem [design-system.md](design-system.md)).
  - ✅ Stage 1 + 2 (2026-05-29): **cả 7 atom** (FsIcon/FsTextField/FsCard/FsButton/FsBadge/FsSwitch/FsProgressBar)
    đã về Fshare (gộp visual Aurora + a11y Fshare, repoint hết, build+runtime verified). FsAurora.Components còn lại
    chỉ shell/HUD/utility.
  - ✅ Cleanup #2 (2026-05-29): chuyển `FsGradientRect` → `Fshare.Components`; atom lib không còn import
    `FsAurora.Components` (phụ thuộc ngược đã cắt — chỉ còn phụ thuộc `FsAurora.Theme`).
  - ✅ Cleanup #1 (2026-05-29): gỡ import `FsAurora.Components as Aurora` thừa ở 16 Pages/Dialogs; chuyển design
    handoff sang `design/handoff/`, xóa rác (uploads) + bản trùng (src/, root html/jsx). `qml/FsAurora/` chỉ còn QML.
- ⬜ **Đóng crash audit**: đối chiếu từng finding trong `docs/CRASH_AUDIT.md` + `docs/FILE_MANAGER_CRASH_AUDIT.md`
  với code hiện tại, đánh dấu Fixed/Won't-fix/Open. Ưu tiên race click-nhanh + `FolderTreeModel` đệ quy không cap depth.

### P2
- ⬜ **Thiết kế lại session/cookie trong `HttpClient`** (đang vá mutex band-aid) khi có dịp refactor.
- ⬜ **Test phần lõi**: `TransferOrchestrator` dispatch dưới tải, `RefreshTokenCoordinator` single-flight + 7-day,
  `FileCacheService` write-through rollback. (Liên quan tech-debt `IFshareApi` bên dưới.)
- ⬜ **i18n**: chạy `lupdate`, audit chuỗi hardcode trong `qml/`, quyết định danh sách ngôn ngữ.

### P3
- ⬜ **SecureStore non-Windows** (Keychain/libsecret) khi port macOS/Linux.
- ⬜ **Quy ước tài liệu sống vs snapshot** để tránh drift tái diễn (xem ASSESSMENT §7).

### Crash-audit residual (không crash — từ đợt close 2026-05-29, xem docs/CRASH_AUDIT.md + FILE_MANAGER_CRASH_AUDIT.md)
> Mọi finding crash nghiêm trọng đã FIXED/mitigated. Đây là các mục robustness/UX/perf còn lại:
- ⬜ **H11** (P2): bọc `context.init()` (main.cpp) trong try-catch → hiện dialog "Init failed" thay vì abort.
- ⬜ **M4** (P2): `SyncService::pauseFolder/resumeFolder` đọc `m_folderToTasks` không lock — thêm `Q_ASSERT` thread hoặc lock.
- ⬜ **M18** (P2): `SyncService::scanFolderInternal` gọi `QFileInfo::isDir()`/`entryInfoList()` blocking trên main thread (freeze nếu network mount) → chuyển sang background.
- ⬜ **M13** (P3): `FileCacheDB::queryFiles` ghép `sortKey` vào ORDER BY — callers hardcoded nên an toàn, thêm whitelist switch để phòng thủ.
- ⬜ **M21** (P3): `SystemTray` — `QMenu` không có parent (rò rỉ nhẹ); tray icon đã an toàn.
- ⬜ **M20** (P3, perf): `BadWordFilter::stripDiacritics` chạy 2 lần — cache normalized form lúc load.
- ⬜ **M3** (P3, minor): `FileSyncWorker::onDispatchReady` đọc `m_userMap` sau khi unlock — userId rỗng nếu cancelAll chen vào (sync fail, không crash).
- ⬜ **H9** (P3): `FolderExpander::crawl` có depth-cap 20 nhưng thiếu cycle-detect (thêm `visited` set như `FolderTreeModel`).
- ⬜ **FM-M4** (P3, UX): context menu ở Medium-card view lệch vị trí (`mapToGlobal` vs `mapToItem`).
- ⬜ **FM-M5** (P2, data): `FileCacheService::m_settingsInFlight` leak nếu settings op fail/timeout không emit signal → write op sau bị "ăn" event, list không refresh. Dùng per-request id hoặc timeout.

### Phòng ngừa hệ thống (từ audit §8)
- ⬜ Quy ước: **mọi lambda async (QtConcurrent::run / SingleShotConnection / QTimer::singleShot) phải capture QPointer cho TẤT CẢ QObject pointer** (đã ghi trong CLAUDE.md gotchas).
- ⬜ Crash reporter (Sentry/Crashpad) — hiện chỉ log local + WER.
- ⬜ CI: clang-tidy (`bugprone-*`, `cert-*`) + script check `Q_INVOKABLE` khớp giữa `.h` và call site `.qml`.

---

## Đã giải quyết trong FsNext (vấn đề của codebase legacy v5.3.0)

> Phần này từng là "Known Issues" của bản cũ — nay đã được FsNext xử lý, giữ lại để đối chiếu lịch sử.

- ✅ **Password storage**: nay mã hóa DPAPI qua `SecureStore` (không còn base64).
- ✅ **Token refresh**: `RefreshTokenCoordinator` silent refresh single-flight.
- ✅ **OAuth secrets**: tách `src/core/api/OAuthSecrets.h` (gitignored), không hardcode.
- ✅ **Global state**: thay bằng DI qua `AppContext` (không còn extern globals).
- ✅ **ActionThread monolith**: thay bằng service layer + `TransferOrchestrator`.
- ✅ **No error recovery**: `AppError` phân loại + retry; session-expired flow có toast.
- ✅ **Disk space check** / **filename conflicts**: đã xử lý (xem `download_test_cases.md`).
- ✅ **Connection pooling / file listing cache**: HttpClient reuse + `FileCacheDB` cache-first.
- 🔶 **Concurrent session / progress persistence**: phát hiện session-expired (HTTP 201); progress persistence một phần
  (resume sidecar cho transfer đang chạy). Còn dư địa hoàn thiện.

---

## API Requests (for Backend team)

### [API REQUEST] 2026-04-15  →  **CLOSED 2026-04-28**
**Module**: Authentication (`/api/user/login`)
**Original issue**: POST /api/user/login returned HTTP 400 Bad Request for all attempts
with app_key `CtnLXisyQaf4mQwfx6aP58ZMQUomck14R7mI7KCe` (2025 vintage).

**Resolution**: After client-side rotation to:
- `app_key = dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` (2026)
- `User-Agent = Fshare_Tool_2026`

login succeeds against the production endpoint.  Maintainer-verified
2026-04-28 against the live test account.  Most plausible explanation: the
backend allowlist had already been updated to require the 2026 key/UA pair,
and the original 2025-04-15 reproduction was against the stale tuple.

Dev bypass that was added as a workaround **has been removed** from
`AuthService.cpp`.

Emergency runbook for future recurrence kept at `docs/runbook_login_400.md`.

---

## Feature Requests (for future consideration)

### 2026-04-15 — Multi-segment download  →  ✅ IMPLEMENTED
**Status**: Done — `DownloadEngine` hỗ trợ multi-segment HTTP Range + resume sidecar `.fsdownload`.
**Legacy reference** (old codebase, not a dependency): the old `downloadfile.cpp` had `attempt_segmented_download()` — probes HTTP 206 Range support, splits file into N segments (default 4), parallel download via CURL multi-handle.
**Impact**: Slower download speeds on high-bandwidth connections, no parallel segments for large files.
**When to implement**: After core UI is finished and basic downloads work.

### 2026-04-15 — Chunked upload with adaptive sizing  →  ✅ IMPLEMENTED (cơ bản)
**Status**: Done — `UploadEngine` chunked + resumable (`queryResumeOffset`). Adaptive sizing theo latency: chưa.
**Legacy reference** (old codebase, not a dependency): the old `uploadfile.cpp` uploaded in 20MB chunks with `Content-Range` headers, per-chunk retry (up to 100), adaptive chunk size based on network latency, session renewal on 401.
**Impact**: Large uploads (>100MB) may fail on unstable connections with no recovery.
**When to implement**: Before production release for upload reliability.

### 2026-04-15 — SpeedMeter rolling window
**Status**: Uses 5-sec window (10 samples × 500ms), legacy uses 10-sec
**Impact**: ETA may bounce more for small files. Not critical.

### 2026-04-28 — Two-Factor Authentication (2FA) login
**Status**: Not implemented. Tracked here for future development.
**Priority**: Medium-High — security-critical feature; required as soon as Fshare
backend exposes a 2FA challenge endpoint or as part of standalone hardening
even before backend support (e.g. for shared workstations).

**Why it matters**: Fshare hosts Vietnamese users' personal cloud storage
(photos, documents, paid VIP content). A leaked password today gives full
account access. Industry baseline (Dropbox, Google Drive, OneDrive) all support
TOTP at minimum. Adding 2FA before account compromise becomes a public
incident is much cheaper than after.

**Backend dependency**: Fshare API does not currently expose a documented 2FA
flow. Implementation requires backend team to:
1. Provide endpoint `POST /api/user/2fa/verify` accepting
   `{session_token, otp_code}` returned from a `205` (or similar) intermediate
   response of `/api/user/login`. Or
2. Stand up TOTP enrollment + verify endpoints under `/api/user/2fa/*` that
   emit / consume RFC 6238 codes. Or
3. SMS OTP — typically slower and costs SMS gateway, not preferred but accepted
   as fallback for users without smartphone authenticator app.

**Frontend design (proposed)**:
- LoginView: after successful `/api/user/login` returning `{code: 205, msg: "2FA required", challenge_token}`, push a NEW screen (`TwoFactorChallengeView.qml`) with
  - 6-digit input (auto-focus, paste-friendly)
  - "Resend code via SMS" link (when applicable)
  - "Use backup code" link (10-digit recovery codes)
  - 30-second countdown ring (TOTP rotation indicator)
- Submit calls `POST /api/user/2fa/verify` with `{challenge_token, otp_code}`.
- On success → standard Session response shape (token + cookie).
- On failure (wrong code) → shake input, allow 5 attempts, then lock challenge
  for 60 s. After 3 lock cycles → expire challenge_token and force re-login.
- SettingsPage: new "Bảo mật" section with toggle "Bật xác thực 2 lớp",
  showing QR for TOTP enroll, list of backup codes, list of trusted devices.

**Storage**:
- `Account/twoFactorEnabled` bool, persisted via SettingsRepository.
- `Account/twoFactorBackupCodes` — 10 single-use recovery codes,
  DPAPI-encrypted via `SecureStore` (same pattern as password).
- Trusted devices: `Account/trustedDeviceTokens` — JSON array of
  `{deviceName, lastSeenIso, deviceToken}`. Server side issues device_token
  after a successful 2FA verify with "Trust this device for 30 days" checked;
  client sends it on subsequent `/api/user/login` to skip 2FA.

**ViewModel**:
- New `TwoFactorViewModel` exposing
  - `Q_PROPERTY(QString challengeToken READ ... NOTIFY challengeChanged)`
  - `Q_PROPERTY(int  remainingAttempts ...)`
  - `Q_PROPERTY(int  retryAfterSeconds ...)`
  - `Q_INVOKABLE void verify(const QString &otp)`
  - `Q_INVOKABLE void requestSmsResend()`
  - `Q_INVOKABLE void useBackupCode(const QString &code)`
  Owned by AppContext, instantiated lazily when AuthService emits
  `twoFactorRequired(challenge_token)`.

**OAuth interaction**: Google/Facebook/FPT-ID login already does its own
2FA at the provider, so the Fshare-side 2FA challenge should be SKIPPED for
OAuth-issued sessions (unless backend explicitly opts to challenge again).

**Test plan**:
- `test_two_factor_view_model.cpp` — verify input validation (6 digits only,
  numeric, no whitespace), countdown timer, lock-out / retry logic.
- Smoke test scenarios:
  1. New 2FA-enabled account → login → enter correct TOTP → success.
  2. Wrong TOTP × 5 → locked 60 s, sixth attempt rejected without API call.
  3. Use backup code → success, code marked used in settings.
  4. Trust device → restart app → login skips 2FA challenge.
  5. Trusted device expiry (>30 days) → 2FA required again.

**Estimated effort**: 5-7 dev days once backend endpoints exist.
Without backend: 2 dev days for self-contained TOTP enrollment that can be
verified locally (against a mock server) — useful as preparation work.

**Dependencies**:
- `oath-toolkit` or hand-rolled HMAC-SHA1 for TOTP (cppcodec already vendored).
- `qrcode-generator` (header-only) for enrollment QR — vendor or use Qt's
  `QImageWriter` + custom QR encoder.

**Cross-references**:
- Token refresh logic in `AuthService::tryRefreshSession` will need 2FA-awareness:
  if a refreshed session triggers `205 2FA required`, surface to user instead
  of failing silently.
- Single-instance message routing (`SingleInstance::messageReceived`) must NOT
  bypass an active 2FA challenge — drop-in URLs queued until the challenge
  resolves.

---

## Technical Debt

### 2026-04-28 — `IFshareApi` interface for mockable AuthService tests
**Why**: `AuthService::login` and `AuthService::tryRefreshSession` cannot be unit-
tested today because they hold a concrete `FshareApi*` pointer and `FshareApi`
has no virtual methods.  Stubbing the network response requires either a fake
HttpClient or — cleaner — an `IFshareApi` abstract base extracted from the 20+
methods on `FshareApi`.
**Effort**: ~1 day. Mostly mechanical (mark each public method virtual, derive
`FshareApi : public IFshareApi`, swap callers from `FshareApi*` to `IFshareApi*`).
**Pay-off**: enables `test_auth_service.cpp` covering login happy / 405 wrong
password / `tryRefreshSession` single-flight with concurrent callers /
session-expired propagation.
