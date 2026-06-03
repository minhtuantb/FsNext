# Yêu thích (Favorites) — UI test cases

> Loại: **UI**. Màn: `FavoritesPage.qml` ↔ `FavoritesViewModel` (giống My Files: list + detail panel,
> breadcrumb, multi-select; thêm "Bỏ yêu thích"). Cờ ⚠️ = rủi ro cao. Tổng: **11 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| FAV-UI-001 | Render trang yêu thích | Đã login, có ≥1 mục yêu thích | 1) Vào Yêu thích 2) Chụp | Header "Yêu thích" + subtitle "N mục đã gắn sao", search theo tên, nút Làm mới, danh sách (Name/Size), detail panel | UI-driver | P1 | no | — |
| FAV-UI-002 | Empty state khi chưa có mục | Tài khoản chưa gắn yêu thích | 1) Vào Yêu thích | Empty state: icon ❤ + "Chưa có mục yêu thích" + "Đánh dấu file hoặc thư mục yêu thích từ trang Quản lý file." | UI-driver | P1 | no | — |
| FAV-UI-003 | Loading state | — | 1) Vào trang lúc đang tải | Spinner + "Đang tải danh sách yêu thích…"; danh sách ẩn cho tới khi xong | UI-driver | P2 | no | — |
| FAV-UI-004 | Search lọc theo tên (live) | Có nhiều mục | 1) Gõ tên vào ô search | Lọc client-side, không phân biệt dấu, cập nhật tức thì; xóa search → hiện lại đủ | qmltest/UI-driver | P2 | no | — |
| FAV-UI-005 ⚠️ | Bỏ yêu thích phản ánh UI | Chọn 1 mục ở root | 1) Bấm "Bỏ yêu thích" (panel/menu) | Mục biến khỏi danh sách; count subtitle giảm; KHÔNG xóa file thật trên cloud (chỉ bỏ gắn sao) | UI-driver | P1 | no | — |
| FAV-UI-006 | Gating nút "Bỏ yêu thích" | Đang trong subfolder | 1) Vào 1 folder yêu thích | Nút "Bỏ yêu thích" ẩn khi `isInFolder=true` (chỉ bỏ ở root) | UI-driver | P2 | no | — |
| FAV-UI-007 | Detail panel single/multi | Có mục | 1) Click 1 mục 2) Ctrl+Click thêm | Single: icon+tên+loại+CHI TIẾT; Multi: "N mục đã chọn" + breakdown "x thư mục · y tệp" | UI-driver | P1 | no | — |
| FAV-UI-008 | Gating action theo loại | — | 1) Chọn folder / media / file-đã-tải | "Mở thư mục" (folder), "Xem trực tiếp" (media), "Mở file" (downloaded non-media); các nút khác (Sao chép/Tải về) luôn có | UI-driver | P1 | no | — |
| FAV-UI-009 | Điều hướng folder + breadcrumb | Folder trong yêu thích | 1) Double-click folder 2) Click ❤ root | Drill-down OK; empty subfolder hiện "Thư mục trống"; ❤ quay về root yêu thích | UI-driver | P2 | no | — |
| FAV-UI-010 | Đồng bộ với My Files | Vừa gắn/bỏ sao ở My Files | 1) Đổi yêu thích ở My Files 2) Quay lại + Làm mới | Danh sách yêu thích phản ánh đúng sau `loadFavorites()` | UI-driver | P2 | no | — |
| FAV-UI-011 | i18n + runtime switch | — | 1) Grep FavoritesPage 2) Đổi EN | Mọi chuỗi qsTr() (header/empty/toast/menu); switch EN dịch hết | tay/UI-driver | P1 | no | — |
