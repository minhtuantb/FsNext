---
title: 15 — Readiness Checklist trước khi bắt đầu code
date: 2026-05-05
status: 🟡 90% READY — còn 4 P0 blocker + 6 P1 coordination
audience: Project Owner, PM, Tech Lead, Backend Lead, Designer
---

# 15. Readiness Checklist — đã đủ chưa?

## 0. Verdict tổng

**🟡 90% READY** — đủ thông tin để Tech Lead **bắt đầu Phase 0 + Phase 1 ngay hôm nay**, NHƯNG có **4 blocker P0** sẽ chặn các phase sau nếu không resolve trong 2 tuần tới.

| Phase | Có thể bắt đầu? | Blocker |
|-------|----------------|---------|
| Phase 0 — Setup & spike | ✅ NGAY | (none) |
| Phase 1 — Core layer | ✅ NGAY | App key format cần backend confirm trong tuần đầu |
| Phase 2 — Auth + Browse + Home | ✅ ~Tuần 4 | Designer ship v1.1 trước Tuần 4 |
| Phase 3 — Player + Search | ⚠️ ~Tuần 7 | **Backend ship Stream URL endpoint** trước Tuần 7 |
| Phase 4 — Settings + Distribute | ✅ ~Tuần 10 | nginx server + domain `tv.streamix.vn` setup |
| Phase 5 — Hardening + Beta | ⚠️ ~Tuần 12 | Test devices đã mua/mượn |
| Phase 6 — V1 Public | ✅ ~Tuần 15 | Slack channel + email list |

---

## 1. Coverage matrix — cái gì đã có, cái gì chưa

| Aspect | Coverage | Document | Trạng thái |
|--------|----------|----------|-----------|
| **Vision & scope** | 100% | 12 | ✅ Chốt 4 quyết định D-1/2/3.5/4 |
| **Stack lựa chọn** | 100% | 03 | ✅ Kotlin + Compose-TV + Media3 |
| **Architecture** | 100% | 02 | ✅ MVVM + Clean Architecture, Hilt, multi-module |
| **API contracts** | 80% | 13 §3 | ⚠️ Stream URL chưa có; refresh + logout là TODO trong gateway |
| **Auth/token model** | 100% | 13 §4.1 | ✅ JWT + refresh + Authenticator code đầy đủ |
| **Per-screen spec** | 95% | 13 §5–23 | ✅ 18 screens × 10 mục mỗi cái |
| **Design system tokens** | 80% | 09 + package v1.0 | ⚠️ v1.0 đã accept, cần v1.1 fix typography + tokens 3 lớp |
| **Component library** | 60% | 09 §8 + package v1.0 | ⚠️ Designer mới làm 14/24 component |
| **Screen catalog** | 100% | 14 | ✅ 80 frames identified |
| **Icons** | 95% | package + 3 icon mới | ✅ 48 icon (45 từ designer + 3 tự tạo); thiếu logo + app icon |
| **Brand assets (logo/splash/banner)** | 0% | — | ❌ **Designer chưa giao** — pending v1.1 |
| **APK distribution** | 100% | 04 rev3 | ✅ Phương án A đã spec đầy đủ |
| **CI/CD** | 100% | 04 §4.7 | ✅ Github Actions yaml mẫu |
| **Localization** | 100% | package localization.csv | ✅ 208 keys vi + en |
| **Motion specs** | 100% | package motion-specs.md | ✅ 10 animations |
| **Roadmap** | 100% | 07 rev2 | ✅ 6 phase, 11.5 tuần |
| **Risks** | 100% | 08 rev2 | ✅ 14 risks + mitigation |
| **User flows** | 100% | 09 §10 | ✅ 6 flows |
| **Test strategy** | 50% | 13 + 14 | ⚠️ QA cần viết test plan chi tiết riêng |
| **Performance budgets** | 100% | 13 §27 | ✅ Cold start, frame rate, memory limits |
| **Accessibility plan** | 60% | 09 §11 | ⚠️ Cần a11y audit từ designer ở v1.1 |

---

## 2. 🔴 4 P0 Blockers — phải resolve trước phase tương ứng

### B1. Stream URL endpoint chưa định nghĩa (Block Phase 3)

**Vấn đề**: Player (S5) cần endpoint `POST /api/v1/sessions/download` để lấy URL stream từ `linkcode`. API gateway hiện tại chỉ ghi tag "Session" trong README nhưng **chưa có path nào trong `openapi.yaml`**, file `v1/session.md` chưa được tạo.

**Tác động**: Phase 3 không thể bắt đầu module Player nếu không có endpoint thật để gọi.

**Action**:
- PM tạo Jira ticket cho Backend Lead với spec đề xuất (xem `13` §10.0.4):
  - Path: `POST /api/v1/sessions/download`
  - Body: `{linkcode: string, password?: string}`
  - Response: `{data: {url, expires_in, expires_at}, request_id}`
  - TTL khuyến nghị ≥ 6h
- ETA backend: ship trước Tuần 6 (để engineering có 1 tuần test trước Phase 3 bắt đầu Tuần 7).
- Workaround tạm: engineer mock interface trong Phase 1, swap implementation khi backend ready.

**Owner**: Backend Lead | **Deadline**: Tuần 6

### B2. Designer ship v1.1 (Block Phase 2)

**Vấn đề**: Audit `11` đã accept v1.0 với conditions. Còn các fix:
- Typography bump (body 22sp min, restore headline 36, label-lg 22)
- Tokens 3 lớp primitive/semantic/component
- Logo + app icon adaptive + splash + banner StreamIX TV (chưa có)
- 10 components còn thiếu visualize (BottomSheet, TextField, Switch, RadioGroup, KeyboardOnScreen, …)
- 80 frames Figma đầy đủ states (xem `14`)
- Figma URL với Dev Mode, hoặc HTML mockup interactive trong package
- Prototype 6 user flows click-through

**Tác động**: Engineering có thể bắt đầu Phase 2 với token v1.0 hiện tại nhưng sẽ phải refactor khi v1.1 về. Càng để lâu càng tốn rework.

**Action**:
- PM communicate audit `11` action items + spec catalog `14` đến designer ngay.
- Designer self-checklist 60 items trong `10 §18`.
- Re-issue meeting cuối Tuần 3.

**Owner**: Designer | **Deadline**: Tuần 3 (để Engineering Phase 2 không phải rework)

### B3. nginx server + domain `tv.streamix.vn` setup (Block Phase 4)

**Vấn đề**: Phương án A cần server tĩnh để host APK + trang `install.html`. Hiện tại chưa có:
- DNS `tv.streamix.vn` (hoặc fallback `tv-update.fshare.local` nội bộ)
- SSL cert (Let's Encrypt hoặc cert nội bộ)
- nginx config theo `04` §4.4.1
- SSH key pair cho CI deploy

**Tác động**: Phase 4 (build/sign/distribute) không complete được nếu không có server.

**Action**:
- DevOps lead provision server + domain + SSL.
- Setup nginx theo template `04` §4.4.1.
- Tạo DEPLOY_KEY + add vào Github Secrets.

**Owner**: DevOps Lead | **Deadline**: Tuần 9 (trước Phase 4 Tuần 10)

### B4. Signing keystore (Block release đầu tiên)

**Vấn đề**: Chưa tạo keystore release. Nếu lần đầu cài bằng debug key, sau release đổi sang release key → user phải gỡ + cài lại (mất history).

**Tác động**: Mỗi build debug → release switch là 1 lần force user reinstall.

**Action**:
- Tech Lead tạo keystore release theo `04` §4.3.1 ngay Tuần 1.
- Backup theo `04` §4.3.3 (USB encrypted + 1Password + giấy in fingerprint).
- Add vào CI Secrets ngay Tuần 1 — mọi APK debug từ giờ ký bằng release key.

**Owner**: Tech Lead | **Deadline**: Tuần 1 (Phase 0)

---

## 3. 🟡 6 P1 Coordination items — cần làm song song

### C1. App key format cho TV

**Cần confirm với backend**:
- Dùng cùng desktop key `dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` (D-4 đã chốt)?
- HOẶC backend cấp prefix platform `tv_v1_<random>`?
- HOẶC rotation key riêng?

**Action**: Tech Lead + Backend Lead meeting 30 phút Tuần 1.

### C2. /me response thiếu traffic field

**Vấn đề**: Spec S7a có ô "Traffic tháng" nhưng `GET /api/v1/me` chỉ trả `storage_used/storage_quota`, không có `traffic_used/traffic_quota`.

**3 lựa chọn**:
1. Backend extend `/me` thêm traffic fields (~0.5 ngày backend).
2. App gọi endpoint khác (chưa có) chỉ để lấy traffic.
3. Bỏ ô traffic ở S7a — chỉ giữ webspace.

**Khuyến nghị**: option 1 (extend /me) — clean nhất.

**Owner**: Backend Lead | **Deadline**: Tuần 4

### C3. Backend endpoints TODO trong gateway

File `v1/auth.md` có 6 endpoints đánh dấu **🚧 TODO**:
- `POST /api/v1/auth/refresh` (V1 dùng)
- `POST /api/v1/auth/logout` (V1 dùng)
- `POST /api/v1/auth/register` (V1 không dùng)
- `POST /api/v1/auth/oauth/{provider}` (V1 cắt)
- `POST /api/v1/auth/password/reset` (V1 không dùng)
- `PATCH /api/v1/auth/password` (V1 không dùng)

**Action**: Backend ưu tiên ship `refresh` + `logout` trước Tuần 4 (để Auth interceptor có thật endpoint thay vì mock).

**Owner**: Backend Lead | **Deadline**: Tuần 4

### C4. Test devices

**Cần có**:
- Xiaomi Mi Box S (~1.5 tr)
- FPT Play Box S (~1 tr)
- TCL Android TV 43" (~6 tr) — hoặc emulator AOSP TV nếu mượn không được
- 1 USB OTG hub (~200k) — không cần V1 vì cắt Downloads

**Action**: PM purchase order ngay Tuần 1 để Phase 0 spike không bị block.

**Owner**: PM | **Deadline**: Tuần 1

### C5. Slack channel + email list (cho Phương án A)

Phương án A phụ thuộc vào communication protocol:
- Tạo Slack channel `#streamix-tv-releases` (announcement-only)
- Tạo Slack channel `#streamix-tv-support` (Q&A)
- Tạo email list `streamix-tv-users@fpt.com.vn`
- Template release message + email blast (xem `04` §4.6.1)

**Owner**: PM + IT | **Deadline**: Tuần 14 (trước Phase 6)

### C6. Brand legal sign-off (formal)

**Vấn đề**: User đã chốt brand "StreamIX TV" và package `vn.streamix.tv`. Tuy nhiên cần formal:
- Marketing approval cho name + logo
- Legal review trademark "StreamIX" có conflict không
- Domain `streamix.vn` (hoặc `tv.streamix.vn`) đã đăng ký chưa
- Nếu trademark conflict → có thể phải đổi tên giữa chừng (rủi ro lớn)

**Owner**: PM + Legal | **Deadline**: Tuần 2 (sớm để có time react nếu fail)

---

## 4. 🟢 P2 — có thể resolve sau, không block

| ID | Item | Khi nào |
|----|------|---------|
| P1 | A11y audit từ designer | Phase 5 |
| P2 | Audio passthrough test ma trận thiết bị | Phase 5 |
| P3 | Vietnamese/English translation review từ native speaker | Phase 5 |
| P4 | Performance benchmark trên Mi Box S | Phase 5 |
| P5 | DMCA takedown process tích hợp với Fshare backend | Phase 6+ |
| P6 | Crashlytics threshold + alerting setup | Phase 5 |
| P7 | Beta channel infrastructure (nếu V1.1 cần) | V1.1 |

---

## 5. Action plan — next 2 tuần

### Tuần 1 (week of 2026-05-04)

**Tech Lead**:
- [ ] Tạo repo `streamix-tv/` package `vn.streamix.tv`
- [ ] Setup Gradle + Convention plugin + Hilt
- [ ] Hello APK build + cài Mi Box S (cần PM mua trước)
- [ ] Tạo signing keystore release + backup 3 nơi (B4)
- [ ] Setup CI Github Actions cơ bản (debug build only, sign chưa cần)
- [ ] Convert tokens v1.0 hiện tại → Compose theme initial
- [ ] Convert 48 SVG (45 từ designer + 3 mới) sang VectorDrawable

**PM**:
- [ ] Communicate audit `11` + scope cuts `12` đến designer (B2)
- [ ] Mua test devices: Mi Box S + FPT Play Box (C4)
- [ ] Schedule meeting Backend cho B1 (Stream URL) + C1 (App key) + C2 (/me traffic) + C3 (refresh/logout)
- [ ] Brand legal review (C6)

**Backend Lead**:
- [ ] Confirm app key format cho TV (C1)
- [ ] Bắt đầu spec stream URL endpoint (B1)

**Designer**:
- [ ] Bắt đầu fix v1.1 theo audit `11` action items + catalog `14`

### Tuần 2 (week of 2026-05-11)

**Tech Lead**:
- [ ] `:core:domain` — port models từ desktop sang Kotlin
- [ ] `:core:network` — Retrofit + AuthInterceptor + AppKeyInterceptor
- [ ] `:core:data` — Room schema (user_cache, file_cache, playback_position) + DataStore
- [ ] Wire Hilt modules
- [ ] Instrumented test login + getUserInfo (mock backend hoặc dev environment)

**Backend Lead**:
- [ ] Ship refresh + logout endpoints (C3)
- [ ] Ship stream URL endpoint draft (B1)

**Designer**:
- [ ] Ship preview v1.1: tokens 3 lớp + typography bump
- [ ] Schedule R1 review meeting Tuần 3

**PM**:
- [ ] Confirm brand approval (C6)
- [ ] Track test devices delivery
- [ ] Setup Jira/Linear board với 80 ticket per screen từ catalog `14`

---

## 6. Sign-off matrix — trước khi engineering bắt đầu Phase 2

| Stakeholder | Cần sign-off cái gì | Deadline |
|-------------|---------------------|----------|
| Project Owner | Toàn bộ scope V1 (đã chốt 12) | ✅ DONE |
| PM | Lộ trình + budget + test device PO | Tuần 1 |
| Tech Lead | Tài liệu 13 implementation spec | Tuần 1 |
| Backend Lead | API contracts (xác nhận endpoints + ship deadline) | Tuần 2 |
| Designer | Audit 11 action items + commitment ship v1.1 | Tuần 1 |
| QA Lead | Test plan dựa trên catalog 14 | Tuần 3 |
| Legal | Brand "StreamIX TV" trademark check | Tuần 2 |
| DevOps | Server + domain + SSL plan | Tuần 4 |

---

## 7. Còn cần trao đổi gì với tôi (Tech Lead) không?

Để tôi có thể proceed code mà không phải hỏi giữa chừng:

### 🟢 Đã đủ — không cần hỏi

- Tech stack ✅
- Architecture ✅
- Per-screen spec ✅
- API contracts (trừ stream URL) ✅
- D-pad mapping ✅
- Tokens & components (v1.0) ✅
- Distribution Phương án A ✅
- Roadmap 11.5 tuần ✅

### 🟡 Có thể cần làm rõ thêm khi bắt đầu code

| Câu hỏi tiềm năng | Khi nào sẽ hỏi |
|-------------------|----------------|
| C1 — App key prefix? | Tuần 1, trước khi build AuthInterceptor |
| Database migration strategy nếu schema đổi V1 → V1.1 | Phase 1 |
| Dark theme có phải variant duy nhất? Có hỗ trợ light V2? | Phase 2, design-driven |
| Continue Watching cross-device sync hay chỉ local? | Phase 3 |
| Yêu thích persist server hay chỉ local? | Phase 2 |
| Onboarding có optional skip-able không? | Phase 2 (đã spec là CÓ skip nhưng cần PM confirm) |
| File pwd protection: server check trước khi cấp stream URL? | Phase 3 |
| Logout: invalidate refresh_token family cả các device khác hay chỉ device hiện tại? | Phase 4 |

→ Tôi sẽ batch các câu hỏi này lại theo phase và hỏi PM 1 lần / phase thay vì rải rác.

### 🔴 Có thể cần Project Owner quyết tiếp nếu phát hiện trong Phase 5 testing

| Tình huống | Cần Owner quyết |
|-----------|-----------------|
| Login completion rate < 90% | Bring back QR ở V1.1? (cancel D-2 cut?) |
| Update adoption < 50% sau 30 ngày | Switch sang Phương án C (notification banner)? |
| 30%+ video MKV không play được trên ExoPlayer | Bring back libVLC fallback ngay V1 thay vì V1.1? |
| TV Box VN cũ Android 5.x crash | Bump minSdk = 23 (cắt support TV cũ)? |

---

## 8. Tóm tắt 1 dòng

> **Đủ thông tin để Tech Lead khởi động Phase 0–1 ngay tuần này. 4 P0 blocker không chặn lúc start nhưng phải đóng trước Tuần 6 (Stream URL) và Tuần 3 (designer v1.1). 2 tuần đầu chủ yếu là setup + coordination với Backend/Designer/Legal — engineering thực sự bắt đầu output code visible từ Tuần 3.**

— Hết readiness checklist —
