# Vault — UI test cases

> Loại: **UI** (i18n / dịch / runtime switch; render dialog Vault). Tổng: **5 case**.
> Cờ ⚠️ ở Tiêu đề = case rủi ro cao.
>
> Ghi chú: Bộ nguồn (`vault-test-cases.md`) chưa tách case riêng cho render dialog
> (DLG-LARGEFILE: 3 nút + checkbox apply-all; DLG-BATCH: progress/nút) — logic của
> các dialog này nằm ở `functional.md` (size-cap) và `scenario.md` (batch flow). Khi
> bổ sung case render thuần (layout/nút/progress), thêm dưới đây với tiền tố VLT-UI-###.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-UI-001 | tr() C++ trong VaultViewModel | — | 1) Grep chuỗi hiển thị trong `VaultViewModel.cpp` (vd "Khóa khôi phục", "Vault") | Mọi chuỗi hiển thị bọc `tr()`; không hardcode trần | tay | P1 | no | — |
| VLT-UI-002 | qsTr() QML Vault | — | 1) Grep `qml/Fshare/Pages/Vault*.qml` + Components mới | Mọi label/nút/toast/lỗi bọc `qsTr()` | tay | P1 | no | — |
| VLT-UI-003 ⚠️ | EN không còn unfinished | `src/i18n/fshare_en.ts` | 1) Mở .ts 2) lọc `type="unfinished"` trên chuỗi Vault | 0 entry unfinished cho chuỗi Vault (progress: 0 unfinished/1060) | tay | P1 | no | — |
| VLT-UI-004 | Runtime switch sang EN | App chạy | 1) Đổi ngôn ngữ 2) vào trang Vault | Toàn bộ UI Vault dịch EN; không còn chuỗi VI sót | UI-driver | P2 | no | — |
| VLT-UI-005 | lupdate sạch sau sửa chuỗi | Sau thêm chuỗi mới | 1) `update_translations` 2) lrelease | Không chuỗi mới bị bỏ; build chèn .qm; verify runtime | tay | P2 | no | — |

## Traceability (ID mới ↔ ID nguồn)

| ID mới | Nguồn |
|---|---|
| VLT-UI-001 | VLT-1001 |
| VLT-UI-002 | VLT-1002 |
| VLT-UI-003 | VLT-1003 |
| VLT-UI-004 | VLT-1004 |
| VLT-UI-005 | VLT-1005 |
