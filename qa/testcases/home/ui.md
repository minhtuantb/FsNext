# Trang chủ (Home + Search) — UI test cases

> Loại: **UI**. Màn: `HomePage.qml` + `HomeSearchOverlay.qml` ↔ `HomeSearchViewModel`.
> Search có máy trạng thái: Idle/TooShort/UrlFile/UrlFolder/UrlMultiple/Keyword/Blocked.
> Cờ ⚠️ = rủi ro cao. Tổng: **14 case**.
> Hợp đồng VM/logic chi tiết: xem `home/functional.md`, `home/validation.md`.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| HOME-UI-001 | Render trang chủ | Đã đăng nhập | 1) Vào Home 2) Chụp | Hiện: ô search (top bar), nút Tải lên, bánh răng Cài đặt, greeting hero, 4 quick-action card, mục "Tiếp tục đang tải", mục "File gần đây" | UI-driver | P1 | no | — |
| HOME-UI-002 | Search < 3 ký tự → TooShort | Home | 1) Gõ "ab" vào search | Border vàng (warn), chip hint "Nhập tối thiểu 3 ký tự…", overlay ẩn, submit bị chặn | qmltest/UI-driver | P1 | no | — |
| HOME-UI-003 | Từ khóa hợp lệ → overlay kết quả | Home, đã login | 1) Gõ ≥3 ký tự 2) Enter | `isSearching` → overlay loading skeleton; sau đó hiện danh sách kết quả; tiêu đề "Kết quả tìm kiếm · N mục" | UI-driver | P1 | no | — |
| HOME-UI-004 | Kết quả rỗng | Home | 1) Tìm từ khóa chắc chắn 0 kết quả | Overlay hiện empty state `Không tìm thấy "%1"` + gợi ý; không spinner kẹt | UI-driver | P2 | no | — |
| HOME-UI-005 ⚠️ | Bad-word → Blocked, fail-closed | Home | 1) Gõ từ khóa chứa bad-word | Border đỏ (danger), hint "Từ khoá chứa nội dung không phù hợp", overlay ẩn, **submit bị chặn** (không gọi API) | qmltest/UI-driver | P0 | no | — |
| HOME-UI-006 | Dán link file Fshare | Home | 1) Dán `fshare.vn/file/<code>` | Border accent, hint "Link file · Nhấn Enter…"; Enter → route mở chi tiết file | UI-driver | P1 | no | — |
| HOME-UI-007 | Dán nhiều link | Home | 1) Dán 2+ URL | Hint "Phát hiện N link…"; Enter → mở tải hàng loạt | UI-driver | P2 | no | — |
| HOME-UI-008 | Esc / click ngoài xóa search | Đang có kết quả | 1) Nhấn Esc (hoặc click ngoài overlay) | text="", state→Idle, overlay+chip auto ẩn | qmltest/UI-driver | P2 | no | — |
| HOME-UI-009 | Phím ↑/↓ + Enter chọn kết quả | Overlay đang mở | 1) ↓ di chuyển highlight 2) Enter | Highlight di chuyển; Enter kích hoạt dòng đang chọn (mở file) thay vì submit | UI-driver | P2 | no | HOME-BUG-0001 (CLOSED) |
| HOME-UI-010 | Infinite-scroll loadMore | Kết quả >1 trang | 1) Cuộn overlay xuống ~50% | `hasMorePages` → gọi `loadMore()` nạp thêm; không nạp trùng | UI-driver | P2 | no | — |
| HOME-UI-011 | Quick-action điều hướng | Home | 1) Click từng card | "Dán link & tải"→mở dialog Thêm URL, "Tải file lên"→Upload, "Thêm folder sync"→Sync, "Tạo link"→Files (đúng trang) | UI-driver | P2 | no | HOME-BUG-0002 (CLOSED) |
| HOME-UI-012 | Filter "File gần đây" | Có lịch sử tải | 1) Click tab Tất cả/Video/Tài liệu/Ảnh | Danh sách lọc đúng loại; empty state khi rỗng "Chưa có file nào gần đây" | UI-driver | P2 | no | — |
| HOME-UI-013 | i18n + runtime switch | — | 1) Grep HomePage/Overlay + tr() HomeSearchViewModel 2) Đổi EN | Mọi chuỗi qsTr/tr; switch EN dịch hết hint state-machine (tr trong .cpp) | tay/UI-driver | P1 | no | — |
| HOME-UI-014 | Chế độ chính xác (quoted "…") | Home | 1) Gõ `"Lồng tiếng"` (có ngoặc kép) | Border accent, hint "Chế độ chính xác · Nhấn Enter để tìm"; Enter → tìm phrase trần (bỏ ngoặc), không lọt bad-word filter sai | UI-driver/qmltest | P2 | no | — |
