# Handoff · Fshare Desktop Design System

## Overview
Bộ thiết kế cho ứng dụng **Fshare Desktop** — app tải lên / tải xuống / đồng bộ cho dịch vụ cloud fshare.vn. Bao gồm:
- **Design system hoàn chỉnh**: typography, colors (light + dark), spacing, radii, shadows, motion, icons, table, form controls, feedback, overlays, guidelines & a11y.
- **Màn hình cốt lõi** (light + dark): Home/Browse, Download, Share Link, Login, Upload (2 biến thể — inline queue & modal-with-folder-tree), Account/VIP, Favorites, Sync (basic + auto-backup overview & detail).
- **File Manager**: hai phương án — 3-panel (tree + grid + detail) và Column view (Miller).

Tất cả được gói trong **1 file HTML duy nhất** `Fshare Desktop.html` dùng React 18 + Babel standalone, tổ chức thành nhiều JSX file bên trong thư mục `src/`.

## About the Design Files
Các file trong bundle này là **design reference được tạo bằng HTML** — prototype minh hoạ look & feel và hành vi, KHÔNG phải production code để copy trực tiếp.

Nhiệm vụ của developer là **tái dựng lại các thiết kế này trong codebase đích** (React / Vue / Electron + React / Qt / native desktop — tuỳ stack) dùng pattern, library và design system riêng của dự án đó. Nếu dự án chưa có stack, chọn framework phù hợp (gợi ý: Electron + React + Vite cho desktop cross-platform, hoặc Tauri nếu cần bundle nhỏ).

## Fidelity
**High-fidelity (hifi)** — mọi giá trị đã chốt:
- Màu sắc, gradient, typography, spacing, radii, shadow đều có hex / px cụ thể (xem `design-tokens.json` và `tokens.css`).
- Motion / easing / duration đã định nghĩa.
- Trạng thái hover / focus / disabled / loading / empty / error đều có trong artboard 02–04 của Component Library.

Developer **nên pixel-perfect** các component và screen. Logic nghiệp vụ (resume upload, encryption, auth) đương nhiên phải viết mới.

---

## Stack gợi ý
| Nhu cầu | Đề xuất |
|---|---|
| Desktop cross-platform | Electron + React 18 + Vite + TypeScript |
| Alternative gọn nhẹ | Tauri + React |
| Styling | CSS Modules hoặc vanilla-extract (KHÔNG nên dùng Tailwind vì token system ở đây rất custom). Hoặc Tailwind với `tailwind.config.js` map tokens từ `design-tokens.json`. |
| Icons | Lucide React (gần như 1-1 với icon set hiện tại). Hoặc tự build bằng file `src/icons.jsx` đã kèm. |
| State | Zustand hoặc Redux Toolkit — upload/download queue cần global store |
| Font | `Geist` (Vercel), `Geist Mono`, `Instrument Serif`, `Be Vietnam Pro` — đều có trên Google Fonts |

---

## Design Tokens

### File gốc
- `design-tokens.json` — nguồn chính (Style Dictionary / Figma Tokens compatible)
- `tokens.css` — CSS custom properties, dán thẳng vào `:root` để dùng ngay với `var(--fs-*)`

### Colors
**Palette Aurora** (bộ màu chính):

| Token | Hex | Dùng cho |
|---|---|---|
| `--fs-accent` | `#FF5B2E` | Primary — button chính, link, highlight |
| `--fs-accent-2` | `#FFAF1D` | Accent vàng — VIP, chart segment |
| `--fs-accent-3` | `#FF3D7F` | Accent hồng — gradient, ribbon |
| `--fs-grad` | `linear-gradient(135deg, #FF5B2E 0%, #FF3D7F 60%, #FFAF1D 100%)` | Gradient brand, button primary, logo mark |
| `--fs-ink` | `#0E0E12` | Text body (light) / bg (dark) |
| `--fs-ink-2` | `#2A2A32` | Secondary text |
| `--fs-ink-3` | `#5C5C66` | Tertiary text |
| `--fs-ink-4` | `#8A8A94` | Placeholder, helper, caption |
| `--fs-bg` | `#F5F4F1` (light) / `#0E0E12` (dark) | Canvas |
| `--fs-panel` | `#FFFFFF` (light) / `#17171E` (dark) | Card, modal |
| `--fs-border` | `#E6E3DC` (light) / `#25252E` (dark) | Divider, stroke nhẹ |
| `--fs-border-strong` | `#C8C3B8` (light) / `#3A3A45` (dark) | Stroke rõ, outline input |
| `--fs-success` | `#0A8A5C` | OK, synced |
| `--fs-warn` | `#C96A00` | Cảnh báo, xoá local |
| `--fs-danger` | `#D53030` | Lỗi, huỷ |

Contrast ratio đã kiểm tra (WCAG AA/AAA) — xem artboard **07 · Guidelines** trong file gốc.

### Spacing scale
`4 · 8 · 12 · 16 · 20 · 24 · 32 · 40 · 48 · 64`

### Radii
| Token | Value | Dùng cho |
|---|---|---|
| `--fs-radius-sm` | 6 | input, chip nhỏ |
| `--fs-radius-md` | 10 | button, input lớn, card nhỏ |
| `--fs-radius-lg` | 14 | card chính |
| `--fs-radius-xl` | 20 | modal, hero card |
| `--fs-radius-pill` | 999 | tab, badge, toggle |

### Typography scale
| Role | Font | Size/Weight |
|---|---|---|
| Display serif | Instrument Serif | 36–56 / 400, italic cho accent |
| H1 | Geist / Be Vietnam Pro | 32 / 700 letter-spacing -0.02em |
| H2 | Geist | 22 / 700 |
| Body | Geist / Be Vietnam Pro | 13–14 / 400–500 line-height 1.55 |
| Label | Geist | 11–12 / 600 |
| Mono | Geist Mono | 10–13 / 500–600 — dùng cho đường dẫn, số liệu, label uppercase |
| Caption | Geist | 10.5–11.5 / 500 |

### Motion
```
--fs-motion-fast: 140ms cubic-bezier(.2,0,0,1)
--fs-motion-base: 220ms cubic-bezier(.2,0,0,1)
--fs-motion-slow: 360ms cubic-bezier(.2,0,0,1)
```
Dùng cho: hover (fast), toggle/open (base), modal/drawer (slow).

### Shadows
```
--fs-shadow-sm: 0 1px 2px rgba(0,0,0,.04), 0 1px 3px rgba(0,0,0,.06)
--fs-shadow-md: 0 4px 14px rgba(255,91,46,.3)            /* gradient glow */
--fs-shadow-lg: 0 20px 50px -10px rgba(0,0,0,.25), 0 0 0 1px var(--fs-border)
--fs-shadow-xl: 0 40px 80px -20px rgba(0,0,0,.25), 0 0 0 1px var(--fs-border)
```

### Z-index
```
sidebar: 10 · header: 20 · dropdown: 100 · modal-overlay: 200 · modal: 210 · toast: 300 · tooltip: 400
```

---

## Screens / Views

Xem file `Fshare Desktop.html` (hoặc `Fshare Desktop · Standalone.html` nếu dev không có môi trường) để khảo sát trực tiếp canvas. Dưới đây là tóm tắt từng màn:

### 1. Login (`AuroraLogin`)
- Split layout: **trái** = brand panel nền Aurora gradient + tagline italic + feature chips; **phải** = form email + password + 3 nút social (Google, Facebook, Zalo).
- States: idle, loading (button spinner), error (banner dưới input), forgot-password link.
- Files: `src/aurora-auth.jsx`

### 2. Home / Browse (`AuroraHome`)
- Sidebar trái (thu gọn được) · Hero stats (dung lượng, tốc độ, files hôm nay) · khu "Gần đây" dạng grid card với thumbnail gradient placeholder · khu "Chia sẻ" với link + passphrase.
- Files: `src/variant-aurora.jsx`

### 3. Upload v1 — Inline queue (`AuroraUpload`)
- Dropdown lớn full-width · destination picker compact · toolbar toggles (auto-link, zip) · **queue list** với 4 active + 3 queued, mỗi row có progress bar, speed, ETA, mã hoá indicator.
- Files: `src/aurora-screens.jsx`

### 4. Upload v2 — Modal + Folder tree picker (`AuroraUploadModal`)
- **Empty state**: canvas trống với dot-grid pattern, modal centered.
- Modal: drop zone compact · input "Tải vào" mở ra **cây thư mục nhiều cấp** (root highlight gradient, expand/collapse chevron, count badge, search box, "Tạo thư mục mới") · password · private toggle · Huỷ/Bắt đầu (disable khi chưa có file).
- Folder tree: recursive component `TreeNode`, hỗ trợ chọn, expand lazy.
- Files: `src/aurora-upload-modal.jsx`

### 5. Download Manager (`AuroraDownloads`)
- Hero block tốc độ tổng · segmented tabs (Đang tải / Đã xong / Lịch sử) · list row với per-file progress, pause/resume/cancel, priority menu.
- Files: `src/variant-aurora.jsx`

### 6. Share Link (`AuroraShare`)
- Preview card file · link generator với shortener · password toggle · expiry date · download limit · social share row · QR code · history table.
- Files: `src/variant-aurora.jsx`

### 7. Account / VIP (`AuroraAccount`)
- Hero card VIP nền gradient ribbon · breakdown dung lượng theo loại file (stacked bar + legend) · perks list · profile · 2FA · sessions · lịch sử thanh toán.
- Files: `src/aurora-screens.jsx`

### 8. Favorites (`AuroraFavorites`)
- Collections ribbon (pinned folders với ảnh cover gradient) · grid thumbnails file đánh dấu sao · filter chip (loại file) · status badge (đã share / riêng tư).
- Files: `src/aurora-screens.jsx`

### 9. Sync (`AuroraSync`) — cặp thư mục & xung đột
- Cặp thư mục local ↔ cloud · side-by-side compare khi có conflict · selective sync checkbox tree · lịch sử.
- Files: `src/aurora-screens.jsx`

### 10. Sync Auto · Overview (`AuroraSyncAuto`)
- **Max 5 thư mục**. Grid 2-column card, mỗi card gồm: folder icon, name, counts (done/total, size), status badge (Đang đồng bộ / Đã đồng bộ / Tạm dừng), path pair với arrow và cloud icon, progress bar khi syncing, toggle enable, delete-local indicator, 3 action icon (mở local / mở cloud / xem chi tiết). Card cuối cùng là **"Thêm thư mục đồng bộ"** dashed border với slot count.
- Files: `src/aurora-sync-auto.jsx`

### 11. Sync Auto · Detail (`AuroraSyncAutoDetail`)
- Chip tabs folder (HoaSen active gradient, qml, platforms) + nút "+" tròn gradient bên phải.
- Toolbar: 4 icon actions (sync now / open folder / history / remove) + 2 labeled switches **"Bật đồng bộ"** & **"Xoá local sau khi tải"** + hiển thị giới hạn tốc độ.
- Pair header strip: path local → cloud, tốc độ, file count, size, status chip.
- File list: mỗi row hiển thị icon, tên (mono), size · mtime · backup time, status badge:
  - `Đã đồng bộ` (green)
  - `Đã xoá local` (amber)
  - `Đang tải` với progress bar inline (accent)
  - `Chờ đến lượt` (grey)
  2 action icon cuối row: copy link, external open.
- Files: `src/aurora-sync-auto.jsx`

### 12. File Manager · A — 3-panel (`AuroraFiles`)
- Folder tree (collapsible) · file grid/list với breadcrumb + view switcher · detail panel phải với thumbnail lớn, meta, activity, actions.
- Files: `src/variant-aurora-files.jsx`

### 13. File Manager · B — Column view (`AuroraFilesV2`)
- Miller columns: mỗi cấp folder là 1 cột, cuộn ngang khi đi sâu. Mỗi cột có header count + search, item active highlight gradient, nhanh cho folder sâu nhiều cấp.
- Files: `src/variant-aurora-files-v2.jsx`

---

## Component library (tóm tắt — xem artboard 02-07 để đọc giá trị)

Atoms: **Button** (primary gradient, secondary outline, ghost, icon, link; sizes sm/md/lg; disabled/loading), **Input** (text, search, password, textarea; focus ring accent 3px alpha; error state với helper text đỏ), **Select**, **Checkbox**, **Radio**, **Toggle** (sm/md), **Slider**, **Chip/Tag**, **Badge** (dot, count, text), **Avatar** (initial, image, stack), **Tooltip**, **Keyboard shortcut badge**.

Molecules: **Alert** (info/success/warn/error với icon + optional dismiss), **Toast** (bottom-right stack với animation slide-up), **Progress bar** (linear + circular), **Skeleton** (shimmer), **Tabs** (segmented pill + underline), **Breadcrumb**, **Pagination**, **Empty state**, **Loading state**.

Overlays: **Modal** (sm/md/lg sizes với header + body + footer pattern), **Drawer** (right slide), **Popover**, **Context menu**, **Dropdown menu** (nested), **Command palette** (⌘K).

Data: **Table** (default / compact / selectable / sortable / sticky header — xem artboard 06).

Navigation: **Sidebar** (expanded / collapsed với icon-only + tooltip), **Window chrome** (macOS + Windows variants).

---

## Icon library

24 icon custom trong `src/icons.jsx` — stroke 1.5 mặc định, 24x24 viewBox.
Mapping tương đương Lucide:
`folder, file, video, image, music, archive, doc, upload, download, cloud, sync, share, link, search, filter, star, trash, home, clock, users, settings, bell, plus, check, x, chevRight, chevDown, more, play, pause, zap, shield, lock, eye, copy, external, menu, sidebar, crown, arrowUp, arrowDown, minWin, maxWin, closeWin`.

Khuyến nghị: dev thay bằng **Lucide React** cho tiện, chỉ giữ những icon custom không có sẵn (vd: `minWin`, `maxWin`, `crown` variant).

---

## Interactions & Behavior

### Upload
- Drag-drop toàn cửa sổ (zone highlight gradient khi dragover).
- Per-file pause/resume/cancel; network loss → auto-resume khi reconnect.
- Chunk upload ≥50MB tự động, retry 3 lần/chunk.
- Speed + ETA cập nhật mỗi 500ms.
- Max 5 concurrent active, rest queued FIFO.

### Sync Auto
- File watcher (chokidar on Electron hoặc native) theo folder.
- Debounce 2s trước khi trigger upload (tránh duplicate khi save liên tục).
- Speed cap configurable (mặc định 5 MB/s để khỏi ảnh hưởng mạng).
- "Xoá local sau khi tải" chỉ xoá sau khi cloud xác nhận 200 OK. Move to recycle bin, không permanent delete.

### Motion guideline
- Hover: `140ms`
- Toggle, tab switch, dropdown open: `220ms`
- Modal, drawer, page transition: `360ms`
- Tất cả dùng `cubic-bezier(.2, 0, 0, 1)` (Material emphasized).

### Keyboard
- `⌘/Ctrl + K`: command palette
- `⌘/Ctrl + U`: new upload
- `⌘/Ctrl + ,`: settings
- `Tab/Shift+Tab`: navigate · `Enter`: primary action · `Esc`: close overlay · `Arrow`: list navigation
- `Space`: toggle checkbox, pause/resume selected transfer

### A11y
- Focus ring: 2px accent với offset 2px.
- Mọi icon-only button có `aria-label` tiếng Việt.
- Toast / Alert dùng `role="alert"` hoặc `aria-live="polite"`.
- Contrast đã kiểm WCAG AA trở lên — xem artboard 07.

---

## State Management

Stores gợi ý (Zustand):
- `authStore` — user, token, refresh logic
- `filesStore` — cây folder, selection, currentPath; subscribe qua cloud API
- `transfersStore` — `uploads[]`, `downloads[]` với status machine (`queued → active → paused → done → error`), speed, ETA
- `syncStore` — `pairs[]` (max 5), watcher status per pair, conflict list
- `uiStore` — theme (light/dark), sidebar collapsed, modal open state

---

## Files trong bundle này

```
Fshare Desktop.html          ← file gốc, source of truth
Fshare Desktop · Standalone.html  ← bản gộp 1-file (dev không rành stack vẫn mở được)
design-canvas.jsx            ← component canvas pan/zoom (chỉ phục vụ preview)
src/
  tokens.jsx                 ← T_A_LIGHT + T_A_DARK, useTA() context hook
  icons.jsx                  ← toàn bộ 45 icon SVG stroke-1.5
  ds-cards.jsx               ← Cover + Analysis + Type + Color card
  ds-foundations.jsx         ← Spacing/Radii/Shadow/Motion/Z-index
  ds-components-1.jsx        ← Button/Input/Select/Toggle/Checkbox/Radio
  ds-components-2.jsx        ← Badge/Chip/Alert/Toast/Tooltip/Avatar
  ds-components-3.jsx        ← Modal/Menu/Tabs/Progress/Skeleton
  ds-icon-library.jsx        ← Grid icon
  ds-table.jsx               ← Table variants
  ds-guidelines.jsx          ← Do/Don't + Voice + Format + Tokens export + A11y
  variant-aurora.jsx         ← Shell (WinChrome, Sidebar, MiniStat, btnGhost…) + Home, Downloads, Share
  aurora-auth.jsx            ← Login
  aurora-screens.jsx         ← Upload (inline), Account, Favorites, Sync basic
  aurora-upload-modal.jsx    ← Upload modal + folder tree picker
  aurora-sync-auto.jsx       ← Sync auto overview + detail
  variant-aurora-files.jsx   ← File Manager · 3-panel
  variant-aurora-files-v2.jsx ← File Manager · Column view
  app.jsx                    ← wiring tất cả artboard vào DesignCanvas
design-tokens.json           ← token JSON (Style Dictionary format)
tokens.css                   ← :root { --fs-* } dán thẳng vào stylesheet
README.md                    ← file này
```

---

## Khuyến nghị triển khai (theo thứ tự ưu tiên)

1. **Foundations** — dán `tokens.css` vào global stylesheet · setup font · setup Geist + Be Vietnam Pro · cài Lucide.
2. **Atoms** — Button, Input, Select, Toggle, Checkbox, Chip, Badge, Avatar. Viết Storybook song song.
3. **Shell** — Window chrome + Sidebar + layout grid cho mọi màn.
4. **Core screens** theo thứ tự nghiệp vụ: Login → Home → Upload (Modal) → Download → Account.
5. **Sync & File Manager** — phức tạp nhất, làm cuối.
6. **Dark mode** từ đầu — mọi component phải nhận `theme` prop hoặc dùng CSS var với `[data-theme="dark"]` override.

## Câu hỏi thường gặp

**Q: Có cần dùng Instrument Serif không?**
A: Nên. Chỉ xuất hiện ở H1 / hero số lớn, tạo moment "editorial" phân biệt với SaaS thông thường. Fallback `Georgia, serif` OK.

**Q: Phương án File Manager nào chọn?**
A: Cả 2 đều hoàn thiện. A (3-panel) quen hơn với user phổ thông, B (column) cho power user có thư mục sâu. Khuyến nghị implement A trước, B làm toggle view sau.

**Q: Upload inline vs Upload modal?**
A: Modal tốt cho empty state và khi user chủ động click "+ Chọn file". Inline tốt khi đã có queue chạy. **Cả 2 cùng tồn tại** — modal là entry point, inline là persistent view khi có transfer.
