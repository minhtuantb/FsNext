---
id: HOME-BUG-0002
title: Quick-action "Dán link & tải" không tự mở dialog Thêm URL (race với Loader lazy của DownloadPage)
status: CLOSED
severity: LOW
area: qml/Main.qml (onAddDownloadRequested + command act.add-dl) ↔ DownloadPage lazy Loader
found_by: HOME-UI-011
found_run: 2026-06-04-home-r1
assignee: "claude (QA+fix cùng phiên)"
fix_commit: ""
verified_run: 2026-06-04-home-r1
created: 2026-06-04
updated: 2026-06-04
---

## Mô tả
Click quick-action **"Dán link & tải"** ở Trang chủ điều hướng đúng sang trang Tải xuống
nhưng KHÔNG tự mở dialog "Thêm URL" (blank) như thiết kế — người dùng phải bấm "Thêm URL"
thủ công. Tính năng ĐÃ được wire (signal `openDownloadDialog` + handler trong DownloadPage)
nhưng bị **race**.

Cơ chế: `Main.qml::onAddDownloadRequested` (và command `act.add-dl`) làm tuần tự:
```js
root.currentPage = Pages.download;   // Loader mới bắt đầu instantiate DownloadPage
if (links rỗng) root.openDownloadDialog();   // EMIT NGAY
```
`DownloadPage` nạp lazy qua `Loader { active: currentPage===Pages.download; sourceComponent: ... }`
và lắng nghe qua `Connections { target: Window.window; function onOpenDownloadDialog(){...} }`.
Khi emit ngay sau khi đổi `currentPage`, DownloadPage chưa instantiate xong / `Window.window`
chưa resolve → Connections chưa gắn → signal **bị lỡ** → dialog không mở. (Loader huỷ item
khi rời trang nên tái hiện MỌI lần vào Tải xuống từ quick-action, không chỉ lần đầu.)

## Repro
1. Trang chủ → click card "Dán link & tải".
- **Expected:** sang trang Tải xuống + dialog "Thêm URL" (blank) tự mở.
- **Actual:** sang trang Tải xuống nhưng KHÔNG có dialog (empty state + nút "Thêm URL").
- **Bằng chứng:** runs/2026-06-04-home-r1/evidence/15-qa-paste.png, 17-qa-paste-retry.png

## Phân tích
Off-by-một-frame: emit signal trước khi listener (DownloadPage) sống. Sửa: defer emit bằng
`Qt.callLater` để chạy sau khi Loader instantiate xong page + Window.window đã resolve.
Áp dụng cho cả 2 site cùng pattern: `onAddDownloadRequested` và command `act.add-dl`.

## Fix
- `Main.qml`: bọc `root.openDownloadDialog()` bằng `Qt.callLater(...)` ở cả onAddDownloadRequested và act.add-dl.
- Commit: <điền khi commit>

## Verify (QA điền sau khi fix)
- ✅ Click "Dán link & tải" → sang Tải xuống + dialog "Thêm URL" mở (`18-qa-paste-fixed.png`).
- Regression: 3 quick-action còn lại (Upload/Sync/Files) vẫn điều hướng đúng.

## Nhật ký trạng thái
- 2026-06-04 10:47 — NEW (QA, run 2026-06-04-home-r1, found_by HOME-UI-011)
- 2026-06-04 10:52 — FIXING (claude)
- 2026-06-04 11:00 — thử Qt.callLater defer emit → **KHÔNG đủ** (Loader incubate trễ hơn 1 tick; retest vẫn không mở)
- 2026-06-04 11:10 — FIXED (chuyển signal `openDownloadDialog` → property flag `pendingOpenAddDialog`,
  DownloadPage tiêu thụ ở `Component.onCompleted` + `onPendingOpenAddDialogChanged` — y pattern `pendingDownloadLinks`)
- 2026-06-04 11:14 — CLOSED (rebuild + retest UI-011 "Dán link & tải" → dialog "Thêm tải xuống" mở; `18-qa-paste-fixed.png`; CrashWatch sạch)
