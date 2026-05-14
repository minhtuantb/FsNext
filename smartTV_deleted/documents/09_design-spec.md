---
title: 09 — FsNext TV — Design Specification & Functional Requirements
version: 1.0
date: 2026-05-04
status: Draft for design review
audience: Design system team, Product, QA, Engineering
owner: Product Manager + Tech Lead
---

# FsNext TV — Design Specification

## 0. Về tài liệu này

Tài liệu mô tả **toàn bộ yêu cầu chức năng và yêu cầu thiết kế** cho FsNext TV (bản client của fshare.vn chạy trên Smart TV / Android TV box, phân phối nội bộ qua APK).

**Đối tượng đọc**: chính là **đội Design System** — sẽ thiết kế toàn bộ UI/UX. Phụ là Product, QA, Engineering để cùng align.

**Phạm vi**:
- Liệt kê mọi màn hình của V1.
- Mô tả mục đích, trạng thái, hành vi của mỗi màn hình.
- Đưa ra design tokens, component library, focus model, motion guideline.
- Đưa ra 6 user flow chính.
- Liệt kê deliverables đội design phải nộp.

**Không thuộc phạm vi tài liệu này**:
- Spec backend API (đã có ở `docs/02_api_contracts.md` của desktop).
- Kiến trúc kỹ thuật Android (đã có ở `02_khuyen-nghi-kien-truc.md`, `03_lua-chon-cong-nghe.md`).
- Test cases (sẽ do QA viết riêng dựa trên tài liệu này).

**Cách đọc**:
- Phần A (mục 1–8): nguyên tắc & foundation — đọc trước khi vẽ.
- Phần B (mục 9): catalog các màn hình — tham chiếu khi vẽ từng screen.
- Phần C (mục 10): user flows — hiểu chuyển cảnh giữa screens.
- Phần D (mục 11–16): ràng buộc, deliverables, ghi chú.

---

# Phần A — Foundation

## 1. Mục tiêu sản phẩm V1

V1 của FsNext TV nhắm tới **một mục tiêu duy nhất**: cho người dùng nội bộ (~50–500 user là nhân viên Fshare, đối tác pilot) **đăng nhập tài khoản Fshare và xem video lưu trữ trên cloud Fshare ngay trên TV**, không phải mở app khác và không phải copy file.

Ba câu chuyện thành công của V1:
1. Nhân viên về nhà, mở TV, vào FsNext, scan QR đăng nhập, chọn phim → xem ngay trong < 2 phút.
2. User đang xem dở một bộ phim, hôm sau bật TV thấy ngay "Tiếp tục từ 00:42:13" → 1 click là xem tiếp.
3. App tự cập nhật trong nền, lần sau user mở thấy bản mới mà không phải làm gì.

V1 **KHÔNG** nhắm tới: upload, share, file ops nặng, RSS, phụ đề search, browser embed. Những việc này dùng app desktop hoặc mobile.

## 2. Personas

Thiết kế phục vụ 3 personas. Khi có xung đột giữa các persona, ưu tiên persona 1.

**Persona 1 — Nhân viên thường (default)**
- Tuổi 25–45, dùng smartphone tốt, không phải kỹ thuật.
- Có TV Android (Sony/Xiaomi/TCL/FPT Box) ở nhà.
- Mục tiêu chính: xem phim/video sau giờ làm.
- Kỳ vọng: app phải mượt như Netflix/YouTube; không sửa lỗi kỹ thuật.

**Persona 2 — Nhân viên kỹ thuật / IT**
- Hiểu sâu, có ADB, biết cài APK.
- Có thể giúp đồng nghiệp cài app.
- Kỳ vọng: có settings nâng cao, log gửi được, debug menu khi cần.

**Persona 3 — Quản lý (đôi khi)**
- Ít dùng, dùng theo dịp (xem demo, họp, present).
- Kỳ vọng: app nhìn chuyên nghiệp, không lỗi khi present.

## 3. Bảy nguyên tắc thiết kế cho TV

Mọi quyết định thiết kế phải xuất phát từ 7 nguyên tắc dưới đây. Khi thiết kế bị tranh cãi, return về 7 nguyên tắc này để ra quyết định.

**N1. Tối thiểu hành động — mọi tác vụ chính trong ≤ 3 lần bấm OK.**
Ví dụ: từ Home → chọn folder → chọn file → bấm OK = play. Không yêu cầu thêm bước "xác nhận" trừ khi hành động phá huỷ (xoá).

**N2. Focus là tâm điểm.** Mỗi screen phải có **đúng một** phần tử focus mặc định khi vào. Focus phải vẽ rõ (border + scale + shadow). Khi back lại, focus restore về chỗ cũ.

**N3. Thông tin tối thiểu, không quá tải mắt.** Mỗi screen tập trung 1 mục đích. Không hiển thị quá 7 phần tử trong vùng nhìn cùng lúc (trừ list scrollable). Bỏ mọi thông tin "cho đẹp" — TV không phải dashboard.

**N4. Tránh hình ảnh nặng và trang trí.** TV box rẻ có RAM 1 GB; mỗi ảnh thumbnail là một fetch network. Quy tắc:
- Chỉ load ảnh có giá trị thông tin (thumbnail file có poster, QR login, app icon).
- Không có illustration trang trí cho empty state — dùng icon vector + text.
- Không có background photo, gradient image — dùng solid color hoặc gradient CSS-style.
- Mỗi screen không vượt quá ~20 ảnh load đồng thời (lazy-load nếu hơn).

**N5. Văn bản tối thiểu.** TV được "xem" không "đọc". Mỗi label ≤ 5 từ; mỗi description ≤ 2 dòng. Loại bỏ help text nội tuyến — chuyển vào Settings → About → FAQ.

**N6. Tương phản & font lớn.** Nền tối, text sáng. Body min 22 sp, title 32–48 sp. Tỷ lệ contrast ≥ 7:1 cho text quan trọng (cao hơn WCAG AA do khoảng cách 3m).

**N7. An toàn cho remote.** Không bao giờ yêu cầu gõ text dài bằng remote D-pad. Login qua QR mặc định. Tìm kiếm, nếu có (V2), phải có shortcut "kết nối keyboard rời" hoặc voice.

## 4. Phạm vi V1

### Trong scope (must-have V1)
- Đăng nhập (QR + email/password + OAuth device-flow).
- Browse thư mục và file của user.
- Xem chi tiết file (metadata trước khi play).
- Phát video nhúng (ExoPlayer) với play/pause, seek, audio track, subtitle.
- Resume playback.
- Tải file về USB OTG hoặc internal storage.
- Settings cơ bản (account, playback, download, network, update, about).
- In-app update (force và optional).
- Onboarding lần đầu.
- Logout / switch user.

### Ngoài scope V1 (V2+)
- Search.
- Voice search (Google Assistant).
- Recommendations row trên launcher TV.
- Cast nhận từ mobile.
- Picture-in-picture.
- Subtitle search online.
- Upload (luôn ngoài scope của TV).
- File ops (rename/move/copy/delete với confirm phức tạp — V1 chỉ hỗ trợ delete file local đã tải).
- Multi-account simultaneously (V1 chỉ 1 account active, có switch).

## 5. Brand & vibe

FsNext TV thừa kế brand từ Fshare nhưng cần **chế độ tối** sâu hơn để dễ nhìn ở phòng khách buổi tối:
- Tone: thân thiện, hiện đại, tin cậy. Không "vui nhộn", không "công nghiệp".
- Cảm hứng tham khảo: Apple TV (typography), YouTube TV (focus model), Netflix TV (immersive list).
- Tránh: nhiều màu sắc rực rỡ, animation hoa hoè, sticker/emoji trong UI chính.

## 6. Design tokens

### 6.1 Color

Hệ màu **dark-first**. Light theme **không** trong scope V1.

| Token | Hex | Mục đích |
|-------|-----|----------|
| `bg/base` | `#0F1218` | Nền chính của app |
| `bg/surface` | `#1A1F2A` | Card, dialog, top bar |
| `bg/surface-hi` | `#252B38` | Card khi focus / selected |
| `bg/scrim` | `#000000` 60% alpha | Overlay phía sau dialog |
| `border/subtle` | `#2A3140` | Đường kẻ phân cách |
| `border/strong` | `#3C465C` | Border container |
| `accent/primary` | `#FFB12C` | Brand Fshare — CTA, focus, highlight |
| `accent/primary-pressed` | `#E69A1E` | CTA khi nhấn |
| `accent/on-primary` | `#0F1218` | Text trên nền primary |
| `text/primary` | `#EEF1F6` | Title, body chính |
| `text/secondary` | `#A9B1BD` | Caption, metadata |
| `text/disabled` | `#5C6374` | Text/icon disabled |
| `state/success` | `#4FCC8E` | Toast thành công, badge VIP |
| `state/warning` | `#F0A23C` | Cảnh báo (mạng yếu, dung lượng thấp) |
| `state/error` | `#E25062` | Lỗi (login fail, play fail) |
| `state/info` | `#5AB4E8` | Thông báo trung tính |
| `focus/ring` | `#FFFFFF` 100% | Vòng focus ngoài (4 dp stroke) |
| `focus/glow` | `#FFB12C` 35% alpha | Halo phía sau focus |

Lưu ý:
- Không dùng pure black (`#000`) cho bg vì OLED TV gây "black crush" — dùng `#0F1218`.
- Pure white (`#FFF`) gây chói khi xem tối — `#EEF1F6` đủ tương phản, dịu hơn.
- Mọi màu accent phải pass contrast ≥ 4.5:1 với background, text quan trọng ≥ 7:1.

### 6.2 Typography

Font chính: **Inter** (variable, weight 400/500/600/700) — Latin extended cho tiếng Việt đầy đủ. Font system fallback `Roboto`, `sans-serif`.

| Token | Size / Line height | Weight | Dùng cho |
|-------|-------------------|--------|----------|
| `display/lg` | 56 sp / 64 sp | 700 | Tiêu đề Hero (Splash, Onboarding) |
| `display/md` | 48 sp / 56 sp | 700 | Tên file ở Detail screen |
| `headline` | 36 sp / 44 sp | 600 | Tiêu đề screen, Section header |
| `title` | 28 sp / 36 sp | 600 | Tiêu đề card, dialog |
| `body/lg` | 24 sp / 32 sp | 500 | Body chính (mô tả, message) |
| `body/md` | 22 sp / 30 sp | 400 | Body phụ |
| `label/lg` | 22 sp / 28 sp | 600 | Button, CTA |
| `label/md` | 20 sp / 26 sp | 500 | Tab, chip |
| `caption` | 18 sp / 24 sp | 400 | Metadata (size, date), helper text |
| `mono` | 22 sp / 30 sp | 500 | Mã version, token, log |

Quy tắc:
- KHÔNG dùng font nhỏ hơn 18 sp ở UI chính (chỉ ngoại lệ: subtitle player do user chỉnh được).
- Letter-spacing: 0 cho mọi token; ngoại lệ `label/lg` `+0.02em` cho cảm giác "buttoned".
- Dùng tính năng OpenType `tnum` cho time elapsed/remaining trong player để số không nhảy chiều rộng khi đếm.

### 6.3 Spacing

Đơn vị cơ bản: **4 dp**. Mọi padding/margin phải là bội số 4.

| Token | dp | Dùng |
|-------|----|------|
| `space/0` | 0 | — |
| `space/1` | 4 | Inside icon-text gap |
| `space/2` | 8 | Inner padding nhỏ |
| `space/3` | 12 | Gap giữa icon và label trong button |
| `space/4` | 16 | Inner padding card, gap giữa items |
| `space/5` | 24 | Inner padding container |
| `space/6` | 32 | Gap giữa các row trên Home |
| `space/7` | 48 | **Padding ngoài screen** (overscan safe) |
| `space/8` | 64 | Gap section lớn |
| `space/9` | 96 | Hero spacing |

Vùng overscan: tất cả nội dung quan trọng phải cách mép màn hình ≥ **48 dp** (`space/7`). Một số TV cũ vẫn cắt 2–5%; 48 dp đảm bảo an toàn.

Grid: dùng **12-column responsive grid** với gutter `space/4` (16 dp) ở 1920×1080 logical canvas. Mỗi column ~140 dp.

### 6.4 Elevation & shadow

TV không có depth tự nhiên (không 3D head-tracking), shadow chỉ để tách lớp. Hai mức:

| Token | Shadow | Dùng |
|-------|--------|------|
| `elev/0` | none | Default |
| `elev/1` | `0 4 dp 12 dp rgba(0,0,0,0.4)` | Card focus, dialog |
| `elev/2` | `0 8 dp 24 dp rgba(0,0,0,0.6)` | Modal lớn, popup |

Không dùng nhiều mức elevation — gây nhiễu mắt ở khoảng cách 3 m.

### 6.5 Corner radius

| Token | dp | Dùng |
|-------|----|------|
| `radius/sm` | 6 | Chip, badge |
| `radius/md` | 12 | Button, card |
| `radius/lg` | 16 | Container lớn, dialog |
| `radius/full` | 9999 | Avatar, circular button |

### 6.6 Iconography

- Icon family: **Material Symbols** — Outlined style, weight 400.
- Kích thước chuẩn: **32 dp** trong UI thường, **40 dp** trong nút lớn, **24 dp** chỉ khi đi kèm text inline.
- Stroke width: 2 dp (cố định bởi font).
- Icon có context phải có `contentDescription` cho TalkBack.
- Custom icon (logo Fshare, ký hiệu VIP) phải vector SVG, < 4 KB mỗi file, palette từ design tokens.

Không dùng icon emoji trong UI chính (chỉ chấp nhận trong content do user tạo).

### 6.7 Imagery rules — quan trọng

Theo nguyên tắc **N4 (tránh hình ảnh nặng)**, áp dụng quy tắc:

- **Thumbnail file**: tỷ lệ 16:9, kích thước hiển thị 320×180 dp (~640×360 px @2x). Lazy load. Có placeholder solid color trong khi load (không spinner mỗi card — gây nhấp nháy).
- **Poster phim**: nếu Fshare API trả poster, dùng tỷ lệ 2:3, 200×300 dp. Cũng lazy load.
- **Avatar user**: 80 dp tròn. Có fallback chữ cái đầu nếu không có ảnh.
- **QR login**: 240×240 dp, vector SVG (không bitmap), nền trắng.
- **Empty state**: dùng icon vector lớn (~96 dp) + text. **KHÔNG** dùng illustration nhân vật.
- **Splash**: chỉ logo Fshare + tên app trên solid bg. Không animation phức tạp.
- **Brand artwork** (nếu cần khẳng định brand ở Settings → About): tối đa 1 ảnh, < 100 KB.
- Toàn bộ APK không được vượt 500 KB image assets local; còn lại load từ network nếu cần.

Format:
- Vector trong APK: SVG (chuyển VectorDrawable bởi Studio).
- Bitmap trong APK: WebP loss-less hoặc lossy 90% chất lượng.
- Bitmap từ network: WebP/JPEG; nếu Fshare backend chưa hỗ trợ WebP, nhận JPEG nhưng yêu cầu backend transcode kích thước phù hợp (320×180 px max).

### 6.8 Motion & animation

Quy tắc: **animation ngắn, có mục đích, dưới 250ms**. TV không có touch responsiveness; animation dài làm app cảm giác chậm.

| Token | Duration | Easing | Dùng |
|-------|----------|--------|------|
| `motion/quick` | 100 ms | `cubic-bezier(0.2, 0, 0, 1)` | Focus shift, hover |
| `motion/standard` | 200 ms | `cubic-bezier(0.4, 0, 0.2, 1)` | Dialog open, snackbar |
| `motion/slow` | 300 ms | `cubic-bezier(0.4, 0, 0.2, 1)` | Screen transition (rare) |
| `motion/zero` | 0 ms | — | Khi `Settings.reduceMotion=true` |

Quy tắc cứng:
- Focus shift: dùng `motion/quick`. Scale focus 1.0 → 1.05 + glow fade in.
- KHÔNG dùng spring physics phức tạp.
- KHÔNG dùng parallax, không Ken-Burns trên ảnh nền.
- Loading: dùng skeleton animation `pulse` 1500 ms (opacity 0.4 ↔ 0.7), không spinning circle ở list.
- Khi TalkBack on hoặc OS reduce-motion: tắt mọi animation không thiết yếu.

## 7. Focus & interaction model

### 7.1 Focus indicator chuẩn

Mọi component focusable phải có visual focus rõ ràng theo chuẩn:
- **Outer ring**: stroke 4 dp, color `focus/ring` (#FFFFFF), bo theo radius của component.
- **Glow**: shadow 16 dp blur, color `focus/glow` (#FFB12C 35% alpha).
- **Scale**: phóng to **1.05×** từ tâm. Áp dụng cho card; với button/input scale 1.0 (chỉ ring + glow).
- **Background**: card đổi từ `bg/surface` sang `bg/surface-hi`.

Khi component được "selected" (đã focus + nhấn OK đang chờ action): thêm overlay accent 15% phía trên.

Khi component focusable nhưng disabled: không vẽ focus ring, mở rộng ring nhẹ với màu `text/disabled`.

### 7.2 D-pad mapping mặc định

Default behavior cho mọi list/grid:

| Phím | Hành vi |
|------|---------|
| Up | Di chuyển focus lên item trên cùng cột. Đụng đỉnh thì không loop. |
| Down | Di chuyển focus xuống item dưới cùng cột. |
| Left | Di chuyển focus sang trái. Đầu hàng thì không loop. |
| Right | Di chuyển focus sang phải. Cuối hàng thì không loop. |
| OK / Center | Activate (click) item đang focus. |
| Back | Quay về screen cha; ở screen gốc → confirm thoát app. |
| Home | Hệ điều hành xử lý — minimize app. App phải save state. |
| Menu | Reserved — V1 chưa dùng. |
| Play/Pause (nếu có) | Toggle play khi đang ở Player. Ở screen khác: ignore. |
| Rewind / Forward | Tua ±10s khi ở Player. |

Trong Player, mapping được override (xem screen S5).

### 7.3 Back behavior

| Context | Hành vi Back |
|---------|--------------|
| Home | Confirm "Thoát Fshare TV?" — Có / Không. Default focus "Không". |
| Browse subfolder | Lên một level thư mục. |
| File Detail | Trở về Browse, focus restore về file đã chọn. |
| Player | Confirm "Tắt phim?" — Có / Không. Default focus "Không". Nếu Có: lưu position, về File Detail. |
| Settings sub-screen | Về Settings Hub. |
| Dialog | Đóng dialog (nếu non-blocking). Block dialog (force update): ignore. |
| Login screens | Ngược về screen trước; ở Login Hub → confirm thoát app. |

### 7.4 Long-press

Dùng tiết kiệm. V1 chỉ dùng:
- Long-press D-pad Left/Right trong Player: tua ±30s (thay vì ±10s).
- Long-press OK trên file card: mở context menu (V2; V1 không có).
- Long-press Back từ bất kỳ đâu: KHÔNG dùng (dễ trigger nhầm); KHÔNG có shortcut "thoát app" qua long-press.

### 7.5 Keyboard rời

User có thể kết nối Bluetooth/USB keyboard. App phải hỗ trợ:
- **Tab / Shift+Tab**: tương đương Right / Left.
- **Arrow keys**: tương đương D-pad.
- **Enter**: tương đương OK.
- **Escape**: tương đương Back.
- **Space**: trong Player = play/pause.
- Trong text input: gõ trực tiếp, không qua on-screen keyboard.

Khi phát hiện keyboard rời (phím gõ vào): tự động hide on-screen keyboard nếu đang hiện.

## 8. Component library — danh sách thành phần dùng chung

Đội design phải build **một component library duy nhất** (Figma Component) cho toàn app. Dưới đây là danh sách 20 component chính. Mỗi component phải có biến thể cho mọi state (default / focus / selected / disabled / pressed).

| # | Component | Mục đích | States cần | Dùng ở screen |
|---|-----------|----------|------------|---------------|
| C1 | `TopBar` | Thanh trên cùng (logo, search-icon-future, settings, avatar) | default | S2, S3, S6 |
| C2 | `Button` (primary / secondary / ghost) | Hành động chính | default, focus, pressed, disabled, loading | mọi screen |
| C3 | `IconButton` | Hành động icon-only (close, more) | default, focus, pressed, disabled | S5a, dialog |
| C4 | `FileCard` (thumbnail) | Hiển thị file/video trong list | default, focus, selected, with-progress, with-badge | S2, S3, S6 |
| C5 | `FolderCard` | Hiển thị folder | default, focus, selected | S2, S3 |
| C6 | `RowSection` | Container của 1 row trên Home (title + horizontal list) | default | S2 |
| C7 | `ListItem` (Settings) | Item trong settings (label + value/chevron) | default, focus, with-switch, with-radio, with-icon, disabled | S7* |
| C8 | `Dialog` (modal) | Confirm, alert | small, medium, large; default & loading footer | S5d, S10, S8 |
| C9 | `BottomSheet` | Track selection, quick action | default, opened, closing | S5b |
| C10 | `Snackbar` / `Toast` | Phản hồi ngắn (download done, error) | info, success, warning, error | global |
| C11 | `ProgressBar` (linear) | Tiến độ tải, tiến độ phim | indeterminate, determinate, in-buffer | S5a, S8a |
| C12 | `ProgressCircular` | Loading ngắn | indeterminate, determinate | global |
| C13 | `Skeleton` | Placeholder khi load list/card | card, list-item, text | global |
| C14 | `EmptyState` | Khi list rỗng | with-icon, with-action | S3, S6 |
| C15 | `ErrorState` | Khi load fail | retry, fatal | S3, S5c, S11 |
| C16 | `TextField` | Input text (login, search) | default, focus, error, disabled, with-icon | S1b |
| C17 | `Switch` | Toggle setting | on, off, focus, disabled | S7* |
| C18 | `RadioGroup` | Chọn 1 trong N | default | S7b, S7c |
| C19 | `Chip` / `Tag` | Badge metadata (HD, VIP, MKV) | filled, outlined, with-icon | S2, S4 |
| C20 | `Avatar` | Ảnh đại diện user | with-image, with-initials, with-status | C1, S7a |

Phụ trợ:
- C21 `KeyboardOnScreen`: bàn phím trên màn hình cho khi phải gõ (Login email).
- C22 `QRCard`: card chứa QR + helper text.
- C23 `PlayerOverlay`: overlay trong player (rất chuyên biệt).
- C24 `Notification` cố định: dùng cho update có sẵn (header với chấm cam).

---

# Phần B — Catalog các màn hình

Mỗi mục dưới đây mô tả **một màn hình hoặc một biến thể**. Format chuẩn:

> **ID • Tên** — *Purpose ngắn*
> **Entry**: từ đâu vào.
> **Exit**: đi tiếp đâu.
> **States**: liệt kê các trạng thái cần thiết kế.
> **Mô tả nội dung**: gồm những thành phần gì, vị trí tương đối, hành vi ra sao.
> **Components**: tham chiếu đến component library ở mục 8.
> **Default focus**: phần tử focus đầu tiên khi vào.
> **Phím cụ thể**: nếu khác mặc định ở 7.2.
> **Edge cases**: tình huống ranh giới phải xử lý.

## 9. Screen catalog

### 9.1 S0 • Splash

> **Purpose**: Hiển thị trong khi app khởi tạo (Hilt, DataStore, check session, kiểm tra update).

**Entry**: launcher TV → app open.
**Exit**:
- Có session valid → S2 Home.
- Không có session → S1 Login Hub.
- Có update force → S8b Force Update.
- Có rollback flag → S8c Rollback (silent, gần như không thấy).

**States**: chỉ một state — loading.

**Mô tả nội dung**: Màn hình tối thuần với logo Fshare ở giữa, kèm tên "Fshare TV" bên dưới logo. Phần dưới logo có thanh loading mảnh (linear progress indeterminate) hoặc 3 chấm pulse. Tổng cộng dưới 3 phần tử thị giác. Không tagline, không version.

**Components**: vector logo + `ProgressBar` (indeterminate, mảnh).

**Phím**: không nhận input.

**Edge cases**:
- Hold > 5 giây: bắt đầu lo có gì đó kẹt — hiển thị nhỏ "Đang khởi động..." text caption.
- Hold > 15 giây: chuyển sang S11 Global error "Khởi động thất bại — thử lại / gửi log".

### 9.2 S1 • Login Hub

> **Purpose**: Cho user chọn cách đăng nhập.

**Entry**: từ S0 (chưa có session) hoặc từ S7a (logout).
**Exit**: S1a / S1b / S1c tuỳ user chọn.

**States**: default; với badge "Đã từng đăng nhập như X — đăng nhập lại?" nếu có lastUserHint trong DataStore.

**Mô tả nội dung**: Trên cùng có logo nhỏ và headline "Đăng nhập Fshare". Dưới headline là 3 lựa chọn xếp dọc: (1) "Đăng nhập bằng QR" — đề xuất, đặt to nhất và default focus. (2) "Đăng nhập bằng email & mật khẩu". (3) "Đăng nhập với Google / FPT ID". Mỗi lựa chọn là một nút lớn với icon trái và label phải. Phía cuối screen có dòng caption "Bằng việc đăng nhập, bạn đồng ý với điều khoản sử dụng" — KHÔNG là link interactive trên TV (user không click; chỉ thông tin).

**Components**: `Button` primary (3 cái); icon QR, mail, OAuth.

**Default focus**: lựa chọn (1) — QR.

**Phím**: mặc định.

**Edge cases**:
- Không có mạng: hiển thị banner trên cùng "Không có kết nối mạng" + nút "Cài đặt mạng" → mở Settings hệ thống Android (`Intent.ACTION_WIRELESS_SETTINGS`).
- App vừa logout: lựa chọn (1) khôn lên thành "Đăng nhập lại — username@email" với QR vẫn là cách chính nhưng pre-fill prompt nếu có thể.

### 9.3 S1a • Login QR

> **Purpose**: User scan QR bằng app Fshare mobile để đăng nhập không phải gõ phím.

**Entry**: S1 → chọn "QR".
**Exit**: thành công → S2 Home; back → S1; timeout → S1a state expired.

**States**:
- `loading`: đang gọi API tạo phiên QR.
- `active`: QR đang hiển thị, đang polling.
- `scanned`: mobile đã quét, đợi user xác nhận.
- `success`: đăng nhập xong (transient < 1 giây trước khi navigate).
- `expired`: phiên QR hết hạn (sau 5 phút), hiển thị nút "Tạo lại".
- `error`: lỗi mạng / server.

**Mô tả nội dung**: Bên trái màn hình: QR lớn (480×480 dp) trên nền trắng có padding để máy scan tốt; phía dưới QR là dãy chấm 1–5 thể hiện đếm ngược thời gian sống (5 phút). Bên phải QR: hướng dẫn 3 bước "1. Mở app Fshare trên điện thoại. 2. Vào Cài đặt → Quét QR đăng nhập TV. 3. Quét mã bên trái". Dưới hướng dẫn có nút phụ "Đăng nhập cách khác" → trở về S1.

Khi state là `scanned`: thay QR bằng tick xanh + "Vui lòng xác nhận trên điện thoại". Khi `success`: tick lớn hơn + "Đăng nhập thành công" rồi navigate.

**Components**: `QRCard` (C22), `Button` secondary, icon check, icon error.

**Default focus**: nút "Đăng nhập cách khác".

**Phím**: mặc định.

**Edge cases**:
- Mạng yếu khiến polling lag: vẫn hiển thị QR; có cảnh báo nhỏ "Mạng chậm, vui lòng đợi".
- QR phiên trùng (user đã scan ở TV khác): server từ chối → thông báo "Phiên QR đã được dùng, tạo phiên mới".

### 9.4 S1b • Login Email & Password

> **Purpose**: Đăng nhập truyền thống cho user không có app mobile.

**Entry**: S1 → chọn email.
**Exit**: thành công → S2; back → S1.

**States**: default, submitting (button loading), error (sai email/password, account khoá), forgot password info.

**Mô tả nội dung**: Form đơn giản trên trục giữa: tiêu đề "Đăng nhập email"; ô Email (TextField, focus trước); ô Password (TextField, masked, có nút show/hide); checkbox "Nhớ tôi 30 ngày" (default ON); nút primary "Đăng nhập"; link text dưới "Quên mật khẩu? Vào fshare.vn trên điện thoại để khôi phục" (chỉ hiển thị, không click vì TV).

Khi user focus vào TextField, on-screen keyboard hiện ở nửa dưới screen. App phải tự động cuộn form để trường input không bị che.

Khi error: hiển thị thông báo đỏ ngay dưới form, dạng "Email hoặc mật khẩu không đúng" — không hiển thị riêng "email sai" hay "pass sai" để bảo mật.

**Components**: `TextField` (C16) ×2, `Switch` (C17), `Button` primary, `KeyboardOnScreen` (C21).

**Default focus**: TextField email.

**Phím**: mặc định + Enter ở password = submit.

**Edge cases**:
- Account locked sau N lần sai: hiển thị "Tài khoản tạm khoá, thử lại sau 15 phút".
- Captcha (nếu Fshare backend yêu cầu): V1 không hỗ trợ — hiện "Vui lòng đăng nhập trên điện thoại / web rồi quét QR".
- Forgot password: V1 chỉ thông báo, không có flow trên TV (đẩy về web).

### 9.5 S1c • Login OAuth Device-Flow

> **Purpose**: Login qua Google / FPT ID mà không cần mở browser embed (vốn UX tệ trên TV).

**Entry**: S1 → chọn OAuth.
**Exit**: thành công → S2; back → S1.

**States**: loading, code-displayed, polling, success, expired, error.

**Mô tả nội dung**: Sau khi chọn provider (1 step trung gian — buttons Google / FPT ID), screen hiện: tiêu đề "Đăng nhập bằng Google" (hoặc FPT ID); hướng dẫn "1. Mở https://fshare.vn/tv-link trên điện thoại. 2. Nhập mã: **A1B2-C3D4** (code 8 ký tự, font mono 64 sp). 3. Đăng nhập Google/FPT ID. Mã sống 5 phút." App polling backend mỗi 5 giây để biết user đã hoàn tất chưa.

**Components**: text mono lớn, hướng dẫn list, `Button` "Hủy".

**Default focus**: nút "Hủy".

**Phím**: mặc định.

**Edge cases**:
- Code expire trước khi user xong: thông báo + nút "Lấy mã mới".
- User huỷ trên điện thoại: nhận signal → quay về S1.

### 9.6 S2 • Home

> **Purpose**: Trung tâm browse — entry chính sau khi login. Hiển thị 4 row.

**Entry**: từ S0 (có session) hoặc sau login thành công.
**Exit**: chọn item → S4 (file) hoặc S3 (folder); chọn TopBar Settings → S7; Back → confirm thoát app.

**States**:
- `loading`: skeleton 4 row.
- `loaded`: data đầy đủ.
- `partial`: một số API call fail (hiển thị row vẫn có data, row fail → ErrorState bên trong).
- `empty-account`: tài khoản hoàn toàn trống file → screen onboarding nhỏ "Tài khoản chưa có file. Upload qua app desktop hoặc mobile."

**Mô tả nội dung**: TopBar trên cùng với logo Fshare bên trái + icon Settings + Avatar bên phải (focus được). Nội dung chính chia 4 row dọc, mỗi row có title bên trái + horizontal scroll list:
- Row 1: "Tiếp tục xem" — chỉ hiện nếu có ≥ 1 entry trong Continue Watching. Card có thumbnail + progress bar mảnh dưới ảnh.
- Row 2: "File gần đây" — file mới upload trong tài khoản, sort by upload date desc, max 30 item.
- Row 3: "Thư mục" — danh sách folder root của user, max 30, FolderCard.
- Row 4: "Đã tải xuống" — file đã tải về local thiết bị; ẩn row nếu rỗng.

D-pad up/down chuyển giữa các row (và TopBar). Left/Right scroll trong row. Khi vào lại Home, focus restore về row + item cũ.

Khi user mới (chưa có activity), chỉ hiện Row 3 "Thư mục" + (nếu trống) empty state.

**Components**: `TopBar` (C1), `RowSection` (C6), `FileCard` (C4), `FolderCard` (C5), `Skeleton` (C13), `EmptyState` (C14).

**Default focus**: item đầu tiên của Row 1 nếu có; nếu không, Row 2; nếu không, Row 3.

**Phím**: mặc định.

**Edge cases**:
- API "list root folders" timeout → hiện row với Skeleton + retry button bên trong row, các row khác vẫn hoạt động bình thường.
- Tài khoản hết VIP / hết hạn: banner subtle trên TopBar (không block) "Tài khoản đã hết VIP — gia hạn trên web".

### 9.7 S3 • Browse — danh sách trong thư mục

> **Purpose**: Browse content của một folder cụ thể.

**Entry**: từ S2 (chọn folder) hoặc từ S3 (chọn subfolder).
**Exit**: chọn folder con → recurse S3; chọn file → S4; Back → folder cha hoặc S2.

**States**: loading skeleton, loaded, empty, error, paginating (đang load thêm trang).

**Mô tả nội dung**: TopBar có breadcrumb thay logo (hiển thị "Home / Phim / Hành động" — hai cấp gần nhất, có dấu rút gọn nếu sâu hơn). Bên dưới TopBar là content area: grid 4 cột × N hàng các card (mix folder + file, folder lên trước). Mỗi card vuông 320×220 dp (16:9 thumbnail trên + 60 dp text dưới). Page size 50 item, scroll xuống cuối tự load thêm; có spinner subtle ở footer khi paginate.

Nếu folder rỗng: EmptyState với icon folder + text "Thư mục trống" + nút "Quay lại".

**Components**: `TopBar` (C1) variant breadcrumb, `FileCard`/`FolderCard`, `EmptyState`, `ErrorState`, `Skeleton`.

**Default focus**: card đầu tiên của grid (top-left).

**Phím**: mặc định.

**Edge cases**:
- Folder có hơn 2000 item (theo giới hạn API hiện tại): hiển thị đủ 2000, banner caption "Hiển thị 2000 file đầu — vui lòng chia nhỏ folder".
- File bị deleted ở backend giữa lúc browse: chọn vào → S4 hiển thị error "File không còn tồn tại".

### 9.8 S4 • File Detail (Pre-play)

> **Purpose**: Cho user xem metadata trước khi quyết định Play hoặc Download.

**Entry**: từ S2 hoặc S3 (chọn file).
**Exit**: Play → S5 Player; Download → start tải, transient toast → ở lại S4; Yêu thích → toggle, ở lại; Back → screen trước.

**States**: loading metadata, loaded, error, file-protected (cần password), file-too-big-to-stream (warn).

**Mô tả nội dung**: Layout 2 cột: cột trái (~40% rộng) là thumbnail lớn (poster 16:9, 720×405 dp); cột phải (~60% rộng) là metadata: tên file (display/md), dòng meta caption "{size} · {duration} · {format}/{codec}"; mô tả nếu có (body/lg, max 4 dòng + "..." nếu dài hơn); 3 nút action xếp dọc: "▶ Phát ngay" (primary, default focus), "⬇ Tải về thiết bị", "⭐ Yêu thích" (toggle); dưới nút là hai dòng caption "Phụ đề: vi (mặc định), en", "Audio: AC3 5.1 (vi), AAC 2.0 (en)".

Nếu là folder bị nhầm vào (API trả type folder): tự navigate về S3.

Nếu file là image hoặc archive (zip, rar): cột trái hiển thị icon thay thumbnail; nút "Phát" thay bằng "Tải về" (vì không xem được trên TV).

**Components**: `Button` primary + secondary, `Chip` cho metadata format, `Skeleton`, `ErrorState`.

**Default focus**: nút "Phát ngay".

**Phím**: mặc định.

**Edge cases**:
- File yêu cầu password: nút "Phát" mở dialog nhập password trước.
- File hỏng (size 0, hash invalid): nút Phát disable + tooltip "File không xem được".
- Streaming URL fail (403 từ backend): toast lỗi + giữ ở S4.

### 9.9 S5 • Player — toàn màn hình

> **Purpose**: Phát video full-screen, immersive.

**Entry**: từ S4 (Phát) hoặc từ S2 Continue Watching (Phát + seek).
**Exit**: Back (confirm) → S4 hoặc S2; xem hết file → S5e Post-play.

**States** (top-level): loading-stream, buffering-mid, playing, paused, ended, error.

**Mô tả nội dung**: Toàn màn hình là video. Khi mới vào, có thể có overlay "Đang tải..." 1–2 giây với title file + spinner subtle ở giữa. Khi playing: không có UI (clean view). Bất kỳ phím nào (trừ Back) → hiện Overlay (S5a) trong 5 giây rồi tự ẩn nếu không tương tác. Phím Back: confirm dialog (S5d).

Đặc biệt: khi vào lần đầu và có resume position trong Room (>30s, <duration-60s): hiện S5d Resume Prompt **trước khi** play, không tự seek silently.

**Components**: AndroidView wrap ExoPlayer, kèm `PlayerOverlay` (C23) Compose phủ trên.

**Default focus**: không có visual focus khi playing; khi Overlay show, focus mặc định trên nút Play/Pause.

**Phím** (override mặc định):
- OK / Center / Play-Pause: toggle play/pause.
- Left / Right: tua ±10s (long-press = ±30s).
- Up: hiện Overlay (nếu chưa); nếu Overlay đang hiện, focus chuyển lên hàng nút trên (nếu có).
- Down: hiện Overlay; nếu Overlay đang hiện, focus xuống.
- Back: confirm exit (S5d).
- 0–9 (number key nếu có): nhảy đến 0%, 10%, 20%... (nice-to-have, không bắt buộc).

**Edge cases**: xem chi tiết ở S5a–S5e.

### 9.10 S5a • Player Overlay (controls)

> **Purpose**: UI điều khiển video, hiện theo demand, ẩn sau 5s.

**Entry**: bất cứ phím nào trong S5 ngoại trừ phím tua đơn lẻ.
**Exit**: 5 giây không tương tác → fade out; user tua hoặc bấm action → reset 5s timer.

**Mô tả nội dung**: Overlay phủ trên video, 3 vùng:
- Trên cùng (~10% top): TopBar nhỏ với nút Back trái và title video phải, gradient nhẹ từ đen mờ xuống transparent.
- Giữa (vùng video): không có UI ngoài (giữ trải nghiệm xem). Có thể có icon play/pause to ở giữa hiện 0.5 giây khi user toggle play/pause (haptic-like feedback bằng visual).
- Dưới cùng (~25% bottom): progress bar to (8 dp height, tracker, thumb 16 dp khi focus); above progress là dòng time `00:42:13 / 02:18:00` (mono); dưới progress là thanh nút: từ trái qua phải [⏮ Prev (nếu có queue)] [⏯ Play/Pause] [⏭ Next] khoảng cách trung tâm; bên phải có [Phụ đề] [Audio] [⚙ More].

**Components**: `PlayerOverlay` (C23), `IconButton` (C3), `ProgressBar` (C11) chuyên biệt cho timeline (có thumb).

**Default focus**: nút Play/Pause khi mới hiện overlay.

**Phím**:
- Up: focus vào nút "Phụ đề/Audio" (hàng trên trong cluster nút).
- Down: chỉ scope nút phụ → focus xuống progress bar (cho phép tua bằng cách focus thumb rồi Left/Right).
- Khi focus thumb progress: Left/Right tua ±10s; OK = pause/play.

**Edge cases**:
- Đang tua mà user thả phím: vị trí mới apply ngay; preview thumbnail (V2 sau khi backend hỗ trợ thumbnails track).
- Buffering giữa chừng: hiện vòng spinner nhỏ giữa video; overlay vẫn ẩn hide-after-timeout bình thường.

### 9.11 S5b • Track Selection (BottomSheet)

> **Purpose**: Chọn track audio hoặc subtitle.

**Entry**: từ S5a, chọn nút "Phụ đề" hoặc "Audio".
**Exit**: chọn xong → BottomSheet đóng → trở về S5; Back → đóng BottomSheet.

**Mô tả nội dung**: BottomSheet trượt từ dưới lên, chiếm ~40% chiều cao. Tiêu đề trên cùng "Chọn phụ đề" hoặc "Chọn audio". Danh sách items: mỗi item là RadioGroup style với label "{language} ({codec})" và indicator selected. Item đầu cho phụ đề: "Tắt phụ đề". Cuối cùng có dòng "(Tự động phát hiện) — file có 2 phụ đề kèm" thông tin caption.

**Components**: `BottomSheet` (C9), `RadioGroup` (C18) variant cho item.

**Default focus**: item đang selected.

**Phím**: Up/Down chuyển focus; OK = chọn và đóng.

**Edge cases**:
- File chỉ có 1 track: vẫn hiển thị, có thể không ý nghĩa nhưng không hide để user không bối rối.

### 9.12 S5c • Player Error

> **Purpose**: Khi không play được file (codec lạ, mạng kém, URL hết hạn).

**Entry**: từ S5 khi Player raise lỗi.
**Exit**: Back → S4; "Thử lại" → S5; "Đổi engine" → S5 với libVLC; "Xem chi tiết lỗi" → mở dialog với log.

**Mô tả nội dung**: Overlay đen 80% phủ video. Trung tâm: icon cảnh báo, headline "Không phát được video", caption mô tả ngắn lỗi (đã localize, không stack trace). Ba nút dọc: "Thử lại", "Đổi sang engine VLC", "Quay lại". Phía dưới các nút có link nhỏ "Gửi log lỗi" (chỉ tech user dùng).

**Components**: `Dialog` variant fullscreen, `Button` primary + secondary, icon error.

**Default focus**: "Thử lại".

### 9.13 S5d • Resume Prompt

> **Purpose**: Hỏi user có muốn resume từ vị trí dừng không.

**Entry**: từ S5 trước khi bắt đầu play; chỉ hiện nếu có saved position 30s < pos < duration−60s.

**Exit**: chọn → S5 (resume hoặc bắt đầu từ đầu).

**Mô tả nội dung**: Dialog medium giữa screen, không phủ toàn màn (cho thấy thumbnail mờ phía sau là hợp lý nhưng không bắt buộc). Title "Tiếp tục từ {time}?". Caption "Bạn đã xem tới {time} ngày {date}". Hai nút ngang: "Bắt đầu lại" (secondary) | "Tiếp tục" (primary, default focus).

**Components**: `Dialog` medium.

**Default focus**: "Tiếp tục".

### 9.14 S5e • Post-play

> **Purpose**: Khi xem hết file. Đề xuất hành động tiếp.

**Entry**: stream chạy tới `ended` event.
**Exit**: chọn → action; back → S4.

**Mô tả nội dung**: Overlay tối phủ video. Trung tâm: tiêu đề "Đã xem xong" + tên file. Hai nút: "Xem lại từ đầu" | "Quay về thư mục" (primary). Nếu folder cha có file kế tiếp (sort by name), thêm nút "Phát file tiếp theo: {filename}" (default focus).

**Components**: `Dialog` variant fullscreen.

**Default focus**: "Phát file tiếp theo" nếu có; nếu không, "Quay về thư mục".

### 9.15 S6 • Downloads (file đã tải về local)

> **Purpose**: Quản lý file đã download xuống thiết bị.

**Entry**: từ S2 row "Đã tải xuống" hoặc TopBar (V2).
**Exit**: chọn item → S5 Player (phát local, không stream); Back → S2.

**States**: loading, loaded, empty, downloading-in-progress (kết hợp).

**Mô tả nội dung**: Grid 4 cột tương tự S3 nhưng card có thêm overlay icon "đã ở thiết bị" (icon chip download). Trên cùng có chip filter: "Tất cả" | "Đang tải" | "Hoàn tất" | "Lỗi". Mỗi item có actions: Phát (default), Xoá local. Nếu file đang tải, card hiển thị progress bar và % thay vì action Phát.

**Components**: `FileCard`, `Chip` filter (C19), `ProgressBar`.

**Default focus**: chip "Tất cả".

**Edge cases**:
- USB OTG bị tháo giữa chừng: file mất reference → hiện badge đỏ "Không tìm thấy file" trên card; chọn vào → dialog "USB đã rút? Cắm lại USB hoặc xoá entry".

### 9.16 S7 • Settings Hub

> **Purpose**: Trung tâm cài đặt, dẫn đến từng sub-screen.

**Entry**: từ S2 TopBar icon Settings.
**Exit**: chọn mục → S7a–f; Back → S2.

**Mô tả nội dung**: 2 cột — bên trái là menu dọc (~30% rộng) với 6 mục: Tài khoản, Phát video, Tải về, Mạng, Cập nhật, Giới thiệu. Bên phải hiện preview của mục đang focus (caption ngắn về nội dung mục đó). Khi user OK trên một mục, navigate vào sub-screen.

**Components**: `ListItem` (C7) cho menu, text caption cho preview.

**Default focus**: mục đầu tiên "Tài khoản".

### 9.17 S7a • Settings — Tài khoản

> **Purpose**: Hiển thị thông tin tài khoản, cho phép logout / switch user.

**Entry**: S7 → "Tài khoản".
**Exit**: Back → S7; "Đăng xuất" → confirm dialog → S1.

**Mô tả nội dung**: Top: Avatar tròn 120 dp + tên user (display/md) + email caption. Dưới đó là một grid 2 cột thông tin: Status VIP (badge), Hết hạn VIP, Webspace tổng/đã dùng (progress bar + text), Traffic tháng (số). Cuối screen có 2 nút secondary: "Chuyển tài khoản", "Đăng xuất".

**Components**: `Avatar` (C20), `Chip` VIP, `ProgressBar` (linear), `Button` secondary.

**Default focus**: "Chuyển tài khoản" (hành động phổ biến nhất).

### 9.18 S7b • Settings — Phát video

> **Purpose**: Cấu hình player.

**Mô tả nội dung**: List dọc các ListItem:
- "Engine phát" → RadioGroup: ExoPlayer (mặc định) / libVLC.
- "Audio passthrough HDMI" → Switch.
- "Tự động phát file kế tiếp" → Switch.
- "Lưu vị trí xem dở" → Switch (default ON).
- "Cỡ phụ đề" → RadioGroup: Nhỏ / Vừa / Lớn / Rất lớn.
- "Màu phụ đề" → RadioGroup: Trắng / Vàng / Cam.
- "Viền phụ đề" → Switch.

**Default focus**: mục đầu tiên.

### 9.19 S7c • Settings — Tải về

> **Purpose**: Cấu hình download.

**Mô tả nội dung**:
- "Vị trí lưu" → RadioGroup detect được: USB ({label}), Internal storage; auto disable USB nếu không cắm.
- "Số luồng tối đa" → RadioGroup: 1 / 2 / 4 / 8.
- "Cảnh báo khi dung lượng còn dưới" → RadioGroup: 100 MB / 500 MB / 1 GB.
- "Tự động tải tiếp khi mất mạng" → Switch.

### 9.20 S7d • Settings — Mạng

> **Purpose**: Hiện trạng thái mạng + truy cập Settings hệ thống.

**Mô tả nội dung**: Section 1: trạng thái hiện tại "Wi-Fi: Connected — {SSID}", "IP: {ip}", "Tốc độ tải xuống đo gần nhất: {Mbps} (cập nhật {time})". Section 2: nút "Mở Cài đặt mạng hệ thống" → mở `Intent.ACTION_WIRELESS_SETTINGS`. Section 3: nút "Kiểm tra tốc độ ngay" — chạy speed test 10 giây.

**Default focus**: nút "Kiểm tra tốc độ ngay".

### 9.21 S7e • Settings — Cập nhật

> **Purpose**: Trang trung tâm về update.

**Mô tả nội dung**:
- Hiển thị version hiện tại lớn (font mono): "FsNext TV 1.0.2 (10002)".
- Caption: "Cài đặt {date}".
- Nút "Kiểm tra cập nhật" → chạy CheckUpdateUseCase ngay.
- ListItem "Kênh cập nhật" → RadioGroup: Stable (mặc định) / Dev (chỉ team).
- ListItem "Tự động kiểm tra mỗi 6 giờ" → Switch (default ON).
- Section "Lịch sử cập nhật" — list các version đã cài (versionName + ngày).

**Default focus**: nút "Kiểm tra cập nhật".

### 9.22 S7f • Settings — Giới thiệu / Gửi log

> **Purpose**: About + support.

**Mô tả nội dung**:
- Logo + tên app + version.
- Một dãy thông tin device: Model, Android version, Free space, Network, App ID hash.
- Nút "Gửi nhật ký lỗi" — tạo bundle log (200 dòng cuối + device info) → upload lên `tv-update.fshare.local/log/feedback` → trả lại ticket ID.
- Caption hỗ trợ: "Liên hệ: tv-support@fshare.vn / Slack #tv-support".
- Link nhỏ tham chiếu "Điều khoản sử dụng / Chính sách quyền riêng tư" — chỉ thông tin, không click.

**Default focus**: nút "Gửi nhật ký lỗi".

### 9.23 S8 • Update Prompt (3 biến thể)

#### S8 — Optional Update

> **Purpose**: Thông báo có bản mới, không bắt buộc.

**Entry**: app start hoặc WorkManager periodic check phát hiện update không-force.
**Exit**: "Cập nhật" → S8a; "Để sau" → đóng dialog, snooze 24h.

**Mô tả nội dung**: Dialog medium giữa screen. Headline "Có bản mới — phiên bản 1.0.2"; body release notes (max 5 dòng); 2 nút: "Để sau" (secondary) | "Cập nhật" (primary, default focus). Nếu Notes dài, dialog cho phép D-pad Up/Down scroll trong vùng note.

#### S8a — Downloading Update

> **Purpose**: Đang tải APK mới.

**Mô tả nội dung**: Dialog không cancel-able (chỉ pause; nhưng V1 không có pause download — chỉ "Hủy"). Headline "Đang tải bản cập nhật". Body: progress bar to + text "{downloaded MB} / {total MB} ({percent}%)". Nút secondary "Hủy" (về S8).

#### S8b — Force Update

> **Purpose**: Phiên bản cũ không còn được hỗ trợ. Bắt buộc update.

**Mô tả nội dung**: Full-screen takeover (không phải dialog). Headline lớn "Cập nhật bắt buộc". Caption "Phiên bản hiện tại không còn được hỗ trợ. Cập nhật để tiếp tục sử dụng." Nút primary lớn "Cập nhật ngay". KHÔNG có nút Back; phím Back bị nuốt.

#### S8c — Rollback (silent)

**Mô tả nội dung**: Nếu RollbackGuard quyết định rollback, hiện toast nhỏ "Phiên bản trước đang được khôi phục..." rồi tự xử lý. Không yêu cầu user thao tác.

### 9.24 S9 • First-run Onboarding

> **Purpose**: Sau khi cài lần đầu và login lần đầu, hướng dẫn nhanh.

**Entry**: chỉ chạy đúng 1 lần ngay sau khi login đầu tiên thành công.
**Exit**: hoàn tất → S2.

**Mô tả nội dung**: 3 bước carousel ngang, mỗi bước 1 màn hình kèm icon vector lớn + headline + 1 dòng body + nút "Tiếp" / cuối là "Bắt đầu":
- Bước 1 — "Điều khiển bằng remote": icon D-pad, body "Dùng phím lên xuống trái phải để chọn, OK để xác nhận, Back để quay lại."
- Bước 2 — "Tiếp tục xem dở": icon đồng hồ play, body "Mỗi video bạn xem được lưu vị trí — quay lại bất cứ lúc nào."
- Bước 3 — "Cập nhật tự động": icon download, body "App tự kiểm tra bản mới, bạn không cần làm gì."

Nút "Bỏ qua" ở góc dưới phải (secondary).

**Components**: vector icons + `Button`.

**Default focus**: nút "Tiếp".

### 9.25 S10 • Confirm Dialogs (chung)

Một component thiết kế chuẩn cho confirm. Variant:

- **Confirm-Destructive** (xoá file local, đăng xuất): headline + body cảnh báo + 2 nút: "Hủy" (default focus) | "Xoá/Đăng xuất" (destructive style — nền đỏ `state/error`, text trắng).
- **Confirm-Standard** (thoát phim, thoát app): 2 nút "Không" (default) | "Có" (primary).
- **Loading-Confirm** (đang xử lý lâu): button có loading state, không cancel-able trong khi loading.

### 9.26 S11 • Global Error / No Network

> **Purpose**: Khi mất mạng giữa chừng hoặc server không phản hồi.

**Mô tả nội dung**: Overlay full-screen với icon lớn (wifi-off hoặc cloud-off), headline "Không có kết nối", body "Vui lòng kiểm tra mạng và thử lại." Hai nút: "Cài đặt mạng" → mở Intent system; "Thử lại" (default focus). Nếu offline > 60s thì giữ overlay; nếu reconnect: tự ẩn.

**Components**: `ErrorState` (C15) variant fullscreen.

### 9.27 S12 • Logout / Switch User

Không phải screen riêng — là **flow** từ S7a → confirm dialog → S1.

Dialog confirm xoá session local: "Đăng xuất khỏi tài khoản {email}? Lịch sử xem dở local sẽ giữ lại." Nút "Hủy" / "Đăng xuất". Sau OK: clear EncryptedSharedPreferences, navigate S1.

---

# Phần C — User Flows

## 10. User flows chính

Tài liệu này mô tả 6 flow cốt lõi end-to-end. Đội design dùng để hiểu chuyển cảnh giữa các screen, build prototype Figma click-through.

### 10.1 Flow 1 — First-run đến lượt xem đầu tiên

Mục tiêu: từ khi cài app lần đầu đến khi xem được video đầu tiên trong **dưới 3 phút**.

```
[Sau khi cài APK xong, mở app]
   → S0 Splash (1–2s)
   → S1 Login Hub
   → user chọn "Đăng nhập QR" (default focus, OK)
   → S1a Login QR
   → user mở app Fshare mobile, scan QR (15–30s)
   → S0 splash chuyển tiếp ngắn
   → S9 First-run Onboarding (3 bước, ~30s)
   → S2 Home
   → user di chuyển focus xuống Row 3 "Thư mục" hoặc Row 2 "File gần đây"
   → chọn item → S4 File Detail
   → focus mặc định nút "Phát ngay" → OK
   → S5 Player loading (1–3s)
   → Playing
```

Điểm cần design chú ý:
- Splash → Login Hub không được "lóa". Transition dùng `motion/quick`, không fade chậm.
- Sau scan QR thành công → S2 không nên nhấp nháy login screen lần nữa. Direct navigate có pre-load skeleton Home.
- Onboarding chỉ chạy đúng 1 lần. Cờ trong DataStore.
- Nếu user "Bỏ qua" Onboarding ở bước 1 → vẫn coi là đã hoàn tất.

### 10.2 Flow 2 — Browse → Play

Đây là flow phổ biến nhất sau khi đã có session.

```
[App đã mở, đang ở S2 Home]
   → user nhấn Down vài lần đến row "Thư mục"
   → chọn folder "Phim" → S3 Browse "Phim"
   → có thể vào subfolder S3 "Hành động"
   → chọn file → S4 File Detail
   → "Phát ngay" → S5 Player
   → xem
   → Back → confirm S5d → "Có" → S4
   → Back → S3
   → Back → S2 (focus restore về row "Thư mục", folder "Phim")
```

Điểm chú ý:
- Focus restoration là quan trọng nhất. Mỗi lần Back, user phải về đúng phần tử cũ (không reset top).
- Khi vào subfolder, breadcrumb cập nhật ngay; loading skeleton khoảng 200–800ms tuỳ network.
- Nếu file là video → focus default trên Phát; nếu là image/zip → trên Tải về.

### 10.3 Flow 3 — Continue Watching

Flow user quay lại sau 1 ngày, tiếp tục phim đang xem.

```
[Mở app, đã có session]
   → S0 Splash
   → S2 Home
   → focus mặc định ở Row 1 "Tiếp tục xem", item đầu tiên (phim đang xem dở mới nhất)
   → user OK luôn
   → S5 Player loading
   → S5d Resume Prompt — "Tiếp tục từ 00:42:13?"
   → focus default "Tiếp tục" → OK
   → Player resume từ giây 42:13
```

Điểm chú ý:
- Row "Tiếp tục xem" phải hiện trước cả "File gần đây" để user 1-click resume.
- Mỗi card trong row này cần progress bar mảnh dưới thumbnail (visual thấy ngay).
- Resume prompt KHÔNG được skip silently. User phải có quyền chọn xem lại từ đầu.

### 10.4 Flow 4 — Tải file về USB

Flow ít dùng hơn nhưng vẫn cần cho user offline-first.

```
[App đang ở S2 hoặc S3]
   → user vào file → S4 File Detail
   → focus xuống "Tải về thiết bị" → OK
   → nếu chưa có USB cắm → dialog confirm "Tải vào Internal storage? Còn 8GB."
   → nếu có USB OTG → dialog chọn vị trí: USB ({label}) (default focus) / Internal
   → confirm → bắt đầu tải
   → Toast "Đã thêm vào tải xuống — {filename}"
   → S4 vẫn ở; user có thể browse tiếp
   → tại bất kỳ lúc nào, vào S2 → row "Đã tải xuống" hiện file đang tải có progress
   → vào S6 Downloads để quản lý
   → khi xong, notification toast trên S2 "Đã tải xong: {filename}"
```

Điểm chú ý:
- Pre-check disk space — nếu không đủ, error rõ ràng "Cần {needed} MB, còn {free} MB".
- Tải background qua Foreground Service — user có thể vào Player xem file khác mà không dừng tải.
- USB rút giữa chừng → tải pause + notification "Đã pause, cắm USB lại để tiếp tục".

### 10.5 Flow 5 — In-app update (auto)

Flow user bị động nhận update.

```
[App start hoặc WorkManager 6h tick]
   → CheckUpdateUseCase fetch manifest
   → có update không-force
   → S2 Home + dialog S8 Optional Update overlay (sau khi data Home đã load)
   → user OK "Cập nhật"
   → S8a Downloading (giữ ở S2 trong bg, dialog phủ trên)
   → tải xong, verify SHA-256, verify signature
   → OS popup "Install update?" (PackageInstaller)
   → user OK trên OS popup
   → app kill, OS cài, app restart → S0 Splash → S2 (đã ở version mới)
```

Lưu ý design:
- Dialog Optional Update KHÔNG được cản trở user khi đang xem phim (S5). Nếu update phát hiện trong khi player đang chạy: defer notification, hiện badge nhỏ trên Settings; chỉ prompt khi quay về S2.
- Force update: bypass tất cả, full-screen takeover.

### 10.6 Flow 6 — Logout / Switch user

```
[App đang ở S2]
   → user vào TopBar → Avatar → S7a Account
   → focus mặc định "Chuyển tài khoản" (hoặc "Đăng xuất")
   → OK → confirm dialog
   → "Đăng xuất" → clear session
   → S1 Login Hub
   → user có thể login tài khoản khác qua QR / email
```

Lưu ý: lịch sử local (resume position, downloaded file) **giữ lại theo file path**, không bị xoá. Nếu user khác login lại đúng tài khoản trước, sẽ thấy lịch sử cũ.

---

# Phần D — Ràng buộc, deliverables, ghi chú

## 11. Accessibility

Bắt buộc V1:
- Mọi interactive element có `contentDescription` rõ ràng.
- Hỗ trợ TalkBack: navigation thông qua TalkBack swipe gestures (chấp nhận trên TV với remote khi user enable từ OS Settings).
- Phụ đề trong player có 4 cỡ điều chỉnh được (xem S7b).
- High contrast mode: option cho user chuyển palette focus từ vàng sang vàng-đen tăng tương phản (V1 nice-to-have, V1.1 chắc chắn).
- Reduce motion: nếu OS report reduce-motion, tắt mọi animation không thiết yếu (focus shift instant, dialog không slide-in).

V1 không hỗ trợ:
- Voice commands (V2).
- Screen magnifier (Android TV ít hỗ trợ, không scope).
- Color blind mode (V1.1).

## 12. Localization

- Default: tiếng Việt (`vi-VN`). Là source.
- Override: English (`en`). Đội design phải kiểm tra cả 2 ngôn ngữ trên mọi screen.
- Quy tắc string: dùng placeholder `{name}` thay vì nối chuỗi trong code → tránh dịch sai.
- Font Inter đã có Latin extended; tiếng Việt đầy đủ dấu, không cần font phụ.
- Date format: `dd/MM/yyyy` (vi), `MMM dd, yyyy` (en).
- Number/size: `1.2 GB` cả hai ngôn ngữ; `1,234 lượt xem` (vi) / `1,234 views` (en).
- KHÔNG localize: brand name "Fshare", "FsNext TV", và các API code nội bộ.

V1 không hỗ trợ RTL (không có ngôn ngữ RTL trong scope). Tuy nhiên tổ chức layout nên dùng `start/end` thay `left/right` để tương lai không khó port.

## 13. Performance budgets

Tham chiếu cho design — không phải target tối đa mà là **giới hạn không vượt**:

| Hạng mục | Budget |
|---------|--------|
| Cold start → S2 Home hiển thị | ≤ 2.5s trên Mi Box S, ≤ 4s trên FPT Play Box (Android 7) |
| Frame budget animation | 60 fps (16.6 ms/frame); rớt < 5% trong 1 phút use |
| Số ảnh thumbnail load đồng thời trên S2 | ≤ 24 (4 row × 6 visible) |
| Kích thước thumbnail từ network | ≤ 60 KB mỗi cái (target 30–40 KB) |
| Image asset trong APK | ≤ 500 KB tổng |
| APK size (universal) | ≤ 30 MB |
| Memory peak | ≤ 250 MB (app + player) |
| Player time-to-first-frame | ≤ 3s (sau khi đã có URL stream) |

Hệ luỵ cho design:
- Không design background ảnh cho mỗi screen → sẽ vượt budget memory.
- Không design illustration vẽ tay nặng → vector hoặc icon.
- Hạn chế lottie animation; nếu dùng, lottie file < 100 KB.

## 14. Yêu cầu deliverables từ đội Design (tóm tắt)

### 14.1 Figma file structure

Tổ chức Figma file thành 4 trang/section:
1. **Foundation**: design tokens (color/typography/spacing), grid, focus chuẩn.
2. **Components**: tất cả 24 component ở mục 8, mỗi component có variant đầy đủ states.
3. **Screens**: nhóm theo S0 → S12, thứ tự như mục 9, mỗi screen có biến thể nếu có (ví dụ S8a/b/c).
4. **Flows / Prototype**: kết nối click-through cho 6 user flow ở mục 10.

Nên có một trang phụ "Asset exports" với mọi vector cần xuất ra cho dev (icons, logo, illustration nếu có).

### 14.2 Design tokens export

Xuất tokens dưới dạng **JSON** theo format `Tokens Studio for Figma` hoặc `Style Dictionary`. Dev sẽ chuyển sang Compose `MaterialTheme` extension. Tối thiểu xuất:
- color (mục 6.1)
- typography (6.2)
- spacing (6.3)
- elevation (6.4)
- radius (6.5)
- motion (6.8)

### 14.3 Asset exports

Vector dạng SVG, đặt tên `kebab-case`:
- `ic-{name}.svg` cho icon (size 24/32/40 dp version theo Material Symbols).
- `illust-{name}.svg` cho illustration nếu có (V1 hạn chế).
- `logo-fshare.svg`, `logo-fshare-tv.svg`.

Bitmap chỉ trong 2 trường hợp duy nhất: poster brand cho splash/about (WebP, < 100 KB) và placeholder thumbnail (1 file solid color SVG, 1 KB).

### 14.4 Prototype

Build click-through prototype trong Figma cho **6 user flow** ở mục 10. Đủ dùng để demo cho stakeholder và làm baseline cho QA viết test. Không cần animation phức tạp; chỉ cần navigation đúng.

### 14.5 Review milestones

Đề xuất 3 mốc review giữa Design và Product/Eng/QA:

| Mốc | Khi nào | Nội dung |
|-----|---------|----------|
| R1 | Sau khi xong Foundation + Component library | Review tokens, focus model, 24 component. Lock token version. |
| R2 | Sau khi xong S0–S5 (luồng login + browse + player) | Review flow chính. Test prototype với 3–5 người trong team. |
| R3 | Sau khi xong full catalog + flows | Final review, handoff cho engineering. |

### 14.6 Working agreements (đề xuất)

- Source of truth: Figma file (link sẽ bổ sung).
- Mọi thay đổi tokens sau R1 cần PM + Tech Lead approve (vì code có thể đã wire).
- Component library version semantic: `lib v0.1` → `v1.0` ở R3.
- Question/comment dùng Figma comments; quyết định lớn ghi vào sheet "Design Decisions" (chia sẻ với eng).

## 15. Open questions

Các điểm cần làm rõ trước/trong khi design:

1. **Backend QR login**: API hiện tại của Fshare có endpoint `qrlogin/start` + polling chưa? Nếu chưa → backend phải build trước. Cần xác nhận với backend team trước mốc R1.
2. **Thumbnail của file phim**: backend có tạo thumbnail tự động chưa? Nếu chưa, dùng generic icon. Cần thumbnail cho Continue Watching và File Detail thì UX mới đẹp.
3. **Stream URL transcode**: hiện file MKV 1080p ~10 GB sẽ stream gốc. Có lộ trình transcode adaptive không? Ảnh hưởng UX "chất lượng trên Wi-Fi yếu".
4. **Account level / VIP UI**: thông tin VIP hiện tại ở desktop có pie chart traffic. TV có cần chi tiết tương đương không, hay đơn giản hoá?
5. **Tên app trên launcher**: "Fshare TV" hay "FsNext"? Quyết định brand chốt trước Splash design.
6. **App icon**: cần design 320×320 px adaptive icon cho Android TV; cần fr layer + bg layer.
7. **Onboarding skip-able lần đầu?** Hiện đề xuất có nút "Bỏ qua" — confirm với PM.
8. **Continue Watching cross-device**: V1 chỉ lưu local; có ý định sync server không? (V2 bàn).

## 16. Glossary

- **APK**: Android Package — file cài app.
- **D-pad**: 5 nút trên remote (lên, xuống, trái, phải, OK).
- **Focus**: phần tử đang được hệ thống chọn làm "active"; phải có visual rõ.
- **Overscan**: vùng mép màn hình mà TV cũ có thể cắt — vùng cấm cho nội dung quan trọng.
- **10-foot UI**: nguyên lý thiết kế cho khoảng cách 3 m (~10 feet).
- **HDR / SDR**: trên V1 chỉ SDR.
- **HEVC / H.265**: codec video phổ biến cho 4K — TV phải hỗ trợ hardware để play mượt.
- **Resume playback**: tự nhớ vị trí đang xem.
- **Sideload**: cài APK trực tiếp, không qua app store.
- **Segment download**: tải song song nhiều phần để tăng tốc.
- **Continue Watching**: row trên Home liệt kê phim đang xem dở.

## 17. Revision history

| Version | Date | Author | Note |
|---------|------|--------|------|
| 1.0 | 2026-05-04 | Product + Tech Lead | Draft khởi tạo cho design review |

— Hết tài liệu Design Specification V1.0 —
