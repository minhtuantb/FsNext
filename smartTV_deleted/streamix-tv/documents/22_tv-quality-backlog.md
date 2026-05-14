---
title: StreamIX TV — Quality Backlog (cần thảo luận)
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Backlog tổng hợp — Items cần thảo luận sau

Mỗi item có:
- **Severity**: P0 (blocker release), P1 (should fix), P2 (nice-to-have).
- **Effort**: S (≤ 4h), M (≤ 1d), L (≤ 3d), XL (> 3d).
- **Câu hỏi cần owner trả lời** (nếu có) để chốt scope.

---

## A. Player & Media (P0)

### B-A1. Hook KEYCODE_MEDIA_PLAY_PAUSE / FFW / REW
**Severity**: P0 — TV remote luôn có nút riêng cho play/pause; user expect.
**Effort**: M
**Plan**: PlayerActivity (hoặc Compose effect) override `onKeyDown` để bắt:
- `KEYCODE_MEDIA_PLAY_PAUSE` → toggle play/pause.
- `KEYCODE_MEDIA_FAST_FORWARD` → seek + 10s.
- `KEYCODE_MEDIA_REWIND` → seek - 10s.
- `KEYCODE_MEDIA_STOP` → exit player.
**Câu hỏi**: Long-press nút FFW có cần seek nhanh hơn (×2, ×3) không?

### B-A2. ExoPlayer release lifecycle audit
**Severity**: P0 — leak memory nếu user nhấn Home khi đang play.
**Effort**: S
**Plan**: Audit `PlayerScreen.kt` đảm bảo `DisposableEffect` release ExoPlayer instance khi composable rời composition. Test: rotate / press Home rồi quay lại → no leak.

### B-A3. Subtitle ASS render
**Severity**: P1
**Effort**: M
**Plan**: media3-ui SubtitleView mặc định render WebVTT/SRT tốt; ASS phức tạp có thể vỡ format. Test trên file MKV có ASS thực tế. Nếu vỡ, dùng external library (libass).

### B-A4. Player error retry button default focus
**Severity**: P1
**Effort**: S
**Plan**: PlayerErrorOverlay → đặt `focusRequester.requestFocus()` lên Retry trong `LaunchedEffect`.

---

## B. UX & Focus (P1)

### B-B1. Audit default focus mọi screen
**Severity**: P1
**Effort**: M
**Plan**: Mỗi screen có `LaunchedEffect(Unit) { focusRequester.requestFocus() }` ở first focusable element.
**Screens cần audit**: Login (email field), Home (first card), Browse (first item), Search (text field), Settings (first item).

### B-B2. Focus restoration sau navigateUp
**Severity**: P1
**Effort**: M
**Plan**: Compose Navigation chưa native restore focus. Workaround: `rememberSaveable` + `focusRestorer()` modifier. Verify trên TV emulator.

### B-B3. Compose semantics audit cho TalkBack
**Severity**: P1
**Effort**: M
**Plan**: Mọi icon-only button (TopBar Search/Settings/Avatar, password visibility toggle) thêm `Modifier.semantics { contentDescription = ... }`.

### B-B4. Pluralization
**Severity**: P2
**Effort**: S
**Plan**: Move "1 file / N files" sang `<plurals>`.
**Files affected**: home_progress_label, browse counter (nếu có).

### B-B5. Date/time locale-aware
**Severity**: P2
**Effort**: S
**Plan**: Dùng `DateUtils.getRelativeTimeSpanString` thay format hardcode.

---

## C. Performance (P0/P1)

### B-C1. Cold start measurement
**Severity**: P0 — release blocker.
**Effort**: M
**Plan**: 
1. Build release APK.
2. Cài trên TV thật (Sony BRAVIA / Xiaomi Mi Box) và Android Studio Profiler.
3. Measure cold start (first launch sau install) — target ≤ 2s.
4. Nếu vượt: defer Hilt subcomponent load, lazy import Coil v.v.

### B-C2. APK size optimization
**Severity**: P1
**Effort**: M
**Plan**:
- Verify R8 minify + shrinkResources release ON (đã có).
- Check `./gradlew :app:bundleRelease` vs `assembleRelease` — universal APK có thể to.
- Target ≤ 30MB.
- Nếu vượt: split per-ABI (cần build pipeline)? Hoặc remove `kotlinx-serialization` nếu chỉ dùng cho Routes.

### B-C3. Memory profile trên TV box rẻ
**Severity**: P1
**Effort**: M
**Plan**: Test trên TV box 1-2GB RAM. Profile Compose recompose hot path.

### B-C4. Frame drop profiling
**Severity**: P1
**Effort**: M
**Plan**: GPU rendering profile + Compose tracing.

---

## D. State persistence (P1)

### B-D1. SavedStateHandle audit
**Severity**: P1
**Effort**: M
**Plan**: Mọi ViewModel save state quan trọng (search query, browse currentPage) qua SavedStateHandle để survive process death.

---

## E. Security (P0)

### B-E1. Bearer Timfshare leak qua git history
**Severity**: P0 — secret committed trong `documents/API_REFERENCE_FOR_REUSE.md` line 32.
**Effort**: S
**Plan**: 
1. Confirm với owner: token này có thực sự là "non-expiring partner token" và có thể public-ish không?
2. Nếu là secret thật: rotate token với partner Timfshare, invalidate cũ.
3. Move token mới ra env var `STREAMIX_TIMFSHARE_BEARER`, KHÔNG commit.
4. Nếu non-secret (partner cố tình đặt expires=0 và share công khai cho ứng dụng Fshare): document rõ trong README để team biết.
**Câu hỏi**: Token đã được partner Timfshare cấp riêng cho ứng dụng "Fshare" — có cần coi là secret strict không? Nếu cần rotate thì owner liên hệ partner.

### B-E2. Cleanup deprecated stub files
**Severity**: P2
**Effort**: S
**Plan**: 
- `AppKeyInterceptor.kt` (stub package-only).
- `TokenRefreshAuthenticator.kt` (stub package-only).
- Sau khi confirm không có reference, xoá hẳn ở wave cleanup.

---

## F. TV compatibility (P0/P1)

### B-F1. Test trên ≥ 2 dòng TV thật
**Severity**: P0 — release blocker.
**Effort**: L
**Plan**:
- Sony BRAVIA Android TV (≥ 2018).
- Xiaomi Mi Box S (Android 9-10).
- Test toàn bộ flow Login → Home → Browse → Player.
- Test sideload qua USB hoặc TV File Manager.

### B-F2. Fire TV (Fire OS) compatibility verify
**Severity**: P1
**Effort**: M
**Plan**: Fire OS = AOSP fork — App nên chạy được nhưng phải:
- Bật "Apps from Unknown Sources" trong Settings → Device → Developer Options.
- Sideload qua "AppStarter" hoặc ADB.
**Câu hỏi**: Có cần support Fire TV trong V1 không? Nếu có, doc separate sideload guide cho Fire OS.

### B-F3. Audit screen size buckets
**Severity**: P2
**Effort**: S
**Plan**: Compose adaptive nhưng test thực tế trên 1280×720 (HD ready) panels — xem text/card size có đọc được từ 3m không.

### B-F4. PNG asset multi-density
**Severity**: P2
**Effort**: M
**Plan**: Hiện UI dùng vector drawables — OK. Nếu designer thêm photo/illustration PNG sau, cần xhdpi/xxhdpi/xxxhdpi buckets.

---

## G. Build & CI (P1)

### B-G1. CI pipeline cho test + build
**Severity**: P1
**Effort**: M
**Plan**:
- GitLab CI hoặc GitHub Actions.
- Pipeline: Pint check → unit test (`./gradlew test`) → assembleDebug → upload artifact.
- Bonus: instrumented test trên emulator (cần GitLab runner with KVM).

### B-G2. Automated APK signing in CI
**Severity**: P2
**Effort**: M
**Plan**: Inject keystore qua secret CI variable. KHÔNG commit keystore.

---

## H. Testing (P0/P1)

### B-H1. ViewModel tests
**Severity**: P1
**Effort**: M
**Plan**:
- HomeViewModelTest: state transitions, sessionExpired flag.
- LoginViewModelTest: error mapping.
- SearchViewModelTest: debounce + query length filter.
- PlayerViewModelTest: PasswordRequired flow.
- Setup `Dispatchers.setMain` + `Turbine` for StateFlow.

### B-H2. Instrumented test setup
**Severity**: P1
**Effort**: L
**Plan**:
- Compose UI test cho LoginScreen, HomeScreen, PlayerScreen (smoke).
- Cần TV emulator (Pixel TV template trong Android Studio).

### B-H3. Coverage report
**Severity**: P2
**Effort**: S
**Plan**: JaCoCo plugin → report HTML, target 70% coverage cho domain + data + viewmodel.

---

## I. Telemetry / GA4 (P2 — chờ owner)

### B-I1. GA4 secret + integration
**Severity**: P2
**Effort**: M
**Plan**: Owner gửi `STREAMIX_GA4_API_SECRET` → integrate Measurement Protocol client.
**Câu hỏi**: TV có cần measurement riêng (TV property mới) hay dùng chung property với desktop?

### B-I2. Anonymous client_id
**Severity**: P2
**Effort**: S
**Plan**: UUID v4 lưu DataStore, không reset khi clear data trừ khi user explicit reset.

---

## J. Documentation cleanup (P2)

### B-J1. Update doc 04 (APK update mechanism)
**Severity**: P2
**Effort**: S
**Plan**: Phương án A đã chốt manual notify — cập nhật doc 04 cho consistent.

### B-J2. README setup cho team developer mới
**Severity**: P2
**Effort**: S
**Plan**: Document JAVA_HOME, env vars (STREAMIX_FSHARE_*, STREAMIX_TIMFSHARE_*), build commands.

---

## Summary table

| Category | P0 | P1 | P2 | Total |
|---|---|---|---|---|
| Player & Media | 2 | 2 | 0 | 4 |
| UX & Focus | 0 | 3 | 2 | 5 |
| Performance | 1 | 3 | 0 | 4 |
| State persist | 0 | 1 | 0 | 1 |
| Security | 1 | 0 | 1 | 2 |
| TV compat | 1 | 1 | 2 | 4 |
| Build & CI | 0 | 1 | 1 | 2 |
| Testing | 0 | 2 | 1 | 3 |
| Telemetry | 0 | 0 | 2 | 2 |
| Docs | 0 | 0 | 2 | 2 |
| **TỔNG** | **5** | **13** | **11** | **29** |

## Khuyến nghị release sequence

**Sprint 1 (P0 — release blocker)**: B-A1, B-A2, B-C1, B-E1, B-F1.
**Sprint 2 (P1 — quality)**: B-B1, B-B2, B-B3, B-A3, B-A4, B-D1, B-C2, B-C3, B-F2, B-G1, B-H1, B-H2.
**Sprint 3 (P2 — polish)**: B-B4, B-B5, B-E2, B-F3, B-F4, B-G2, B-H3, B-I1, B-I2, B-J1, B-J2.
