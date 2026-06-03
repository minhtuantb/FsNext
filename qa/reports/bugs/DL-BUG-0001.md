---
id: DL-BUG-0001
title: "Tải về" từ Yêu thích hiện toast thành công nhưng task KHÔNG xuất hiện ở Tải xuống (Đang tải + Lịch sử đều 0)
status: NEW
severity: MEDIUM
area: FavoritesViewModel / DownloadViewModel (đường "Tải về")
found_by: monkey-chaos-r1 (Chaos setup — kích download để kill giữa chừng)
found_run: 2026-06-03-monkey-chaos-r1
assignee: ""
fix_commit: ""
verified_run: ""
created: 2026-06-03
updated: 2026-06-03
---

## Mô tả (⚠️ CANDIDATE — cần xác nhận, có biến nhiễu, xem mục Lưu ý)
Ở trang **Yêu thích**, chọn file `IELTS nền tảng 5.0 phần 1 (Nghe - đọc).part1.rar` (4.00 GB) →
bấm **"Tải về"** → hiện toast `✓ Đã thêm vào tải về — IELTS…part1.rar`. NHƯNG trang **Tải xuống**:
**Đang tải = 0 file**, **Hàng đợi = 0**, **Tổng = 0**, tab **Lịch sử = 0**. Không có file ghi ra thư
mục tải, không có log dòng download. ⇒ Toast báo thành công nhưng task không thực sự được tạo/hiển thị.

## Repro
1. Yêu thích → chọn 1 file (test: rar 4GB, `secure=1`).
2. Panel "Thông tin" → bấm "Tải về".
3. Quan sát toast → sang Tải xuống (Đang tải + Lịch sử).
- **Expected:** task xuất hiện ở "Đang tải" (đang chạy hoặc trong hàng đợi nếu auto-download tắt).
- **Actual:** toast "Đã thêm vào tải về" nhưng 0 task ở cả 2 tab.
- **Bằng chứng:** runs/2026-06-03-monkey-chaos-r1/evidence/09-fav-after-taive.png (toast),
  10-downloads-recheck.png (Đang tải 0), 11-downloads-history.png (Lịch sử 0).

## Re-test đã chạy (cùng run) — 2026-06-03 16:5x
- **Bật auto-download = true qua registry + relaunch + lặp lại** → vẫn toast OK nhưng **Đang tải 0 / Hàng đợi 0 / Tổng 0**, .chaostest rỗng (evidence 12-retest-autodl-on.png).
- ⇒ **Loại trừ auto-download** là nguyên nhân. Bug tái hiện vững **3 lần**.
- CHƯA cô lập được: Favorites-only? file `secure=1`? file thuộc-sở-hữu-chính-mình? (chuyển fix-dev khoanh vùng).

## Lưu ý / biến nhiễu còn lại
1. ~~Auto-download tắt~~ — ĐÃ loại trừ ở re-test trên.
2. **File `secure=1`**: đường tải file secure có thể khác (resolve link async có thể fail thầm).
3. **Tester đã đổi `Download/folder` = D:\Work\FsNext\.chaostest qua registry** trước đó (đã khôi phục sau test).
4. **Lái mù UI** — dù toast đã xác nhận click trúng.

## Re-test đề xuất (xác nhận trước khi coi là bug thật)
- Lặp với **download-folder mặc định** + **auto-download BẬT** + file **không secure** (vd test_file_10MB.bin).
- Thử cùng thao tác từ **My Files** (không phải Favorites) để biết có Favorites-specific không.
- Theo dõi log `addDownload`/`enqueue` + transfer-history DB ngay sau khi bấm.

## Fix (process fix điền)
- Thay đổi:
- Commit:

## Nhật ký trạng thái
- 2026-06-03 16:53 — NEW/CANDIDATE (QA, run 2026-06-03-monkey-chaos-r1)
