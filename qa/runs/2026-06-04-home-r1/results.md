# Run — Home / Search (UI automation) — 2026-06-04 r1

- **Module:** Trang chủ (Home + Search overlay) — `HomePage.qml` / `HomeSearchOverlay.qml` ↔ `HomeSearchViewModel`
- **Mục tiêu:** verify trên GIAO DIỆN THẬT các hành vi search vừa được phủ unit test
  (classification state-machine + overlay results/empty + fail-closed bad-word + URL routing).
- **Tool:** UI-driver `qa/automation/ui-driver/fsnext-ui.ps1` (PrintWindow + SendInput) + clipboard paste.
- **Account test:** `taikhoantestfshare@gmail.com` (KHÔNG account thật).
- **Môi trường:** Windows, 2 màn hình (app maximize ở màn phải, rect L=1912 — hợp lệ), DPI 100%.
- **Build:** `output/FsNext.exe` (dev) 2026-06-04 10:01.
- **Kết quả:** **18/18 PASS** sau fix (2 case ban đầu FAIL → fix → retest PASS).
  **2 bug tìm & đóng trong phiên:** HOME-BUG-0001 (MEDIUM, Enter mở dòng overlay) +
  HOME-BUG-0002 (LOW, quick-action "Dán link" không mở dialog). CrashWatch sạch, app alive suốt phiên.
- **Phụ:** dialog "Thêm tải xuống" + Settings hiện "Thư mục lưu = C:\Windows" — đúng triệu chứng **DL-BUG-0002** (đã có, NEW).
- **i18n cleanup:** sau test UI-013 đã set lại `HKCU\Software\FPT\FsNext\UI\language = vi`
  (instance đang chạy còn hiển thị EN, sẽ về vi ở lần mở sạch kế).

## Map case → kết quả

| Case (UI / FN tương ứng) | Input | Kỳ vọng | Bằng chứng | Kết quả |
|---|---|---|---|---|
| HOME-UI-002 / FN-003 — TooShort | `ab` | Chip vàng "Nhập tối thiểu 3 ký tự để tìm kiếm"; overlay ẩn; submit chặn | `01-tooshort.png` | ✅ PASS |
| HOME-UI-003 / FN-020 — Keyword → overlay results | `big100` | Overlay "KẾT QUẢ TÌM KIẾM · 2 mục"; 2 dòng `big100(1).bin`/`big100.bin` (icon+size+chevron) | `06-results.png` | ✅ PASS |
| HOME-UI-004 / FN-022 — Empty state | `iphone`, `HELLO` | Overlay "KHÔNG CÓ KẾT QUẢ · Không tìm thấy "%1"" + gợi ý | `02-keyword.png`, `diag-type.png` | ✅ PASS |
| HOME-UI-005 / FN-005 ⚠️ — Bad-word Blocked (fail-closed) | `phim sex` | Chip **đỏ** "Từ khoá chứa nội dung không phù hợp"; overlay ẩn; KHÔNG search | `04-blocked.png` | ✅ PASS |
| HOME-UI-006 / FN-009 — URL file | `https://www.fshare.vn/file/ABC123XYZ` | Chip accent "Link file · Nhấn Enter để xem chi tiết" | `05-urlfile.png` | ✅ PASS |
| HOME-UI-007 / FN-011 — Nhiều URL | 2 link fshare | Chip accent "Phát hiện 2 link · Nhấn Enter để mở tải hàng loạt" | `07-multiurl.png` | ✅ PASS |
| HOME-UI-008 — Esc xoá search | (có text) → `{ESC}` | text=""; state Idle; chip + overlay ẩn | `08-esc-cleared.png` | ✅ PASS |
| **HOME-SC-001** — Click dòng overlay → mở FileDetailSheet | search `big100` → click dòng `big100.bin` | FileDetailSheet mở ĐÚNG file: tiêu đề `big100.bin`, `100 MB · 03/06/2026 19:01`, nút Tải về máy / Sao chép link / Mở trên fshare.vn. `{ESC}` đóng sheet sạch, về overlay | `sc1-10-overlay.png`, `sc1-11-detail.png`, `sc1-12-closed.png` | ✅ PASS |
| **HOME-UI-009** — ↑/↓ + Enter chọn dòng overlay | search `big100` → `↓` → `Enter` | `↓` highlight dòng 0; `Enter` mở FileDetailSheet dòng highlight | `sc1-20-highlight.png`, `sc1-21-enter-open.png` (FAIL), `sc1-22-enter-fixed.png` (PASS sau fix) | 🔴→✅ **FAIL→FIXED→PASS** (HOME-BUG-0001) |
| **HOME-UI-014** — Chế độ chính xác (quoted) | `"Lồng tiếng"` | Chip "Chế độ chính xác · Nhấn Enter"; tìm phrase trần (không bị bad-word chặn) | `09-quoted.png` (overlay 4 mục phim "Lồng Tiếng") | ✅ PASS |
| **HOME-UI-012** — Filter "File gần đây" | tab Video / Tài liệu | Lọc đúng loại | `10-filter-video.png` (chỉ .mkv), `11-filter-doc.png` (chỉ .txt) | ✅ PASS |
| **HOME-UI-011** — Quick-action điều hướng | click 4 card | Upload/Sync/Files đúng trang; "Dán link & tải"→dialog Thêm URL | `12-qa-upload.png`, `13-qa-sync.png`, `14-qa-files.png`, `18-qa-paste-fixed.png` | 🔴→✅ **FAIL→FIXED→PASS** (HOME-BUG-0002; "Dán link" ban đầu không mở dialog) |
| **HOME-SC-004** — Click ngoài overlay → đóng | search → click vùng dưới | Overlay đóng + search về Idle; click bị catcher nuốt (dòng dưới không mở) | `sc4-30-overlay-open.png`, `sc4-31-dismissed.png` | ✅ PASS |
| **HOME-SC-007** — Nhiều URL Enter → dialog tải hàng loạt | dán 2 link + Enter | Sang Tải xuống + dialog "Thêm tải xuống" pre-fill 2 link + badge "2 file" | `sc7-32-multiurl-dialog.png` | ✅ PASS |
| **HOME-SC-005** — File URL Enter → FileDetailSheet | dán URL thật + Enter | Sheet mở đúng "mini_live.bin · 50.0 MB · 04/06/2026 08:32" | `sc5-33-urlfile-chip.png`, `sc5-34-file-detail.png` | ✅ PASS |
| **HOME-SC-006** — Folder URL Enter → trình duyệt folder | dán URL folder + Enter | FolderBrowserDialog mở ("ZZZFOLDER1 · 0 mục · Folder trống" — graceful, không crash) | `sc6-35-urlfolder-chip.png`, `sc6-36-folder-route.png` | ✅ PASS |
| **HOME-UI-013** — i18n runtime switch | Settings → English | Toàn bộ Home dịch EN (sidebar, greeting "Hi, Test, shall we continue?", quick-actions, "Recent files", "Uploaded · 2 hours ago") + hint state-machine từ C++ tr() "Enter at least 3 character(s) to search"; tên file (data) giữ nguyên | `sc-i18n-43-home-en.png`, `sc-i18n-44-tooshort-en.png` | ✅ PASS |
| (smoke) Render Home + đăng nhập | — | Top bar search + quick-actions + "File gần đây" | `00-home.png` | ✅ PASS |

## Ghi chú vận hành (quan trọng cho lần sau)
- **SendKeys không tin cậy để nhập chuỗi vào ô search:**
  1. **IME tiếng Việt (Telex) đang bật** → gõ `phim sex` ra `phim sẽ` (e+x = dấu ngã);
  2. SendKeys **rớt ký tự** dưới tải → `www` thành `ww` → URL không khớp regex.
  ⇒ **Luôn nhập bằng clipboard:** `Set-Clipboard $txt; Fs-Type "^v"`. (Bổ sung cho memory
  `feedback-ui-driver-confirm-dialog`: trước chỉ ghi FsFolderPicker; nay áp dụng cho cả ô search.)
- **Phạm vi search**: trả về file CỦA TÀI KHOẢN (không phải search public) → `iphone`/`HELLO`
  ra rỗng là ĐÚNG (account chỉ có file test .bin/.txt); `big100` khớp → ra 2 mục.
- App ở màn hình phụ (rect L=1912) — `Fs-Click`/`Fs-Grab` vẫn đúng (toạ độ tuyệt đối + PrintWindow).

## Liên hệ
- Đối ứng unit test: `tests/test_home_search_viewmodel.cpp` (CTest `HomeSearchViewModel`, 41 pass).
- Đã lái: HOME-SC-001 (mở file từ overlay → FileDetailSheet) ✅.
- Chưa lái UI: HOME-SC-002 (debounce/stale trực quan), HOME-SC-009 (infinite-scroll) —
  cần keyword trả >1 trang (>30 mục); account test chỉ có ~2 mục/keyword nên hoãn.
