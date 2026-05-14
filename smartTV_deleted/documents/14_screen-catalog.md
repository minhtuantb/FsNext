---
title: 14 — Catalog toàn bộ màn hình, states, dialogs (StreamIX TV V1)
date: 2026-05-05
version: 1.0
status: Reference — đọc song song với `13_implementation-spec-v1.md`
audience: Designer, QA, PM, Engineering
---

# 14. Catalog toàn bộ màn hình

Tài liệu này **liệt kê đầy đủ** mọi màn hình, sub-screen, dialog, overlay, state, snackbar trong StreamIX TV V1. Format compact (1 dòng / item) để scan nhanh và dùng làm checklist.

Mỗi item có ID duy nhất. Tài liệu 13 spec chi tiết các "screens chính"; tài liệu này bổ sung **mọi state variant + dialog + snackbar** mà spec 13 không list riêng.

---

## 0. Tóm tắt số liệu

| Loại | Số lượng V1 | Số lượng V2+ |
|------|-------------|--------------|
| Root screens | 9 | 0 |
| Sub-screens (Settings nested) | 4 | 0 |
| Onboarding steps | 2 | (cắt 1) |
| Player overlays/sheets/dialogs | 4 | (cắt S5e) |
| Global dialogs (template-based) | 7 instances | — |
| Global overlays | 3 | — |
| State variants (theo screen) | 47 | — |
| Snackbar/Toast types | 4 | — |
| **Tổng artifacts cần thiết kế** | **~80 frame Figma** | — |

So với spec gốc `09_design-spec.md` đã có 27 screens; sau quyết định cắt 4 module (`12`) còn **18 screens chính + state variants**. Tài liệu này expand đầy đủ.

---

## 1. Root screens (9)

Là màn hình full-screen có URL trong navigation graph. Mỗi root có riêng route, riêng ViewModel, riêng test suite.

| ID | Tên | Route | Module Gradle | Spec § | Status |
|----|-----|-------|---------------|--------|--------|
| **S0** | Splash | `Routes.Splash` | `:app` | 13 §5 | ✅ V1 |
| **S1** | Login (email + password) | `Routes.Login` | `:feature:auth` | 13 §6 | ✅ V1 |
| **S2** | Home | `Routes.Home` | `:feature:home` | 13 §7 | ✅ V1 |
| **S3** | Browse Folder | `Routes.Browse(folderPath, folderName)` | `:feature:browse` | 13 §8 | ✅ V1 |
| **S4** | File Detail | `Routes.FileDetail(linkcode)` | `:feature:browse` | 13 §9 | ✅ V1 |
| **S5** | Player Full-screen | `Routes.Player(linkcode, resumePos?)` | `:feature:player` | 13 §10 | ✅ V1 |
| **S7** | Settings Hub | `Routes.Settings` | `:feature:settings` | 13 §15 | ✅ V1 |
| **S9** | Onboarding | `Routes.Onboarding` | `:feature:onboarding` | 13 §20 | ✅ V1 |
| **S12** | Search | `Routes.Search(initialQuery?)` | `:feature:search` | 13 §23 | ✅ V1 |

**Cắt khỏi V1** (đã quyết định ở `12_scope-decisions-v1.md`):
- ~~S1a~~ Login QR
- ~~S1c~~ Login OAuth Device-Flow
- ~~S6~~ Downloads (file đã tải về local)
- ~~S8~~ Update prompts (4 variants)

---

## 2. Sub-screens (4) — Settings nested

Là full-screen nhưng đường dẫn navigation đi qua S7 Settings Hub. Mỗi sub-screen là 1 entry trong list của S7.

| ID | Tên | Route | Spec § | Status |
|----|-----|-------|--------|--------|
| **S7a** | Settings → Tài khoản | `Routes.SettingsSub(Section.Account)` | 13 §16 | ✅ V1 |
| **S7b** | Settings → Phát video | `Routes.SettingsSub(Section.Playback)` | 13 §17 | ✅ V1 |
| **S7d** | Settings → Mạng (info-only) | `Routes.SettingsSub(Section.Network)` | 13 §18 | ✅ V1 |
| **S7f** | Settings → Giới thiệu | `Routes.SettingsSub(Section.About)` | 13 §19 | ✅ V1 |

**Cắt khỏi V1**:
- ~~S7c~~ Settings → Tải về (vì cắt Downloads)
- ~~S7e~~ Settings → Cập nhật (vì Phương án A manual install)

---

## 3. Player overlays / sheets / dialogs (4)

Là UI overlay phủ trên S5 Player. Không có route riêng, được control bằng PlayerViewModel state.

| ID | Tên | Loại | Trigger | Spec § | Status |
|----|-----|------|---------|--------|--------|
| **S5a** | Player Overlay (controls) | overlay (top + bottom gradient) | Bất kỳ phím trong S5 | 13 §11 | ✅ V1 |
| **S5b** | Track Selection | BottomSheet | S5a → "Phụ đề" / "Audio" | 13 §12 | ✅ V1 |
| **S5c** | Player Error | full overlay 80% | ExoPlayer raise error | 13 §13 | ✅ V1 |
| **S5d** | Resume Prompt | dialog medium | Khi vào S5 có saved position | 13 §14 | ✅ V1 |

**Cắt khỏi V1**:
- ~~S5e~~ Post-play (Designer cut từ HTML hub).

---

## 4. Onboarding steps (2)

Carousel ngang trong S9. Mỗi step là 1 frame riêng.

| ID | Tên | Status |
|----|-----|--------|
| **S9-1** | Step 1 — Điều khiển bằng remote (icon `ic_dpad`) | ✅ V1 |
| **S9-2** | Step 2 — Tiếp tục xem dở (icon `ic_replay`) | ✅ V1 |

**Cắt khỏi V1**:
- ~~S9-3~~ Step 3 — Cập nhật tự động (vì Phương án A manual không có auto-update).

---

## 5. Global dialogs (template-based, 7 instances)

Tất cả dialogs đều dùng **component C8 `Dialog`** với 3 variants (Standard / Destructive / Loading) — xem 13 §21. Tài liệu liệt kê 7 instances cụ thể trong V1:

| ID | Trigger | Variant C8 | Default focus | Status |
|----|---------|------------|---------------|--------|
| **D1** | Back từ Home → "Thoát StreamIX TV?" | Standard | "Không" | ✅ V1 |
| **D2** | Back từ Player → "Tắt phim?" | Standard | "Không" | ✅ V1 |
| **D3** | S7a → "Đăng xuất khỏi tài khoản?" | Destructive | "Hủy" | ✅ V1 |
| **D4** | S4 → File yêu cầu password | Loading (input field) | password TextField | ✅ V1 |
| **D5** | S5d Resume Prompt | Standard | "Tiếp tục" | ✅ V1 |
| **D6** | S7f → "Cách cập nhật" (QR + URL) | Info (no-confirm, just close) | "Đóng" | ✅ V1 |
| **D7** | Token hết hạn giữa chừng → "Phiên đăng nhập hết hạn, đăng nhập lại?" | Standard | "Đăng nhập lại" | ✅ V1 |

**Cắt khỏi V1** (vì module bị cắt):
- ~~D-update-optional~~ Update prompt optional
- ~~D-update-force~~ Force update full-screen
- ~~D-download-conflict~~ Cảnh báo trước khi download (đã cắt)
- ~~D-download-usb-removed~~ USB tháo giữa chừng

---

## 6. Global overlays (3)

Là overlay full-screen có thể trigger từ **bất kỳ screen nào** khi điều kiện toàn cục xảy ra. Không có URL — render bằng global state.

| ID | Tên | Trigger | Component | Status |
|----|-----|---------|-----------|--------|
| **G1** | No network overlay | Network state Disconnected > 5s | C15 ErrorState fullscreen | ✅ V1 |
| **G2** | Server unavailable | Bất kỳ API trả `SERVICE_UNAVAILABLE` (503) trong 30s | C15 ErrorState fullscreen | ✅ V1 |
| **G3** | Fatal init error | Splash hold > 15s, Hilt init fail, app_key invalid | C15 ErrorState fullscreen | ✅ V1 |

**Note**: Mất mạng giữa lúc đang phát video → KHÔNG hiện G1 toàn màn (sẽ phá trải nghiệm xem); thay bằng banner small trong Player Overlay (xem §10 dưới).

---

## 7. State variants per screen (47)

Mỗi root screen có nhiều state. Designer phải vẽ frame riêng cho mỗi state. Engineering implement state machine trong ViewModel.

### S0 Splash (4 states)

| State ID | Tên | Visual khác biệt |
|----------|-----|------------------|
| S0/init | Initializing | Logo + progress mảnh |
| S0/verify | Verifying session | Logo + caption "Đang xác thực..." |
| S0/route | Routing | Logo + fade-out 200ms |
| S0/fail | InitFailed | Show G3 fatal error overlay |

### S1 Login (5 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S1/idle | Form trống | Default hoặc auto-fill email cũ |
| S1/idle-fill | Form auto-fill từ EncryptedPrefs | Email pre-filled |
| S1/submit | Submitting | Button loading spinner |
| S1/error | Error | Red banner dưới form |
| S1/success | Login OK | Transient < 500ms, button check icon |

Specific error displays:
| Error variant | Message |
|---------------|---------|
| S1/error-cred | "Email hoặc mật khẩu không đúng" (`AUTH_INVALID_CREDENTIALS`) |
| S1/error-locked | "Tài khoản tạm khoá, liên hệ hỗ trợ" (`AUTH_USER_LOCKED`) |
| S1/error-validation | Highlight field + `details.field` (`VALIDATION_FAILED`) |
| S1/error-throttle | "Quá nhiều lần thử, đợi 15 phút" (`RATE_LIMIT_EXCEEDED`) |
| S1/error-appkey | Trigger G3 fatal (APP_KEY_INVALID) |
| S1/error-network | "Không có kết nối, kiểm tra mạng" |

### S2 Home (4 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S2/loading | Loading | 3 row skeleton |
| S2/loaded | Loaded full | Đầy đủ Continue/Recent/Folders |
| S2/partial | Partial | 1+ row error inline (giữ row khác hoạt động) |
| S2/empty | Empty account | Toàn bộ rỗng → empty state với icon + caption |

### S3 Browse (5 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S3/loading | Loading initial | Grid skeleton 4×3 |
| S3/loaded | Loaded | Grid card đầy đủ |
| S3/paginating | Loading more | Spinner subtle ở footer grid |
| S3/empty | Folder rỗng | Icon `ic_folder` + "Thư mục trống" |
| S3/error | Load failed | C15 ErrorState với retry |

### S4 File Detail (5 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S4/loading | Loading | Skeleton title + meta |
| S4/loaded-video | Loaded video file | Button "Phát ngay" enabled |
| S4/loaded-non-playable | Loaded image/zip | Button "Phát" hidden, chỉ "Yêu thích" |
| S4/protected | Password required | Trigger D4 dialog |
| S4/error | Error (not found / forbidden) | C15 inline với back button |

### S5 Player (7 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S5/loading-url | Loading stream URL | Black + "Đang lấy URL..." caption |
| S5/buffering-init | Buffering ban đầu | Spinner center, video chưa hiện |
| S5/playing | Playing (clean view) | Video full, không UI |
| S5/paused | Paused | Hiển thị overlay S5a (không auto-hide) + icon pause to giữa |
| S5/buffering-mid | Buffering giữa stream | Spinner subtle, video freeze |
| S5/ended | Stream kết thúc | Black + "Đã xem xong" + back to S4 |
| S5/error | Error → trigger S5c | (xem S5c) |

### S5a Player Overlay (3 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S5a/visible-playing | Visible while playing | Bottom controls, top back+title |
| S5a/visible-paused | Visible while paused | Center icon pause to + bottom controls |
| S5a/hidden | Hidden auto-after-5s | Toàn bộ overlay alpha 0 |

### S5b Track Selection (2 variants × 3 states = 6)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S5b-sub/loading | Subtitle list loading | Skeleton 3 rows |
| S5b-sub/list | Subtitle list rendered | RadioGroup các track + "Tắt phụ đề" |
| S5b-sub/empty | Không có sub track | Caption "File không có phụ đề" |
| S5b-aud/loading | Audio list loading | Skeleton |
| S5b-aud/list | Audio list rendered | RadioGroup track |
| S5b-aud/single | Single audio track | Caption "Chỉ có 1 audio track" + disable |

### S5c Player Error (4 variants)

| State ID | Tên | Body message |
|----------|-----|--------------|
| S5c/codec | Codec không hỗ trợ | "Định dạng video không hỗ trợ. Thử đổi sang VLC engine." |
| S5c/network | Mạng yếu / lỗi | "Mất kết nối. Vui lòng thử lại." |
| S5c/url-expired | URL stream hết hạn (refresh fail) | "Phiên xem hết hạn. Tải lại để tiếp tục." |
| S5c/forbidden | File mất quyền truy cập | "Không có quyền xem file này." |

### S5d Resume Prompt (1 state — chỉ visible/hidden)

| State ID | Tên |
|----------|-----|
| S5d/visible | Resume Prompt hiển thị |

### S7 Settings Hub (1 state — focus changes)

| State ID | Tên |
|----------|-----|
| S7/loaded | Loaded — preview content thay đổi theo focus |

### S7a Account (3 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S7a/loading | Loading | Skeleton avatar + grid |
| S7a/loaded-vip | Loaded VIP user | Chip VIP + expiry |
| S7a/loaded-free | Loaded Free user | Chip Free + caption "Nâng cấp VIP trên web" |
| S7a/offline | Offline cache | Caption "Đang offline" subtle |

### S7b Playback (1 state)

| State ID | Tên |
|----------|-----|
| S7b/loaded | Loaded với 7 toggle/radio |

### S7d Network (2 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S7d/connected | Wi-Fi connected | Hiển thị SSID + IP + link speed |
| S7d/disconnected | Wi-Fi disconnected | Banner đỏ "Không có kết nối" + button Settings |

### S7f About (1 state)

| State ID | Tên |
|----------|-----|
| S7f/loaded | Loaded |

### S9 Onboarding (3 states)

| State ID | Tên |
|----------|-----|
| S9/step1 | Step 1 D-pad |
| S9/step2 | Step 2 Resume |
| S9/transition | Transition giữa step (slide L↔R) |

### S12 Search (5 states)

| State ID | Tên | Khác biệt |
|----------|-----|-----------|
| S12/idle | Chưa nhập | Recent searches chip + suggestions |
| S12/typing | Đang gõ < 2 ký tự | Caption "Nhập ≥ 2 ký tự" |
| S12/searching | Searching | Skeleton grid |
| S12/results | Có kết quả | Grid 4 cột |
| S12/empty | Không có kết quả | Icon + "Không tìm thấy file phù hợp" |

---

## 8. Snackbar / Toast (4 types × N usage)

Component **C10 Snackbar** với 4 variants. Hiển thị bottom-center, auto-dismiss 4s, tối đa 1 snackbar tại 1 thời điểm.

### 8.1 Variants (color + icon)

| Variant | Color token | Icon | Use |
|---------|-------------|------|-----|
| **info** | `state/info` | `ic_info` | Thông báo trung tính |
| **success** | `state/success` | `ic_success` | Action thành công |
| **warning** | `state/warning` | `ic_warning` | Cảnh báo non-fatal |
| **error** | `state/danger` | `ic_error` | Lỗi non-fatal |

### 8.2 Snackbar trong V1 (theo nguồn trigger)

| ID | Trigger | Variant | Message |
|----|---------|---------|---------|
| T1 | Logout thành công | success | "Đã đăng xuất" |
| T2 | Mất mạng giữa Browse | warning | "Mạng yếu — đang dùng cache" |
| T3 | API rate limited | warning | "Quá nhiều yêu cầu, vui lòng thử lại sau" |
| T4 | File deleted server-side | error | "File không còn tồn tại" |
| T5 | Token refresh failed | error | "Phiên hết hạn, vui lòng đăng nhập lại" |
| T6 | Yêu thích thành công | success | "Đã thêm vào Yêu thích" |
| T7 | Yêu thích bỏ thành công | info | "Đã xóa khỏi Yêu thích" |
| T8 | Setting đã save | success | "Đã lưu cài đặt" |
| T9 | Player URL refreshed silently | info | "Đã làm mới phiên xem" |
| T10 | Search keyword quá ngắn | info | "Nhập tối thiểu 2 ký tự" |
| T11 | Wi-Fi reconnected | success | "Đã kết nối lại" |

---

## 9. Player Overlay banner (in-screen, không phải toàn cục)

Banner xuất hiện trong S5 Player **không** dùng C10 Snackbar (vì sẽ vướng video). Dùng banner riêng top hoặc bottom với `bg/scrim 70%`:

| ID | Trigger | Position | Auto-hide |
|----|---------|----------|-----------|
| P1 | Mạng yếu giữa stream | top | 5s |
| P2 | Đang refresh URL stream (silent) | top | tự ẩn khi xong |
| P3 | Audio passthrough warning ("AVR không hỗ trợ DTS") | bottom | 5s |
| P4 | Tua đến cuối | center | 1s |
| P5 | Tua quá đầu | center | 1s |

---

## 10. Component reuse map — screens × components

Cross-reference giữa 24 component trong design system (`09 §8`) và screens nào dùng:

| Component | Screens dùng |
|-----------|--------------|
| C1 TopBar | S2, S3, S6 (cắt), S12 |
| C2 Button | S1, S4, S5c, S7a, S7d, S7f, S9, dialog footer |
| C3 IconButton | S5a, S12 (clear), dialogs (close) |
| C4 FileCard | S2 (Recent + Continue), S3, S12 (results) |
| C5 FolderCard | S2 (Folders), S3 |
| C6 RowSection | S2 |
| C7 ListItem | S7, S7b, S7d, S7f |
| C8 Dialog | D1–D7 |
| C9 BottomSheet | S5b |
| C10 Snackbar | T1–T11 |
| C11 ProgressBar (linear) | S0 splash, Player timeline thumb, S7a webspace |
| C12 ProgressCircular | Buffering S5/buffer-mid, dialog loading |
| C13 Skeleton | S2/loading, S3/loading, S4/loading, S7a/loading, S12/searching |
| C14 EmptyState | S2/empty, S3/empty, S12/empty |
| C15 ErrorState | S3/error, S5c, S7a fail, G1, G2, G3 |
| C16 TextField | S1, S12, D4 (password) |
| C17 Switch | S7b (audio passthrough, auto-next, save position, subtitle outline) |
| C18 RadioGroup | S5b (track), S7b (engine, sub size, sub color) |
| C19 Chip | S2 (badge VIP/HD), S4 (size/duration/format), S12 (recent searches) |
| C20 Avatar | C1 TopBar (S2), S7a |
| C21 KeyboardOnScreen | S1 (email/password input), S12 (search) |
| C22 QRCard | S7f (cách cập nhật dialog D6) |
| C23 PlayerOverlay | S5a |
| C24 NotificationBadge | S2 TopBar (icon update available — V1.1+ với Phương án C) |

---

## 11. Navigation graph (text)

```
[Cold start]
    │
    ▼
[S0 Splash]
    ├─→ [G3 Fatal] (init failed)
    ├─→ [S1 Login] (no session)
    └─→ [S2 Home] (valid session)

[S1 Login]
    ├─→ [S9 Onboarding] (lần đầu) ─→ [S2 Home]
    └─→ [S2 Home] (đã onboarded)

[S2 Home]
    ├─[Card]─→ [S4 File Detail] | [S3 Browse]
    ├─[Continue Watching card]─→ [S5 Player(resumePos>0)]
    ├─[TopBar Search]─→ [S12 Search]
    ├─[TopBar Settings]─→ [S7 Settings Hub]
    ├─[TopBar Avatar]─→ [S7a Account]
    └─[Back]─→ [D1 Exit confirm] ─→ exit app | back

[S3 Browse]
    ├─[Folder card]─→ [S3 Browse(subfolder)]
    ├─[File card]─→ [S4 File Detail]
    └─[Back]─→ parent S3 | S2

[S4 File Detail]
    ├─[Phát ngay]─→ [S5 Player]
    ├─[File protected]─→ [D4 Password] ─→ [S5 Player]
    ├─[Yêu thích]─toggle (Room)
    └─[Back]─→ caller (S2 / S3 / S12)

[S5 Player]
    ├─[Bất kỳ phím]─→ [S5a Overlay]
    ├─[ResumePos > 30s]─→ [S5d Resume Prompt] ─→ [S5 playing]
    ├─[Player error]─→ [S5c Error] ─→ retry | back
    ├─[STATE_ENDED]─→ back to S4 (cắt S5e Post-play)
    └─[Back]─→ [D2 Tắt phim?] ─→ back to caller

[S5a Overlay]
    ├─[Phụ đề]─→ [S5b Subtitle Sheet]
    ├─[Audio]─→ [S5b Audio Sheet]
    ├─[Settings]─→ (overlay menu, V1.1)
    └─[Auto-hide 5s]─→ S5 clean

[S7 Settings Hub]
    ├─[Tài khoản]─→ [S7a]
    ├─[Phát video]─→ [S7b]
    ├─[Mạng]─→ [S7d]
    ├─[Giới thiệu]─→ [S7f]
    └─[Back]─→ S2

[S7a Account]
    └─[Đăng xuất]─→ [D3 Logout?] ─→ API logout ─→ [S1]

[S7f About]
    └─[Cách cập nhật]─→ [D6 QR Dialog]

[S12 Search]
    ├─[Result card]─→ [S4] | [S3]
    ├─[Recent chip]─→ fill query, search
    └─[Back]─→ S2

[S9 Onboarding]
    ├─[Tiếp]─→ S9-2
    ├─[Bắt đầu / Bỏ qua]─→ [S2]
    └─[Back]─→ same as Bỏ qua

[Global trigger từ bất kỳ screen]
    ├─[Mất mạng > 5s]─→ [G1 No Network overlay]
    ├─[Server 503]─→ [G2 Server Unavailable]
    ├─[Token refresh fail]─→ [D7 Session Expired] ─→ [S1]
    └─[Crash uncaught]─→ Crashlytics (handled, không UI)
```

---

## 12. Bảng tổng hợp — designer cần thiết kế bao nhiêu frames Figma

| Loại | Số frames cần |
|------|---------------|
| 9 root screens × avg 4 states | ~36 |
| 4 sub-screens × avg 2 states | 8 |
| 2 onboarding steps + transition | 3 |
| 4 player overlays/sheets/dialogs × variants | 14 (S5a×3 + S5b×6 + S5c×4 + S5d×1) |
| 7 dialog instances | 7 |
| 3 global overlays | 3 |
| 5 player banners | 5 |
| 11 snackbar instances | 4 (chỉ 4 variants base; messages chỉ là text, không cần frame riêng) |
| **TỔNG** | **~80 frames** |

(So với 27 screens trong spec gốc 09, sau cuts còn ~80 frames vì có states và variants.)

---

## 13. Checklist designer cần verify v1.1

Khi designer ship v1.1 sau audit (xem `11`), phải có đủ frames cho:

### V1 priority — bắt buộc

- [ ] S0/init + S0/verify + S0/route + S0/fail (4 frames)
- [ ] S1/idle + S1/idle-fill + S1/submit + S1/error (×6 specific) + S1/success (10 frames)
- [ ] S2/loading + S2/loaded + S2/partial + S2/empty (4 frames)
- [ ] S3/loading + S3/loaded + S3/paginating + S3/empty + S3/error (5 frames)
- [ ] S4/loading + S4/loaded-video + S4/loaded-non-playable + S4/protected + S4/error (5 frames)
- [ ] S5 7 states + S5a 3 + S5b 6 + S5c 4 + S5d 1 = 21 frames
- [ ] S7 1 + S7a 3 + S7b 1 + S7d 2 + S7f 1 = 8 frames
- [ ] S9 3 frames
- [ ] S12 5 frames
- [ ] D1–D7 7 frames
- [ ] G1, G2, G3 = 3 frames
- [ ] P1–P5 player banners = 5 frames
- [ ] Snackbar 4 variants = 4 frames

**Tổng minimum**: ~80 frames cho V1.

### Nice-to-have

- [ ] Light theme variant (nếu V1.1)
- [ ] High contrast accessibility variant
- [ ] Subtitle preview demo trong S7b (text mẫu render với mỗi font size)
- [ ] Tablet/4K layout (nếu mở rộng nền tảng)

---

## 14. Tổng hợp QA cần test screens nào

| Test type | Screens phải có test |
|-----------|---------------------|
| Smoke test (every release) | S0, S1, S2, S3, S4, S5, S7 (chính), S12 |
| Flow E2E | First-run + login + browse + play + exit (Flow 1 trong 09 §10) |
| Negative test | S1 error variants, S3 error, S4 error, S5c, G1, G2, D7 |
| Edge case | Folder 2000 items, video 4h dài, file pwd, network drop mid-stream |
| Accessibility | Tất cả screens với TalkBack on |
| Performance | S2 cold start, S3 paginate 1000 items, S5 time-to-first-frame |
| Localization | Mọi screen × vi + en |

QA bộ test phải có:
- 9 smoke test scripts (1 per root screen)
- 5 E2E flow scripts (Flow 1–5)
- ~30 negative test cases
- ~10 edge case tests
- a11y audit per screen
- Performance benchmark scripts

---

— Hết catalog màn hình V1 —
