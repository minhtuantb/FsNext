---
id: DL-BUG-0001
title: "Tải về" từ Yêu thích hiện toast thành công nhưng task KHÔNG xuất hiện ở Tải xuống (Đang tải + Lịch sử đều 0)
status: FIXED
severity: MEDIUM
area: FavoritesViewModel / DownloadViewModel (đường "Tải về")
found_by: monkey-chaos-r1 (Chaos setup — kích download để kill giữa chừng)
found_run: 2026-06-03-monkey-chaos-r1
assignee: "Claude (QA-dev)"
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

## Phân tích nguyên nhân gốc (root-cause class)
Triệu chứng "toast báo thành công nhưng task không được tạo" sinh ra từ việc **phản hồi thành công bị tách rời
hoàn toàn khỏi kết quả enqueue thực**:

- `FavoritesPage._downloadItem()` (và biến thể multi-select, và `FileManagerPage`) gọi
  `downloadViewModel.addDownload(...)` rồi **luôn** hiện toast `"Đã thêm vào tải về"` — bất kể task có được tạo hay không.
- `DownloadViewModel::addDownload()` (trả `void`) có **3 đường return sớm KHÔNG tạo task**:
  1. URL không hợp lệ → `emit downloadBlocked` (chỉ DownloadPage lắng nghe).
  2. Thư mục hệ thống (`isSystemFolder`) → `emit downloadBlocked` (chỉ DownloadPage lắng nghe).
  3. **Disk quá đầy** (`TransferService::addDownload`, ngưỡng 50 MB): return im lặng — comment ghi "user sees a toast"
     nhưng **không hề `emit` signal nào** (xác nhận bằng grep). Đây là đường im lặng tuyệt đối, không hiển thị lý do
     trên BẤT KỲ trang nào.
- FavoritesPage/FileManagerPage **không** kết nối `downloadViewModel.downloadBlocked` → ngay cả khi VM phát lý do,
  người dùng Favorites/MyFiles cũng chỉ thấy mỗi toast thành công giả.

⇒ Bất kể đường từ chối nào kích hoạt, người dùng đều thấy "Đã thêm vào tải về" trong khi 0 task được tạo (khớp y
hệt triệu chứng: 0 ở Đang tải/Hàng đợi/Tổng/Lịch sử + không có dòng log download). DownloadPage KHÔNG dính bug vì
nó hiển thị danh sách task tại chỗ + đã có handler `onDownloadBlocked`.

**Lưu ý về biến nhiễu của reporter:** không tái hiện được trigger môi trường chính xác bằng phân tích tĩnh (cần
login Fshare thật + favorite `secure=1` + lượt tải thật; `.chaostest` không phải thư mục hệ thống và đĩa dev khó
<50 MB). Tuy nhiên đây đúng là **lớp nguyên nhân** tạo ra triệu chứng. Fix loại bỏ triệu chứng về mặt cấu trúc:
toast thành công CHỈ phát khi một task thực sự được enqueue; mọi lý do từ chối nay đều nổi lên thành toast lỗi thật.

## Fix
**Lõi (làm phản hồi trung thực cho mọi entry point "Tải về"):**
- `TransferService::addDownload()`: `void → bool` (true = đã enqueue, false = bị từ chối). Đường disk-full nay
  `emit downloadRejected(reason)` + `return false` (trước đây im lặng).
- Thêm signal `TransferService::downloadRejected(QString)`.
- `DownloadViewModel::addDownload()`: `void → int` (số task thực sự enqueue). Forward `downloadRejected →
  downloadBlocked` trong ctor. System-folder/invalid trả `0`.

**QML (chỉ confirm khi enqueue thật + nổi lý do từ chối):**
- `FavoritesPage.qml`: `_downloadItem` + multi-select chỉ `showToast(success)` khi `addDownload(...) > 0`; thêm
  `Connections { target: downloadViewModel; onDownloadBlocked }` → toast lỗi.
- `FileManagerPage.qml`: tương tự (`downloadEnqueued` chỉ phát khi `> 0`) + handler `onDownloadBlocked`.
- `DownloadPage.qml`: đổi title toast block "…thư mục hệ thống" → "Không thể tải về" (generic, nay cũng cover disk-full).

**Lan tỏa (cùng bug-class, cpp-qt-reviewer phát hiện):**
- `RemoteShareViewModel`: 3 call site (`downloadCurrentFile`/`downloadFolderItem`/`downloadSelected`) nay
  gate toast thành công theo return value của `addDownload` (đếm `queued += addDownload(...)`).

**i18n:** thêm bản EN cho 3 chuỗi mới ("Không thể tải về" = "Cannot download"; disk-full = "Not enough free disk
space at \"%1\". Please choose a different destination folder.") — lupdate/lrelease: 1063 finished, 0 unfinished.

- Files: `src/core/services/TransferService.{h,cpp}`, `src/viewmodels/DownloadViewModel.{h,cpp}`,
  `src/viewmodels/RemoteShareViewModel.cpp`, `qml/Fshare/Pages/{FavoritesPage,FileManagerPage,DownloadPage}.qml`,
  `src/i18n/fshare_en.ts`.
- Commit: (điền SHA sau khi commit)

## Verify (2026-06-03)
- ✅ **Build Release** (scripts/build.bat) PASS — exe relink 17:32:53 (xác nhận C++ mới compile + link).
- ✅ **qmllint** FavoritesPage + FileManagerPage EXIT=0 — không phát sinh lỗi mới (chỉ warning unqualified-access
  có sẵn toàn file, là context property).
- ✅ **i18n** lrelease: 1063 finished, **0 unfinished**.
- ✅ **Smoke launch**: app khởi động sạch, không lỗi QML runtime (libsodium/AppContext/QML context OK).
- ✅ **cpp-qt-reviewer**: PASS các bug-class (lambda/QPointer, double-emit loại trừ nhau, signal/slot type-safe,
  MVVM layering); phát hiện thêm P1 RemoteShareViewModel (đã fix) + 2×P2 (đã xử lý / ghi chú).
- ⚠️ **Chưa verify live đúng repro gốc** (cần tài khoản Fshare thật + favorite secure=1 + lượt tải thật). Khuyến
  nghị QA re-test in-field: bấm "Tải về" file thường (default folder) → phải thấy task ở "Đang tải"; ép disk đầy /
  thư mục hệ thống → nay phải thấy toast LỖI thật, không còn toast "Đã thêm vào tải về" giả.

## Ghi chú phạm vi (out-of-scope, không sửa lệch)
- `DownloadViewModel.cpp:232` — chuỗi system-folder hiện là tiếng Anh ("Cannot download to system folder…") trong
  khi quy ước repo là VI-nguồn. Là inconsistency **CÓ SẴN từ trước**, không do fix này tạo → để lại, đề xuất cleanup riêng.
- RemoteShare sheet chưa có handler `onDownloadBlocked` riêng → khi bị từ chối chỉ KHÔNG hiện toast giả (không còn
  sai), lý do nổi qua toast của DownloadPage nếu hiển thị. Edge nhỏ, ngoài trọng tâm bug Favorites.

## Nhật ký trạng thái
- 2026-06-03 16:53 — NEW/CANDIDATE (QA, run 2026-06-03-monkey-chaos-r1)
- 2026-06-03 — TRIAGED→FIXING (Claude, QA-dev): claim bug, bắt đầu khoanh vùng đường "Tải về" từ Favorites
- 2026-06-03 — FIXING→FIXED (Claude): xác định root-cause class (toast tách rời kết quả enqueue + 3 đường từ chối
  im lặng, đặc biệt disk-full không phát signal); fix lõi + QML + lan tỏa RemoteShareVM; build/lint/i18n/smoke PASS.
  Triệu chứng bị loại bỏ về cấu trúc. Chờ QA re-test in-field (tài khoản thật) để CLOSED.
