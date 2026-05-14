---
title: 10 — Yêu cầu Design Handoff (export từ Design system → Engineering)
version: 1.0
date: 2026-05-04
audience: Design system team
owner: Tech Lead Android + Product Manager
status: Bắt buộc đáp ứng trước khi engineering bắt đầu Phase 2 UI shell
---

# Yêu cầu Design Handoff

## 0. Mục đích

Khi đội Design báo "đã thiết kế xong" trên Figma, engineering không thể bắt đầu implement nếu deliverables không đầy đủ và đúng format. Tài liệu này liệt kê **toàn bộ yêu cầu** để handoff thành công, bao gồm:

- File Figma phải có cấu trúc gì.
- Phải export ra những asset nào, định dạng nào, đặt tên ra sao.
- Tokens xuất theo schema nào để chuyển sang Compose tự động.
- Specs từng component và từng screen cần annotate gì.
- Prototype, motion, localization phải có gì.
- Quy trình review và chốt handoff.

Ngắn gọn: nếu mọi mục trong checklist mục 13 đều TICK, engineering sẽ bắt đầu được trong **2 ngày**. Nếu thiếu, mỗi item thiếu = thêm 0.5–2 ngày tuỳ mức độ.

## 1. Tổng quan deliverables

10 nhóm deliverables — **bắt buộc đủ trước khi handoff**:

| # | Nhóm | Định dạng | Ai dùng | Block engineering nếu thiếu? |
|---|------|-----------|---------|------------------------------|
| D1 | File Figma master | Figma cloud, có Dev Mode license | Eng + QA + PM | **CÓ** |
| D2 | Design tokens | JSON (Tokens Studio hoặc Style Dictionary) | Eng | **CÓ** |
| D3 | Icon assets | SVG (vector) | Eng | **CÓ** cho V1 |
| D4 | Logo & brand assets | SVG + PNG | Eng | **CÓ** |
| D5 | App icon (launcher) | PNG/XML adaptive | Eng | **CÓ** |
| D6 | Splash screen | SVG + spec | Eng | **CÓ** |
| D7 | Illustration / placeholder | SVG hoặc WebP | Eng | KHÔNG (ít dùng theo nguyên tắc N4) |
| D8 | Component specs | Trong Figma (Dev Mode) | Eng | **CÓ** |
| D9 | Screen specs (mọi state) | Trong Figma | Eng + QA | **CÓ** |
| D10 | Prototype 6 user flows | Figma prototype | QA + PM | **CÓ** |
| D11 | Motion / animation specs | Tài liệu + Lottie nếu có | Eng | KHÔNG (default sẽ dùng motion tokens nếu thiếu) |
| D12 | Localization keys + strings | CSV hoặc XLSX | Eng | **CÓ** |
| D13 | Sample / placeholder content | Trong Figma | Eng + QA | **CÓ** |
| D14 | Accessibility audit | Tài liệu/báo cáo | Eng + QA | KHÔNG (nice-to-have) |

Mỗi nhóm chi tiết ở các mục 2–14.

---

## 2. D1 — File Figma master

### 2.1 Truy cập

- File Figma đặt trong **team workspace** của công ty (KHÔNG file cá nhân designer).
- Quyền: ít nhất Tech Lead Android + 2 Senior Eng + QA Lead + PM có quyền **View + Dev Mode**. Designer giữ Edit.
- File phải có **Dev Mode** bật (Figma Professional trở lên).
- Link file dán vào tài liệu kế hoạch dự án; KHÔNG share qua chat thoáng.

### 2.2 Cấu trúc Pages

File Figma phải có **đúng 6 pages** theo thứ tự dưới đây. Designer KHÔNG được tự ý đổi tên/thứ tự (vì PM/Eng tham chiếu):

1. **🟡 Cover** — bìa file: tên dự án, version, designer, last update, status (Draft / In review / Released).
2. **📐 Foundation** — design tokens (color, typography, spacing, elevation, radius, motion), grid system, focus model, sample focus rendering.
3. **🧱 Components** — toàn bộ 24 component của library (xem `09_design-spec.md` §8). Mỗi component đứng độc lập, có variants đầy đủ, có annotation states.
4. **📱 Screens** — toàn bộ screen S0–S12 (xem `09_design-spec.md` §9). Sắp xếp theo thứ tự ID. Mỗi screen có mọi state.
5. **🔁 Flows / Prototype** — frames cho 6 user flow (xem `09_design-spec.md` §10) đã connect thành prototype click-through.
6. **📦 Assets to export** — tập hợp riêng tất cả assets cần export (icon, logo, illustration). Mỗi asset là một frame có tên đúng convention (xem §6.4).

### 2.3 Naming conventions trong Figma

- **Page**: emoji + tên, như danh sách trên.
- **Frame**: theo screen ID. Ví dụ: `S2 / Home / loaded`, `S5a / Player Overlay / focus on play`.
- **Component**: `{Group}/{Name}`. Ví dụ: `Card/FileCard`, `Button/Primary`.
- **Variant property**: dùng property name chuẩn (`State=Default | Focus | Selected | Disabled`), không tự đặt.
- **Style**: token name `category/subcategory/variant`. Ví dụ: `color/text/primary`, `typography/title`.
- KHÔNG dùng tên Vietnamese không dấu trong Figma (nhầm lẫn key tokens). Tên hiển thị cho người dùng (button label) thì OK.

### 2.4 Library publishing

- Foundation và Components phải được **publish** ra một Figma Library team-wide. Engineering sẽ subscribe để bất cứ thay đổi tokens nào tự động sync vào Dev Mode specs.
- Library phải có version semantic (sử dụng Figma branching nếu có Enterprise; nếu Pro → tag bằng tên trong Cover page).

### 2.5 File health

- Không có "Local style" lẻ — mọi style phải dùng từ Foundation.
- Không có component lẻ ngoài thư viện — mọi instance phải point tới Components page.
- "Detached instance" báo đỏ trong Figma — phải ≤ 5 trên toàn file (chấp nhận edge case prototype).
- Không có "Hidden layer" thừa từ exploration cũ — clean trước khi handoff.

---

## 3. D2 — Design tokens (JSON export)

### 3.1 Vì sao cần JSON

Engineering không "đọc bằng mắt" tokens từ Figma. Cần JSON để chạy script chuyển sang Kotlin/Compose code. Mỗi lần token đổi → re-run script → code update.

### 3.2 Format chấp nhận

**Ưu tiên**: format của plugin **Tokens Studio for Figma** (`design-tokens.json`).
**Backup**: format **Style Dictionary** (`tokens.json`).

Hai format đều support; designer chọn 1, ghi rõ lựa chọn vào Cover page.

### 3.3 Nội dung tối thiểu

Mỗi nhóm phải xuất:

```json
{
  "color": {
    "bg": {
      "base": { "value": "#0F1218", "type": "color" },
      "surface": { "value": "#1A1F2A", "type": "color" },
      "surface-hi": { "value": "#252B38", "type": "color" }
    },
    "text": {
      "primary": { "value": "#EEF1F6", "type": "color" },
      "secondary": { "value": "#A9B1BD", "type": "color" }
    },
    "accent": { "primary": { "value": "#FFB12C", "type": "color" } },
    "state": { ... },
    "focus": { ... }
  },
  "typography": {
    "display-md": {
      "value": {
        "fontFamily": "Inter",
        "fontWeight": 700,
        "fontSize": "48",
        "lineHeight": "56",
        "letterSpacing": "0"
      },
      "type": "typography"
    }
  },
  "spacing": {
    "1": { "value": "4", "type": "spacing" },
    "2": { "value": "8", "type": "spacing" }
  },
  "radius": { ... },
  "elevation": {
    "elev-1": {
      "value": "0 4 12 0 rgba(0,0,0,0.4)",
      "type": "boxShadow"
    }
  },
  "motion": {
    "quick": {
      "value": { "duration": "100", "easing": "cubic-bezier(0.2,0,0,1)" },
      "type": "transition"
    }
  }
}
```

### 3.4 Token naming rules

- Tên token = tên category/subcategory/variant kebab-case.
- Reference token (alias) phải dùng `{token.path}` syntax. Ví dụ:
  ```json
  "button-primary-bg": { "value": "{color.accent.primary}", "type": "color" }
  ```
- KHÔNG hard-code hex trong tokens "semantic" — phải reference về tokens "primitive". Nguyên tắc 2 lớp:
  - **Primitive**: `color/orange/500 = #FFB12C`
  - **Semantic**: `color/accent/primary = {color.orange.500}`
  - **Component**: `color/button/primary/bg = {color.accent.primary}`

### 3.5 Export quy trình

1. Designer chạy plugin Tokens Studio → "Export to JSON".
2. Commit file `design-tokens.json` vào repo `fsnext-tv/design-system/` qua PR.
3. CI có script `./gradlew generateTheme` đọc JSON → sinh `FsTvTheme.kt`.

### 3.6 Mapping JSON → Compose (engineering tự lo, info cho design biết)

Để designer hiểu ràng buộc:

| JSON path | Compose equivalent |
|-----------|--------------------|
| `color.accent.primary` | `MaterialTheme.colorScheme.primary` |
| `color.bg.base` | `MaterialTheme.colorScheme.background` |
| `color.bg.surface` | `MaterialTheme.colorScheme.surface` |
| `color.text.primary` | `MaterialTheme.colorScheme.onBackground` |
| `typography.display-md` | `MaterialTheme.typography.displayMedium` |
| `spacing.4` | `FsTokens.Spacing.md` (custom extension) |
| `radius.md` | `FsTokens.Radius.md` |
| `motion.quick` | `FsTokens.Motion.Quick` (custom: pair Duration + Easing) |

→ Designer giữ token names ổn định; nếu phải đổi, báo Eng trước 1 tuần.

---

## 4. D3 — Icon assets (SVG)

### 4.1 Format

- **SVG** vector (KHÔNG bitmap).
- Viewbox vuông: `0 0 24 24` (hoặc `0 0 32 32` cho icon kích thước lớn).
- Stroke width chuẩn 2 px (cho stroke-style icon) hoặc filled (cho solid icon).
- Color dùng `currentColor` để Compose tint linh hoạt:
  ```svg
  <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
    <path d="..." fill="currentColor"/>
  </svg>
  ```
- KHÔNG có `<defs>` thừa, KHÔNG embed style.
- KHÔNG có meta data, comment, generator tag — clean trước export. Dùng plugin **SVGOMG** hoặc **SVGO** để minify.
- File size mỗi icon ≤ 4 KB.

### 4.2 Set icons V1

Engineering cần **đầy đủ** danh sách dưới đây. Thiếu bất kỳ icon nào sẽ dùng placeholder Material default → mất brand consistency.

| Tên file | Dùng ở screen | Mô tả |
|----------|--------------|-------|
| `ic_play.svg` | S5a | Tam giác play |
| `ic_pause.svg` | S5a | Hai gạch pause |
| `ic_skip_next.svg` | S5a | Tua next |
| `ic_skip_prev.svg` | S5a | Tua prev |
| `ic_forward_10.svg` | S5a | +10s |
| `ic_replay_10.svg` | S5a | -10s |
| `ic_subtitles.svg` | S5a, S5b | CC |
| `ic_audio_track.svg` | S5a, S5b | Note nhạc |
| `ic_settings.svg` | S2, S5a | Bánh răng |
| `ic_back.svg` | mọi screen | Mũi tên trái |
| `ic_close.svg` | dialog | Dấu X |
| `ic_check.svg` | confirm, selected | Check |
| `ic_chevron_right.svg` | settings list | > |
| `ic_chevron_down.svg` | dropdown | v |
| `ic_search.svg` | TopBar (V2 placeholder) | Kính lúp |
| `ic_download.svg` | S4, S6 | Mũi tên xuống |
| `ic_favorite.svg` | S4 | Sao rỗng |
| `ic_favorite_filled.svg` | S4 | Sao đặc |
| `ic_folder.svg` | S2, S3 | Folder rỗng |
| `ic_folder_open.svg` | S3 breadcrumb | Folder mở |
| `ic_file.svg` | S3 | File generic |
| `ic_video.svg` | S3 | File video |
| `ic_audio.svg` | S3 | File audio |
| `ic_image.svg` | S3 | File ảnh |
| `ic_archive.svg` | S3 | File zip/rar |
| `ic_qr.svg` | S1 | Mã QR |
| `ic_email.svg` | S1, S7a | Phong bì |
| `ic_oauth_google.svg` | S1c | Google logo (dùng branded) |
| `ic_oauth_fpt.svg` | S1c | FPT ID logo (branded) |
| `ic_user.svg` | S7a | Người |
| `ic_logout.svg` | S7a | Mũi tên ra |
| `ic_wifi.svg` | S7d, S11 | Sóng wifi |
| `ic_wifi_off.svg` | S11 | Wifi bị gạch |
| `ic_warning.svg` | dialog warning | Tam giác cảnh báo |
| `ic_error.svg` | error state | Vòng tròn ! |
| `ic_info.svg` | toast info | Vòng tròn i |
| `ic_success.svg` | toast success | Vòng tròn ✓ |
| `ic_dpad.svg` | S9 onboarding | D-pad illustration nhỏ |
| `ic_remote.svg` | tutorial | Remote TV |
| `ic_update.svg` | S7e, S8 | Mũi tên xoay tròn |
| `ic_vip_badge.svg` | S7a, S2 | Vương miện hoặc chữ VIP |
| `ic_storage.svg` | S7a, S7c | Khối lưu trữ |
| `ic_usb.svg` | S7c | Cổng USB |
| `ic_play_circle.svg` | S5e post-play | Play trong vòng |
| `ic_replay.svg` | S5e | Mũi tên xoay ngược |

**Tổng**: ~45 icon. Có thể thêm/bớt theo final design.

### 4.3 Đặt tên & cấu trúc

- Format: `ic_{name}.svg` — toàn bộ snake_case.
- KHÔNG có hậu tố `_24dp` trong tên file (kích thước render do Compose decide qua `Modifier.size()`).
- Nếu icon có biến thể filled/outlined → 2 file riêng: `ic_favorite.svg` (outlined) và `ic_favorite_filled.svg`.
- Đặt trong folder Figma export `assets/icons/`.

### 4.4 Validation trước handoff

Designer chạy checklist mỗi icon:
- [ ] Viewbox `0 0 24 24` (hoặc `32 32`)
- [ ] Color = `currentColor` (không hex cứng trừ icon brand như Google logo)
- [ ] Đã chạy SVGO minify
- [ ] File size ≤ 4 KB
- [ ] Render đúng ở 24 dp, 32 dp, 40 dp
- [ ] Có pixel-grid alignment ở stroke (không bị blur)

---

## 5. D4 — Logo & brand assets

### 5.1 Cần xuất

| Asset | Format | Kích thước | Dùng |
|-------|--------|-----------|------|
| `logo_fshare.svg` | SVG | viewBox 240×60 | Logo chữ Fshare gốc, dùng ở TopBar |
| `logo_fshare_tv.svg` | SVG | viewBox 320×80 | Logo "Fshare TV" cho Splash, About |
| `logo_fshare_mark.svg` | SVG | viewBox 64×64 | Mark vuông (chỉ chữ F hoặc icon) cho Avatar fallback |
| `logo_fshare_white.svg` | SVG | viewBox 240×60 | Logo trên nền tối (mặc định) |
| `logo_fshare_dark.svg` | SVG | viewBox 240×60 | Logo trên nền sáng (cho doc, web) |
| `wordmark_fsnext_tv.svg` | SVG | viewBox 320×60 | Wordmark "FsNext TV" type-treated |

### 5.2 Quy tắc clear-space

Designer phải document clear-space xung quanh logo (vùng cấm đặt nội dung):
- Clear-space = chiều cao 1 đơn vị "F" trong logo.
- Tài liệu này dạng PDF 1 trang đính kèm trong Figma Cover.

### 5.3 Minimum size

- Logo chữ: tối thiểu **96 dp** chiều ngang.
- Logo mark: tối thiểu **32 dp**.
- Dưới mức này, dev không được dùng — design phải nói rõ thay thế bằng gì (thường là chỉ mark).

---

## 6. D5 — App icon (launcher)

Android TV launcher có 2 yêu cầu icon khác Android phone:

### 6.1 Banner (bắt buộc cho Android TV)

- Kích thước: **320 × 180 px** (TV banner ratio 16:9).
- Format: PNG (sRGB), KHÔNG transparent (background nguyên hình).
- Tên file: `ic_banner.png` đặt trong `res/drawable-xhdpi/`.
- Nội dung: logo + wordmark + có thể accent color background. Designer kiểm tra render trên emulator Android TV trước khi giao.

### 6.2 Adaptive icon (Android 8+)

Hai layer XML + foreground/background image:

- Foreground: `ic_launcher_foreground.svg` (vector) — chỉ logo mark, viewbox 108×108 dp với "safe zone" 66×66 dp ở giữa (hệ điều hành sẽ mask).
- Background: `ic_launcher_background.svg` hoặc 1 màu solid — Designer chỉ định màu. Mặc định: `#0F1218` (bg/base).
- Round icon: optional, giống foreground nhưng có circle mask preview.

Designer xuất 2 file SVG, engineering chuyển sang VectorDrawable:
```
res/drawable/ic_launcher_foreground.xml
res/drawable/ic_launcher_background.xml
res/mipmap-anydpi-v26/ic_launcher.xml
```

### 6.3 Notification icon

- File: `ic_notification.svg` viewbox 24×24, monochrome (white only — Android tô màu).
- Cho download progress notification, update notification.

### 6.4 Asset frame trong Figma

Đặt mọi asset xuất trong page **📦 Assets to export**, mỗi asset là một frame có:
- Tên frame = tên file mong muốn (kèm extension): `ic_play.svg`, `ic_banner.png`.
- Frame size = kích thước thật (24×24 hoặc 320×180, …).
- Property "Export" của frame được set sẵn (SVG hoặc PNG 1x).

Engineering chỉ cần "Select all → Export" là ra đúng tên file.

---

## 7. D6 — Splash screen

Android 12+ dùng **SplashScreen API** — không tự design ảnh splash full-screen như xưa. Yêu cầu:

- **`ic_splash_logo.svg`** — 1 vector logo vuông (viewbox 288×288 dp, content trong vùng 192×192 dp giữa). Đây là "splash icon".
- **Splash background color**: 1 hex color, mặc định `#0F1218`.
- Optional: **`ic_splash_branding.svg`** — wordmark hiện ở dưới cùng, height 80 dp.

Designer cung cấp các file trên + spec:
- Animation enter (fade hay scale): chấp nhận default Android 12+ (KHÔNG tự custom phức tạp).
- Tổng thời gian splash: **≤ 1.5 giây**. Sau thời gian này, dù app chưa ready, vẫn fade ra để show S0 hoặc S2.

---

## 8. D7 — Illustration & placeholder (V1 hạn chế)

Theo nguyên tắc N4 (tránh hình ảnh nặng), V1 chỉ dùng illustration ở 3 chỗ:

| Asset | Dùng ở | Định dạng |
|-------|--------|-----------|
| `illust_empty_folder.svg` | EmptyState ở S3 khi folder trống | SVG, ≤ 8 KB |
| `illust_no_network.svg` | S11 Global error | SVG, ≤ 8 KB |
| `illust_no_files.svg` | S2 khi tài khoản trống | SVG, ≤ 8 KB |

Quy tắc:
- Vector-style đơn giản, monochrome hoặc 2-color (1 main + 1 accent).
- Không có nhân vật, không có scene phức tạp.
- Render ổn ở size 96 dp đến 240 dp.

Placeholder thumbnail (dùng khi ảnh từ network chưa load xong):
- 1 file SVG `placeholder_thumbnail.svg` viewbox 320×180, solid color `bg/surface-hi` + icon `ic_video.svg` ở giữa với opacity 30%.

---

## 9. D8 — Component specs (trong Figma Dev Mode)

### 9.1 Mỗi component cần annotate

Trong Figma Dev Mode, mỗi component (24 component theo `09_design-spec.md` §8) phải có spec đầy đủ. Engineering inspect từ Dev Mode sẽ thấy:

- **Anatomy**: tên các sub-element được đánh số/gán nhãn.
- **States**: tất cả variants (default/focus/selected/disabled/pressed/loading).
- **Dimensions**:
  - Width × height của component (fixed hoặc min/max).
  - Padding inner.
  - Gap giữa sub-element.
- **Tokens**: mỗi color/text/spacing dùng đều phải reference token (không hard-code) — Dev Mode sẽ hiển thị tên token.
- **Behavior notes**: text annotation trong Figma cạnh component nếu có hành vi đặc biệt (ví dụ Card focus → scale 1.05 → cách padding shift ra sao).
- **Touch / focus target**: kích thước hit area tối thiểu 48 × 48 dp ngay cả khi visual nhỏ hơn.

### 9.2 Checklist cho mỗi component

Designer tự verify:
- [ ] Có ít nhất 4 state: default, focus, pressed, disabled.
- [ ] Tất cả color = token reference.
- [ ] Tất cả text = typography token reference.
- [ ] Tất cả spacing = bội số 4 và dùng spacing token.
- [ ] Có frame "examples" với 3–5 use cases (dài, ngắn, có icon, không icon).
- [ ] Có annotation note cho hành vi không trivially nhìn ra được.

### 9.3 Component checklist tổng

Engineering có quyền block handoff nếu thiếu một trong 24 component (`09_design-spec.md` §8 mục C1–C24). Từng component:

- C1 TopBar (default, with-back, with-search-icon, with-avatar)
- C2 Button (primary, secondary, ghost; default/focus/pressed/disabled/loading)
- C3 IconButton (small, medium, large; với mọi state)
- C4 FileCard (default, focus, with-progress, with-badge-vip, with-badge-secure)
- C5 FolderCard
- C6 RowSection (with title, with-action-link)
- C7 ListItem (text-only, with-switch, with-radio, with-icon, with-chevron, two-line)
- C8 Dialog (small, medium, large; with-loading-button)
- C9 BottomSheet
- C10 Snackbar (info, success, warning, error)
- C11 ProgressBar linear (indeterminate, determinate, with-buffer-secondary)
- C12 ProgressCircular
- C13 Skeleton (card, list-item, text-line)
- C14 EmptyState (with-icon-only, with-icon-and-action)
- C15 ErrorState (with-retry, fatal)
- C16 TextField (default, focus, error, disabled, with-leading-icon)
- C17 Switch (on, off, focus, disabled)
- C18 RadioGroup (item: default, focus, selected, disabled)
- C19 Chip (filled, outlined, with-icon)
- C20 Avatar (image, initials, with-status-dot)
- C21 KeyboardOnScreen (full QWERTY VN/EN)
- C22 QRCard
- C23 PlayerOverlay (đặc biệt — anatomy phức tạp, cần spec chi tiết hơn)
- C24 NotificationBadge

---

## 10. D9 — Screen specs (mọi state)

### 10.1 Mỗi screen cần frames cho mọi state

Theo `09_design-spec.md` §9, có ~27 screens. Mỗi screen phải có **một frame riêng cho mỗi state** đã liệt kê. Ví dụ S2 Home có 4 state → 4 frame:

- `S2 / Home / loading`
- `S2 / Home / loaded`
- `S2 / Home / partial`
- `S2 / Home / empty-account`

Engineering không "đoán" từ default state ra empty/error state — phải có visual gốc.

### 10.2 Annotation bắt buộc trên mỗi screen

Designer thêm **annotation layer** (text dạng chú thích) trên mỗi screen ghi:

1. **Screen ID + Name** (góc trên trái frame).
2. **Default focus** — vẽ vòng đặc biệt + chú thích "Default focus on load".
3. **D-pad behavior khác mặc định** — nếu có (ví dụ ở Player).
4. **Edge cases** — text note bên cạnh frame liệt kê edge case từ `09_design-spec.md`.
5. **Open questions** — note gắn vào element nào chưa chốt.

Annotation dùng plugin **Figma Notes** hoặc style chuẩn của design team (text màu accent, frame border dashed). Không dùng sticky-note Figjam (lạc lõng).

### 10.3 Multi-state grouping

Các screen có nhiều state (như S5 Player với 6 state) → group thành 1 section, có header frame "S5 — Player" rồi xếp dọc các state bên dưới. Dễ navigate.

### 10.4 Responsive / overscan

Mỗi screen vẽ trên canvas **1920 × 1080** (TV 1080p logical pixel). Trong canvas có guide:
- 5% overscan vùng (border đỏ mảnh).
- 12-column grid với gutter 16 dp.

Designer KHÔNG vẽ cho 4K (3840×2160) — Compose tự scale. Tuy nhiên typography phải đảm bảo readability ở 4K (test bằng zoom Figma lên 200%).

---

## 11. D10 — Prototype 6 user flows

### 11.1 Yêu cầu

- Trong page **🔁 Flows / Prototype**, build click-through prototype cho 6 flow đã liệt kê ở `09_design-spec.md` §10:
  1. First-run đến lượt xem đầu tiên
  2. Browse → Play
  3. Continue Watching
  4. Tải file về USB
  5. In-app update
  6. Logout / Switch user

- Mỗi flow là một section dọc trong Figma, có header text frame "Flow N — {tên}".
- Connection sử dụng đúng entry/exit của mỗi screen ở Phần B.
- Trigger interaction = "On Click" hoặc "On Key Press" (Figma không support D-pad thật, nhưng key press tương đương).

### 11.2 Animation prototype

- Mặc định transition: **Instant** hoặc **Smart Animate ≤ 200 ms**.
- Bridging transition giữa screens: **Push** hoặc **Move-in** (theo direction).
- KHÔNG dùng "Dissolve" với duration > 300 ms.

### 11.3 Hand-off cho QA

Prototype được QA dùng làm baseline test. Nên có một frame "Start" cho mỗi flow để QA nhấn play → test toàn flow.

Link prototype phải share cho QA (View + Comment) cùng lúc handoff Eng.

---

## 12. D11 — Motion / animation specs

V1 motion đơn giản, hầu hết dùng tokens (mục 6.8 của `09_design-spec.md`). Nhưng có 5 trường hợp cần **spec chi tiết riêng**:

1. **Focus shift** giữa các card trong row (motion/quick + scale 1.05). Designer ghi spec frame-by-frame: t=0 default, t=100ms focused.
2. **BottomSheet slide-in** ở S5b track selection. Y-translate từ 100% → 0%, fade 0 → 1, duration 200 ms.
3. **Player overlay fade in/out**. Opacity 0 → 1 in 100 ms; fade out chậm hơn 200 ms để đỡ giật khi user vừa thả phím.
4. **Splash → Home enter**. Splash logo scale 1.0 → 1.1 fade out 200 ms; Home content fade in 200 ms.
5. **Skeleton pulse**. Opacity 0.4 ↔ 0.7, 1500 ms cycle, ease-in-out.

Spec theo bảng:

| Animation | From | To | Duration | Easing | Stagger |
|-----------|------|----|---------|--------|---------|
| Focus shift | scale 1.0, glow 0 | scale 1.05, glow 0.35 | 100 ms | ease-out (cubic-bezier(0.2,0,0,1)) | 0 |
| BottomSheet | y +100%, opacity 0 | y 0, opacity 1 | 200 ms | standard | 0 |
| Overlay in | opacity 0 | opacity 1 | 100 ms | quick | 0 |
| Overlay out | opacity 1 | opacity 0 | 200 ms | standard | 0 |
| Splash exit | scale 1.0 opacity 1 | scale 1.1 opacity 0 | 200 ms | quick | 0 |
| Skeleton | opacity 0.4 | opacity 0.7 | 1500 ms | ease-in-out, repeat | 0 |

Nếu có trường hợp đặc biệt nữa, designer thêm vào bảng này (đặt trong Foundation page).

### 12.1 Lottie

V1 KHÔNG khuyến khích Lottie (theo nguyên tắc N4 + budget). Nếu thực sự cần (ví dụ "đang upload" indicator phức tạp), xuất:
- File `.json` Lottie ≤ 50 KB.
- Có markers cho start/loop/end.
- Test render ổn trên Mi Box S (CPU yếu).

---

## 13. D12 — Localization keys + strings

### 13.1 Format

Designer cung cấp file **CSV hoặc XLSX** chứa:

| key | vi (default) | en | context | screen |
|-----|--------------|----|----|--------|
| `login_qr_title` | Đăng nhập bằng QR | Sign in with QR | Tiêu đề S1a | S1a |
| `login_qr_step_1` | Mở app Fshare trên điện thoại | Open Fshare app on your phone | Hướng dẫn | S1a |
| `home_section_continue` | Tiếp tục xem | Continue watching | Title row 1 | S2 |
| `player_resume_dialog_title` | Tiếp tục từ {time}? | Resume from {time}? | Dialog | S5d |
| `error_no_network_title` | Không có kết nối | No connection | S11 | S11 |

### 13.2 Quy tắc key naming

- snake_case.
- Format: `{screen}_{element}_{variant}`. Ví dụ: `login_qr_title`, `settings_account_logout_button`.
- Tránh tên chung chung như `text_1`, `error`.
- Tránh trùng key — verify bằng pivot table.

### 13.3 Quy tắc string

- Placeholder: `{name}`, `{time}`, `{count}` — KHÔNG dùng `%s`, `%d` (designer Figma không hiển thị đẹp).
- Pluralization: tránh nếu được (TV ít context); nếu cần, có 2 key: `_one` và `_other`.
- Length: viết ngắn gọn, tránh quá dài (TV không đọc nhiều).
- Nếu chiều dài tiếng Anh khác hẳn tiếng Việt (do encode), test xem có overflow trong UI không. Báo eng nếu cần truncate.

### 13.4 Số lượng strings ước

V1 có khoảng **180–250 string keys**. Designer maintain song song với file Figma — đổi text trong Figma đồng thời update CSV.

### 13.5 Format engineering import

Engineering chuyển CSV sang `res/values/strings.xml` (vi) và `res/values-en/strings.xml` (en) qua script. Designer chỉ cần đảm bảo CSV đúng format.

---

## 14. D13 — Sample / placeholder content

Trong Figma, mỗi screen mockup nên dùng **content thực tế Việt Nam**, không "Lorem ipsum":

- Tên file: `Phim hành động — John Wick 4 (2023) 1080p.mkv`, `Tài liệu — Thiết kế hệ thống Q3.pdf`.
- Tên folder: `Phim`, `Backup máy tính`, `Tài liệu công việc 2026`.
- Tên user: `Nguyễn Văn A`, `Trần Thị B`.
- Email: `nva@fpt.com.vn`, `ttb@fshare.vn`.
- Số liệu: realistic — file 12.4 GB, không 1.0 KB; user có 250 GB / 1 TB không 1 MB / 100 MB.

Lý do: lúc QA test, có data test giống hệt → catch overflow/visual bug sớm. Eng cũng dùng làm seed data cho test.

Designer KHÔNG dùng tên người thật của đồng nghiệp (riêng tư).

---

## 15. D14 — Accessibility audit (nice-to-have)

Designer chạy audit và xuất báo cáo PDF:

### 15.1 Color contrast

Kiểm tra mọi cặp text/background ở component và screen:
- Body text ≥ 7:1 (cao hơn WCAG AAA do TV xa).
- Button label ≥ 4.5:1.
- Disabled text ≥ 3:1.

Báo cáo: bảng pass/fail, screenshot.

### 15.2 Focus visibility

Mỗi component focus state phải pass test:
- Border ring ≥ 3 dp.
- Contrast ring vs surrounding ≥ 3:1.

### 15.3 Tap target (focus target)

Mỗi interactive element ≥ 48 × 48 dp dù visual có thể nhỏ hơn (padding ẩn ok).

### 15.4 Text scaling

Tất cả text dùng `sp` (scalable). Test với system font scale 1.3× xem có overflow không.

---

## 16. Naming & file organization conventions tổng hợp

### 16.1 Asset filenames

| Loại | Pattern | Ví dụ |
|------|---------|-------|
| Icon | `ic_{name}.svg` | `ic_play.svg` |
| Logo | `logo_{name}.svg` | `logo_fshare.svg` |
| Illustration | `illust_{name}.svg` | `illust_empty_folder.svg` |
| Bitmap | `img_{name}.webp` | `img_brand_about.webp` |
| App icon | `ic_launcher_{layer}.svg` | `ic_launcher_foreground.svg` |
| Banner | `ic_banner.png` | `ic_banner.png` |
| Notification | `ic_notification.svg` | `ic_notification.svg` |
| Splash | `ic_splash_{role}.svg` | `ic_splash_logo.svg` |

### 16.2 Folder cấu trúc khi giao zip

Khi designer zip toàn bộ assets gửi engineering:

```
fsnext-tv-design-handoff-v1.0/
├── README.md                    ← bản tóm tắt content + version
├── design-tokens.json           ← D2
├── localization.csv             ← D12
├── motion-specs.md              ← D11 (bảng motion)
├── accessibility-audit.pdf      ← D14 (optional)
├── icons/                       ← D3
│   ├── ic_play.svg
│   ├── ic_pause.svg
│   └── …
├── logos/                       ← D4
│   ├── logo_fshare.svg
│   └── …
├── app-icon/                    ← D5
│   ├── ic_launcher_foreground.svg
│   ├── ic_launcher_background.svg
│   ├── ic_banner.png
│   └── ic_notification.svg
├── splash/                      ← D6
│   ├── ic_splash_logo.svg
│   └── ic_splash_branding.svg
├── illustrations/               ← D7
│   ├── illust_empty_folder.svg
│   └── …
└── figma-link.txt              ← URL Figma file (D1)
```

### 16.3 Versioning

- Mỗi handoff có version: `v1.0`, `v1.1`, …
- Update tokens hoặc components → bump minor: `v1.0` → `v1.1`.
- Đổi structure file → bump major: `v1.0` → `v2.0`.
- Mỗi version có một zip + 1 entry trong sheet "Handoff log".

---

## 17. Quy trình review trước khi chốt handoff

### 17.1 Pre-handoff check (Designer tự verify)

Designer chạy checklist mục 18 trước khi gửi cho Tech Lead. Báo cáo "Đã passed self-check" trong Slack/Teams.

### 17.2 Handoff meeting

Họp 60 phút giữa Designer + PM + Tech Lead + QA Lead. Agenda:

1. Walkthrough các deliverables (15 phút).
2. Inspect Dev Mode trên 3 component bất kỳ (10 phút).
3. Run prototype 1 flow đầu (5 phút).
4. Q&A + identify gap (20 phút).
5. Quyết định: **Accept** / **Accept with action items** / **Block** (10 phút).

### 17.3 Sau handoff

- Designer giữ trách nhiệm: 2 tuần đầu sau handoff, designer là first-responder cho mọi câu hỏi engineer.
- Bug visual phát hiện trong implementation: tạo ticket trong Jira/Linear gắn label `design-fix`.
- Thay đổi tokens/components sau handoff: phải qua **change request** (CR) — designer + PM + Tech Lead approve mới merge.

---

## 18. Checklist tổng — 60 mục bắt buộc

Designer dùng checklist này ngay trước khi gửi handoff. Không tick đủ = không được gửi.

### File Figma (D1)
- [ ] File trong team workspace, có Dev Mode bật
- [ ] Đúng 6 pages theo thứ tự Cover / Foundation / Components / Screens / Flows / Assets
- [ ] Cover page có version + status + designer + last updated
- [ ] Foundation page có grid, focus example, mọi token visualize
- [ ] Components page có ≥ 24 component, mỗi cái ≥ 4 state
- [ ] Screens page có toàn bộ 27 screen × tất cả state đã liệt kê (`09_design-spec.md` §9)
- [ ] Flows page có 6 prototype connect đầy đủ
- [ ] Assets page tổ chức theo folder export
- [ ] Quyền View + Dev Mode đã grant cho Eng/QA/PM
- [ ] Library đã publish ra team (subscribe-able)
- [ ] Detached instance < 5
- [ ] Không có hidden layer thừa

### Tokens (D2)
- [ ] File `design-tokens.json` xuất đầy đủ color/typography/spacing/radius/elevation/motion
- [ ] Có 2 lớp: primitive + semantic
- [ ] Reference dùng `{token.path}`
- [ ] Token names match Figma library

### Icons (D3)
- [ ] Đủ ≥ 45 icon trong danh sách §4.2
- [ ] Mỗi SVG viewbox 24×24 hoặc 32×32
- [ ] Color = `currentColor`
- [ ] File ≤ 4 KB
- [ ] Đặt tên `ic_{name}.svg`
- [ ] Tổ chức trong folder `icons/`

### Logo & brand (D4)
- [ ] Có 6 file logo SVG đủ variant
- [ ] Có doc clear-space + minimum-size
- [ ] Test render ở 32 dp đến 320 dp

### App icon (D5)
- [ ] Banner 320×180 PNG
- [ ] Foreground + background SVG cho adaptive
- [ ] Notification icon monochrome
- [ ] Test render trên emulator Android TV

### Splash (D6)
- [ ] Splash logo SVG 288×288
- [ ] Splash background color hex
- [ ] Spec animation ≤ 1.5s

### Illustration (D7)
- [ ] 3 illustration SVG (empty folder, no network, no files)
- [ ] Placeholder thumbnail SVG
- [ ] Mỗi file ≤ 8 KB

### Components (D8)
- [ ] 24 component đủ trong Components page
- [ ] Mỗi component ≥ 4 state
- [ ] Tokens reference đầy đủ (không hex cứng)
- [ ] Focus target ≥ 48 dp
- [ ] Có annotation behavior khi cần

### Screens (D9)
- [ ] 27 screens × tất cả states đã render
- [ ] Annotation default focus, edge cases trên mỗi screen
- [ ] Canvas 1920×1080 với guide overscan + grid 12 col
- [ ] Tokens reference (không hex cứng)

### Prototype (D10)
- [ ] 6 flows connected
- [ ] Transition Instant hoặc Smart Animate ≤ 200ms
- [ ] Frame "Start" cho mỗi flow
- [ ] Share link cho QA

### Motion (D11)
- [ ] Bảng motion spec ≥ 6 entry trong Foundation
- [ ] Lottie nếu dùng ≤ 50 KB

### Localization (D12)
- [ ] CSV/XLSX với key + vi + en + context + screen
- [ ] Key naming `{screen}_{element}_{variant}`
- [ ] Placeholder dùng `{name}` không `%s`
- [ ] Sample 180–250 keys

### Sample content (D13)
- [ ] Tên file/folder/user thực tế VN
- [ ] Số liệu realistic
- [ ] Không tên đồng nghiệp thật

### Accessibility (D14, optional)
- [ ] Báo cáo contrast ratio
- [ ] Focus visibility audit
- [ ] Tap target ≥ 48 dp
- [ ] Text scale 1.3× test

---

## 19. Tools & licenses cần thiết

Engineering xác nhận trước handoff:

| Tool | Lý do | Owner |
|------|-------|-------|
| Figma Professional+ | Cần Dev Mode | Design team |
| Tokens Studio plugin | Export JSON | Design team |
| SVGOMG / SVGO | Minify SVG | Design team |
| Figma Notes plugin | Annotation | Design team |
| Lottie LottieFiles | Nếu dùng Lottie | Design team |
| Apksigner / aapt | Verify icon trong APK build | Eng team |
| Asset Studio (Android Studio) | Convert SVG → VectorDrawable nếu cần | Eng team |

---

## 20. Q&A protocol sau handoff

Sau khi accept handoff, các kênh tương tác:

- **Câu hỏi chi tiết spec**: comment trực tiếp trên Figma frame liên quan; designer trả lời trong 1 ngày làm việc.
- **Bug visual** trong implementation: ticket Jira/Linear label `design-fix`, gán cho designer.
- **Đổi tokens / components**: CR với 3 sign-off (Designer + Tech Lead + PM); merge vào library mới có hiệu lực.
- **Câu hỏi gấp**: Slack channel `#fsnext-tv-design`, không DM.

Tuần đầu sau handoff: standup 15 phút mỗi sáng có designer tham gia. Sau 2 tuần: schedule demand-only.

---

## 21. Tóm tắt 1 trang cho designer

Nếu chỉ có 60 giây để đọc, đây là phần quan trọng:

1. **Figma file 6 pages** (Cover / Foundation / Components / Screens / Flows / Assets), publish library, grant Dev Mode cho Eng/QA/PM.
2. **Tokens JSON** xuất bằng Tokens Studio, có 2 lớp primitive + semantic.
3. **45+ icon SVG** viewbox 24, currentColor, ≤ 4 KB.
4. **6 logo SVG** + clear-space doc.
5. **App icon adaptive** (foreground + background SVG) + banner 320×180 PNG + notification icon.
6. **Splash logo** 288×288 SVG + background color.
7. **24 component** đầy đủ states với token references.
8. **27 screens × mọi state** annotated default focus + edge cases.
9. **Prototype 6 user flows** click-through.
10. **Motion spec** bảng cho 6 animation chính.
11. **Localization CSV** key + vi + en, ~200 entries.
12. **Sample content** thực tế VN, không Lorem ipsum.

Zip toàn bộ thành `fsnext-tv-design-handoff-v1.0.zip` + share link Figma + email handoff.

— Hết tài liệu yêu cầu Design Handoff —
