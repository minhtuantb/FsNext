# Cài đặt (Settings) + Thông tin người dùng — UI test cases

> Loại: **UI**. Màn: `SettingsPage.qml` ↔ `SettingsViewModel`; `UserInfoPage.qml` ↔ `UserInfoViewModel`
> (+ đổi ngôn ngữ qua `LanguageViewModel`). Cờ ⚠️ = rủi ro cao. Tổng: **13 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| SET-UI-001 | Render đủ section Settings | Đã login | 1) Mở Cài đặt 2) Chụp | Hiện section: Chung, Khi tên file trùng, Ngôn ngữ, Hiệu suất tải, Tải xuống, Kết nối (proxy), Thông tin (version) | UI-driver | P1 | no | — |
| SET-UI-002 | Toggle Chung phản ánh state | Settings | 1) Bật/tắt từng switch (auto login, dark mode, minimize to tray, confirm on close…) | Switch đổi trạng thái + property VM cập nhật; hiệu lực hợp đồng (vd dark mode đổi theme) | UI-driver | P1 | no | — |
| SET-UI-003 | Proxy fields gating | Settings | 1) Đổi proxy mode: Không dùng → Hệ thống → Thủ công | Ô host + port **chỉ hiện** khi mode = Thủ công (proxyMode==2); ẩn ở 2 mode kia | qmltest/UI-driver | P1 | no | — |
| SET-UI-004 ⚠️ | Validate port | Proxy = Thủ công | 1) Nhập port ngoài 0–65535 / ký tự | Bị clamp/chặn về dải hợp lệ; không lưu giá trị rác | qmltest | P1 | no | — |
| SET-UI-005 | NumberSpinner clamp dải | Settings | 1) Tăng/giảm vượt min-max (DL threads 1–16, segments 1–32, UL 1–8, slots 0–32) | Spinner clamp đúng dải, không vượt | qmltest/UI-driver | P2 | no | — |
| SET-UI-006 | Chọn thư mục tải xuống | Settings | 1) Bấm folder picker 2) Chọn thư mục | Native dialog mở; đường dẫn chọn hiển thị; `effectiveDownloadFolder` cập nhật | tay | P2 | no | — |
| SET-UI-007 | File-conflict policy radio | Settings | 1) Chọn từng option (Đổi tên/Ghi đè/Bỏ qua/Hỏi mỗi lần) | Đúng 1 radio active; `fileConflictPolicy` cập nhật | qmltest/UI-driver | P2 | no | — |
| SET-UI-008 | Persist sau restart | Settings | 1) Đổi vài setting 2) Đóng app 3) Mở lại | `applySettings()` lưu; giá trị giữ nguyên sau khởi động lại (QSettings) | tay | P1 | no | — |
| SET-UI-009 | Reset về mặc định | Đã đổi setting | 1) Bấm Reset | `resetSettings()` khôi phục mặc định; UI cập nhật theo | UI-driver | P2 | no | — |
| SET-UI-010 | Đổi ngôn ngữ runtime | App đang chạy | 1) Đổi segmented Vi → En | Toàn UI retranslate ngay (không cần restart); không sót chuỗi VI | UI-driver | P1 | no | — |
| SET-UI-011 | Render UserInfo | Đã login | 1) Mở Thông tin người dùng | Hiện: avatar (hoặc gradient+initial), tên, email + badge "ĐÃ XÁC MINH", gói/level, hạn VIP, 3 thanh storage (không bảo đảm/bảo đảm/lưu lượng), 4 AcctLink | UI-driver | P1 | no | — |
| SET-UI-012 | Storage bar màu theo % | UserInfo | 1) Quan sát thanh dung lượng | Màu: >90% đỏ, >75% cam, còn lại xanh; số used/total/free khớp | UI-driver | P2 | no | — |
| SET-UI-013 | Refresh UserInfo + AcctLink | UserInfo | 1) Bấm refresh 2) Bấm 1 AcctLink | `refresh()` cập nhật số liệu; AcctLink mở browser đúng URL fshare.vn | UI-driver | P2 | no | — |
