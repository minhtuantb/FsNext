# Vault — Scenario test cases

> Loại: **scenario** (luồng nhiều bước end-to-end: batch config→progress→done, auto-upload sau encrypt). Tổng: **12 case**.
> Cờ ⚠️ ở Tiêu đề = case rủi ro cao (security/fail-closed); 🔴 = lỗ hổng nghi ngờ cao.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-SC-001 | addFolder đệ quy nhiều cấp | VM unlocked + uploader wired; thư mục có file nhiều cấp con | 1) `addFolder(dir)` | Mọi file (Subdirectories) được encrypt; mỗi → `<basename>.fshenc` trong vault | UI-driver | P0 | no | — |
| VLT-SC-002 | batchProgress tăng dần | Batch ≥3 file | 1) `addFilesUpload(...)` | `batchProgress(done,total,name)` emit tăng dần tới total; thứ tự done=1..N | UI-driver | P1 | no | — |
| VLT-SC-003 | fileOpFinished tổng kết có lỗi | Batch 5 file, 1 file input không tồn tại | 1) Chạy batch | `fileOpFinished("encrypt", false, "4/5", firstErr)`; 4 file ok hợp lệ | UI-driver | P1 | no | — |
| VLT-SC-004 ⚠️ | retryFailed chỉ chạy file lỗi | Batch xong còn `lastFailed_` | 1) `retryFailed()` | Chỉ chạy lại file lỗi; nếu nguồn có lại → ok; `lastFailed_` clear trước khi chạy | UI-driver | P1 | no | — |
| VLT-SC-005 ⚠️ | cancelBatch giữa chừng | Batch nhiều file đang chạy | 1) `cancelBatch()` | Dừng giữa các file (atomic flag); file còn lại vào `failedInputs` (retry được); file xong hợp lệ | UI-driver | P0 | no | — |
| VLT-SC-006 ⚠️🔴 | Trùng tên basename flat ghi đè | 2 file cùng tên `a.txt` ở 2 thư mục con | 1) `addFolder` | GAP nghi ngờ: cả hai → `a.txt.fshenc` cùng vault → file 2 GHI ĐÈ file 1 (flat theo basename, không cảnh báo). Kiểm mất dữ liệu im lặng | UI-driver | P0 | no | VLT-BUG-0004 |
| VLT-SC-007 | Batch khi auto-lock giữa lô | Auto-lock kích hoạt giữa batch | 1) Chạy batch tới khi auto-lock | Worker dùng `snapshotKey` (bản sao master key độc lập) → batch đang chạy vẫn hoàn tất dù vault auto-lock | UI-driver | P1 | no | — |
| VLT-SC-008 | Empty folder | Thư mục rỗng | 1) `addFolder(dir)` | Không làm gì (files rỗng → return); không emit lỗi | UI-driver | P2 | no | — |
| VLT-SC-009 | Reentrancy guard batch | Batch đang chạy (`fileBusy_`) | 1) Gọi `addFiles` lần 2 | Bị chặn (`beginEncrypt` return khi `fileBusy_`); không chạy chồng | UI-driver | P1 | no | — |
| VLT-SC-010 | cancel rồi retry | Cancel batch → còn failed | 1) `cancelBatch()` 2) `retryFailed()` | File bị hủy chạy lại được; flag cancel reset (`store(false)` đầu runEncryptBatch) | UI-driver | P1 | no | — |
| VLT-SC-011 | Auto-upload sau encrypt | autoUpload=true, folder="/"; VM unlocked; uploader lambda | 1) `addFilesUpload([f], true, "/")` | Sau encrypt OK → `uploader_({out}, "/")` được gọi; emit `fileUploadQueued(name)` | UI-driver | P0 | no | — |
| VLT-SC-012 | Auto-upload chỉ file ok | Batch 1 ok + 1 lỗi, autoUpload | 1) Chạy batch | Chỉ file ok (`okOutputs`) được upload; file lỗi không | UI-driver | P1 | no | — |

## Traceability (ID mới ↔ ID nguồn)

| ID mới | Nguồn | ID mới | Nguồn |
|---|---|---|---|
| VLT-SC-001 | VLT-701 | VLT-SC-007 | VLT-707 |
| VLT-SC-002 | VLT-702 | VLT-SC-008 | VLT-708 |
| VLT-SC-003 | VLT-703 | VLT-SC-009 | VLT-709 |
| VLT-SC-004 | VLT-704 | VLT-SC-010 | VLT-710 |
| VLT-SC-005 | VLT-705 | VLT-SC-011 | VLT-801 |
| VLT-SC-006 | VLT-706 | VLT-SC-012 | VLT-805 |
