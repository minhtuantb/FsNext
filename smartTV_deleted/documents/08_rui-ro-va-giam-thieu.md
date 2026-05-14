---
title: 08 — Rủi ro & giảm thiểu
date: 2026-05-04
revision: 2 — cập nhật risks sau scope cuts ở 12
---

# 08. Rủi ro & giảm thiểu

> **⚠️ Cập nhật 2026-05-04**: Sau quyết định ở `12_scope-decisions-v1.md`, có 3 rủi ro mới phát sinh từ scope cuts. Một số rủi ro cũ đã bị xoá (do module liên quan bị cắt). Xem §8.0 dưới trước khi đọc full danh sách cũ.

## 8.0 Cập nhật rủi ro V1 (2026-05-04)

### Rủi ro mới phát sinh từ scope cuts

| ID | Rủi ro | L | I | Score | Owner | Status |
|----|--------|---|---|-------|-------|--------|
| **R-S1** | UX login bằng D-pad email/password tệ | 4 | 3 | **12 — CAO** | PM | Open |
| **R-S2** | App không update tự động (Phương án A) → version cũ tồn tại lâu | 5 | 3 | **15 — CAO** | PM + IT | Open |
| **R-S3** | App không hoạt động offline (cắt Downloads) | 3 | 3 | **9 — TRUNG BÌNH** | PM | Accepted |

#### R-S1 chi tiết

**Mô tả**: Cắt QR + OAuth = user duy nhất gõ email + password bằng D-pad. Login trở thành rào cản lớn nhất ở first-run. Nguyên tắc N7 ("an toàn cho remote") bị vi phạm.

**Mitigation**:
- C21 KeyboardOnScreen tối ưu cao: layout T9-style, remember email gần nhất, suggest domain (`@fpt.com`, `@gmail.com`, `@fshare.vn` button shortcuts).
- "Nhớ tôi 30 ngày" mặc định ON; consider 90 ngày nếu security cho phép.
- Hướng dẫn rõ "kết nối keyboard rời USB/BT" trên Login screen.
- Đo metric login completion rate ở Phase 5; nếu < 90% → bring back QR ở V1.1.

#### R-S2 chi tiết

**Mô tả**: Phương án A không có in-app update + không Play Store = user stuck phiên bản đầu cài. Khi có security fix critical, không có cách push nhanh.

**Mitigation**:
- Slack channel `#streamix-tv-releases` strictly enforced — mọi release post tại đây với template (xem `04_apk-va-update.md` rev 3 §4.6.1).
- Email blast bắt buộc cho list nhân viên có TV.
- Reminder mỗi 7 ngày nếu adoption < 70% (đo qua nginx log).
- Critical bug: hot-fix protocol < 4h từ phát hiện đến deploy (`04` §4.6.5).
- Pilot < 50 user → IT có thể gọi điện cá nhân hoá nếu cần.
- ⚠️ Chấp nhận: không có kill switch; user paranoid có thể chạy version 6 tháng cũ.
- V1.1 evaluation: nếu adoption < 50% sau 30 ngày → migrate sang Phương án C (notification banner).

#### R-S3 chi tiết

**Mô tả**: Bỏ Downloads = mất mạng → không xem được gì. Cảnh xem ở khách sạn / nhà nghỉ Wi-Fi yếu → trải nghiệm tệ.

**Mitigation**:
- UI rõ ràng "Cần kết nối mạng" (S11 đã có).
- Buffer aggressive cho stream để chịu mạng dập dụng.
- Status: **Accepted** — V1 chấp nhận trade-off này; V2 reconsider.

### Rủi ro cũ đã GIẢI QUYẾT / KHÔNG ÁP DỤNG sau scope cuts

| ID cũ | Tên | Lý do remove |
|-------|-----|--------------|
| R-T5 | Update flow gặp lỗi → app chết, user phải gỡ tay | Không còn update flow trong app |
| R-O3 | Tỷ lệ adopt update thấp → user chạy version cũ với bug | Replaced bởi R-S2 (chính xác hơn) |
| R-L1 (recommendations row) | Khuyến khích vi phạm bản quyền | Designer đã loại bỏ public recommendations từ V1 — risk giảm xuống mức TRUNG BÌNH |

### Rủi ro cũ vẫn còn nguyên giá trị

R-T1 (ExoPlayer codec lạ), R-T2 (TV Box VN cũ), R-T3 (stream URL TTL), R-T4 (download engine port — **không còn áp dụng vì cắt download**), R-O1 (CDN sập), R-O2 (Fshare API breaking), R-O4 (support ticket), R-T6 (mất signing key — đặc biệt nghiêm trọng với Phương án A), R-P1 (user ngại sideload — vẫn rất relevant), R-P2 (cannibalize), R-P3 (mạng VN yếu).

### Bảng ưu tiên risk V1 cập nhật

| ID | Rủi ro | Score |
|----|--------|-------|
| R-L1 | Pháp lý bản quyền (giảm sau scope cut) | 16 → ~12 |
| R-T1 | ExoPlayer không phát hết MKV | 16 |
| R-P1 | User ngại sideload | 16 |
| **R-S2** | Update adoption thấp (Phương án A) | **15 (mới)** |
| R-T2 | TV Box VN cũ | 12 |
| R-O2 | Fshare API breaking | 12 |
| R-O4 | Support ticket tăng | 12 |
| **R-S1** | UX login D-pad email/password | **12 (mới)** |
| R-T6 | Mất signing key (đặc biệt Phương án A) | 10 |
| R-T3 | Stream URL TTL | 9 |
| **R-S3** | Không offline | **9 (mới)** |
| R-P3 | Mạng VN yếu | 9 |
| R-O1 | CDN sập | 8 |
| R-L2 | Play Protect flag | 6 (giảm — không Play Protect concern khi không submit Play Store) |
| R-P2 | Cannibalize | 6 |

3 rủi ro hàng đầu để focus mitigate trong Phase 5: **R-T1** (ExoPlayer), **R-S2** (update adoption), **R-P1** (sideload UX).

---

## 8.1 Phần đầu (rev 1) giữ nguyên cho historical reference

Đánh giá theo ma trận: **Likelihood (L)** và **Impact (I)** trên thang 1–5; **Score = L×I**. Mọi rủi ro Score ≥ 12 cần kế hoạch giảm thiểu rõ ràng và owner xác định.

## 8.1 Rủi ro pháp lý & nội dung

### R-L1. Bị xem là "khuyến khích vi phạm bản quyền"

| Trường | Giá trị |
|--------|---------|
| Likelihood | 4 |
| Impact | 5 |
| Score | **20 — CAO** |

**Mô tả**: Fshare cũng như nhiều cloud storage VN có nội dung do user upload, một phần là phim/nhạc bản quyền. Nếu app TV làm UX phát phim quá tiện (Continue Watching, Recommendations), Fshare/app dễ bị phía sở hữu bản quyền (Hollywood, K+, FPT Play, BHD…) khiếu nại với lý do "TV app vận hành như một dịch vụ streaming lậu".

**Mitigation**:
- Pháp lý: rà soát ToS Fshare → đảm bảo TV app chỉ là client truy cập file của chính user; KHÔNG khám phá public/index file người khác.
- Sản phẩm: **không có search public**, không có "trending", không suggest file của user khác. App là personal cloud client — đúng phạm vi.
- Mặc định **TURN OFF** Recommendations row trên launcher trong V1; bật sau khi có ý kiến pháp lý.
- DMCA-style takedown process song song với Fshare backend.

### R-L2. Phân phối ngoài Play Store có thể bị Google flag

| L | I | Score |
|---|---|-------|
| 2 | 3 | **6 — TRUNG BÌNH** |

**Mô tả**: Một số phiên bản Android TV (đặc biệt Google TV mới) chặn cài app từ unknown source mạnh hơn; có thể hiện cảnh báo "Harmful app" qua Play Protect.

**Mitigation**:
- Submit app lên Play Console (free) với target Android TV — không cần publish public, chỉ để Play Protect đánh dấu signature là known.
- Đặt domain `tv-update.fshare.vn` reputation tốt: HTTPS, TLS 1.3, không chứa nội dung khác.
- Hướng dẫn user disable Play Protect cho app này trong tài liệu.

## 8.2 Rủi ro kỹ thuật

### R-T1. ExoPlayer không phát được nhiều file MKV của user

| L | I | Score |
|---|---|-------|
| 4 | 4 | **16 — CAO** |

**Mô tả**: User Fshare lưu rất nhiều file MKV với codec đa dạng (HEVC 10-bit, AV1, RMVB, AC3/DTS). ExoPlayer Media3 không cover hết.

**Mitigation**:
- Bắt buộc tích hợp libVLC-Android làm fallback từ V1 (thay vì roadmap V1.1).
- Auto-detect: ExoPlayer raise `ERROR_CODE_DECODER_INIT_FAILED` → tự switch libVLC mà không hỏi user.
- Test ma trận 30+ file mẫu **trước** beta.
- Cảnh báo "Định dạng có thể không hỗ trợ — hãy thử engine VLC trong Settings" khi cả hai engine fail.

### R-T2. TV Box Việt cũ (Android 5/6) thiếu API

| L | I | Score |
|---|---|-------|
| 3 | 4 | **12 — CAO** |

**Mô tả**: FPT Play Box, MyTV Box phổ biến nhưng nhiều bản chạy Android 5.1/6.0. `PackageInstaller` API behavior khác, `REQUEST_INSTALL_PACKAGES` không tồn tại (Android 8+), Compose-TV minSdk 21 nhưng có thể bug.

**Mitigation**:
- Phase 0 spike trên đúng các thiết bị này — confirm sớm.
- Có code path khác cho Android 5/6: dùng `Intent.ACTION_INSTALL_PACKAGE` (đã deprecated nhưng còn chạy) thay vì `PackageInstaller`.
- Nếu thiết bị quá cũ không thể support → cắt khỏi danh sách target chính thức, ghi rõ trong FAQ.

### R-T3. Stream URL của Fshare có TTL ngắn, không hỗ trợ HLS

| L | I | Score |
|---|---|-------|
| 3 | 3 | **9 — TRUNG BÌNH** |

**Mô tả**: Stream URL hết hạn giữa chừng phim → 403; không có ABR.

**Mitigation**:
- Logic tự refresh URL khi 403 → `setMediaItem` mới + `seekTo(currentPosition)` (đã ghi ở [05](05_player-media-nhung.md)).
- Đề xuất backend Fshare: tăng TTL stream URL lên ≥ 24h, hoặc thêm refresh endpoint trả URL mới mà không phải gọi lại getDownloadUrl.
- V2: backend cung cấp HLS manifest cho file đã transcode → ABR.

### R-T4. Multi-segment download port lại Kotlin có bug

| L | I | Score |
|---|---|-------|
| 3 | 3 | **9 — TRUNG BÌNH** |

**Mô tả**: Port từ C++ `DownloadEngine` (10 năm tuned) sang Kotlin có thể tạo regressions: race condition, memory leak với coroutines, retry/backoff khác.

**Mitigation**:
- Reuse unit test logic: port cả 4 test (`test_budget_manager`, `test_speed_meter`, `test_filename_sanitizer`, `test_fshare_url`).
- Test parity: tải 1 file 4 GB so sánh thời gian và checksum giữa desktop và TV.
- Bật detail log dev: số segment, retry count, throughput từng segment.
- Đệm 1 tuần buffer trong Phase 4 cho download tuning.

### R-T5. Update flow gặp lỗi lỗi cài → app chết, user phải gỡ tay

| L | I | Score |
|---|---|-------|
| 2 | 5 | **10 — CAO** |

**Mô tả**: APK mới có bug fatal → user mở app crash → không có update mới để fix vì app crash trước khi check.

**Mitigation**:
- Rollback tự động: detect crash 3 lần liên tiếp trong 60s → install version cũ. Cần lưu link version trước trong manifest.
- "Safe mode" launch: bấm giữ Back 5 giây khi app boot → vào safe mode chỉ chạy update screen, không load full UI.
- Background update check trong WorkManager — chạy ngay cả khi user chưa mở app, để đẩy hot-fix nhanh.
- Beta channel + chỉ ~5% user → soft launch trước khi đẩy stable cho 100%.

### R-T6. APK signing key bị mất

| L | I | Score |
|---|---|-------|
| 2 | 5 | **10 — CAO** |

**Mô tả**: Mất keystore = không bao giờ update được app cho user hiện tại nữa. Phải release APK ký key mới → user phải gỡ và cài lại, mất hết data local (history, resume position).

**Mitigation**:
- Backup keystore: ít nhất 3 bản — USB key trong két leadership + 1 bản encrypted lưu cloud (1Password/Bitwarden) + 1 bản giấy ghi alias/password.
- Sử dụng **APK Signature Scheme v3** với key rotation từ ngày 1 → có thể rotate key tương lai.
- Document rõ access control: ai có quyền dùng keystore, log mỗi lần ký.

## 8.3 Rủi ro vận hành

### R-O1. CDN sập / DDoS đúng lúc release

| L | I | Score |
|---|---|-------|
| 2 | 4 | **8 — TRUNG BÌNH** |

**Mitigation**:
- Multi-CDN: primary CloudFlare, fallback BunnyCDN. Manifest chứa cả 2 URL, client thử lần lượt.
- Manifest cache TTL ngắn (5 phút), APK cache TTL 24h. Cập nhật được nhanh.
- Smoke test pre-release: download manifest + APK từ 3 vùng địa lý.

### R-O2. Fshare backend đổi schema API breaking

| L | I | Score |
|---|---|-------|
| 3 | 4 | **12 — CAO** |

**Mô tả**: Backend Fshare đổi response format → app TV crash hoặc parse fail trên hàng nghìn thiết bị. Bản desktop có thể update dễ qua Windows installer; TV update chậm hơn (user phải accept update).

**Mitigation**:
- API contract test trên CI: gọi tất cả endpoint thật mỗi sáng, check schema.
- Coordination với backend team: 2 tuần notice trước mọi breaking change.
- Versioning: app gửi `X-Client-Version` header → backend hỗ trợ legacy schema cho version cũ ít nhất 90 ngày.
- Lenient parsing: dùng Moshi với `@JsonClass(generateAdapter = true)` + default values cho field optional → field lạ không crash.

### R-O3. Tỷ lệ adopt update thấp → user chạy version cũ với bug

| L | I | Score |
|---|---|-------|
| 4 | 3 | **12 — CAO** |

**Mô tả**: User TV ít chủ động update. Stuck version 1.0 dù 1.2 đã có 2 tháng.

**Mitigation**:
- Force update threshold: `minSupportedVersionCode` đẩy cao mỗi 60 ngày → version quá cũ (3 release) bắt buộc update.
- Update prompt rõ ràng: "Cập nhật để xem được phim mới" — message functional, không kỹ thuật.
- Theo dõi `version distribution` trong analytics, đặt KPI 80% lên latest sau 7 ngày.

### R-O4. Support ticket tăng đột biến

| L | I | Score |
|---|---|-------|
| 4 | 3 | **12 — CAO** |

**Mô tả**: User không tự sửa được vì TV không có dev tools. Mỗi vấn đề thành ticket.

**Mitigation**:
- In-app log viewer + "Gửi log" 1 nút: gửi 200 dòng cuối + device info đến support email/portal. Giảm 70% ticket "không thể repro".
- FAQ chi tiết tiếng Việt: 20 câu phổ biến nhất (cách cài, cách login, không xem được phim, mất tiếng…).
- Video hướng dẫn 60s cho từng dòng TV.
- Beta channel để filter bug trước khi tới mass user.

## 8.4 Rủi ro sản phẩm / chiến lược

### R-P1. User không hiểu phải sideload, ngại không cài

| L | I | Score |
|---|---|-------|
| 4 | 4 | **16 — CAO** |

**Mitigation**:
- Trang `tv.fshare.vn` thiết kế cực kỳ đơn giản, có bước → bước.
- Companion app mobile (Fshare app đang có) có chức năng "Cài lên TV" — push APK qua mạng nội bộ. UX tốt nhất.
- Video demo cài đặt 60s trên YouTube/Facebook Fshare.
- Tape/banner trong app mobile: "Có TV thông minh? Cài Fshare TV ngay".

### R-P2. Bản TV cannibalize bản desktop / không justify cost

| L | I | Score |
|---|---|-------|
| 2 | 3 | **6 — TRUNG BÌNH** |

**Mitigation**:
- Định vị rõ: TV là tiêu thụ (xem), desktop là quản lý (upload, share, ops). Hai personas khác nhau.
- Đo metric riêng: DAU TV, watch time TV, retention TV.
- Phase 0–6 đã structured để break early nếu phase 0 cho thấy cost vượt benefit.

### R-P3. Stream chất lượng kém vì mạng VN không ổn định

| L | I | Score |
|---|---|-------|
| 3 | 3 | **9 — TRUNG BÌNH** |

**Mitigation**:
- Buffer aggressive: prebuffer 15s, max buffer 50s.
- Cảnh báo trước khi mở: nếu speedtest < 5 Mbps → "Mạng yếu, có thể bị giật".
- V2: ABR/HLS giải quyết tận gốc.

## 8.5 Bảng tóm tắt theo độ ưu tiên

| ID | Rủi ro | Score | Owner | Status |
|----|--------|-------|-------|--------|
| R-L1 | Khuyến khích vi phạm bản quyền | 20 | Legal + PM | Open |
| R-T1 | ExoPlayer không phát hết MKV | 16 | Tech Lead | Open |
| R-P1 | User ngại sideload | 16 | PM + Marketing | Open |
| R-T2 | TV Box VN cũ thiếu API | 12 | Tech Lead | Open — Phase 0 verify |
| R-O2 | Fshare API breaking | 12 | Tech Lead + Backend | Open |
| R-O3 | Update adoption thấp | 12 | PM | Open |
| R-O4 | Support ticket tăng | 12 | Support + Tech | Open |
| R-T5 | Update flow tự brick | 10 | Tech Lead | Mitigated (rollback designed) |
| R-T6 | Mất signing key | 10 | DevOps + Sec | Mitigated (3-bản backup) |
| R-T3 | Stream URL TTL ngắn | 9 | Tech Lead + Backend | Mitigated (auto-refresh) |
| R-T4 | Bug khi port download engine | 9 | Tech Lead | Mitigated (test parity) |
| R-P3 | Mạng VN yếu cho stream | 9 | PM | Open |
| R-O1 | CDN sập | 8 | DevOps | Mitigated (multi-CDN) |
| R-L2 | Play Protect flag | 6 | DevOps | Open |
| R-P2 | Cannibalize / cost không justify | 6 | PM | Open |

## 8.6 Quy trình review rủi ro

- **Weekly**: review trong stand-up Phase 0–4.
- **Bi-weekly** sau Phase 5: stakeholder review, đóng/mở rủi ro mới.
- **Trước M5 (public release)**: mọi rủi ro Score ≥ 12 phải có status "Mitigated" hoặc "Accepted with documented compensating control".

## 8.7 Kết luận chương 8

3 rủi ro cao nhất cần action ngay từ Phase 0:

1. **R-L1** (pháp lý): rà ToS, không expose public file → cần PM + Legal review trước Phase 1.
2. **R-T1** (ExoPlayer): bắt buộc plan libVLC fallback từ V1 → impact Phase 3 timeline (đã đệm).
3. **R-P1** (sideload UX): trang `tv.fshare.vn` + companion app push APK → cần coordination với mobile team từ Phase 4.

Các rủi ro còn lại có mitigation chuẩn industry — theo dõi qua weekly review.

— Hết bộ tài liệu phân tích & khuyến nghị Smart TV.
