# FsNext — Design System & QML Module Convention

> Quyết định kiến trúc cho lớp QML, để chấm dứt tình trạng 2 bộ component song song (xem `docs/ASSESSMENT.md` P1).
> Cập nhật 2026-05-29.

## Quyết định

- **`Fshare.Components` là nhà của MỌI atom UI tái sử dụng** (button, icon, textfield, badge, switch, card,
  progressbar, dialog, list item...). Thư viện này có a11y/keyboard (ship v6.0) + semantic variants.
- **`FsAurora.Theme` là design token** (`AuroraTheme`, `AuroraColors`) — singleton dùng khắp nơi. **Không** đổi.
- **`FsAurora`** chỉ giữ **shell / layout / HUD / trang khung**: `FsSidebar`, `FsFontLoader`, `FsGradientRect`,
  `FsPageHeader`, `FsScrollPage`, `FsToastHost`, `HomeSearchOverlay`, `TransferHudPanel`, `Sparkline`, `FsMiniHud`,
  `FsAurora.Windows.MiniHudWindow`, `FsAurora.Pages` (HomePage, LoginView, ShowcasePage).
- **Không tạo atom mới trong `FsAurora.Components`.** Atom mới → `Fshare.Components`.

## Quy tắc import (tránh "ambiguous type")

Nhiều file import CẢ `Fshare.Components` (unqualified) LẪN `FsAurora.Components 1.0 as Aurora` (aliased). Vì hai
module từng có atom trùng tên, quy tắc:
- Atom dùng **unqualified** (`FsButton {`) → resolve về `Fshare.Components`.
- Mảnh shell chỉ-có-ở-Aurora dùng **`Aurora.`** (`Aurora.FsSidebar`).
- **Trong file thuộc `FsAurora.Components`** (vd `FsButton.qml`, `FsSidebar.qml`) cần atom Fshare → import
  `import Fshare.Components 1.0 as Fsh` rồi viết `Fsh.FsIcon` (KHÔNG import unqualified — sẽ làm các atom Aurora
  cùng-module bị ambiguous khi chúng còn trùng tên).
- File ở thư mục con của `Fshare.Components` resolve sibling atom qua **same-module** (không cần import).

## Bảng ánh xạ 7 atom trùng tên

| Atom | Trạng thái | Bản chuẩn | Ghi chú |
|---|---|---|---|
| FsIcon | ✅ Stage 1 (2026-05-29) | Fshare | Render giống hệt + a11y; `Aurora.FsIcon`→`FsIcon`, đã xóa bản Aurora |
| FsTextField | ✅ Stage 1 | Fshare | Superset (shake + token font); xóa bản Aurora, LoginView/Showcase dùng `Fsh.FsTextField` |
| FsCard | ✅ Stage 1 | Fshare | Superset (lift hover); xóa bản Aurora |
| FsButton | ✅ Stage 2 (2026-05-29) | Fshare | Merge: impl Aurora (gradient/glow/FsIcon/loading/link/lift) + a11y/keyboard/focus-ring Fshare; union variant (+success), size "default"=md; dùng `Aurora.FsGradientRect`. Repoint 77 `Aurora.FsButton`→`FsButton` |
| FsBadge | ✅ Stage 2 (2026-05-29) | Fshare | Fshare superset (variant + alias); đã thêm `danger` cho đủ bộ |
| FsSwitch | ✅ Stage 2 | Fshare | Fshare a11y/keyboard/focus-ring + thêm `label` optional (Aurora) |
| FsProgressBar | ✅ Stage 2 | Fshare | Union: `value`/`indeterminate`/`trackHeight` (Aurora gradient) + `status`/`barHeight` (Fshare semantic); gradient khi status="default", solid khi status set |

**✅ Cả 7 atom đã ở `Fshare.Components`** (2026-05-29). `FsAurora.Components` giờ chỉ còn shell/HUD:
FsSidebar, FsFontLoader, FsPageHeader, FsScrollPage, FsToastHost, HomeSearchOverlay, TransferHudPanel,
Sparkline, FsMiniHud.

**✅ Phụ thuộc ngược đã cắt** (cleanup #2, 2026-05-29): `FsGradientRect` đã chuyển sang `Fshare.Components`;
`qml/Fshare/Components/` (atom lib) **không còn file nào import `FsAurora.Components`** → atom lib chỉ phụ thuộc
`FsAurora.Theme`. (FsButton/FsProgressBar dùng `FsGradientRect` same-module; FsSidebar/LoginView/ShowcasePage dùng
`Fsh.FsGradientRect`.)

**✅ Cleanup #1 (2026-05-29):** đã gỡ `import FsAurora.Components 1.0 as Aurora` **thừa** ở 16 Pages/Dialogs
(các file còn dùng hợp lệ `Aurora.FsPageHeader/FsScrollPage/Sparkline/FsSidebar/...` thì giữ). Design handoff
(jsx/tokens/html/README) đã chuyển khỏi cây runtime `qml/` sang **`design/handoff/`**; xóa rác (`uploads/`
screenshots) + bản trùng (`qml/FsAurora/src/` = dup của handoff/src, root `*.html`/`design-canvas.jsx`).
`qml/FsAurora/` giờ chỉ còn `.qml` + `qmldir`.

## Verify khi đụng atom

QML resolve type lúc **runtime** → build PASS chưa đủ. Phải chạy `output/FsNext.exe` và mở các surface dùng atom
đó, soi stderr/`%APPDATA%/FPT/FsNext/fsnext.log` tìm `"X is not a type"`. Static check nhanh: file dùng bare atom
phải hoặc ở cùng `Fshare/Components/` (same-module) hoặc `import Fshare.Components 1.0` (unqualified).
