---
title: 07 — Lộ trình triển khai
date: 2026-05-04
revision: 2 — cập nhật scope V1 sau quyết định ở 12
---

# 07. Lộ trình triển khai StreamIX TV

> **⚠️ Cập nhật scope V1 (2026-05-04)**: theo `12_scope-decisions-v1.md`, V1 đã cắt: Login QR, Login OAuth, Downloads, In-app update. Brand đổi sang **StreamIX TV** với package `vn.streamix.tv`. Phân phối qua Phương án A (manual). Lộ trình rút từ 15 → ~11 tuần.

## 7.0 Lộ trình mới sau scope cuts

| Phase | Tên | Cũ → Mới | Mục tiêu chốt |
|-------|-----|----------|---------------|
| 0 | Setup & spike | 1 tuần | Build APK chạy trên Mi Box S + FPT Box, package `vn.streamix.tv` |
| 1 | Core layer port | 2 tuần | Domain + Data + Network + Auth (chỉ email/password) |
| 2 | UI shell + Login + Browse | **3 → 2.5 tuần** | Login email + browse folder + list file (bỏ QR/OAuth screens) |
| 3 | Embedded Player + Search | 3 tuần | Player + Search V1 (designer thêm vào scope) |
| 4 | Settings + Build/Distribute | **2 → 1 tuần** | Settings 4 sub-screen (bỏ Download/Update/Network); CI/CD + portal nginx |
| 5 | Hardening + UAT + Beta | 3 tuần | Test, beta channel, fix bug |
| 6 | Public V1 release | 1 tuần | Slack/email release, monitor adoption qua nginx log |

**Tổng**: ~12.5 → 11.5 tuần (so với 15 tuần ban đầu).

## 7.0.1 Module engineering KHÔNG còn build

Các module dưới đây bị bỏ hoàn toàn khỏi V1 — engineering không phải lo:

- `feature/auth/qr` — login QR + polling endpoint
- `feature/auth/oauth` — OAuth device flow Google/FPT ID
- `feature/download` — toàn bộ download engine, USB OTG handling, disk-space check
- `feature/update` — UpdateChecker, UpdateDownloader, UpdateInstaller, RollbackGuard, InstallTracker
- `feature/settings/download` — sub-screen Settings → Download
- `feature/settings/update` — sub-screen Settings → Update
- WorkManager periodic update check
- Manifest JSON parsing
- PackageInstaller integration
- Permission `REQUEST_INSTALL_PACKAGES`, `RECEIVE_BOOT_COMPLETED`

→ Net tiết kiệm: ~3.5 tuần engineering.

## 7.0.2 Engineering có thể bắt đầu NGAY (Phase 0–1) trong khi designer làm v1.1

Không phải đợi handoff v1.1. Phase 0 + một phần Phase 1:
- Setup repo `streamix-tv/` với package `vn.streamix.tv`.
- Build APK Hello World, cài thử Mi Box S + FPT Box.
- Convert tokens v1.0 hiện tại sang Compose theme initial (sẽ refactor khi v1.1 về với typography bump).
- Convert 45 icon SVG sang VectorDrawable.
- Wire Hilt + Navigation + base Activity.
- Setup Room DB schema theo `02_khuyen-nghi-kien-truc.md` §2.3.
- Setup Retrofit + FshareApiService.
- Bắt đầu auth module (chỉ logic email/password, không UI — chờ design).
- Setup CI/CD theo `04_apk-va-update.md` §4.7 (rev 3 — đơn giản hoá Phương án A).
- Setup nginx portal `tv.streamix.vn` theo `04` §4.4.

Phần dưới đây giữ nguyên từ rev 1 cho historical reference; số tuần cần rút theo bảng 7.0 ở trên.

---

## 7.1 Tổng quan 6 phase (rev 1 — đã supersede bởi 7.0)

| Phase | Tên | Thời lượng | Mục tiêu chốt |
|-------|-----|-----------|---------------|
| 0 | Setup & spike | 1 tuần | Build APK trống chạy trên TV box VN |
| 1 | Core layer port | 2 tuần | Domain + Data + Network + Auth API gọi được |
| 2 | UI shell + Login + Browse | 3 tuần | Login QR + browse folder + list file |
| 3 | Embedded Player | 3 tuần | Phát video MKV/MP4, resume, subtitle nhúng |
| 4 | Download + Settings + Update | 2 tuần | Tải về USB, settings, in-app update |
| 5 | Hardening + UAT + Beta | 3 tuần | Test ma trận thiết bị, beta channel |
| 6 | Public V1 release | 1 tuần | Phát hành public, monitor 30 ngày |

**Tổng**: ~15 tuần (~3.5 tháng) tới V1 public.

## 7.2 Chi tiết từng phase

### Phase 0 — Setup & Spike (Tuần 1)

**Deliverable**:
- Repo `fsnext-tv/` mới (giữ separate với `FsNext` desktop để tránh nhầm).
- Gradle 8.7+, AGP 8.5+, Kotlin 2.0, Hilt, Compose-TV BOM, Media3 1.4.
- App "Hello TV" build APK chạy được trên ít nhất 2 thiết bị test:
  - Xiaomi Mi Box S (Android TV 9.0)
  - FPT Play Box (Android TV 7.x — quan trọng vì TV Box Việt phổ biến)
- CI build APK trên Github Actions / GitLab CI, ký bằng debug key.
- ADR-001: chọn Compose-TV thay vì Leanback (tài liệu hoá).

**Acceptance**: APK 5 MB chạy được, hiện 1 màn hình "Fshare TV — Hello", focus được nút "Click me".

**Rủi ro**: TV Box VN có thể có Android quá cũ → minSdk lùi xuống 19. Cần xác nhận thiết bị ngày đầu.

### Phase 1 — Core layer port (Tuần 2–3)

**Deliverable**:
- `core/domain` — model User, FileItem, TransferTask, AppSettings, ApiResponse, AppError. Port 1-1 từ `src/core/models/`.
- `core/network` — Retrofit + OkHttp + `FshareApiService` cover TẤT CẢ endpoint của bản desktop (login, getUserInfo, listFiles, getDownloadUrl, file ops xem 5.4 docs/02). Cookie + app_key signing như cũ (XOR-encoded `dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt`).
- `core/data` — Room DB schema (user, playback_position, download_history, settings); DataStore cho preference; Repository pattern cho mỗi domain.
- `core/common` — port `SpeedMeter`, `FileNameSanitizer`, `BudgetManager`, `FshareUrl` từ desktop (đã có unit test desktop, viết lại unit test Kotlin).
- Hilt modules đầy đủ.

**Acceptance**: instrumented test login → getUserInfo → listFiles trên thiết bị thật, không crash. Coverage `core/*` ≥ 80%.

### Phase 2 — UI Shell + Login + Browse (Tuần 4–6)

**Deliverable**:
- `app/MainActivity` single-activity, navigation Compose graph.
- `feature/auth`:
  - Login bằng email/password.
  - Login bằng QR (gọi endpoint `/api/user/qrlogin/start` + polling — nếu Fshare API hỗ trợ; nếu không, đề xuất backend thêm).
  - OAuth device flow cho Google/FPT ID.
  - Auto-login bằng refresh token.
- `feature/browse`:
  - Home screen với "Tiếp tục xem", "File gần đây", "Thư mục".
  - Folder browse (lazy load, breadcrumb, sort).
  - File detail screen với metadata + nút Phát/Tải/Yêu thích.
- Theme `FsTvTheme` Compose-TV với palette Aurora.
- 5 user UAT internal pilot.

**Acceptance**: user mở app → login QR → thấy danh sách file → mở detail. Mọi tương tác bằng remote D-pad, không touch.

### Phase 3 — Embedded Player (Tuần 7–9)

**Deliverable**:
- `feature/player` với ExoPlayer Media3 default.
- Custom Compose overlay controls (play/pause, seek bar, ±10s/±30s, exit).
- Track selection (audio + subtitle) UI bottom-sheet.
- Subtitle nhúng (MKV) + subtitle ngoài cùng folder (.srt) hỗ trợ.
- Resume playback (Room `playback_position` upsert mỗi 5s).
- Stream URL refresh khi 403 (TTL expire) — tự retry.
- Engine switch UI: ExoPlayer ↔ libVLC trong Settings (libVLC integration ở mức POC, chỉ enable nếu cần).
- "Tiếp tục xem" row trên Home đọc từ Room.

**Acceptance**: 
- Phát thử 20 file mẫu trên 3 thiết bị, ≥ 18/20 phát được trên ExoPlayer (90%).
- Resume từ vị trí dừng chính xác ±2s.
- Subtitle bật/tắt được; chuyển track audio được.

### Phase 4 — Download + Settings + Update (Tuần 10–11)

**Deliverable**:
- `feature/download`:
  - Tải 1 file về USB OTG hoặc internal storage.
  - Multi-segment download (port logic từ `DownloadEngine.cpp`, viết lại Kotlin coroutines + OkHttp Range).
  - Progress notification.
  - Foreground service WorkManager.
  - Disk-space pre-check 50 MB headroom (giữ logic D6 từ ADR 003).
- `feature/settings`:
  - Account, ngôn ngữ, chất lượng, vị trí lưu, engine player.
  - Phụ đề: kích thước, font, màu.
  - Channel update: stable/beta.
- `feature/update`:
  - Manifest checker (gọi `tv-update.fshare.vn/manifest/stable.json`).
  - Downloader (OkHttp + progress).
  - Installer (PackageInstaller + verify SHA-256 + verify signature).
  - Rollback nếu crash 3 lần liên tiếp.

**Acceptance**: download 1 file 4 GB không lỗi; in-app update flow end-to-end thành công trên 2 thiết bị (Android 8 và 12).

### Phase 5 — Hardening + UAT + Beta (Tuần 12–14)

**Deliverable**:
- Test ma trận thiết bị (5–8 device): Xiaomi Mi Box S, Mi Box 4K, Sony Bravia, TCL Android TV, FPT Play Box, MyTV Box, Chromecast w/Google TV, Sharp Aquos.
- Test format media: 20+ file mẫu (MP4/MKV/AVI/FLV) × các codec.
- Test mạng: Wi-Fi tốt, Wi-Fi yếu (2 Mbps), Ethernet, mất mạng giữa chừng.
- Test crash recovery, process death, low memory.
- Crashlytics integration verified, log buffer in-app.
- Beta channel deploy: 50–100 user pilot trong 2 tuần.
- Fix toàn bộ P0/P1 bug từ pilot.

**Acceptance**: 
- Crash-free user rate ≥ 99% trong 7 ngày cuối beta.
- ANR rate < 0.1%.
- Tỷ lệ login success ≥ 98%.
- Tỷ lệ stream play success (file tương thích) ≥ 95%.
- ≥ 50 user beta gửi survey, NPS ≥ 30.

### Phase 6 — Public V1 release (Tuần 15)

**Deliverable**:
- Trang download `tv.fshare.vn` với hướng dẫn.
- APK universal + ARM64 trên CDN.
- Manifest stable.json.
- Documentation người dùng (FAQ, hướng dẫn cài đặt).
- Marketing assets: video demo 60s, ảnh chụp màn hình 4K.
- Press kit cho team comm.
- Monitoring dashboard (Crashlytics + custom funnel).

**Sau release** — duy trì 30 ngày sát:
- Daily check Crashlytics top 10 issue.
- Weekly hot-fix cycle nếu có P0.
- Tuần 2 sau launch: retrospective, plan V1.1.

## 7.3 Resource estimation

### Team (FTE)

| Vai trò | FTE | Phase tham gia |
|---------|-----|----------------|
| Tech Lead Android | 1.0 | 0–6 (full) |
| Senior Android Engineer | 2.0 | 1–6 |
| Backend (API support, QR login endpoint) | 0.5 | 1, 2 |
| QA Engineer (TV expertise) | 1.0 | 3–6 |
| UX Designer (10-foot) | 0.5 | 0–3 (front-loaded) |
| DevOps (CI/CD, signing, CDN) | 0.3 | 0, 4, 6 |
| Product Manager | 0.3 | toàn bộ |

**Tổng**: ~5.6 FTE × 15 tuần = **84 FTE-tuần** ≈ 21 person-month.

### Hardware test lab (cần mua/mượn)

- Xiaomi Mi Box S (~1.5 tr) ×1
- FPT Play Box S (~1 tr) ×1
- TCL Android TV 43" (~6 tr) ×1
- Sony Bravia 4K (~10 tr) — mượn từ marketing nếu có
- Chromecast w/Google TV 4K (~1.5 tr) ×1
- 1 USB OTG hub + 3 USB drive 64GB

**Tổng**: ~25 triệu VND cho test lab cơ bản.

### Infrastructure

- CDN cho APK + manifest: ~$10–30/tháng đầu.
- Backend bổ sung endpoint QR login: nếu chưa có, +0.5 FTE backend × 2 tuần.
- Crashlytics: free tier đủ.
- Signing: 1 keystore, lưu offline.

## 7.4 Mốc quan trọng (key milestones)

| Mốc | Tuần | Định nghĩa "đạt" |
|-----|------|------------------|
| M0 — Hello APK trên TV box VN | 1 | APK build & cài chạy được Mi Box + FPT Box |
| M1 — Login + browse hoạt động | 6 | Demo end-to-end trước stakeholder |
| M2 — Phát video đầu tiên trên TV | 9 | Demo phát + tua + subtitle |
| M3 — Update flow complete | 11 | Update từ v0.9 lên v1.0 success |
| M4 — Beta open to 50 user | 12 | Có dashboard, có log, có user feedback channel |
| M5 — V1 public release | 15 | Trang download live, APK trên CDN |

## 7.5 Roadmap V1.x → V2

Sau V1, theo backlog ưu tiên:

**V1.1 (1 tháng sau V1)**:
- Bug fix từ telemetry.
- Voice search Google Assistant.
- Recommendations row trên Android TV launcher.

**V1.2 (2 tháng sau)**:
- Cast nhận từ app mobile (companion app).
- Picture-in-picture cho audio.
- Hỗ trợ subtitle ASS/SSA tốt hơn (qua libVLC).

**V2 (6 tháng sau V1)**:
- Tizen + WebOS web app — codebase thứ hai.
- KMP share core/domain với mobile companion app.
- ABR/HLS nếu Fshare backend hỗ trợ transcode.

**V2.5+**:
- Game console (PS5/Xbox) qua Web app.
- Apple TV — native Swift app, codebase thứ ba.

## 7.6 Definition of Done cho V1

V1 chỉ release public khi:
- [ ] Test ma trận pass trên ≥ 5 thiết bị
- [ ] Crash-free user ≥ 99% trong 7 ngày
- [ ] Login flow success rate ≥ 98%
- [ ] Stream play success rate ≥ 95% (file tương thích)
- [ ] APK universal ≤ 30 MB
- [ ] Cold start ≤ 2.5s trên Mi Box S
- [ ] In-app update tested end-to-end
- [ ] Trang `tv.fshare.vn` live với hướng dẫn cho 5 dòng TV phổ biến
- [ ] Crashlytics + analytics dashboard live
- [ ] User documentation (FAQ + install guide) bản tiếng Việt
- [ ] Hot-fix process documented (1 ngày từ phát hiện đến hot-fix release)

## 7.7 Kết luận chương 7

15 tuần, 5.6 FTE, 6 phase. Phase 3 (Player) và Phase 5 (Hardening) là rủi ro cao nhất — phải đệm thời gian (đã đệm 3 tuần mỗi phase). Phase 0 spike sớm là quan trọng để xác nhận TV Box VN không có rào cản kỹ thuật bất ngờ.

Tiếp theo: rủi ro & giảm thiểu ([08](08_rui-ro-va-giam-thieu.md)).
