# Download / Upload / Sync — BÁO CÁO BUG (QA đợt 2)

> Ngày: 2026-06-02. Phạm vi: Download (multi-segment/resume), Upload (chunked/resume),
> Sync (folder watch + scan + auto-upload).
> Phương pháp: bộ test tự động mới `tests/test_sync_scan_robustness.cpp` (11 case) +
> chạy lại 11 bộ test nền tảng liên quan (resume DB, orchestrator, budget, sanitizer,
> url, sync repo) + **đọc code trực tiếp** (đối chiếu `qa/testcases/transfer-sync/functional.md`).
> Engine download/upload nói chuyện trực tiếp với CDN/Fshare qua libcurl → các bug trên
> đường mạng được xác định bằng đọc code (file:line đã verify), không tách mạng được trong unit test.

## 1. Tổng quan thực thi test

| Bộ test | Kết quả |
|---|---|
| `test_sync_scan_robustness` (11 case mới) | **11 PASS** — xác nhận hành vi → bằng chứng cho BUG-SY-01, BUG-SY-02 |
| `test_sync_scan` (cũ, 14 case) | PASS |
| `test_transfer_history_db`, `test_history_repository` (resume snapshot) | PASS — TR-004/005 OK |
| `test_transfer_orchestrator`, `test_budget_manager`, `test_priority_scheduler` | PASS |
| `test_sync_repository`, `test_filename_sanitizer`, `test_fshare_url`, `test_speed_meter` | PASS |
| **Tổng auto** | **22/22 PASS** (0 fail) |

**Phòng thủ đã xác nhận TỐT (PASS / verify code — không phải bug):**
- **Download path-traversal**: URL filename đi qua `FileNameSanitizer::sanitize` (TransferService.cpp:1178) — strip `/` `\`,
  collapse `..`, thay ký tự cấm, escape DOS device. → KHÔNG có lỗ hổng như Vault BUG-VLT-01.
- **Download segment math**: `e = (i==N-1) ? size-1 : ((i+1)*size/N - 1)` (DownloadEngine.cpp:457-458) — không hụt byte
  ở biên; final check `fi.size()==fileSize` (DL-101) chặn file thiếu.
- **Download server bỏ qua resume/Range**: single-segment xóa partial restart 0; multi-segment set `rangeBroken` →
  xóa + fallback (DL-107/DL-008) — không nối byte sai offset.
- **Upload reject 0-byte** (TransferService.cpp:528-534) + sanitize tên + reject file không tồn tại.
- **Upload seek-guard**: `FSEEKO64` fail (file cắt cụt quá EOF) → "Failed to seek source file" fatal, không gửi byte rác
  (UploadEngine.cpp:301-304).
- **Resume snapshot (ADR D12)**: `progress_json` không bị clobber khi đổi state; `loadInFlight` khôi phục về **Paused**;
  upload giữ `realUrl`. (TR-004/005 PASS).
- **Shutdown drain**: chờ ≤2000ms/thread, không `terminate()` (tránh heap-corrupt) — TransferService dtor.
- **Sync reentrancy**: `markScanInFlight`/dirty-coalesce chặn 2 walk song song; `QPointer` guard + re-validate trên
  main thread trước khi đụng state.
- **SyncScanner**: cycle-guard `canonicalFilePath` (symlink/junction không lặp vô hạn); BFS iterative (cây sâu 60 cấp,
  1500 file đều OK); unicode VN round-trip đúng; oversized vẫn liệt kê + seen (không bị mark Missing nhầm).

---

## 2. Danh sách bug

### 🔴 BUG-SY-01 — Thư mục tên phổ thông (build/out/bin/dist/target/obj) bị bỏ qua THẦM LẶNG (HIGH, mất dữ liệu)
- **Nguồn:** test `commonDirNamesSilentlySkipped` **PASS** (chứng minh hành vi) + đọc code.
- **Vị trí:** `src/core/services/SyncScanner.cpp:59-76` — `scanShouldSkipDir` có skip-list dev-centric:
  `build, dist, target, out, bin, obj, node_modules, __pycache__, .*` …
- **Tác động:** FsNext là client **đa dụng** (phim/ảnh/tài liệu), KHÔNG phải dev-tool. Người dùng có folder hợp lệ
  tên đúng các từ này (vd `Hình ảnh/out`, `Game/bin`, `Dự án/build`) → **toàn bộ nội dung không bao giờ được sao lưu,
  KHÔNG cảnh báo**. Khác với `.*`/`$RECYCLE.BIN`/`System Volume Information` (rác thật), các tên `build/out/bin/dist/
  target/obj` là từ tiếng Anh thường gặp.
- **Repro:** tạo sync folder có `out/report.pdf`; scan → file không bao giờ lên Fshare, không lỗi.
- **Fix đề xuất:** thu hẹp default skip về rác-thật (`.*`, `$RECYCLE.BIN`, `System Volume Information`, `node_modules`,
  `__pycache__`). Đưa `build/dist/out/bin/obj/target` thành **gợi ý ignorePatterns mặc định mà người dùng SỬA/TẮT được**
  (per-folder), thay vì skip cứng. Tối thiểu: tài liệu hóa + hiện cảnh báo "đã bỏ qua N thư mục dev".

### 🔴 BUG-SY-02 — Sửa file <1 giây với CÙNG kích thước không được phát hiện (MEDIUM-HIGH, mất cập nhật)
- **Nguồn:** test `mtimeSecondPrecisionCollisionPossible` **PASS** (tạo collision (size,mtime)) + đọc code.
- **Vị trí:** `SyncScanner.cpp:149` `sf.mtime = fi.lastModified().toSecsSinceEpoch()` (độ phân giải **giây**);
  so sánh diff `prev.size == sf.size && prev.mtime == sf.mtime` tại `SyncService.cpp:636-637`.
- **Tác động:** file bị ghi đè nội dung khác nhưng **cùng size, trong cùng giây đồng hồ** → chữ ký trùng → diff coi
  "Synced không đổi" → bản mới KHÔNG được upload. Hay gặp với file nhỏ ghi nhanh (log, config, .json cùng độ dài).
- **Fix đề xuất:** dùng `toMSecsSinceEpoch()` cho cả scan + lưu repo (NTFS 100ns → ms đủ phân biệt). Lưu ý migration:
  giá trị cũ lưu theo giây sẽ khác ms → một lần re-upload. Giảm thiểu bằng so sánh "khác >1s" hoặc lưu thêm cờ
  version. Bền vững nhất: khi `size` bằng nhau, so thêm hash nội dung (chỉ tính khi nghi ngờ) — chi phí I/O cao hơn.

### 🔴 BUG-SY-03 — Lỗi tạo subfolder bị nuốt → cache "đã tạo" sai → upload kẹt Failed (MEDIUM, robustness)
- **Nguồn:** đọc code.
- **Vị trí:** `SyncService.cpp:744-768` (`ensureSubdirsThenEnqueue`, worker). Khi `api->createFolderInPath` trả lỗi
  (network/transient), code chỉ `qDebug() << "(treating as created)"` rồi **luôn `createdOk.insert(rel)`** (dòng 768,
  và `rootCreated=true` dòng 750 bất kể lỗi). Lên main thread, các rel này được nạp vào cache `m_createdSubdirs`.
- **Tác động:** thư mục thực ra CHƯA tạo trên Fshare bị đánh dấu "đã tạo". Upload sau trỏ vào path không tồn tại → fail;
  lần scan kế **cache hit** nên KHÔNG thử tạo lại → file kẹt Failed cho tới khi reset cache. Tự gây lỗi dây chuyền.
- **Fix đề xuất:** chỉ `createdOk.insert(rel)` / `cache.insert` khi `!res.isError()` **hoặc** lỗi là "already exists"
  (message chứa "exist", case-insensitive — server trả "Folder name already exists", xem FshareApi.cpp:631). Lỗi thật →
  KHÔNG cache (để scan sau tạo lại) + mark các file thuộc subdir đó `Failed/Pending` để retry.

### 🟠 BUG-SY-04 — File kẹt state "Uploading" vĩnh viễn nếu task chết bất thường (MEDIUM, không retry)
- **Nguồn:** đọc code.
- **Vị trí:** `SyncService.cpp:640-641` — `applyScanResult` `continue` khi `prev.state==Uploading` (chống enqueue chồng).
  State chỉ thoát Uploading qua `onUploadFinished` (SyncService.cpp:953-961).
- **Tác động:** nếu app **crash/kill** giữa upload (hoặc task bị mất không phát `syncUploadFinished`), entry kẹt
  `Uploading` trong repo. Lần khởi động sau không có task tương ứng → mọi scan luôn `continue` → file **không bao giờ
  được thử lại**, dù vẫn còn trên đĩa và chưa lên Fshare.
- **Fix đề xuất:** lúc `setUserId`/khởi động, reconcile: mọi `SyncFileEntry` state `Uploading` mà KHÔNG có task live
  tương ứng → hạ về `Pending` (hoặc Failed) để scan kế re-enqueue. (Tương tự resume-in-flight của TransferService.)

### 🟠 BUG-UL-01 — File bị thu nhỏ giữa upload có thể gửi chunk thiếu byte (MEDIUM, corrupt)
- **Nguồn:** đọc code.
- **Vị trí:** `UploadEngine.cpp:283-321`. `fileSize` được snapshot lúc tạo task; mỗi chunk khai
  `Content-Range: bytes from-to/total` + `CURLOPT_POSTFIELDSIZE_LARGE=chunkLen`. Seek-guard (dòng 301) chỉ bắt khi
  `fromTarget` **vượt EOF**. Nếu file co lại NHẸ (offset vẫn hợp lệ nhưng đuôi mất), `chunkReadCallback` đọc được
  `< chunkLen` byte → curl gửi thiếu so với Content-Range đã khai → server ghép sai/treo chunk.
- **Tác động:** xác suất thấp (cần sửa file đúng lúc upload) nhưng có thể tạo file hỏng trên server mà không báo rõ.
- **Fix đề xuất:** trước vòng chunk (hoặc mỗi chunk), `fstat`/`QFileInfo` lại size; nếu khác `task.fileSize` snapshot →
  abort với lỗi rõ ("Tập tin nguồn đã thay đổi khi đang tải lên"). Rẻ, an toàn.

### 🟡 BUG-SY-05 — File 0 byte không bao giờ được đồng bộ (LOW, giới hạn đã biết)
- **Nguồn:** test `seenRelExcludesSkippedAndZero` + `skipsZeroByteFiles` PASS.
- **Vị trí:** `SyncScanner.cpp:133` bỏ mọi `fi.size()==0`.
- **Đánh giá:** **KHÔNG hẳn bug** — Fshare API từ chối upload file rỗng (TransferService.cpp:528), nên dù không skip
  cũng không lên được. Tuy vậy hành vi hiện tại là **im lặng** (file rỗng như `.gitkeep`, placeholder biến mất khỏi
  "bản sao lưu" không dấu vết).
- **Fix đề xuất (tùy chọn):** tài liệu hóa giới hạn; hoặc hiện chỉ báo "đã bỏ qua N file rỗng" trong activity feed.

### 🟡 BUG-SY-06 — Activity log có thể lệch cột nếu dữ liệu chứa ký tự phân tách (LOW)
- **Nguồn:** đọc code.
- **Vị trí:** `SyncRepository.cpp` (activity encode tab-separated + `\x1f` row sep). relPath/errorMessage chứa tab hoặc
  `\x1f` (hiếm nhưng filename hợp lệ trên POSIX có thể chứa tab) → decode lệch cột.
- **Fix đề xuất:** escape/encode field (vd percent-encode hoặc base64 từng field), hoặc lưu JSON thay vì tách ký tự.

---

## 3. Mức độ & thứ tự fix (đề xuất)

| # | Bug | Mức | Loại | Rủi ro fix | Verify được bằng test? |
|---|---|---|---|---|---|
| 1 | BUG-SY-03 cache poisoning | 🔴 MEDIUM | robustness | Thấp (localized) | Manual (network) |
| 2 | BUG-SY-04 stale Uploading | 🟠 MEDIUM | không retry | Thấp-TB (reconcile startup) | Integration (cần VM/service) |
| 3 | BUG-UL-01 re-stat size | 🟠 MEDIUM | corrupt | Thấp (thêm guard) | Manual (network) |
| 4 | BUG-SY-01 skip-list | 🔴 HIGH | mất dữ liệu | TB (đổi hành vi — product decision) | **Auto** (cập nhật test robustness) |
| 5 | BUG-SY-02 mtime ms | 🔴 MED-HIGH | mất cập nhật | TB-Cao (migration re-upload) | **Auto** (test scanner ms) |
| 6 | BUG-SY-05 / BUG-SY-06 | 🟡 LOW | giới hạn | Thấp | Auto/Manual |

**Lưu ý quan trọng:** #4 (skip-list) và #5 (mtime precision) là **quyết định sản phẩm + có hệ quả migration**
(đổi hành vi mặc định / re-upload hàng loạt). Không nên tự ý áp dụng — cần maintainer chốt scope. #1, #2, #3 là fix
phòng thủ rủi ro thấp nhưng **chỉ verify được trên đường mạng thật** (không có trong CI hiện tại).

Mỗi bug nên kèm regression test khi fix (auto cho SY-01/SY-02/SY-05/SY-06; integration cho SY-04; manual checklist cho
UL-01/SY-03 + các case Manual P0 trong test-cases doc).

---

## 4. ĐÃ FIX & VERIFY (phiên 2026-06-02)

Theo quyết định maintainer: áp dụng **nhóm phòng thủ rủi ro thấp** (không đổi hành vi mặc định, không migration).
SY-01 (skip-list) và SY-02 (mtime ms) **HOÃN** — cần chốt scope sản phẩm + cân nhắc re-upload hàng loạt.

| Bug | Fix | Vị trí | Verify |
|---|---|---|---|
| **BUG-SY-03** | `folderNowExists()` — chỉ cache subdir là "đã tạo" khi `!isError()` hoặc lỗi "already exists"; lỗi thật → KHÔNG cache + log warning → scan sau tạo lại | `SyncService.cpp` `ensureSubdirsThenEnqueue` | Compile OK trong app; logic localized; còn cần test mạng thủ công |
| **BUG-SY-04** | `setUserId`: mọi `SyncFileEntry` state `Uploading` (stale từ phiên trước) → hạ `Pending` + lưu repo → scan kế re-enqueue | `SyncService.cpp` `setUserId` | Compile OK; `SyncRepository`/`SyncScan` regression PASS |
| **BUG-UL-01** | Re-stat `QFileInfo(task.sourcePath).size()` trước mỗi chunk; khác snapshot `fileSize` → abort sạch (đóng file + curl cleanup) với lỗi rõ | `UploadEngine.cpp` vòng chunk | Compile OK; `UploadViewModel`/`UploadStagingViewModel` PASS |

**Kết quả verify build + test:**
- Build app `FsNext.exe` + `test_upload_viewmodel` link thành công (3 fix biên dịch sạch).
- Regression: **15/15 PASS** — SyncScan, SyncScanRobustness, SyncRepository, TransferHistoryDb, HistoryRepository,
  BudgetManager, PriorityScheduler, TransferOrchestrator, DownloadViewModel, UploadViewModel, UploadStagingViewModel,
  TransferTask, FileNameSanitizer, FshareUrl, SpeedMeter. 0 fail.

## 5. QUYẾT ĐỊNH MAINTAINER (2026-06-02)

- **BUG-SY-02 (mtime giây) → WONTFIX (by design).** Giữ độ phân giải **giây**, KHÔNG đổi sang ms (tránh re-upload
  hàng loạt + migration). Chấp nhận giới hạn: sửa file <1s cùng size hiếm gặp.
- **BUG-SY-05 (file 0 byte) → WORKING AS INTENDED.** File 0 byte **không upload** (Fshare API từ chối file rỗng).
  Hành vi đúng, không cần fix.
- **Folder mất tích → đã đúng, không cần sửa.** Sync folder do user chọn được lưu (`SyncRepository`) cho lần sau;
  khi không tìm thấy (`rootExists==false`, SyncService.cpp:616-619) → giữ nguyên record + file entries, chỉ
  `folderMissing` (không tự xóa, không lỗi cứng). Folder xuất hiện lại → sync tiếp. `deleteFolder` chỉ chạy khi user
  chủ động xóa (SyncService.cpp:272-293).

- **BUG-SY-01 (skip-list cho thư mục con) → WONTFIX.** Giữ skip-list dev-centric (build/out/bin/dist/target/obj/
  node_modules…) cho thư mục con. Root do user chọn KHÔNG bị skip; chỉ subfolder trùng tên mới bị bỏ — chấp nhận
  như lọc rác. Không đổi code.

**Còn lại (LOW):**
- BUG-SY-06 (LOW): escape activity field (tab/`\x1f`) — chưa chốt.
- Toàn bộ case **Manual P0** trong `transfer-sync-test-cases.md` (kill-app resume thật, đĩa đầy, mất mạng, file lớn)
  cần QA thủ công với tài khoản Fshare thật — chưa tự động hóa được (engine dùng libcurl trực tiếp).
