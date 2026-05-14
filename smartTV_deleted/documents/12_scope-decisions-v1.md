---
title: 12 — Scope Decisions V1 (chính thức)
date: 2026-05-04
status: APPROVED — sign-off bởi user (project owner)
supersedes: phần scope của 01, 09 (mục §4 Phạm vi V1) — chỉ những phần mâu thuẫn
---

# 12. Scope Decisions V1 — Quyết định chính thức

## 0. Bối cảnh

Sau audit handoff v1.0 (xem `11_review-handoff-v1.md`), project owner đã ra quyết định chính thức 2026-05-04:

> "đồng ý đổi tên thành 'StreamIX TV', đồng ý Cắt 4 module P0 khỏi V1 không qua CR:
> * Login QR (S1a)
> * Login OAuth (S1c)
> * Downloads (S6 + S7c)
> * In-app update (S7e + S8 4 variant). chốt internal APK distribution"

Tài liệu này ghi nhận quyết định và đồng bộ các tài liệu liên quan. Đây là **source of truth** cho scope V1; mọi tài liệu cũ (`09_design-spec.md`, `04_apk-va-update.md`, `01_phan-tich-hien-trang.md`) phải đọc song song với tài liệu này khi có mâu thuẫn.

---

## 1. Quyết định D-1: Brand chính thức = "StreamIX TV"

### 1.1 Decision

- **Tên app**: StreamIX TV
- **Package name**: `vn.streamix.tv` ✅ chốt 2026-05-04
- **Tagline**: "Phát mọi định dạng từ Fshare" (theo localization key `app_tagline`)
- **Thương hiệu mẹ**: Fshare (vẫn là sản phẩm của Fshare; "StreamIX TV" là sub-brand độc lập về namespace)
- **Domain phân phối** (đề xuất): `tv.streamix.vn` hoặc `releases.streamix.vn` — chờ DNS/SSL setup. Có thể tạm dùng `tv-update.fshare.local` cho V1 internal nếu chưa setup domain riêng.

### 1.2 Hệ luỵ

- README dự án (`D:\Work\FsNext\README.md` của repo nếu có) cần update.
- Project instructions hiện tại "Phát triển ứng dụng desktop client cho fshare.vn" vẫn đúng về scope ("phục vụ fshare.vn") nhưng tên product output là "StreamIX TV".
- Mọi string đã có "Fshare TV" / "FsNext" trong design package giữ nguyên là "StreamIX TV" (designer đã rename, OK).
- Splash, app icon, banner Android TV — tất cả dùng wordmark "StreamIX TV" + dấu hiệu "by Fshare" nhỏ phía dưới (đề xuất).

### 1.3 Action

- ✅ Đã chốt — designer tiếp tục v1.1 với brand "StreamIX TV".
- ⏳ Designer phải sản xuất logo + app icon + splash + banner theo brand mới (chưa giao trong handoff v1.0).

---

## 2. Quyết định D-2: V1 scope cắt 4 module

### 2.1 Decision

Cắt khỏi V1:

| Module | Screens liên quan | Lý do chấp nhận |
|--------|------------------|-----------------|
| Login QR | S1a | Đơn giản hoá V1; user vẫn login được qua email/password |
| Login OAuth (Google / FPT ID) | S1c | Cùng lý do trên |
| Downloads (offline) | S6, S7c | Bỏ chức năng tải về local; V1 chỉ stream |
| In-app update flow | S7e, S8 (4 variant), 1 phần S7e | Update sẽ thực hiện ngoài app — xem D-3 dưới |

Giữ trong V1:

- Login email + password (S1, S1b → gộp thành 1 screen).
- Player (S5, S5a, S5b, S5c, S5d).
- Browse, File Detail, Home, Search.
- Settings: Account (S7a), Playback (S7b), Appearance, About — KHÔNG có Download/Update/Network sub-screen.
- Onboarding, Confirm dialogs, Global error.

### 2.2 Hệ luỵ kỹ thuật

#### Auth đơn giản hoá

- Chỉ giữ `LoginUseCase(email, password)`.
- Bỏ `OAuthUseCase`, `QRLoginUseCase`, polling endpoint.
- Backend không phải build endpoint `qrlogin/start` (open question §15.1 của tài liệu 09 → giải quyết: KHÔNG cần V1).
- Module `feature/auth` rút lại ~40% so với plan ban đầu.

#### UX login trên TV

- ⚠️ **Cảnh báo còn nguyên**: gõ email/password bằng D-pad rất tệ. Mitigation:
  - On-screen keyboard tối ưu (KeyboardOnScreen — C21 component).
  - "Nhớ tôi 30 ngày" default ON → user gõ 1 lần/tháng.
  - Hỗ trợ **keyboard rời USB/BT** (đã spec §7.5 của 09).
- Không có lựa chọn QR → user mới onboarding sẽ mất 1–3 phút thay vì < 30 giây.

#### Downloads bỏ hoàn toàn

- Bỏ row "Đã tải xuống" trên Home (S2 chỉ còn 3 row: Continue Watching, File gần đây, Thư mục).
- Bỏ `DownloadEngine`, `DownloadViewModel`, `WorkManager` cho download.
- Bỏ logic disk-space pre-check (không cần).
- Bỏ USB OTG handling.
- Tiết kiệm ~3–4 tuần engineering effort (Phase 4 của lộ trình `07_lo-trinh-trien-khai.md` rút từ 2 tuần → 1 tuần).
- ⚠️ Hệ luỵ: app **không hoạt động khi mất mạng** — phải có UX rõ ràng "Cần kết nối mạng để xem".

#### In-app update bỏ

- Bỏ `UpdateChecker`, `UpdateDownloader`, `UpdateInstaller`, `RollbackGuard`, `WorkManager` periodic check, `InstallTracker`.
- Bỏ S7e Settings → Update screen.
- Bỏ S8 4 variant (optional/downloading/force/rollback).
- Bỏ permission `REQUEST_INSTALL_PACKAGES`.
- Tiết kiệm ~1 tuần engineering effort.
- ⚠️ Tạo ra **gap nghiêm trọng** — xem D-3.

### 2.3 Action

- ✅ Đã chốt — designer tiếp tục v1.1 không thiết kế các screen đã cắt.
- ✅ Engineering không build các module đã cắt.
- ⚠️ V2 sẽ cần đánh giá lại có khôi phục các module này không.

---

## 3. Quyết định D-3: Distribution = Internal APK (giữ nguyên), KHÔNG có in-app update

### 3.1 Decision

- **Phân phối**: Internal APK qua server nội bộ (theo `04_apk-va-update.md` §4.4, phương án A nginx hoặc B R2). KHÔNG Play Store.
- **Update**: KHÔNG có cơ chế in-app update tự động. App không gọi manifest, không tự tải, không tự cài.

### 3.2 Mâu thuẫn cần giải quyết: HOW updates happen?

Quyết định D-2 cắt in-app update + D-3 giữ internal APK = **không có cách auto-update**. User chạy version cũ mãi cho đến khi tự cài đè APK mới.

Tài liệu 04 §4.0 đã viết: *"Từ lần thứ hai trở đi, app **tự kiểm tra version, tự tải, tự cài** mà không cần user vào Settings → Unknown sources mỗi lần."* — phần "tự kiểm tra/tự tải/tự cài" giờ bị bỏ.

### 3.3 3 phương án cho update workflow

Project owner cần chọn 1 trong 3 phương án dưới (hoặc đề xuất khác). Mỗi phương án có hệ luỵ riêng:

#### Phương án A — Manual notify + manual install (đơn giản nhất)

Cách hoạt động:
- IT/admin release APK mới → upload lên server nội bộ.
- Thông báo qua Slack `#streamix-tv-releases` / email / Zalo nội bộ.
- User mở browser TV (hoặc dùng phone push qua AirDrop/USB) → tải APK mới → cài đè.

Pros:
- 0 effort engineering — không cần build update infrastructure.
- 0 cost server cho manifest API.
- Giữ đúng tinh thần "đơn giản hoá V1".

Cons:
- User adoption thấp — user không nhận thấy/lười update.
- Không có way để force critical security fix.
- Bug critical phát hiện sau release → mất 1–2 tuần để propagate fix tới mọi user.
- Mỗi update = mỗi user phải làm "lần đầu cài" lại — UX dở.

→ **Phương án này phù hợp nếu**: V1 pilot < 50 user, có nhân viên kỹ thuật/IT support trực tiếp; chấp nhận update không đồng bộ.

#### Phương án B — Silent auto-update qua MDM (phức tạp nhất)

Cách hoạt động:
- Mỗi TV của nhân viên enrol vào hệ thống MDM (Headwind, Hexnode, hoặc giải pháp Fshare tự host).
- IT push APK qua MDM → TV tự cài qua silent install (MDM có quyền `INSTALL_PACKAGES` system-level).
- User không phải làm gì.

Pros:
- Zero-touch update.
- Có thể force update.
- Có monitoring (biết version từng TV).

Cons:
- Setup MDM mất 2–4 tuần infrastructure.
- Cost license MDM (thường > $5/device/year).
- TV phải enrol MDM trước — onboarding nặng.
- Vượt scope V1 "đơn giản hoá".

→ **Phương án này phù hợp nếu**: V2 mở rộng > 200 user, công ty có IT department và budget MDM.

#### Phương án C — Notification banner trong app + manual install (cân bằng) ⭐

Cách hoạt động:
- App khi mở vẫn check 1 endpoint nhẹ (`GET /version-check`) → biết version mới nhất.
- Nếu có version mới: hiện **banner subtle** ở Settings → About (icon dot cam) HOẶC toast nhỏ trên Home "Có bản mới X.Y.Z — cập nhật thủ công".
- User vào Settings → About → "Cách cập nhật" → modal hướng dẫn 3 bước với QR code dẫn về URL APK trên server.
- App KHÔNG tự tải, KHÔNG tự cài. User phải tự download APK qua phone/USB rồi cài.

Pros:
- User biết có update (vs Phương án A — user mù tịt).
- Vẫn đơn giản — không cần PackageInstaller, không cần verify signature, không cần rollback logic.
- Engineering effort: ~2 ngày (chỉ là 1 endpoint + 1 banner + 1 modal hướng dẫn).
- Có thể tạo "force update" bằng cách block app khởi động nếu version < `minSupportedVersionCode` từ server.

Cons:
- Vẫn có gap adoption (user phải tự cài).
- Hơn Phương án A một chút effort.

→ **Phương án này khuyến nghị**: cân bằng tốt nhất giữa scope và UX. Khôi phục một phần update infrastructure đã bỏ, nhưng nhẹ hơn nhiều full in-app update.

### 3.4 Khuyến nghị

**🎯 Đề xuất chọn Phương án C** (notification + manual install).

Lý do:
- Giữ tinh thần "cắt in-app update" (không tự tải/tự cài) → respect quyết định của project owner.
- Không phá scope V1 → effort 2 ngày, không phải 1 tuần như full in-app.
- Tránh gap user adoption như Phương án A.
- Không phải đầu tư MDM ngay V1.

Nếu Project Owner đồng ý Phương án C, tài liệu `04_apk-va-update.md` sẽ giữ §4.1–§4.4 (build/sign/host) và §4.10 (CI/CD), bỏ §4.6 (UpdateChecker/Downloader/Installer chi tiết), §4.8 (auto-update flow), §4.9 (rollback). Thay vào đó §4.6 mới sẽ là "Notification-only version check" với code rút gọn ~1/10 dung lượng cũ.

### 3.5 ✅ DECIDED — Phương án A

**Project Owner đã chọn 2026-05-04: Phương án A (Manual notify + manual install).**

Cụ thể:
- App **KHÔNG** check version, **KHÔNG** có endpoint `/version-check`, **KHÔNG** có banner update.
- IT/admin release APK mới → upload lên server nội bộ → notify qua Slack/email/Zalo nội bộ.
- User mở browser TV (hoặc dùng phone) → tải APK → cài đè thủ công.
- App không có concept của "update" — mỗi cài là "first install" về mặt UX.

Hệ luỵ:
- `04_apk-va-update.md` rewrite hoàn toàn: bỏ §4.5 (Manifest), §4.6 (UpdateChecker/Downloader/Installer), §4.8 (Auto-update flow), §4.9 (Rollback), §4.11 (Telemetry update). Giữ §4.1–§4.4 (build/sign/host) và §4.10 (CI/CD đơn giản hoá), §4.7 (First install) trở thành flow chính, không phải edge case.
- App APK không cần permission `REQUEST_INSTALL_PACKAGES`, `INTERNET` chỉ dùng cho stream/auth.
- Dependency `androidx.work:work-runtime-ktx` không cần cho update (vẫn có thể cần cho stream).
- Tiết kiệm thêm ~3 ngày engineering so với Phương án C.

Risk R-S2 (version cũ tồn tại lâu) → mitigation cần protocol IT chặt chẽ:
- Slack channel `#streamix-tv-releases` bắt buộc — mọi release post tại đây.
- Email blast cho list nhân viên có TV.
- Reminder mỗi 7 ngày nếu version distribution analytics cho thấy < 70% lên latest.
- Cho phép app tiếp tục chạy với mọi version — không có kill switch (vì kill switch cần endpoint, mâu thuẫn với "bỏ in-app update").

---

## 4. Bảng đối chiếu V1 trước/sau quyết định

| Thành phần | V1 trước (spec gốc) | V1 sau (đã chốt) | Tiết kiệm effort |
|------------|--------------------|--------------------|------------------|
| Brand | Fshare TV / FsNext | StreamIX TV | 0 (tương đương) |
| Login QR | có | bỏ | -3 ngày eng + ~5 ngày backend |
| Login OAuth (Google + FPT) | có | bỏ | -2 ngày eng |
| Login email/password | có | có (giữ nguyên) | 0 |
| Home | 4 row | 3 row (bỏ "Đã tải xuống") | -0.5 ngày |
| Browse | có | có | 0 |
| File Detail | có | có | 0 |
| Player + Resume + Subtitle | có | có | 0 |
| Search | V2 | **V1** (designer thêm) | +5 ngày eng |
| Downloads | có | bỏ | -7 ngày eng |
| Settings/Account | có | có | 0 |
| Settings/Playback | có | có | 0 |
| Settings/Download | có | bỏ | -1 ngày |
| Settings/Network | có | bỏ (designer cắt) | -1 ngày |
| Settings/Update | có | bỏ | -2 ngày |
| Settings/About | có | có | 0 |
| In-app update full flow | có | bỏ | -5 ngày |
| Notification version check | — | TBD (Phương án C nếu chọn) | +2 ngày nếu C |
| Onboarding | có | có (đơn giản hoá: chỉ 2 bước thay 3) | -0.5 ngày |
| First install + APK distribution | có | có (giữ) | 0 |

**Net**: tiết kiệm ~22 ngày engineering, thêm ~5 ngày cho Search → ròng ~17 ngày = ~3.5 tuần.

**Lộ trình mới**: rút từ 15 tuần xuống ~11–12 tuần cho V1 public.

---

## 5. Tài liệu bị ảnh hưởng — cần cập nhật

| Tài liệu | Cần update? | Khi nào |
|----------|-------------|---------|
| `01_phan-tich-hien-trang.md` | ⚠️ §1.4 cần ghi chú "DEPRECATED — xem 12" | Sau khi user xác nhận tài liệu này |
| `02_khuyen-nghi-kien-truc.md` | nhẹ — Module Gradle bỏ `feature/download`, `feature/update` (chỉ commenting out trong tài liệu) | Sau D-3 |
| `03_lua-chon-cong-nghe.md` | không | — |
| `04_apk-va-update.md` | **rewrite phần update** sau khi chọn phương án A/B/C | Chờ D-3.5 |
| `05_player-media-nhung.md` | bỏ S5b track selection nếu designer thực sự cắt — cần xác nhận lại | Sau D-2.4 |
| `06_ux-10foot-dpad.md` | nhẹ — bỏ section liên quan Download | Optional |
| `07_lo-trinh-trien-khai.md` | rewrite Phase 4 (gọn hơn), cập nhật tổng số tuần | Sau D-3 |
| `08_rui-ro-va-giam-thieu.md` | thêm 2 risk mới (xem §7) | Sau D-3 |
| `09_design-spec.md` | §4 Phạm vi V1 cần update — bỏ Login QR/OAuth/Downloads/In-app update khỏi "in scope" | Optional (giữ làm history) |
| `10_design-handoff-requirements.md` | giữ nguyên — tài liệu này là yêu cầu chung | — |
| `11_review-handoff-v1.md` | update verdict BLOCK → ACCEPT (conditional on D-3.5) | Sau D-3.5 |
| `README.md` | thêm link vào tài liệu 12, update overview | Ngay |

Đề xuất: KHÔNG rewrite tài liệu cũ — giữ làm historical record. Tài liệu 12 (cái này) là source of truth khi có mâu thuẫn. Mỗi tài liệu cũ có thể thêm 1 dòng note ở đầu "*Cập nhật scope: xem 12_scope-decisions-v1.md*".

---

## 6. Verdict mới cho audit handoff (cập nhật `11_review-handoff-v1.md`)

Verdict cũ: **🔴 BLOCK** (3 vấn đề nền tảng — brand, scope cuts, distribution).

Verdict mới: **🟡 ACCEPT WITH CONDITIONS**

- ✅ Brand: APPROVED (D-1).
- ✅ Scope cuts: APPROVED (D-2).
- ⏳ Distribution + update workflow: PENDING D-3.5 — chờ project owner chọn phương án A/B/C.

Action items còn lại từ audit (cần resolve trước khi engineering bắt đầu Phase 2 UI shell):

| AI cũ | Trạng thái mới | Note |
|-------|----------------|------|
| AI-1 Chốt brand | ✅ DONE | StreamIX TV |
| AI-2 Khôi phục 4 module P0 | ❌ CANCELLED | Giữ scope cuts đã chốt |
| AI-3 Sửa giả định Play Store | ⚠️ PARTIAL | Internal APK chốt; update workflow cần D-3.5 |
| AI-4 Figma URL hoặc HTML interactive | ⏳ OPEN | Designer giao v1.1 |
| AI-5 Tokens 3 lớp | ⏳ OPEN | Designer giao v1.1 |
| AI-6 Bump typography sizes | ⏳ OPEN | **Quan trọng — body 22sp min** |
| AI-7 Thêm `ic_notification`, fix `ic_oauth_google` | ⚠️ PARTIAL | `ic_oauth_google` không cần (cắt OAuth); `ic_notification` vẫn cần nếu chọn Phương án C |
| AI-8 Bổ sung components còn thiếu | ⏳ OPEN | Recount sau khi cắt scope (có thể chỉ thiếu 4–6 component thay 10) |
| AI-9 Bổ sung screens còn thiếu | ⏳ OPEN | Bỏ S1a/S1c/S6/S7c/S7e/S8 khỏi danh sách thiếu; thêm verify S5b/e |
| AI-10 Prototype interactive | ⏳ OPEN | Designer giao v1.1 |
| AI-11 Tokens layer component | ⏳ OPEN | Defer V1.1 OK |

---

## 7. Risk mới phát sinh từ quyết định

Bổ sung vào `08_rui-ro-va-giam-thieu.md`:

### R-S1. UX login bằng D-pad email/password tệ

| L | I | Score |
|---|---|-------|
| 4 | 3 | **12 — CAO** |

**Mô tả**: Cắt QR + OAuth = user duy nhất gõ email + password bằng D-pad. Login trở thành rào cản lớn nhất ở first-run. Nguyên tắc N7 ("an toàn cho remote, không gõ keyboard dài") bị vi phạm.

**Mitigation**:
- C21 KeyboardOnScreen phải tối ưu cực kỳ tốt: layout T9-style, remember email gần nhất, suggest domain (`@fpt.com`, `@gmail.com`, `@fshare.vn` dạng button).
- "Nhớ tôi 30 ngày" mặc định ON, persist 90 ngày nếu được.
- Hướng dẫn rõ ràng "kết nối keyboard rời USB/Bluetooth" trên Login screen.
- Xem lại sau Phase 5 hardening — nếu metric login completion < 90% → bring back QR ở V1.1.

### R-S2. App không update tự động → version cũ tồn tại lâu

| L | I | Score |
|---|---|-------|
| 5 | 3 | **15 — CAO** |

**Mô tả**: Không có in-app update + không có Play Store = user bị stuck phiên bản đầu cài. Khi có security fix critical, không có cách push nhanh.

**Mitigation**:
- Phương án C (notification banner) ít nhất alert user.
- Không-cách phương án (Phương án A): IT phải có protocol rõ — Slack channel + email blast + 1-week reminder.
- App có **kill switch endpoint nhẹ**: nếu version trong blocklist từ server → app refuse khởi động với thông báo "Cập nhật ngay từ <URL>". Effort 1 ngày.

### R-S3. App không hoạt động offline

| L | I | Score |
|---|---|-------|
| 3 | 3 | **9 — TRUNG BÌNH** |

**Mô tả**: Bỏ Downloads = mất mạng → không xem được gì. Cảnh xem phim ở khách sạn / nhà nghỉ có Wi-Fi yếu → trải nghiệm tệ.

**Mitigation**:
- UI rõ ràng "Cần kết nối mạng" khi offline (S11 Global Error đã có).
- Buffer aggressive cho video stream để chịu được mạng dập dụng.
- V2 đánh giá lại có cần Download không.

---

## 8. Cập nhật README và mục lục

- Thêm tài liệu 12 vào README.
- Đánh dấu các tài liệu cũ "**có nội dung được override bởi 12**" mà không xoá để giữ historical context.

---

## 9. Tóm tắt action ngay

**Project Owner cần làm**:
1. **Quyết định D-3.5**: chọn phương án A / B / C cho update workflow (xem §3.3). Đề xuất Phương án C.
2. Confirm package name `vn.fshare.streamix.tv` hoặc đề xuất khác.

**PM cần làm**:
1. Communicate D-1 + D-2 + D-3 đến designer + engineering team.
2. Update lịch Phase 2/4 trong project plan: rút ~3 tuần.
3. Schedule D-3.5 decision meeting trong tuần này.

**Designer cần làm** (sau khi nhận communication):
1. Tiếp tục v1.1 với scope đã chốt.
2. Vẫn phải fix AI-4, AI-5, AI-6 (Figma access, tokens 3 lớp, typography bump).
3. Thêm logo / app icon / splash với brand StreamIX TV.

**Tech Lead cần làm**:
1. Phase 0 spike — build Hello APK trên Mi Box S + FPT Box.
2. Setup repo + CI cơ bản theo `02_khuyen-nghi-kien-truc.md`.
3. Convert tokens hiện tại sang Compose theme initial (sẽ refactor khi v1.1 về).
4. Bắt đầu Auth module backend (chỉ email/password, không OAuth/QR).
5. KHÔNG bắt đầu module Download / Update (đã cắt).

---

## 10. Sign-off

| Quyết định | Status | Ngày | Người duyệt |
|-----------|--------|------|-------------|
| D-1 Brand = StreamIX TV | ✅ APPROVED | 2026-05-04 | Project Owner |
| D-2 Cut 4 modules | ✅ APPROVED | 2026-05-04 | Project Owner |
| D-3 Internal APK distribution | ✅ APPROVED | 2026-05-04 | Project Owner |
| D-3.5 Update workflow A/B/C | ✅ APPROVED — Phương án A | 2026-05-04 | Project Owner |
| D-4 Package name = `vn.streamix.tv` | ✅ APPROVED | 2026-05-04 | Project Owner |

— Hết quyết định scope V1 —
