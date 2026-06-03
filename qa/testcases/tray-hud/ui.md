# Tray + Transfer HUD — UI test cases

> Loại: **UI**. Tray: `SystemTray.cpp` (Qt Widgets). HUD: `MiniHudWindow.qml` + `TransferHudPanel.qml`
> + `Sparkline.qml` ↔ `TransferHudViewModel`. Tray popup = TransferHudPanel(compact=true).
> Cờ ⚠️ = rủi ro cao. Tổng: **13 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| TRAY-UI-001 | Tray icon màu theo trạng thái | App chạy | 1) Idle 2) Có transfer 3) Có task lỗi | Icon: xám (idle) / cam (active) / đỏ (error-only); tooltip đúng "Sẵn sàng" / "Đang chuyển N mục…" / "N mục lỗi" | tay/UI-driver | P1 | no | — |
| TRAY-UI-002 | Menu chuột phải tray | App chạy | 1) Chuột phải tray icon | Menu: Hiện cửa sổ, (Vault nếu tồn tại), Hiện mini HUD, Tạm dừng tất cả, Cài đặt thông báo…, Thoát | tay | P1 | no | — |
| TRAY-UI-003 | "Vault" trong menu có điều kiện | — | 1) Chưa có vault 2) Có vault | Mục Vault ẩn khi chưa có vault; hiện "Khóa/Mở khóa Vault" đúng trạng thái khi có | tay | P2 | no | — |
| TRAY-UI-004 | "Tạm dừng tất cả" gating | — | 1) Không có active 2) Có active | Mục disabled khi không có transfer active; enable khi có | tay | P2 | no | — |
| TRAY-UI-005 | Single vs double click tray | App chạy | 1) Click 1 lần 2) Double-click | Single → toggle tray popup (sau guard 250ms); Double → mở cửa sổ chính | tay | P2 | no | — |
| TRAY-UI-006 | Mini HUD hiện khi có transfer | Minimize + đang tải | 1) Minimize app 2) Bắt đầu transfer | `shouldShowMini=true` → MiniHudWindow hiện (frameless, on-top, không vào taskbar) | UI-driver | P1 | no | — |
| TRAY-UI-007 | HUD render nội dung | Đang có transfer | 1) Hiện mini HUD 2) Chụp | Pill LIVE (chấm pulse), tổng tốc độ ↓/↑, sparkline, danh sách top-5 transfer (icon/tên/tiến độ/tốc độ), footer DL/UL/SYNC + "Dán link" | UI-driver | P1 | no | — |
| TRAY-UI-008 | Pill trạng thái LIVE/IDLE/PAUSED | — | 1) Đang chạy 2) Tạm dừng 3) Rảnh | Pill đổi: LIVE (xanh) / PAUSED (hổ phách) / IDLE (xám); `runState` khớp | UI-driver | P2 | no | — |
| TRAY-UI-009 | Pause/Resume từ HUD | Đang tải | 1) Bấm ⏸ all 2) Bấm ▶ all 3) Bấm ⏸ 1 row | `pauseAll/resumeAll/pauseTask` hiệu lực; progress bar + tốc độ phản ánh | UI-driver | P1 | no | — |
| TRAY-UI-010 | Empty state HUD | Không có transfer | 1) Mở HUD lúc rảnh | Hiện "Không có lượt chuyển nào" + icon; không có dòng rác | UI-driver | P2 | no | — |
| TRAY-UI-011 | Overflow chip | >5 transfer | 1) Tạo >5 transfer | Hiện "+ N mục khác"; click → mở cửa sổ chính (expand) | UI-driver | P2 | no | — |
| TRAY-UI-012 | Drag + snap mini HUD | HUD đang hiện | 1) Kéo HUD tới mép màn hình | Kéo mượt (damped); snap mép (±24px → cách 8px); vị trí được lưu, giữ sau restart | UI-driver | P2 | no | — |
| TRAY-UI-013 | i18n tray (C++) + HUD (QML) | — | 1) Grep tr() SystemTray.cpp + qsTr() HUD QML 2) Đổi EN | Menu tray (tr, có plural %n), nhãn HUD (qsTr) bọc đủ; switch EN dịch hết, plural đúng | tay/UI-driver | P1 | no | — |
