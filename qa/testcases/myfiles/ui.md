# My Files (quản lý file cloud) — UI test cases

> Loại: **UI**. Màn: `FileManagerPage.qml` + `Pages/FileManager/*` ↔ `FileManagerViewModel`.
> 2-pane (danh sách + detail panel), cache-first, multi-select, context menu động.
> Cờ ⚠️ = rủi ro cao (xóa/bảo mật). Tổng: **14 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| MYFILES-UI-001 | Render trang + toolbar | Đã login | 1) Vào My Files 2) Chụp | Hiện toolbar (search, New Folder, Refresh, toggle list/grid), breadcrumb home, danh sách file/folder (cột Name/Size), detail panel phải | UI-driver | P1 | no | — |
| MYFILES-UI-002 | Cache-first load | Đã từng mở folder | 1) Vào lại folder | Hiện cache **ngay**, spinner refresh chạy nền; sau refresh danh sách cập nhật, không nháy trắng | UI-driver | P1 | no | — |
| MYFILES-UI-003 | Loading + empty folder | Folder rỗng | 1) Mở folder trống | Loading "Loading files…" rồi empty state "No files in this folder" + mô tả; danh sách ẩn | UI-driver | P2 | no | — |
| MYFILES-UI-004 | Detail panel — chưa chọn | Trang có file | 1) Không chọn gì | Panel hiện "Chọn file hoặc thư mục để xem thông tin" + icon | UI-driver | P2 | no | — |
| MYFILES-UI-005 | Detail panel — chọn 1 file | Có file | 1) Click 1 file | Panel: icon+tên+loại, section THAO TÁC (Tải về/Sao chép link/Di chuyển/Đặt mật khẩu/bảo mật), CHI TIẾT (kích thước/ngày tạo/sửa đổi) | UI-driver | P1 | no | — |
| MYFILES-UI-006 | Multi-select bật bulk action | ≥2 item | 1) Ctrl+Click nhiều / Ctrl+A | Badge số lượng "N", "N mục đã chọn" + breakdown "x thư mục · y tệp"; nút bulk (Di chuyển/Mật khẩu/Bật-Tắt bảo mật) hiện | UI-driver | P1 | no | — |
| MYFILES-UI-007 | Gating nút theo loại item | — | 1) Chọn folder 2) Chọn file đã tải 3) Chọn media | Folder: không có "Tải về"; file downloaded: hiện "Mở file"/"Mở thư mục chứa" + chấm xanh; media: hiện "Phát"/"Xem trực tiếp" | UI-driver | P1 | no | — |
| MYFILES-UI-008 | Badge cờ bảo mật trên tên | File secure/password/directlink | 1) Quan sát cột Name | Hiện đúng icon: khiên (secure), khóa (password), link (directlink) theo cờ item | UI-driver | P2 | no | — |
| MYFILES-UI-009 | Context menu động (chuột phải) | Có file | 1) Chuột phải file 2) Chuột phải folder | Menu đúng ngữ cảnh: file→Rename/Copy Link/Tải về/Move/Set Password/Delete; folder→Mở thư mục, không "Tải về"; media→"Xem trực tiếp" | UI-driver | P1 | no | — |
| MYFILES-UI-010 | Điều hướng folder + breadcrumb | Có folder | 1) Double-click folder 2) Click segment breadcrumb | Vào trong folder, breadcrumb thêm segment; click segment/❤ home quay về đúng cấp | UI-driver | P1 | no | — |
| MYFILES-UI-011 | Toggle list/grid | Trang có file | 1) Bấm nút grid 2) Bấm nút list | `viewMode` đổi "list"↔"medium"; layout render đúng (card 168×178 ở grid) | UI-driver | P2 | no | — |
| MYFILES-UI-012 ⚠️ | Xóa có xác nhận | Chọn ≥1 file | 1) Delete / menu Delete | Dialog xác nhận hiện; chỉ xóa khi xác nhận; Hủy → không xóa; không xóa nhầm item khác | UI-driver | P0 | no | — |
| MYFILES-UI-013 | Phím tắt | Trang có file | 1) Ctrl+F 2) Ctrl+A 3) Ctrl+C 4) ↑/↓ 5) Backspace | Ctrl+F focus search; Ctrl+A chọn all; Ctrl+C copy link+toast; mũi tên di con trỏ; Backspace lùi folder | UI-driver | P2 | no | — |
| MYFILES-UI-014 | i18n + runtime switch | — | 1) Grep FileManagerPage + components 2) Đổi EN | Mọi label/menu/empty-state/toast bọc qsTr(); switch EN dịch hết | tay/UI-driver | P1 | no | — |
