---
id: HOME-BUG-0001
title: Overlay search — Enter mở dòng đang highlight không hoạt động (đọc sai role offset)
status: CLOSED
severity: MEDIUM
area: qml/FsAurora/Components/HomeSearchOverlay.qml (activateHighlighted)
found_by: HOME-UI-009
found_run: 2026-06-04-home-r1
assignee: "claude (QA+fix cùng phiên)"
fix_commit: ""
verified_run: 2026-06-04-home-r1
created: 2026-06-04
updated: 2026-06-04
---

## Mô tả
Trên overlay kết quả tìm kiếm Trang chủ, điều hướng bằng phím ↑/↓ rồi nhấn **Enter**
KHÔNG mở `FileDetailSheet` cho dòng đang highlight (ô search bị clear nhưng không có
sheet nào hiện). Path click CHUỘT vào dòng thì ĐÚNG (HOME-SC-001 pass) — chỉ path
bàn phím lỗi.

Nguyên nhân: `HomeSearchOverlay.qml::activateHighlighted()` hardcode **sai role offset**
khi đọc `FileListModel`:
```js
const item = ...data(..., Qt.UserRole + 2);   // chú thích "NameRole" — SAI, +2 = LinkcodeRole
const lc   = ...data(..., Qt.UserRole + 1);   // chú thích "LinkcodeRole" — SAI, +1 = IdRole
```
`FileListModel` enum: `IdRole=UserRole+1`, `LinkcodeRole=UserRole+2`, `NameRole=UserRole+3`
(xác nhận ở `FileListModel.cpp::data()`). ⇒ `lc` nhận **IdRole** (id số) thay vì linkcode
→ `fileActivated()` phát linkcode sai → `openFile` thất bại → không mở sheet (off-by-one).
Path chuột dùng `required property string linkcode/name` (Qt map theo TÊN role) nên đúng.

## Repro (các bước tái hiện)
1. Trang chủ → gõ keyword có kết quả (vd `big100` → overlay 2 mục).
2. Nhấn `↓` (Down) — dòng đầu được highlight (OK, `sc1-20-highlight.png`).
3. Nhấn `Enter`.
- **Expected:** mở `FileDetailSheet` cho dòng đang highlight (như click chuột).
- **Actual:** ô search clear về rỗng, KHÔNG có sheet nào hiện (`sc1-21-enter-open.png`).
- **Bằng chứng:** runs/2026-06-04-home-r1/evidence/sc1-20-highlight.png, sc1-21-enter-open.png

## Phân tích
`qml/FsAurora/Components/HomeSearchOverlay.qml` hàm `activateHighlighted()` (≈ dòng 57-62):
role offset lệch +1. Sửa: `lc` → `Qt.UserRole + 2` (LinkcodeRole), `item/name` → `Qt.UserRole + 3` (NameRole).

## Fix
- Thay đổi: sửa 2 offset trong `activateHighlighted()` (+1→+2 cho linkcode, +2→+3 cho name) + sửa chú thích.
- Commit: <điền khi commit>

## Verify (QA điền sau khi fix)
- ✅ Chạy lại HOME-UI-009: `↓` + `Enter` → mở ĐÚNG FileDetailSheet của dòng highlight
  (`big100(1).bin`, khác dòng click chuột `big100.bin` → chứng tỏ linkcode per-row đã đúng).
  Bằng chứng: `sc1-22-enter-fixed.png`. Không crash (CrashWatch sạch).
- Regression: click chuột (HOME-SC-001) vẫn mở đúng (`sc1-11-detail.png`).

## Nhật ký trạng thái
- 2026-06-04 10:41 — NEW (QA, run 2026-06-04-home-r1, found_by HOME-UI-009)
- 2026-06-04 10:45 — FIXING (claude)
- 2026-06-04 10:50 — FIXED (sửa role offset +1→+2 linkcode, +2→+3 name trong activateHighlighted)
- 2026-06-04 10:58 — CLOSED (rebuild FsNext + retest UI-009 PASS, run 2026-06-04-home-r1)
