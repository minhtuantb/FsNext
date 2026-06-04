# Trang chủ (Home + Search) — Scenario / kịch bản test cases

> Loại: **scenario** (end-to-end nhiều bước, user journey). Chạy bằng **UI-driver**
> (`qa/automation/ui-driver/fsnext-ui.ps1`) chụp ảnh từng bước hoặc **runbook tay**
> khi có dialog/route sang trang khác. Bằng chứng → `qa/runs/.../evidence/`.
> Tiền đề chung: đã đăng nhập account TEST; ở trang Home. Tổng: **10 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| HOME-SC-001 | Tìm từ khoá → mở file từ overlay | Home, có kết quả thật | 1) Gõ ≥3 ký tự 2) Chờ overlay hiện kết quả 3) Click 1 dòng | overlay loading→results; click dòng → `showFileFromLinkcode` → FileDetailSheet mở đúng file; search box vẫn giữ text | UI-driver | P1 | no | — |
| HOME-SC-002 | Gõ → sửa → kết quả cập nhật (debounce + seq) | Home | 1) Gõ "matrix" 2) Trước khi kết quả về, xoá gõ "inception" | overlay cuối cùng hiển thị kết quả của "inception"; KHÔNG nhấp nháy kết quả "matrix" cũ (stale bị bỏ) | UI-driver | P1 | no | — |
| HOME-SC-003 | Keyword → Esc → Idle sạch | có kết quả | 1) Esc | text=""; overlay + hint chip ẩn; state Idle; không còn spinner | UI-driver | P2 | no | — |
| HOME-SC-004 | Click ngoài overlay → đóng | overlay mở | 1) Click vùng nội dung dưới topBar | `overlayDismissCatcher` xoá text → overlay đóng; click KHÔNG lọt xuống ScrollView | UI-driver | P2 | no | — |
| HOME-SC-005 | Dán link file → mở chi tiết | Home | 1) Dán `fshare.vn/file/<code>` 2) Enter | border accent + hint; Enter → `showFileUrlRequested` → FileDetailSheet; text được clear sau route thành công | UI-driver | P1 | no | — |
| HOME-SC-006 | Dán link folder → duyệt | Home | 1) Dán `.../folder/<code>` 2) Enter | `showFolderUrlRequested` → mở folder browser | UI-driver | P2 | no | — |
| HOME-SC-007 | Dán nhiều link → tải hàng loạt | Home | 1) Dán 3 URL 2) Enter | `addDownloadRequested(joined)` → dialog Download mở pre-filled 3 link | UI-driver | P1 | no | — |
| HOME-SC-008 | ↑/↓ điều hướng + Enter mở dòng chọn | overlay nhiều kết quả | 1) ↓↓ 2) Enter | highlight di chuyển; Enter mở dòng đang chọn (KHÔNG re-submit keyword); text clear | UI-driver | P2 | no | — |
| HOME-SC-009 | Infinite-scroll nạp trang kế | kết quả > 1 trang (≥30) | 1) Cuộn overlay xuống ~50% cuối | `loadMore()` nạp page 2 (append); không trùng dòng; cuộn tiếp đến hết → dừng nạp | UI-driver | P2 | no | — |
| HOME-SC-010 | Journey quick-action → quay lại Home → recent cập nhật | có lịch sử tải | 1) Click "Tải file lên" 2) Upload xong 1 file 3) Về Home | card "Tiếp tục đang tải" hiện khi đang chạy; xong → "File gần đây" có file mới đầu danh sách (newest-first), badge ↑ "Đã tải lên" | UI-driver/tay | P1 | no | — |

## Traceability
- Search journey ↔ functional HOME-FN-018..027 + overlay UI HOME-UI-003/009/010.
- Route URL ↔ HOME-FN-009..013 + HOME-UI-006/007.
- Recent/quick-action ↔ HOME-UI-011/012.
- HOME-SC-002 là kịch bản người-dùng cho race HOME-FN-023 ([[project-homesearch-clearresults-race]]).
