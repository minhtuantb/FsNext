# Transfer HUD — Đặc tả UX & Triển khai chi tiết

> **Trạng thái:** Spec v3 (2026-05-27) — X = minimize-to-tray (đồng bộ với feature `feedback-close-window-ux` đã ship + code P0-P3) + thêm Windows Taskbar Progress
> **Phạm vi:** System Tray icon · Tray Popup · Mini HUD window · Balloon notifications · **Windows Taskbar Progress** · vòng đời cửa sổ (X / minimize / focus) · settings
> **Liên quan:** `mockup/transfer-hud-preview.html` (mockup trực quan), memory `project-transfer-hud-spec`, `feedback-close-window-ux`
> **Đối tượng đọc:** Dev implement + reviewer. Tài liệu đủ chi tiết để code & nghiệm thu.

### Changelog
- **v3 (2026-05-27):** Chốt **X = minimize-to-tray** (giữ confirm dialog Quit/Minimize/Cancel đã ship). Thêm **minimize (_) → mini HUD**. Thêm **Windows Taskbar Progress**. Giữ `minimizeToTray` + `confirmOnClose`.
- v2 (2026-05-27): (huỷ) — từng đề xuất X=quit; đã phản biện và đảo lại vì FsNext là app nền, X=quit ngược convention ngành (Dropbox/OneDrive/Drive/qBittorrent đều X=tray).
- v1: spec gốc P0-P3.

---

## 0. Tóm tắt: mô hình cuối vs code P0-P3

Code P0-P3 đã implement **X → confirm dialog → (Minimize) hide-to-tray + mini HUD**. Spec v3 **giữ nguyên** hướng đó (đúng convention app nền) và bổ sung 2 phần:

| Hành vi | Code P0-P3 | **Spec v3 (cuối)** | Cần làm |
|---|---|---|---|
| Nút **X** | Confirm dialog (Huỷ/Thu nhỏ/Thoát hẳn) | **Giữ nguyên** | — đã có |
| → chọn "Thu nhỏ" | Hide-to-tray + mini nếu transfer | **Giữ nguyên** | — đã có |
| → chọn "Thoát hẳn" | Qt.quit() | **Giữ nguyên** (confirm đã chống mất data) | — đã có |
| Nút **minimize (_)** | Taskbar (chuẩn), KHÔNG mini | **Taskbar + mini HUD nếu có transfer** | ➕ thêm `onVisibilityChanged` |
| **Windows Taskbar Progress** | Không có | **Thanh progress trên nút taskbar khi có transfer** | ➕ mới (C++ COM) |
| Setting `minimizeToTray` | Có | **Giữ** | — |
| Setting `confirmOnClose` | Có | **Giữ** | — |
| Tray icon tồn tại khi | App chạy | App chạy (mất khi Thoát hẳn) | — đã có |

→ Delta thực tế nhỏ (§13): chỉ 2 phần net-new. Phần lớn P0-P3 giữ nguyên.

---

## 1. Mục tiêu & nguyên tắc

### 1.1 Mục tiêu sản phẩm
Người dùng tải/upload/sync file qua Fshare. Khi họ thu nhỏ cửa sổ để làm việc khác, họ vẫn cần:
- **Liếc nhanh** tiến độ mà không phải bật lại cửa sổ chính.
- **Nhận thông báo** khi xong/lỗi.
- **Thao tác tối thiểu** (pause/resume/mở task) ngay tại chỗ.

### 1.2 Nguyên tắc thiết kế (kim chỉ nam)
1. **Tôn trọng OS convention** — minimize xuống taskbar như mọi app; X thoát như mọi app. Không gây bất ngờ.
2. **Mini HUD = phụ trợ, không thay thế** — chỉ hiện khi cửa sổ chính không hiển thị + có việc đáng theo dõi. Tự biến mất khi vô nghĩa.
3. **Một nguồn dữ liệu** — `TransferHudViewModel` là cầu duy nhất; Tray, Popup, Mini đều chiếu từ đó.
4. **Hiển thị, không quản lý sâu** — HUD phục vụ "liếc + 1 click". Quản lý phức tạp → mở cửa sổ chính.
5. **Không phá dữ liệu** — thoát app khi đang transfer phải có xác nhận; không bao giờ im lặng giết download.
6. **Đúng Aurora design system** — token màu/spacing/radius/typography lấy từ `AuroraColors.qml` + `AuroraTheme.qml`.
7. **Reduce-motion aware** — mọi animation gate qua `AuroraTheme.reduceMotion`.

---

## 2. Mô hình vòng đời ứng dụng (App Lifecycle Model)

### 2.1 Các trạng thái app

```
┌──────────────────────────────────────────────────────────────────────┐
│                          APP PROCESS ALIVE                              │
│                                                                         │
│  ┌──────────────┐  minimize(_)  ┌─────────────────┐                    │
│  │ FOREGROUND   │ ────────────▶ │ MINIMIZED       │                    │
│  │ (main shown) │ ◀──────────── │ (taskbar button)│                    │
│  └──────┬───────┘   restore      └────────┬────────┘                   │
│         │ X → confirm dialog              │ restore (taskbar/tray dbl)  │
│         │   ├─ "Thu nhỏ" ───────┐         │                            │
│         │   └─ "Thoát hẳn" ──┐  │         │                            │
│         ▼                    │  ▼         ▼                            │
│  ┌─────────────┐             │ ┌──────────────────┐                    │
│  │   QUIT      │ ◀───────────┘ │ TRAY-ONLY        │                    │
│  │ (process    │               │ (ẩn khỏi taskbar,│                    │
│  │  exit)      │               │  sống trong tray)│                    │
│  └─────────────┘               └──────────────────┘                    │
│                                                                         │
│  Tray icon: LUÔN hiện khi process alive. Mất khi QUIT.                  │
│  Mini HUD:  khi (MINIMIZED hoặc TRAY-ONLY) + có transfer active/pending │
└──────────────────────────────────────────────────────────────────────┘
```

**3 trạng thái "main không chiếm màn hình":**
- **MINIMIZED** — nút `_`: thu xuống taskbar, taskbar button còn.
- **TRAY-ONLY** — X → chọn "Thu nhỏ": ẩn hẳn khỏi taskbar, chỉ còn tray icon.
- Cả hai → `window.visible === false` hoặc `visibility === Window.Minimized` → điều kiện cho Mini HUD.

**Định nghĩa "main không hiển thị" (điều kiện Mini HUD):**
`!window.visible || window.visibility === Window.Minimized`.

> ✅ **Đúng convention app nền** (Dropbox/OneDrive/Drive/qBittorrent): X = thu nhỏ giữ chạy, thoát hẳn là hành động có chủ đích (qua confirm dialog hoặc tray menu). Không bao giờ im lặng giết download.

### 2.2 Bảng điều khiển cửa sổ (Window controls)

| Nút | Điều kiện | Hành vi |
|---|---|---|
| **X (close)** | `confirmOnClose=true` (default) | Hiện confirm dialog 3 lựa chọn: **Huỷ / Thu nhỏ vào khay / Thoát hẳn** (+ "Không hỏi lại"). Đã ship (`feedback-close-window-ux`). |
| **X (close)** | `confirmOnClose=false` + `minimizeToTray=true` | Im lặng thu nhỏ vào tray (TRAY-ONLY) + mini HUD nếu có transfer |
| **X (close)** | `confirmOnClose=false` + `minimizeToTray=false` | Im lặng thoát hẳn (`Qt.quit()`) |
| **→ "Thu nhỏ vào khay"** | — | Hide window (TRAY-ONLY) + mini HUD nếu transfer + `showMiniOnMinimize` |
| **→ "Thoát hẳn"** | — | `Qt.quit()` — confirm đã đủ vai trò chống mất data |
| **Minimize (_)** | Không có transfer | Thu xuống taskbar (chuẩn Windows). KHÔNG mini |
| **Minimize (_)** | Có transfer + `showMiniOnMinimize=true` | Thu taskbar **+ hiện Mini HUD** ⬅ **NET-NEW spec v3** |
| **Maximize/Restore (□)** | — | Chuẩn Windows |

### 2.3 Bảng hành vi System Tray

| Thao tác | Hành vi |
|---|---|
| **Single-click trái** | Toggle Tray Popup (anchor tray icon). Guard 250ms |
| **Double-click** | Restore + focus main → FOREGROUND (từ MINIMIZED hoặc TRAY-ONLY) |
| **Right-click** | Context menu (§5.4) |
| **Hover** | Tooltip động (§5.3) |

### 2.4 Confirm dialog khi X (đã ship — giữ nguyên)

Dialog 3 nút (`feedback-close-window-ux`, ship 2026-05-25):

```
┌──────────────────────────────────────────────────┐
│  Thoát Fshare?                                ✕  │
│  ──────────────────────────────────────────────   │
│  Chọn "Thu nhỏ vào khay" để giữ app chạy nền      │
│  (các tải xuống đang chạy sẽ tiếp tục). Chọn       │
│  "Thoát hẳn" để đóng hoàn toàn.                    │
│                                                    │
│  ☐ Không hỏi lại                                  │
│                                                    │
│            [ Huỷ ]  [ Thu nhỏ vào khay ]  [ Thoát hẳn ]│
└──────────────────────────────────────────────────┘
```

- "Không hỏi lại" ghi cả `confirmOnClose=false` + `minimizeToTray` theo nút đã chọn → lần sau im lặng.
- **An toàn dữ liệu**: vì "Thoát hẳn" là lựa chọn tường minh (user phải bấm), không cần thêm confirm "đang có transfer" nữa — dialog này đã đóng vai trò đó. (Tùy chọn nâng cao §9: nếu `confirmOnClose=false` + `minimizeToTray=false` + đang có transfer → vẫn nên cảnh báo 1 lần.)
- "Thoát hẳn" nên `pauseAll()` + persist (ADR D12 snapshot) trước `Qt.quit()`.

---

## 3. Kịch bản chi tiết (Scenarios)

Mỗi kịch bản: **Trigger → Trạng thái các thành phần → Hành vi**.

### S1 — Khởi động app
- Trigger: user mở FsNext.exe.
- Tray icon: hiện màu **idle** (xám).
- Main window: hiện FOREGROUND (login nếu chưa đăng nhập).
- Mini HUD: ẩn. Tray Popup: ẩn.

### S2 — Minimize khi KHÔNG có transfer
- Trigger: bấm nút `_`.
- Main: xuống taskbar (Window.Minimized).
- Mini HUD: **KHÔNG hiện** (không có gì để theo dõi).
- Tray: giữ idle.

### S3 — Minimize khi CÓ transfer (luồng chính của Mini HUD)
- Trigger: bấm `_` khi `activeTotal + pendingTotal > 0` và `showMiniOnMinimize=true`.
- Main: xuống taskbar.
- Mini HUD: **hiện** ở vị trí đã lưu (hoặc default góc dưới-phải), fade-in 80ms.
- Tray: màu **active** (accent).
- Mini hiển thị: sparkline + top-5 rows + footer.

### S4 — App khác đè lên / mất focus
- Trigger: user click sang Chrome/VSCode khi main đang FOREGROUND.
- Main: vẫn "shown" (chỉ mất focus, nằm sau) → **KHÔNG kích hoạt Mini HUD** (vì main chưa minimize).
- Tray: giữ nguyên trạng thái.
- Nếu Mini HUD ĐANG hiện (do trước đó minimize): mini **giữ nguyên** (StaysOnTop), tiếp tục cập nhật. Không bị app khác che.
- Nếu Tray Popup đang mở: **auto-dismiss** (mất activation).

### S5 — Click X
- Trigger: bấm X.
- `confirmOnClose=true` → confirm dialog §2.4 (Huỷ / Thu nhỏ vào khay / Thoát hẳn):
  - **Thu nhỏ vào khay** → hide window (TRAY-ONLY) + mini HUD nếu có transfer + setting.
  - **Thoát hẳn** → pauseAll + persist + `Qt.quit()`. Tray biến mất, mini đóng.
  - **Huỷ** → giữ FOREGROUND.
- `confirmOnClose=false` → im lặng theo `minimizeToTray` (tray-only + mini, HOẶC quit).

### S5b — Windows Taskbar Progress (NET-NEW spec v3)
- Trigger: có ≥1 transfer Active (bất kể main đang ở trạng thái nào).
- Nút taskbar của FsNext hiển thị **thanh progress native** = tiến độ tổng hợp (tổng bytesTransferred / tổng fileSize của các task Active).
- Trạng thái progress:
  - Active → `Normal` (xanh).
  - Có failed, không active → `Error` (đỏ).
  - Paused all → `Paused` (vàng).
  - Idle → `NoProgress` (ẩn thanh).
- Đây là feedback **0-intrusion** — không cần cửa sổ nào, hiển thị ngay trên taskbar. Bổ trợ Mini HUD.

### S6 — Single-click tray icon (peek)
- Trigger: click trái tray icon (sau guard 250ms).
- Tray Popup: hiện anchored tray icon, tail ▼ trỏ về icon, fade-in.
- Nội dung: compact (top-5, không sparkline, không pause-all/close).
- Nếu chưa đăng nhập → popup empty state "Đăng nhập để xem trạng thái" + CTA mở cửa sổ.

### S7 — Double-click tray icon (restore)
- Trigger: double-click tray.
- Guard 250ms hủy single-click intent.
- Main: restore từ Minimized → FOREGROUND + focus.
- Mini HUD: **đóng** (main đã hiện, mini thừa).
- Tray Popup: đóng nếu đang mở.

### S8 — Transfer hoàn tất
- Trigger: 1 task chuyển sang Complete.
- Balloon: "Đã hoàn tất: filename" (gated `notifyOnTransferDone`). Mang `taskId` để click.
- Tray: nếu hết active → về idle; còn active → giữ active.
- Mini HUD (nếu đang hiện): row chuyển trạng thái Complete (✓), sau đó archive theo TTL.

### S9 — Transfer thất bại
- Trigger: 1 task chuyển sang Error.
- Balloon: "Tải thất bại: filename" + error message (icon Critical đỏ).
- Tray: nếu không còn active → màu **error** (đỏ). Nếu còn active → giữ active (active mask error).
- Mini HUD: failed row lên đầu, border-left đỏ + nút ⟲ Thử lại.

### S10 — Click balloon notification
- Trigger: user click vào toast.
- Main: restore + focus FOREGROUND.
- Routing: `balloonClicked(taskId)` → `hudVM.focusTask(taskId)` → chuyển page Download/Upload + `ListView.positionViewAtIndex(Center)` + highlight pulse 1.5s.
- Nếu `taskId` rỗng (toast cũ) → chỉ restore main.

### S11 — Kéo Mini HUD + snap + persist
- Trigger: kéo header mini.
- Mini di chuyển realtime theo cursor.
- Thả gần edge < 24px → snap về edge + gap 8px (animation 150ms nếu !reduceMotion).
- Persist `(x, y, screenName)` vào QSettings (`Hud/miniWindowX/Y/Screen`).

### S12 — Tất cả idle → Mini auto-hide
- Trigger: `activeTotal + pendingTotal + failedTotal + syncPending == 0` liên tục 30s.
- Mini: fade-out 220ms → hide.
- Tray: về idle.
- Grace 30s để user kịp thấy "mọi thứ xong".

### S13 — Restore main khi Mini đang hiện
- Trigger: double-click tray / click taskbar / click ⛶ trên mini.
- Main: FOREGROUND.
- Mini: đóng ngay (qua `root.onVisibilityChanged`: nếu main visible & mini visible → dismiss).

### S14 — Multi-monitor
- Mini lưu `screenName` (QScreen::name). Restore về đúng monitor.
- Monitor biến mất (unplug) → vị trí lưu nằm ngoài virtualGeometry → reset default góc dưới-phải primary.
- Tray Popup: anchor theo screen chứa tray icon; fallback bottom-right nếu `geometry()` invalid.

### S15 — Quit từ tray menu
- Trigger: right-click tray → "Thoát".
- Giống S5 (confirm nếu có transfer) → `Qt.quit()`.

### S16 — Hiện Mini HUD thủ công
- Trigger: tray menu → "Hiện mini HUD" / phím tắt (tùy chọn).
- Mini hiện bất kể main state. Hữu ích khi muốn widget nổi đè app khác.
- Nếu main đang FOREGROUND → vẫn cho hiện (user chủ động yêu cầu), nhưng khuyến nghị: chỉ enable menu item này khi main minimized HOẶC có transfer.

### S17 — Transfer mới bắt đầu khi Mini đã bị dismiss thủ công
- Trigger: user đóng mini (✕) trước đó → `userDismissed=true`. Sau đó thêm download mới.
- Mini: **KHÔNG tự hiện lại** (tôn trọng intent). Chỉ re-arm khi: minimize lại / restore main / tray menu hiện mini.

### S18 — Reduce motion ON
- Mọi fade/snap/slide → step-cut tức thời. Sparkline vẫn cập nhật 1Hz nhưng không "wave" animation.

### S19 — Single instance (mở file thứ 2)
- Trigger: mở FsNext.exe lần 2 / Chrome native host gửi URL.
- Instance đang chạy: restore main FOREGROUND + xử lý URL (pre-fill download dialog). Không spawn cửa sổ thứ 2.

### S20 — Đăng xuất khi Mini đang hiện
- Trigger: logout.
- Mini: đóng (không còn dữ liệu user). Tray: idle. Main: về login.

---

## 4. Ma trận trạng thái Tray Icon

| Điều kiện | Màu icon | Token | Tooltip |
|---|---|---|---|
| `failed > 0 && active == 0` | Đỏ | `AuroraColors.danger` `#D53030` | `Fshare — lỗi N` |
| `active > 0` | Accent (cam) | `AuroraColors.accent` `#FF5B2E` | `Fshare — đang chuyển N · chờ M` |
| `active == 0 && pending == 0 && failed == 0` | Xám | `#9CA3AF` (ngoài brand, neutral) | `Fshare` |

- Ưu tiên: **Error > Active > Idle**. Active "che" error (đang chạy thì đỏ gây hiểu lầm).
- Icon vẽ runtime: rounded square (radius 22% cạnh) + chữ "F" trắng bold Segoe UI (60% cạnh). 7 size 16–128px.
- Active/Error đọc màu **live** từ `AuroraColors` singleton (qua `auroraColor()` helper); hex chỉ là fallback. Idle hardcode (neutral, ngoài palette).
- Chỉ `setIcon()` khi state đổi (cache `m_currentState`).

> **Lưu ý Windows 11**: tray icon mặc định nằm trong **overflow menu** (`^`). User phải kéo ra/pin để thấy trực tiếp. Đây là OS behavior; ta không can thiệp được. Cân nhắc ghi hướng dẫn pin icon trong onboarding/help.

---

## 5. Đặc tả từng thành phần

### 5.1 Main Window
- Kích thước: 1100×720, min 800×560.
- Quản lý transfer đầy đủ (Download/Upload/Sync pages).
- `onClosing` → logic §2.4. `onVisibilityChanged` → khi minimized + transfer → trigger mini; khi visible → dismiss mini.
- Khi restore từ taskbar/tray → dismiss mini.

### 5.2 Mini HUD Window
**Bản chất**: frameless `Window` (Qt.Tool | FramelessWindowHint | WindowStaysOnTopHint), width **360px**, height auto theo content.

**Layout (full mode, `compact=false`):**
```
┌──────────────────────────────────────────────┐ ← drag handle (header band)
│ Fshare        ↓ 12.4 MB/s · ↑ 3.1 MB/s  ⏸ ⛶ ✕│  Row 1: brand + speed + actions (40px)
│  ╱╲╱╲╱─╲╱╲─╲╱╲╱╲─╲  (sparkline 60s)          │  Row 2: 32px
│ ──────────────────────────────────────────────│
│ ↓ setup.exe              42% · 2.1 MB/s        │  Row 3-7: top-5 (52px/row)
│   ▓▓▓▓▓▓░░░░░░░░░░  Còn 4 phút                 │
│ ...                                            │
│ + 4 mục khác                              ›    │  overflow chip (30px)
│ ──────────────────────────────────────────────│
│ ⟲ Đồng bộ: 12 file chờ        DL 3/3 · UL 1/2 │  footer (28px)
└──────────────────────────────────────────────┘
```

**Data (binding từ `transferHudViewModel`):**
| Vùng | Property | Kiểu |
|---|---|---|
| Tốc độ DL | `totalDownloadSpeedText` | QString "12.4 MB/s" |
| Tốc độ UL | `totalUploadSpeedText` | QString |
| Pause toggle | `runState` | "running"/"paused"/"idle" |
| Sparkline | `speedHistory` | QVariantList[60] double bytes/s |
| Top rows | `topItems` (model) | TransferHudTopModel |
| Overflow | `overflowCount` | int |
| Sync footer | `syncPending` | int |
| Budget footer | `transferBudgetViewModel.active*/max*` | int |

**Tương tác:**
| Thao tác | Hành vi |
|---|---|
| Kéo header | Di chuyển window (xem S11) |
| ⏸/▶ | `pauseAll()` / `resumeAll()` |
| ⛶ | `expandRequested` → restore main + dismiss mini |
| ✕ | `closeRequested` → `dismissMini()` (suppress đến re-arm) + hide |
| Hover row | Hiện ⏸/▶/⟲ tùy state |
| Click row body | `rowClicked(id)` → restore main + focusTask. **Mini KHÔNG đóng** (khác popup) |
| Click row ⟲ (failed) | `resumeTask(id)` |
| Click "+N mục khác" | `expandRequested` → mở main |

**Vòng đời:**
- Show: S3 (minimize+transfer), S16 (thủ công).
- Hide: S12 (idle 30s), S13 (restore main), ✕ thủ công.
- Persist vị trí: S11.

### 5.3 System Tray Icon
- Xem §4 (ma trận trạng thái).
- API: `setHudHint(active, pending, failed)` → đổi màu + tooltip.
- Signals: `togglePopupRequested` (single-click), `showWindowRequested` (double-click), `pauseAllRequested`, `quitRequested`, `showMiniRequested`, `openSettingsRequested`, `balloonClicked(taskId)`.

### 5.4 Tray Popup
**Bản chất**: frameless Window anchored tray, width **320px** (compact hơn mini), tail ▼.

**Layout (compact mode, `compact=true`):**
```
                          ┌──────────────────────────────┐
                          │ Fshare    ↓12.4 ·↑3.1   ⛶    │  header (36px, không pause/close)
                          │ ──────────────────────────────│
                          │ ↓ setup.exe      42% · 2.1MB/s│  rows 44px (compact), KHÔNG sparkline
                          │ ↑ video.mp4      78% · 1 phút │
                          │ + 4 mục khác              ›   │
                          │ ──────────────────────────────│
                          │ ⟲ Sync 12 chờ    DL 3/3·UL1/2│  footer
                          └──────────────────────────────┘ ▼ (tail → tray)
```

**Khác Mini HUD:**
| | Mini HUD | Tray Popup |
|---|---|---|
| Width | 360 | 320 |
| Row height | 52px | 44px |
| Sparkline | Có | Không |
| Pause-all / Close btn | Có | Không (qua tray menu) |
| Drag | Có | Không |
| Persist vị trí | Có | Không (anchor tray) |
| Auto-hide idle | 30s | Không |
| Dismiss focus-out | Không | **Có** |
| Click row | Mở main, mini ở lại | Mở main + **đóng popup** |
| Tail ▼ | Không | Có |

**Mutual exclusion**: Mini đang hiện + click tray → đóng mini, mở popup. Không bao giờ 2 surface cùng tồn tại.

### 5.5b Windows Taskbar Progress (NET-NEW spec v3)

**Bản chất**: vẽ thanh progress + state lên nút taskbar qua COM `ITaskbarList3` (Win32). Qt6 đã **bỏ** `QtWinExtras`/`QWinTaskbarButton` (chỉ có ở Qt5) → phải dùng raw COM.

**Triển khai (C++, Windows-only):**
- File mới: `src/platform/TaskbarProgress.{h,cpp}` (guard `#ifdef Q_OS_WIN`).
- Khởi tạo: `CoCreateInstance(CLSID_TaskbarList, ... IID_ITaskbarList3)` → `HrInit()`.
- Cần HWND của cửa sổ chính: lấy từ `QQuickWindow::winId()` (qua engine root) — gọi sau khi window shown.
- API expose:
  - `setProgress(double ratio)` → `SetProgressValue(hwnd, completed, total)` (dùng 0..1000 scale).
  - `setState(State)` → `SetProgressState(hwnd, TBPF_NORMAL/ERROR/PAUSED/NOPROGRESS)`.
- Wiring: connect `hudVM` → tính `aggregateProgress` + `runState`/`failedTotal` → push vào TaskbarProgress mỗi lần đổi (debounce theo countersChanged, đã có 250ms).

**Data cần từ hudVM** (thêm property):
| Property | Kiểu | Mô tả |
|---|---|---|
| `aggregateProgress` | double (0..1) | Σ bytesTransferred / Σ fileSize của task Active. -1 khi idle. |

**State mapping:**
| Điều kiện | TBPFLAG |
|---|---|
| `activeTotal > 0` | `TBPF_NORMAL` |
| `failedTotal > 0 && activeTotal == 0` | `TBPF_ERROR` |
| `runState == "paused"` | `TBPF_PAUSED` |
| idle (không có gì) | `TBPF_NOPROGRESS` |

**Edge:**
- COM init fail / không phải Windows → no-op (mọi setter guard null).
- HWND chưa sẵn sàng (trước first show) → defer set lần đầu qua `Qt.callLater` / 1 lần ở `Application started`.
- macOS/Linux: bỏ qua (Linux Unity launcher có API riêng — out of scope).

### 5.5 Balloon Notifications
- Qua `QSystemTrayIcon::showMessage(title, msg, icon, 4000ms)`.
- Gated `notifyOnTransferDone`.
- Variants:
  - **Success**: `title="Đã hoàn tất"`, `msg=filename`, icon Information.
  - **Error**: `title="Tải thất bại: filename"`, `msg=errorMessage`, icon Critical.
  - **Multi (debounce 2s)**: nếu ≥2 task xong liên tiếp → gộp "Đã hoàn tất N file" + tên 2 file đầu + "và M khác". *(Hiện chưa implement debounce-gộp — xem §13 backlog.)*
- Click balloon → S10 (mang taskId qua `clickContextId`).
- Idempotent: mỗi `taskId` chỉ notify 1 lần per terminal state.

---

## 6. Đặc tả dữ liệu — TransferHudViewModel

**Inputs (constructor):** `UploadViewModel* + DownloadViewModel* + SyncViewModel* + TransferBudgetViewModel* + TransferService*`.

**Refresh:** debounce 250ms — mọi signal input (totalSpeedChanged, runStateChanged, model rows*, budget usageChanged, sync pendingCountChanged) → `scheduleRefresh()` → `refreshAll()` walk 1 lần.

**Speed sampling:** QTimer 1Hz → `sampleSpeedTick()` → sum tốc độ Active tasks → append ring buffer 60 → emit `speedHistoryChanged` (suppress khi all-zero steady-state).

**Properties (Q_PROPERTY, đọc-only từ QML):**
| Property | Kiểu | NOTIFY | Mô tả |
|---|---|---|---|
| `totalDownloadSpeedText` | QString | speedChanged | "12.4 MB/s" / "" |
| `totalUploadSpeedText` | QString | speedChanged | |
| `runState` | QString | runStateChanged | running/paused/idle |
| `activeTotal` | int | countersChanged | task Active |
| `pendingTotal` | int | countersChanged | Queued+Paused |
| `failedTotal` | int | countersChanged | Error |
| `syncPending` | int | countersChanged | mirror SyncVM.pendingCount |
| `overflowCount` | int | topItemsChanged | tổng hiển thị - 5 |
| `topItems` | TransferHudTopModel* | CONSTANT | top-5 sorted |
| `speedHistory` | QVariantList | speedHistoryChanged | 60 double bytes/s |
| `shouldShowMini` | bool | shouldShowMiniChanged | (active+pending+failed+sync >0) && !userDismissed |

**Q_INVOKABLE:** `pauseAll/resumeAll/pauseTask(id)/resumeTask(id)/cancelTask(id)/focusTask(id)/dismissMini()/acknowledgeMini()`.

**Signals:** `taskFocusRequested(page, taskId)`, `transferDone(taskId, fileName, success, errorMessage)`.

**TransferHudTopModel roles:** taskId, direction(0=dl/1=ul/2=sync), fileName, progress(0..1), speedText, etaText, status(TransferState int), errorMessage, page(1=download/2=upload).
**Sort:** Error > Active > Paused > Queued > Complete; trong cùng bucket → progress desc.

---

## 7. Settings — danh sách đầy đủ

| Key (QSettings) | Kiểu | Default | UI label | Nhóm | Trạng thái |
|---|---|---|---|---|---|
| `General/minimizeToTray` | bool | `true` (Win) | "Thu nhỏ vào khay khi đóng cửa sổ" | Chung | ✅ đã có |
| `General/confirmOnClose` | bool | `true` | "Hỏi xác nhận khi đóng cửa sổ" | Chung | ✅ đã có |
| `Hud/showOnHideToTray` | bool | `true` | **đổi label →** "Hiện mini HUD khi thu nhỏ" | Giao diện | ✅ đã có (đổi tên hiển thị) |
| `Hud/notifyOnTransferDone` | bool | `true` | "Thông báo khi tải xong / lỗi" | Thông báo | ✅ đã có |
| `Hud/showTaskbarProgress` | bool | `true` | "Hiện tiến độ trên thanh taskbar" | Giao diện | ➕ mới |
| `Hud/miniWindowX/Y/Screen` | int/QString | `-1`/`""` | (ẩn, auto-persist) | — | ✅ đã có |
| `Hud/idleHideSec` | int | `30` | (ẩn, power-user) | — | (constant trong QML) |

**Ghi chú:**
- `showOnHideToTray` đặt tên hơi lệch (giờ trigger cả minimize lẫn hide-to-tray). **Giữ key** (tránh migration vỡ), chỉ đổi label hiển thị thành "Hiện mini HUD khi thu nhỏ". Property/biến nội bộ giữ tên cũ.
- Không bỏ `minimizeToTray`/`confirmOnClose` — chúng vẫn đúng vai trò.

**UI settings (SettingsPage) — Section "Chung":**
- `minimizeToTray` (đã có), `confirmOnClose` (đã có).
**Section "Giao diện":**
- `showOnHideToTray` label mới "Hiện mini HUD khi thu nhỏ" (đã có).
- `showTaskbarProgress` (➕ thêm toggle mới).
**Section "Thông báo":**
- `notifyOnTransferDone` (đã có).
- Mỗi toggle dùng `SettingsToggle` bind 2 chiều `settingsViewModel.<prop>`.

---

## 8. Thiết kế giao diện theo Aurora Design System

### 8.1 Token màu (từ `AuroraColors.qml`)
| Dùng cho | Light | Dark | Token |
|---|---|---|---|
| Card bg | `#FFFFFF` | `#17171E` | `panel` |
| Page bg | `#F5F4F1` | `#0E0E12` | `bg` |
| Border | `#E6E3DC` | `#25252E` | `border` |
| Divider | rgba(0,0,0,.05) | rgba(255,255,255,.05) | `divider` |
| Text chính | `#0E0E12` | `#F5F4F1` | `ink1` |
| Text phụ | `#5C5C66` | `#A0A0AC` | `ink3` |
| Caption | `#8A8A94` | `#8A8A98` | `ink4` |
| Accent (UL, progress, active) | `#FF5B2E` | (same) | `accent` |
| Gradient progress | `#FF5B2E → #FF3D7F` | (same) | `accent → accent3` |
| Download icon | `#2566E5` | (same) | `info` |
| Sync icon | `#0A8A5C` | (same) | `success` |
| Error/failed | `#D53030` | (same) | `danger` |

### 8.2 Spacing / radius / typography (từ `AuroraTheme.qml`)
- Spacing grid 4px: `sp1=4 sp2=8 sp3=12 sp4=16 sp6=24`.
- Radius: card `radiusLg=14`, row/button `radiusSm=6`/`radiusMd=10`, pill `999`.
- Padding panel: `sp4 (16)`. Row hover radius: 6-8.
- Font: `fontSans` (Geist/Be Vietnam Pro), `fontMono` (Geist Mono) cho số/tốc độ (tabular-nums).
- Type scale: brand 12px Bold, speed 13px Medium mono, filename 13px Medium, meta 11-12px, caption 11px.

### 8.3 Kích thước thành phần
| | Mini HUD | Tray Popup |
|---|---|---|
| Width | 360 | 320 |
| Padding | 16 (sp4) | 14 |
| Row height | 52 | 44 |
| Header height | 40 | 36 |
| Sparkline height | 32 | — |
| Progress bar height | 4px (mini) | 3px (popup) |
| Drop shadow | `0 12 32 rgba(0,0,0,.10/.45)` | (same) |

### 8.4 Animation
| Hiệu ứng | Duration | Easing | Gate |
|---|---|---|---|
| Mini fade-in | 80ms | OutCubic | !reduceMotion |
| Mini fade-out | 220ms (durBase) | OutCubic | !reduceMotion |
| Popup fade-in | 80ms | OutCubic | !reduceMotion |
| Popup fade-out | 140ms (durFast) | OutCubic | !reduceMotion |
| Snap edge | 150ms | OutCubic | !reduceMotion |
| Progress bar fill | 220ms | OutCubic | !reduceMotion |
| Row hover bg | 140ms (durFast) | — | !reduceMotion |
| Focus row highlight | 220ms opacity, clear sau 1500ms | OutCubic | !reduceMotion |

### 8.5 Tham chiếu mockup
`mockup/transfer-hud-preview.html` — render đầy đủ 5 mini states + tray 3 màu + popup + so sánh, có toggle light/dark. Dùng làm "design contract" khi review pixel.

---

## 9. Quyết định mở cần chốt khi implement

1. **Thoát khi KHÔNG có transfer**: thoát ngay (đề xuất) hay vẫn confirm nhẹ? — Spec mặc định: thoát ngay; `confirmOnQuit` chỉ áp khi có transfer.
2. **Phím tắt hiện Mini HUD**: có thêm `Ctrl+Shift+H` không? — Optional, P-sau.
3. **Multi-toast gộp**: implement debounce-gộp balloon ngay hay để sau? — Đề xuất để sau (P-polish).

---

## 10. Acceptance Criteria (nghiệm thu)

**Vòng đời cửa sổ:**
- [ ] X khi không transfer → app thoát ngay, tray icon biến mất.
- [ ] X khi có transfer → confirm dialog "Thoát & dừng / Huỷ"; Huỷ giữ app; Thoát dừng + persist + quit.
- [ ] Minimize (_) khi không transfer → taskbar, KHÔNG mini.
- [ ] Minimize (_) khi có transfer + setting ON → taskbar + mini hiện < 100ms.
- [ ] Minimize khi setting `showMiniOnMinimize=false` → KHÔNG mini.

**Mini HUD:**
- [ ] Mini hiển thị đúng tốc độ/top-5/sparkline so với cửa sổ chính.
- [ ] Kéo mini → thả gần edge → snap. Restart app → vị trí giữ nguyên đúng monitor.
- [ ] Idle 30s → mini fade out.
- [ ] Restore main (double-click tray / taskbar / ⛶) → mini đóng.
- [ ] Click row → main mở + scroll tới task + highlight; mini KHÔNG đóng.
- [ ] ✕ mini → ẩn + không tự hiện lại đến khi re-arm.

**Tray:**
- [ ] Single-click → popup < 100ms anchored tray; click lần 2 → đóng.
- [ ] Double-click → restore main, popup/mini đóng, không nháy.
- [ ] Click ngoài popup → auto-dismiss.
- [ ] Icon đổi màu đúng idle/active/error theo §4.
- [ ] Right-click → menu 5 mục hoạt động.

**Notification:**
- [ ] Transfer xong → balloon "Đã hoàn tất: filename" (nếu setting ON).
- [ ] Transfer lỗi → balloon Critical + tray đỏ (nếu không còn active).
- [ ] Click balloon → main mở + focus đúng task.
- [ ] Tắt `notifyOnTransferDone` → không balloon, tray vẫn đổi màu.

**Hiệu năng:**
- [ ] HUD idle CPU < 0.5%.
- [ ] Sparkline 1Hz không thrash; debounce 250ms hoạt động.

**Đa màn hình / edge:**
- [ ] Unplug monitor chứa mini → reset default, không crash.
- [ ] Reduce motion → animation step-cut.

---

## 11. Component Data Contracts (cho dev)

### Main.qml (root ApplicationWindow)
```qml
onClosing: (close) => {
    // §2.4: confirm nếu có transfer; thoát ngay nếu không + setting
}
onVisibilityChanged: {
    if (visibility === Window.Minimized) {
        if (settingsViewModel.showMiniOnMinimize
            && transferHudViewModel.shouldShowMini) {
            transferHudViewModel.acknowledgeMini();
            miniHudWindow.bindToVisibility(true);
        }
    } else if (visible && visibility !== Window.Minimized) {
        // restore → đóng mini
        if (miniHudWindow.visible) miniHudWindow.dismissWithFade();
    }
}
```

### SystemTray (C++)
- `setHudHint(active, pending, failed)` ← main.cpp connect `hudVM.countersChanged`.
- Double-click → `showWindowRequested` → main.cpp: `w->show(); w->raise(); requestActivate()` (restore từ minimized).

### TransferHudViewModel
- `shouldShowMini` công thức: `(activeTotal + pendingTotal + failedTotal + syncPending) > 0 && !m_userDismissed`.
- `focusTask(id)` → emit `taskFocusRequested(page, id)`.

---

## 12. Sơ đồ luồng tín hiệu (Signal flow)

```
[Window controls]
  X ──────────────▶ onClosing ──(có transfer?)──▶ confirmQuitDialog ──▶ pauseAll+persist+Qt.quit()
  _ ──────────────▶ onVisibilityChanged(Minimized) ──(transfer + setting)──▶ mini.bindToVisibility(true)
  restore ────────▶ onVisibilityChanged(Windowed) ──▶ mini.dismissWithFade()

[Tray]
  single-click ───▶ 250ms guard ─▶ togglePopupRequested ─▶ Main.toggleTrayPopup(rect) ─▶ popup
  double-click ───▶ showWindowRequested ─▶ main restore+focus ─▶ (onVisibilityChanged) mini dismiss
  menu Hiện mini ─▶ showMiniRequested ─▶ Main.showMiniHud()
  menu Cài đặt ──▶ openSettingsRequested ─▶ main + navigateToSettings()
  menu Thoát ────▶ quitRequested ─▶ (confirm) Qt.quit()

[Transfer events]
  taskCompleted ──▶ hudVM.transferDone(id,name,true,"") ─▶ tray.showNotification(...,id)
  taskFailed ─────▶ hudVM.transferDone(id,name,false,err) ─▶ tray balloon Critical + icon đỏ
  balloon click ──▶ tray.balloonClicked(id) ─▶ main restore + hudVM.focusTask(id)
  focusTask ──────▶ taskFocusRequested(page,id) ─▶ Main switch page + page.focusTask(id) ─▶ ListView highlight

[HUD visibility]
  counters change ▶ hudVM.shouldShowMiniChanged ─▶ (main hidden?) mini.bindToVisibility(shouldShow)
  idle 30s ───────▶ mini idleHideTimer ─▶ mini.dismissWithFade()
  drag release ───▶ mini snap ─▶ settingsViewModel.saveMiniWindowPosition(x,y,screen)
```

---

## 13. Delta so với code P0-P3 (việc cần làm cho spec v3)

> Code P0-P3 đã đúng hướng X=minimize-to-tray. Spec v3 chỉ thêm 2 phần net-new + 1 chỉnh label.

### Net-new 1: Minimize (_) → Mini HUD
- **Main.qml**: thêm handler `onVisibilityChanged`:
  ```qml
  onVisibilityChanged: {
      if (visibility === Window.Minimized) {
          if (settingsViewModel.showOnHideToTray   // reuse key
              && transferHudViewModel && transferHudViewModel.shouldShowMini) {
              transferHudViewModel.acknowledgeMini();
              miniHudWindow.bindToVisibility(true);
          }
      } else if (visible && visibility !== Window.Minimized) {
          if (miniHudWindow.visible) miniHudWindow.dismissWithFade();
      }
  }
  ```
- Lưu ý: `import QtQuick.Window` cho enum `Window.Minimized` (Main.qml đã import gián tiếp; verify).
- Hành vi onClosing (X→tray) giữ nguyên — đã có.

### Net-new 2: Windows Taskbar Progress (§5.5b)
- File mới `src/platform/TaskbarProgress.{h,cpp}` — COM ITaskbarList3.
- `TransferHudViewModel`: thêm property `aggregateProgress` (double 0..1, -1 idle) + emit trong `recomputeFromLists()`.
- `main.cpp`: tạo `TaskbarProgress` sau khi window shown; connect `hudVM.countersChanged` + `aggregateProgress` → push.
- CMakeLists: thêm TaskbarProgress.cpp/.h. Link `Ole32` (Windows, cho CoCreateInstance — thường đã có qua các lib khác).
- Setting `Hud/showTaskbarProgress` gate.

### Chỉnh nhỏ: label settings
- **SettingsPage.qml**: đổi label toggle `showOnHideToTray` → "Hiện mini HUD khi thu nhỏ". Thêm toggle `showTaskbarProgress`.
- **AppSettings/SettingsService/SettingsViewModel/SettingsRepository**: thêm `showTaskbarProgress` (pattern y hệt `notifyOnTransferDone`).

### Không đổi (giữ nguyên P0-P3)
- onClosing confirm dialog, tray 3-màu, popup, mini drag/snap/persist, sparkline, balloon click, focusTask, tray menu.

**Ước lượng:** ~1 ngày (onVisibilityChanged ~1h · TaskbarProgress COM ~4h · settings ~1h · test ~2h).

---

## 14. Phụ lục — Câu hỏi onboarding cho user mới (gợi ý)

Vì tray icon Windows 11 ẩn trong overflow, cân nhắc 1 toast/tooltip lần đầu minimize-có-transfer:
> "Fshare đang chạy nền. Tiến độ hiển thị ở cửa sổ nhỏ này. Ghim icon Fshare ra khay hệ thống để truy cập nhanh."

---

*Hết. Mọi thay đổi spec ghi changelog ở đầu file + cập nhật memory `project-transfer-hud-spec`.*
