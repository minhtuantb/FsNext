---
id: MYFILES-BUG-0001
title: Icon "+" không render — caller truyền name "+" nhưng asset là plus.svg
status: CLOSED
severity: LOW
area: FsIcon callers (FileManagerToolbar.qml, Main.qml, ShowcasePage.qml)
found_by: monkey-chaos-r1 (Burst 2 — hammer Refresh My Files)
found_run: 2026-06-03-monkey-chaos-r1
assignee: QA-dev
fix_commit: fa2b01d
verified_run: 2026-06-03-monkey-chaos-r1
created: 2026-06-03
updated: 2026-06-03
---

## Mô tả
`FsIcon` cố mở `qrc:/qml/Fshare/Icons/+.svg` (không tồn tại). Trong `qml/Fshare/Icons/`
file thực tên là **`plus.svg`**, nhưng một số caller truyền `icon: "+"` → icon dấu cộng
KHÔNG render (hoặc về fallback) ở mọi nơi dùng "+": nút **New Folder** (toolbar My Files),
quick-action **Thêm tải xuống**, ShowcasePage. Lỗi cosmetic nhưng spam log WARN liên tục.

## Repro
1. Vào My Files (hoặc Trang chủ) — nơi có nút "+" (New Folder / Thêm tải xuống).
2. Quan sát icon "+" + log `%APPDATA%\FPT\FsNext\fsnext.log`.
- **Expected:** icon dấu cộng hiển thị; không WARN.
- **Actual:** `[WARN] FsIcon.qml:40 QML QQuickImage: Cannot open: qrc:/qml/Fshare/Icons/+.svg` lặp lại.
- **Bằng chứng:** runs/2026-06-03-monkey-chaos-r1 (log trích trong results.md).

## Phân tích (QA)
- Caller: [FileManagerToolbar.qml:85](../../../qml/Fshare/Pages/FileManager/FileManagerToolbar.qml) `icon: "+"`,
  [Main.qml:528](../../../qml/Main.qml) `icon: "+"`, ShowcasePage.qml:234.
- Asset có sẵn: `qml/Fshare/Icons/plus.svg`. **Fix gợi ý:** đổi caller `icon: "+"` → `icon: "plus"`
  (hoặc thêm alias `+` → `plus` trong FsIcon, hoặc copy plus.svg → +.svg). Ưu tiên sửa caller cho nhất quán.

## Fix (process fix điền)
- Thay đổi:
- Commit:

## Verify (QA điền sau khi fix)
- Chạy lại: kiểm icon render + 0 WARN "+.svg" trong log khi mở My Files/Home.
- Run:

## Fix
- Đổi `icon: "+"` → `icon: "plus"` ở FileManagerToolbar.qml, Main.qml, ShowcasePage.qml. Commit **fa2b01d**.

## Verify
- Rebuild + mở My Files: nút New Folder hiện icon "+", **0 WARN `+.svg`** trong log (evidence 14-verify-myfiles-icon.png).

## Nhật ký trạng thái
- 2026-06-03 16:42 — NEW (QA, run 2026-06-03-monkey-chaos-r1)
- 2026-06-03 17:05 — FIXED + CLOSED (fa2b01d, verify run monkey-chaos-r1)
