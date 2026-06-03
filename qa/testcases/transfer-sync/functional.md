# BỘ TEST CASE TOÀN DIỆN — Download / Upload / Sync (FsNext)

| Trường | Giá trị |
|---|---|
| Phạm vi | **Download** (multi-segment, resume), **Upload** (chunked, resume), **Sync** (folder watch + scan + auto-upload) |
| Module dưới test | `src/core/transfer/{DownloadEngine,UploadEngine,TransferWorker,TransferQueue,TransferOrchestrator,BudgetManager,PriorityScheduler,SpeedMeter}`, `src/core/services/{TransferService,SyncService,SyncScanner,FolderExpander,BatchFileResolver}`, `src/core/repositories/{HistoryRepository,TransferHistoryDb,SyncRepository}`, `src/viewmodels/{DownloadViewModel,UploadViewModel,UploadStagingViewModel,SyncViewModel}`, `src/core/util/{FileNameSanitizer,FshareUrl}` |
| Spec nguồn | `docs/architecture/overview.md`, ADR D9/D11/D12 (resume snapshot), M18 (sync async scan), code đọc trực tiếp |
| Loại test | **Unit** (QtTest, pure logic / filesystem), **Integration** (VM + service, không mạng), **Manual** (cần Fshare server thật / kill-app / đĩa đầy / mất mạng) |
| Ưu tiên | **P0** chặn release (mất dữ liệu / corrupt / crash / rò rỉ), **P1** quan trọng, **P2** nên có |
| Cập nhật | 2026-06-02 |

> **Cách đọc:** Cờ ⚠️ = case nghi ngờ dễ lộ bug (đã phân tích code, xem mục cuối "Khu vực rủi ro cao").
> 🔴 = nghi ngờ bug nghiêm trọng. Nhiều case download/upload là **Manual** vì engine dùng libcurl nói chuyện
> trực tiếp với CDN/Fshare (không tách mạng được trong unit test) — phần tự động hóa tập trung vào
> SyncScanner (filesystem thuần), parser, sanitizer, và persistence/resume (SQLite in-memory).
>
> Hằng số tham chiếu (đọc từ code):
> - Download: segment mặc định 16 (clamp [1,32]); `MIN_BYTES_PER_SEGMENT`=2 MB; `MIN_SEGMENTED_BYTES`=2 MB;
>   single-retry 5; segment-retry 5; backoff 1/2/4/8/16s; sidecar `.fsdownload` flush mỗi 1s; headroom đĩa 16 MiB.
> - Upload: chunk mặc định 20 MB; `MIN_UPLOAD_CHUNK`=5 MB; max 1 GB; chunk-retry 5; low-speed <1 KB/s trong 60s;
>   resume kiểu GCS (PUT `Content-Range: bytes */size` → 308 + `Range:` header).
> - Sync: tối đa 5 folder; rescan timer 5 phút (300000 ms); `mtime` = **giây** (`toSecsSinceEpoch`); bỏ file 0 byte;
>   oversize > 1 GB → Failed; activity log FIFO 50; speed-limit mặc định 5 MB/s.
> - Snapshot tiến độ (ADR D12): mỗi 5s + mỗi lần đổi state; resume khôi phục về **Paused** (không tự chạy).

---

# PHẦN A — DOWNLOAD

## NHÓM D1 — Luồng download chính & multi-segment

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| DL-001 | Single-segment | Link file < 2 MB | `addDownload(url,"",dir)` | File tải đủ; `fi.size()==fileSize`; state `Complete`; không còn `.fsdownload` | Manual | P0 |
| DL-002 | Multi-segment | File > 2 MB, CDN hỗ trợ Range | Download với segments=16 | File đúng size byte-for-byte; sidecar bị xóa khi xong; mọi segment ghi đúng offset | Manual | P0 |
| DL-003 ⚠️ | Segment boundary | File chia hết cho N segment (vd 16 MB / 16) | Download | Segment cuối kết thúc đúng `fileSize-1`; không thiếu/thừa byte (off-by-one ở `(i+1)*size/N - 1`) | Manual | P0 |
| DL-004 ⚠️ | Segment boundary lẻ | File size nguyên tố (vd 9999983 B) / 16 segment | Download | Tổng các segment = đúng fileSize; segment cuối bù phần dư; không hụt byte | Manual | P0 |
| DL-005 | Adaptive clamp | File 5 MB + segments=16 | Download | Bị clamp xuống ~2 segment (`size/req < MIN_BYTES_PER_SEGMENT`); không tạo segment < 2 MB | Manual | P1 |
| DL-006 | Segments=1 | settings segment=1 | Download file lớn | Đi nhánh single-segment; tải đủ | Manual | P1 |
| DL-007 | Pre-allocate | Multi-segment fresh start | Bắt đầu | File được `resize(fileSize)` full size trước khi ghi; journal `.fsdownload` tạo mới | Manual | P1 |
| DL-008 ⚠️ | CDN không hỗ trợ Range | Server trả HTTP 200 cho RANGE (không 206) | Multi-segment | Phát hiện `rangeBroken` → xóa file+journal → fallback single-segment; không corrupt | Manual | P0 |
| DL-009 | HTTP/2 multiplex | CDN hỗ trợ h2 | Multi-segment | Dùng 1 connection multiplex; ghi đồng thời qua `m_fileWriteMutex` không đua | Manual | P1 |
| DL-010 | Folder download | Link folder Fshare | `addFolderDownload(url,"",dir)` | `FolderExpander` crawl đệ quy; mỗi file → 1 task `addDownload`; tiến độ scan emit | Manual | P1 |
| DL-011 ⚠️ | FolderExpander maxDepth | Folder lồng > 20 cấp | Crawl | Dừng ở `maxDepth=20`; không đệ quy vô hạn; báo cáo số file tìm thấy | Manual | P1 |

## NHÓM D2 — Validate dữ liệu / lỗi server

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| DL-101 ⚠️ | Final size check | Download xong | So `fi.size()` với `fileSize` | Lệch → `emit failed` (truncated/corrupt); KHÔNG báo Complete file thiếu | Manual | P0 |
| DL-102 | Content-Length probe | HEAD request | `probeFileInfo` | Đọc `fileSize` từ Content-Length; `rangeSupported` từ `Accept-Ranges` | Manual | P1 |
| DL-103 ⚠️ | Probe size = 0 | Server không trả Content-Length | Probe | `fileSize=0` → đi single-segment; xử lý an toàn (không chia 0) | Manual | P0 |
| DL-104 | HTTP 404 | Link bị xóa | Download | `failed("404")`; phân loại **permanent** (isTransientDownloadError=false) → KHÔNG retry | Manual | P0 |
| DL-105 | HTTP 401 | Token hết hạn / chưa login | Download | `failed("401")`; permanent → cần re-login; không retry vô ích | Manual | P0 |
| DL-106 | HTTP 403 / 416 | Link hết hạn token / range sai | Download | Permanent error; dừng sạch; partial giữ hay xóa đúng quy ước | Manual | P1 |
| DL-107 ⚠️ | Server bỏ qua resume | Có partial, server trả HTTP 200 (không 206) khi `resumeFrom>0` | Resume single-segment | Xóa partial, restart từ 0 (line ~879); không nối nhầm vào partial cũ → corrupt | Manual | P0 |
| DL-108 | URL canonicalize | URL có `?token=...` | `addDownload` | Giữ nguyên query token; không cắt mất | Unit | P1 |
| DL-109 | FshareUrl parse | URL file / folder / rác | `FshareUrl::parse` | Phân loại đúng Kind; linkcode trích đúng; URL rác → không crash | Unit | P1 |
| DL-110 | Low-speed stall | Tốc độ < 1 KB/s kéo dài 60s | Download | curl ngắt (LOW_SPEED) → vào retry/backoff; không treo vô hạn | Manual | P1 |

## NHÓM D3 — Edge cases & filesystem

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| DL-201 ⚠️ | File 0 byte | Link file rỗng (size=0) | Download | `resize(0)` ok / single-segment ghi 0 byte; `fi.size()==0==fileSize` → Complete; KHÔNG kẹt | Manual | P1 |
| DL-202 | File rất lớn (>10 GB) | Đủ dung lượng | Download | offset dùng `int64`/`curl_off_t`; không tràn 32-bit; tải đủ | Manual | P1 |
| DL-203 | Đĩa đầy preflight | Trống < `needed + 16 MiB` | Bắt đầu | `failed` ngay (fatal, không retry); không tạo file dở | Manual | P0 |
| DL-204 ⚠️ | Đĩa đầy giữa chừng | Hết chỗ khi đang ghi | Download | `fwrite` fail → `failed`; partial giữ để resume hoặc xóa; không crash | Manual | P0 |
| DL-205 | Đường dẫn không ghi được | Thư mục read-only | Bắt đầu | `Không thể tạo/mở file đích`; permanent error | Manual | P1 |
| DL-206 🔴 | Path traversal filename | URL filename = `..\..\evil.exe` | Dispatch | `FileNameSanitizer::sanitize` strip `\`,`/`, collapse `..` → basename an toàn (đã verify code dòng 1178) | Unit | P0 |
| DL-207 | Ký tự cấm Windows | filename chứa `: * ? " < > \|` | Dispatch | sanitize thay `_`; tên hợp lệ; tải được | Unit | P1 |
| DL-208 | Tên DOS reserved | filename `CON.txt`, `COM1` | Dispatch | sanitize prepend `_`; không đụng device | Unit | P1 |
| DL-209 | Unicode VN | filename "phim hành động.mp4" | Download | `openFileUnicode`/`_wfopen` ghi đúng tên UTF-16; mở được | Manual | P1 |
| DL-210 | Trùng tên đích | File cùng tên đã tồn tại | Download | `uniqueDestinationPath` thêm ` (1)`,` (2)`…; KHÔNG ghi đè (trừ partial resume hợp lệ) | Unit | P0 |
| DL-211 ⚠️ | Trùng tên vs partial resume | Có partial cùng tên size < fileSize | Download | Engine nhận diện partial (`size<fileSize`) để resume, KHÔNG bị `uniqueDestinationPath` đổi thành tên mới mất resume | Manual | P1 |
| DL-212 | filename rỗng từ URL | URL path kết thúc `/` | Dispatch | Fallback `task.fileName` → "download"; không tạo tên rỗng | Unit | P1 |
| DL-213 | uniqueDestinationPath cap | > 9999 file trùng | Dispatch | Cap 10000 vòng → fallback; không loop vô hạn | Unit | P2 |

## NHÓM D4 — Pause / Resume / Cancel / Kill-app (DOWNLOAD)

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| DL-301 | Pause | Đang download | `pauseTask(id)` | curl callback abort qua `m_paused`; partial + sidecar GIỮ; state Paused | Manual | P0 |
| DL-302 | Resume sau pause | Đã pause | `resumeTask(id)` | Single: `CURLOPT_RESUME_FROM_LARGE`; multi: đọc journal offset; tải tiếp không tải lại từ 0 | Manual | P0 |
| DL-303 ⚠️ | Cancel | Đang download | `cancelTask(id)` | `m_abort`; xóa `localPath` + `.fsdownload`; slot orchestrator được release | Manual | P0 |
| DL-304 ⚠️🔴 | Kill app giữa chừng (single) | Đang ghi single-segment | Tắt cứng app (End process) | Lần mở lại: task khôi phục về **Paused**; resume dùng partial size làm offset; KHÔNG mất/đúp byte | Manual | P0 |
| DL-305 ⚠️🔴 | Kill app giữa chừng (multi) | Đang ghi multi-segment | Tắt cứng app | Journal `.fsdownload` (flush 1s) + file full-size còn → resume từng segment theo `cur` offset; mất tối đa ~1s tiến độ; file không corrupt | Manual | P0 |
| DL-306 ⚠️ | Kill ngay sau preallocate | Multi-segment vừa `resize(fileSize)`, chưa ghi byte | Kill | Mở lại: file full-size 0-data + journal "cur==start" → resume từ đầu mỗi segment; không nhầm là "đã xong" | Manual | P1 |
| DL-307 ⚠️ | Snapshot tiến độ DB | Đang download | Quan sát DB | `persistProgressSnapshots` ghi `bytes/total/retry` mỗi 5s + mỗi đổi state; `loadInFlight` đọc lại được | Unit | P0 |
| DL-308 ⚠️ | Resume in-flight khi khởi động | DB có task Queued/Active/Paused | Khởi động lại | Tất cả về **Paused** (không auto-resume); giải mã snapshot JSON đúng `bytesTransferred/fileSize` | Unit | P0 |
| DL-309 ⚠️ | Shutdown drain | Đang download, app đóng "đúng cách" | Đóng app | Destructor signal abort + quit, **chờ ≤2000ms/thread**; KHÔNG `terminate()` (tránh heap corrupt 0xc0000374) | Manual | P0 |
| DL-310 ⚠️ | Pause→Cancel race | Đang pause, bấm cancel | cancel khi đang chờ CV | Cả 2 set flag + notify CV; không deadlock/đua | Manual | P1 |
| DL-311 | pauseAll / resumeAll | Nhiều task | `pauseAll()`/`resumeAll()` | Mọi task đổi state đồng loạt; orchestrator nhất quán | Manual | P2 |
| DL-312 ⚠️ | Sidecar size mismatch | Journal khai size khác file thực | Resume | `readSidecar` chỉ hợp lệ khi data file đúng `fileSize` → lệch → bỏ journal, tải lại sạch | Manual | P1 |

---

# PHẦN B — UPLOAD

## NHÓM U1 — Luồng upload chính & chunked

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| UL-001 | Single chunk | File < 20 MB | `addUpload([f],folderId)` | `createUploadSession` → 1 chunk PUT → parse linkcode; state Complete | Manual | P0 |
| UL-002 | Multi chunk | File > 20 MB | Upload | Nhiều chunk `Content-Range: bytes from-to/total`; `fromTarget` tiến đúng; ghép đủ | Manual | P0 |
| UL-003 ⚠️ | Chunk cuối | File không chia hết chunk | Upload | Chunk cuối `toTarget=min(from+size,fileSize)`; `chunkLen` đúng phần dư; server nhận đủ | Manual | P0 |
| UL-004 | Adaptive chunk | Mạng nhanh/chậm | Upload file lớn | `currentChunkSize` điều chỉnh về TARGET 60s/chunk, clamp [5MB, ceiling]; không < 5MB | Manual | P1 |
| UL-005 ⚠️ | Halve khi lỗi | Chunk fail tạm thời | Upload | Chunk giảm /2 tới sàn 5MB, set `chunkCeiling`, re-query offset, retry | Manual | P1 |
| UL-006 | Parse linkcode | Server trả JSON `url`/`linkcode`/`link` | Hoàn tất | Trích đúng link; hoặc body là URL trần → nhận | Manual | P1 |
| UL-007 ⚠️ | Linkcode rỗng | Server trả body không có link hợp lệ | Hoàn tất | `failed("không trả về link hợp lệ")`; KHÔNG báo Complete với clipboard rác | Manual | P0 |
| UL-008 | Init session path | folderId "" / "0" | `createUploadSession` | Chuẩn hóa path "/"; prepend "/" nếu thiếu | Manual | P1 |
| UL-009 | secured / directLink | flags upload | `addUpload(...,secured,directLink)` | Cờ truyền đúng vào session; file đặt mật khẩu nếu secured | Manual | P2 |

## NHÓM U2 — Validate dữ liệu / lỗi

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| UL-101 | File không tồn tại | path sai | `addUpload` | Reject sớm (`!fi.exists()`); không tạo task ma | Unit | P0 |
| UL-102 ⚠️ | File 0 byte | file rỗng | `addUpload` | Reject với "Không thể upload file rỗng (0 bytes)" (đã verify dòng 528-534) | Unit | P0 |
| UL-103 | Sanitize tên gửi server | tên có `\/:*?"<>\|!,@#$^`, `..`, `--` | `addUpload` | Thay `_`, trim; rỗng sau sanitize → reject | Unit | P1 |
| UL-104 | HTTP 401/403 | token hết hạn / cấm | Upload chunk | **fatal** — không retry/halve; route lên TransferService | Manual | P0 |
| UL-105 ⚠️ | HTTP 507 quota | Tài khoản hết dung lượng | Upload | fatal; báo lỗi quota rõ ràng; không retry vô ích | Manual | P0 |
| UL-106 ⚠️ | INVALID_UPLOAD_SESSION | Server hủy session giữa chừng | Upload | `emit sessionExpired` → TransferService tạo session mới + re-queue; không hiện lỗi cho user | Manual | P0 |
| UL-107 ⚠️🔴 | File bị sửa khi đang upload (shrink) | File bị cắt nhỏ giữa upload | Chunk kế | `FSEEKO64` tới `fromTarget` fail nếu vượt EOF → "Failed to seek source file" fatal; KHÔNG gửi byte rác. NHƯNG nếu shrink ít (offset vẫn hợp lệ) → read callback đọc thiếu < `chunkLen` → **nghi corrupt** (xem rủi ro) | Manual | P0 |
| UL-108 ⚠️ | File bị sửa khi đang upload (grow) | File to thêm giữa upload | Upload | `fileSize` snapshot lúc tạo task không đổi → phần thêm KHÔNG được gửi; link trỏ bản cũ (chấp nhận được nhưng cần xác nhận) | Manual | P1 |
| UL-109 | File bị lock process khác | File đang mở exclusive | Upload | `openFileUnicode` fail → `failed`; không crash | Manual | P1 |
| UL-110 ⚠️ | queryResumeOffset validate | Server trả `Range: 0-N` với N ≥ size | Resume probe | Defensive: bỏ qua nếu âm/vượt total → restart 0; không seek lậu | Manual | P1 |
| UL-111 | Token trong realUrl hết hạn | URL session chứa token hết hạn | Upload | Server 4xx → `queryResumeOffset` trả -1 → sessionExpired → session mới | Manual | P1 |

## NHÓM U3 — Edge & Staging

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| UL-201 | File rất lớn (>50 GB) | Đủ quyền | Upload | offset/size `int64`; chunk tới 1GB; không tràn | Manual | P1 |
| UL-202 | Tên unicode/emoji | "tài liệu 📄.pdf" | Upload | `_wfopen` đọc đúng; tên gửi server hợp lệ | Manual | P2 |
| UL-203 | Trùng tên trên server | Đã có file cùng tên folder | Upload | Hành vi server (đa số tạo bản mới); xác nhận không lỗi client | Manual | P2 |
| UL-204 | Staging commit | UploadStagingViewModel hydrate | `commit()` | Đọc lại danh sách staging từ SettingsRepository; addUpload đúng từng file | Integration | P1 |
| UL-205 | Staging service null | TransferService=nullptr | `addUpload` | Short-circuit `!m_service`; không crash | Integration | P2 |
| UL-206 | Move task order | Nhiều task queue | `moveTaskUp/Down/First/Last` | Thứ tự đổi đúng; `taskOrderChanged` emit | Integration | P2 |

## NHÓM U4 — Pause / Resume / Cancel / Kill-app (UPLOAD)

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| UL-301 ⚠️ | Pause giữa chunk | Đang upload | `pauseTask` | Read callback `CURL_READFUNC_ABORT`; park không giữ socket; `requeryBeforeNext=true` | Manual | P0 |
| UL-302 ⚠️ | Resume sau pause | Đã pause | `resumeTask` | `queryResumeOffset` xác định offset server đã nhận → tiếp từ đó; không gửi trùng/thiếu | Manual | P0 |
| UL-303 | Cancel | Đang upload | `cancelTask` | `m_abort`; đóng file + curl cleanup; slot release | Manual | P0 |
| UL-304 ⚠️🔴 | Kill app giữa upload | Đang upload file lớn | Tắt cứng app | Mở lại: task về Paused; snapshot lưu **realUrl** (session URL) → resume dùng lại session; nếu URL hết hạn → sessionExpired → session mới | Manual | P0 |
| UL-305 ⚠️ | Pause trong lúc createUploadSession | Session đang được tạo async | `pauseTask` ngay | Slot release trước khi spawn engine; realUrl giữ lại; resume tái dùng (race line ~721-735) | Manual | P1 |
| UL-306 ⚠️ | Cancel trong lúc createUploadSession | Session đang tạo | `cancelTask` ngay | Check task còn tồn tại trước khi spawn engine; nếu mất → release slot, KHÔNG spawn engine mồ côi | Manual | P0 |
| UL-307 ⚠️ | Snapshot upload realUrl | Đang upload | Quan sát DB | snapshot chứa `realUrl`; `loadInFlight` khôi phục để resume cùng session | Unit | P0 |
| UL-308 ⚠️ | Chunk ceiling sau session-renew | Đã halve chunk → session expired → renew | Renew | Engine MỚI được spawn → `currentChunkSize`/`chunkCeiling` reset mặc định (xác nhận không kẹt ở size nhỏ) | Manual | P1 |
| UL-309 | Shutdown drain upload | Đang upload, đóng app | Đóng | Chờ ≤2000ms/thread; file đóng tự nhiên; không leak fd / heap corrupt | Manual | P0 |
| UL-310 ⚠️ | Low-speed watchdog | Upload <1KB/s 60s | Upload | curl abort → retry/backoff → halve; không treo | Manual | P1 |

---

# PHẦN C — SYNC (Folder watch + scan + auto-upload)

## NHÓM S1 — SyncScanner: filesystem walk (TỰ ĐỘNG HÓA ĐƯỢC)

Tiền điều kiện: dựng cây thư mục thật dưới `QTemporaryDir`; gọi `scanFilesystem(snap, maxFileSize)` trực tiếp.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-001 | Walk phẳng | Root có 3 file | scan watchSubfolders=false | Chỉ 3 file root; không xuống con; relPath = tên file | Unit | P0 |
| SY-002 | Walk đệ quy | Cây 3 cấp | scan watchSubfolders=true | Mọi file; relPath forward-slash; relDir đúng subfolder | Unit | P0 |
| SY-003 ⚠️ | File 0 byte bị bỏ | Có file rỗng + file thường | scan | File 0 byte **bị bỏ qua** (`fi.size()==0`) → KHÔNG xuất hiện trong `files`/`seenRel`. **Xác nhận đây là hành vi cố ý hay mất dữ liệu thầm lặng** | Unit | P0 |
| SY-004 | Skip-list hệ thống | Có `.git`, `node_modules`, `*.tmp`, `~$x.docx`, `Thumbs.db` | scan | Dir/junk bị prune; không vào kết quả | Unit | P1 |
| SY-005 | Ignore patterns user | `*.psd, *.iso` | scan | File khớp pattern bị bỏ; merge trên skip-list hệ thống | Unit | P1 |
| SY-006 ⚠️ | Oversized | File > maxFileSize (1 GB) | scan | `sf.oversized=true` (vẫn trong `files`); diff sau sẽ mark Failed, không upload | Unit | P1 |
| SY-007 ⚠️🔴 | Symlink/junction cycle | Junction trỏ về tổ tiên | scan | Cycle guard `canonicalFilePath` + `visitedDirs` → KHÔNG vòng lặp vô hạn; walk kết thúc | Unit | P0 |
| SY-008 | Broken symlink | Symlink target đã xóa | scan | `canon.isEmpty()` → skip; không crash | Unit | P1 |
| SY-009 ⚠️ | mtime giây | File sửa 2 lần trong <1s, cùng size | scan 2 lần | `mtime` (giây) GIỐNG nhau → diff coi "Synced không đổi" → **bỏ lần sửa thứ 2** (nghi mất cập nhật) | Unit | P0 |
| SY-010 | Root không tồn tại | localPath rỗng / đã xóa | scan | `rootExists=false`; `files` rỗng; không crash | Unit | P0 |
| SY-011 | Unicode path | Thư mục/tên VN + emoji | scan | relPath UTF-8 đúng; không rớt byte | Unit | P1 |
| SY-012 | Đường dẫn rất sâu/dài | > 260 ký tự (Windows) | scan | Không crash; xử lý path dài (long-path) hoặc bỏ qua an toàn | Unit | P2 |
| SY-013 | Cây rất nhiều file | 5.000+ file | scan | Hoàn tất trong thời gian hợp lý; không stack overflow (BFS iterative, không đệ quy) | Unit | P2 |
| SY-014 | parseIgnorePatterns | "  , *.a ,, *.b " | parse | Trim, bỏ rỗng → ["*.a","*.b"]; rỗng → [] | Unit | P2 |
| SY-015 | relDir tính đúng | File ở `a/b/c.txt` | scan | relDir="a/b"; file root → relDir="" | Unit | P1 |

## NHÓM S2 — Diff & state machine (applyScanResult)

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-101 ⚠️ | File mới → enqueue | Scan thấy file chưa có trong state | applyScanResult | Tạo entry state=**Uploading** (không Pending); `addSyncUpload`; lưu repo | Integration | P0 |
| SY-102 | Synced không đổi | size+mtime khớp prev Synced | applyScanResult | `continue` (bỏ qua, không re-upload) | Integration | P0 |
| SY-103 ⚠️ | Đang Uploading | prev state Uploading | applyScanResult | `continue` (không enqueue chồng) | Integration | P0 |
| SY-104 ⚠️🔴 | Re-upload sau fail (Uploading kẹt) | Task fail mạng nhưng state vẫn Uploading | scan lại | Cần re-enqueue khi không còn task thực; **nghi: nếu state kẹt Uploading mà task đã chết → file không bao giờ thử lại** | Integration | P0 |
| SY-105 | Missing detection | File từng Synced nay bị xóa local | applyScanResult | Mark `Missing`, **giữ linkcode** (bản Fshare còn); không xóa remote | Integration | P1 |
| SY-106 ⚠️ | File xóa giữa scan và enqueue | File biến mất sau khi walk, trước addSyncUpload | enqueue | `addSyncUpload` trả taskId rỗng → mark `Failed` "Không thể thêm vào hàng đợi"; không crash | Integration | P0 |
| SY-107 ⚠️ | Folder root biến mất giữa chừng | Root bị xóa sau walk | applyScanResult | Re-validate → `folderMissing` emit; abort; không upload ghost | Integration | P0 |
| SY-108 | Không pending | Mọi file đã Synced | applyScanResult | `folderSynced(0)`; clearScanInFlight; return sớm | Integration | P1 |
| SY-109 ⚠️ | Reentrancy guard | scan đang chạy, có yêu cầu scan mới | scanFolderInternal | `markScanInFlight` trả false → set dirty; KHÔNG chạy 2 walk song song; sau xong → 1 follow-up scan | Integration | P0 |
| SY-110 ⚠️ | makeScanSnapshot | Đổi config (ignore/watchSub) khi scan chạy | edit giữa chừng | Walk dùng snapshot cũ (value copy); scan kế dùng config mới; không tear inputs | Integration | P1 |

## NHÓM S3 — Subdir creation & upload routing

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-201 | Tạo root + subfolder | File trong subdir mới | ensureSubdirsThenEnqueue | Tạo root rồi subfolder parent→child; rồi enqueue upload đúng fsharePath | Manual | P0 |
| SY-202 ⚠️🔴 | Lỗi tạo subfolder bị nuốt | API `createFolderInPath` lỗi (network) | ensureSubdirs | Code log "treating as created" + **vẫn `createdOk.insert(rel)`** (dòng 746-768) → cache poisoned → upload sau có thể fail vào path không tồn tại. **Nghi bug** | Manual | P0 |
| SY-203 | Idempotent "đã tồn tại" | Subfolder đã có trên Fshare | ensureSubdirs | "Already exists" coi như OK (đúng) | Manual | P1 |
| SY-204 | Cache subdir | Lần scan 2 cùng subdir | ensureSubdirs | Dùng `m_createdSubdirs` cache, không gọi lại API thừa | Manual | P2 |
| SY-205 ⚠️ | Sắp xếp parent→child | Nhiều subdir lồng nhau | ensureSubdirs | newRelDirs sort theo độ sâu; tạo cha trước con; không lỗi thứ tự | Manual | P1 |

## NHÓM S4 — Watcher / Timer / Lifecycle

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-301 | addFolder validate | path không tồn tại / không phải dir / trùng | `addFolder` | Reject với errorOut; không thêm | Integration | P0 |
| SY-302 | Cap 5 folder | Đã có 5 folder | `addFolder` thứ 6 | Reject "đã đạt giới hạn"; không thêm | Integration | P1 |
| SY-303 | Watcher fire | Drop file vào folder | directoryChanged | `onPathChanged` match folder → scan Normal priority | Manual | P1 |
| SY-304 ⚠️ | Watcher storm | Drop 100 file cùng lúc | nhiều event | Reentrancy guard + dirty coalesce → không 100 scan; cuối cùng quét đủ | Manual | P1 |
| SY-305 | Rescan timer | Chờ 5 phút | timeout | `rescanAll` scan Background priority mọi folder enabled | Manual | P2 |
| SY-306 ⚠️ | Watcher path cap | Cây > ~10k thư mục con | rebuildWatcher | `addPaths` rớt path thừa âm thầm → dựa timer 5' bù; xác nhận vẫn sync được | Manual | P1 |
| SY-307 ⚠️ | Slow/UNC mount | Folder trên SMB/VPN timeout | rebuildWatcher (main thread) | `QFileInfo::isDir()` stat có thể **block GUI** — xác nhận mức đơ; cân nhắc đẩy off-main | Manual | P1 |
| SY-308 | setFolderEnabled | Toggle pause | enable/disable | Disabled → không scan; release dir handle (Windows move được); enable → scan lại | Integration | P1 |
| SY-309 | autoSync master toggle | Tắt master | scan request | `m_autoSyncEnabled=false` → mọi scan bỏ qua (master beats per-folder) | Integration | P1 |
| SY-310 | removeFolder | Có folder + tasks | `removeFolder` | `forgetFolderTasks`; xóa state+repo; log FolderRemoved; rebuildWatcher | Integration | P1 |

## NHÓM S5 — Kill-app / async guard / deleteAfterUpload

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-401 ⚠️🔴 | Kill khi scan đang chạy off-main | `QtConcurrent::run` walk đang chạy | Tắt app / SyncService destroy | `QPointer guard` chặn marshal-back; nếu service chết, walk xong nhưng **lambda không chạy → file đã scan không ghi repo** (chấp nhận: scan lại lần sau) | Integration | P0 |
| SY-402 ⚠️ | Kill khi đang upload sync | Sync upload đang chạy | Kill app | Task upload resume như UL-304 (snapshot realUrl); SyncFileEntry vẫn Uploading → scan lại đối chiếu | Manual | P0 |
| SY-403 ⚠️ | deleteAfterUpload thành công | Toggle bật, upload OK | onUploadFinished | `QFile::remove(localPath)`; mark Missing; log DeletedLocal; chỉ xóa khi remove() OK | Integration | P0 |
| SY-404 ⚠️🔴 | deleteAfterUpload — remove fail | File bị lock / read-only | onUploadFinished | `remove()` fail → log warning, **state vẫn Synced, file KHÔNG xóa** → user tưởng đã dọn nhưng còn. Xác nhận có cảnh báo người dùng không | Integration | P1 |
| SY-405 ⚠️ | deleteAfterUpload — file đã biến mất | File bị xóa ngoài trước khi remove | onUploadFinished | remove fail (không tồn tại) → giữ Synced; đúng | Integration | P2 |
| SY-406 | retryFailed | Có file Failed | `retryFailed` | Flip về Pending/recompute → scan re-enqueue; linkcode cũ giúp dedup nếu thực ra đã lên | Integration | P1 |
| SY-407 ⚠️ | retryFailed nhân đôi trên Fshare | File thực ra đã upload OK dù báo lỗi | retry | Nếu linkcode mất → upload lại → **2 bản trên Fshare** (nghi trùng lặp); nếu linkcode còn → skip | Manual | P1 |
| SY-408 | Folder disabled khi đang upload | Disable giữa upload | onUploadFinished | Task in-flight vẫn hoàn tất; SyncFileEntry cập nhật đúng; scan kế bỏ qua (disabled) | Integration | P2 |
| SY-409 | Timer sau khi service hủy | SyncService destroy | shutdown | `m_rescanTimer` dừng; không callback sau hủy; pool drain | Integration | P1 |

## NHÓM S6 — SyncRepository persistence

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| SY-501 | Round-trip folder | saveFolder | loadFolders | Mọi field khớp; v6 fields (watchSub/ignore/speed) đọc lại đúng | Unit | P1 |
| SY-502 | Pre-v6 upgrade | folder thiếu key v6 | loadFolders | Default an toàn (watchSub=true, ignore="", speed=5MB/s); không crash | Unit | P1 |
| SY-503 | File entry hash collision | 2 relPath khác | saveFile | Hash SHA1-16 không đè nhau; load đúng từng file | Unit | P2 |
| SY-504 | Activity FIFO cap | > 50 entry | appendActivity | Giữ 50 mới nhất; encode/decode tab-separated không vỡ với ký tự đặc biệt | Unit | P2 |
| SY-505 ⚠️ | Activity field có tab/`\x1f` | relPath/error chứa ký tự phân tách | appendActivity→load | **Nghi:** ký tự phân tách trong dữ liệu làm lệch cột khi decode | Unit | P2 |
| SY-506 | deleteAllFiles | Có nhiều file | resetFolderState | Clear list + subtree; sync() | Unit | P2 |

---

# PHẦN D — TẦNG CHUNG (Orchestrator / Budget / Resume DB)

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| TR-001 | Budget slot cap | Nhiều task | enqueue vượt cap | `BudgetManager` giới hạn slot/pool + global cap; floor cho Background | Unit | P1 |
| TR-002 | Priority order | Mix Interactive/Normal/Background | dispatch | `PriorityScheduler` pop đúng thứ tự ưu tiên | Unit | P1 |
| TR-003 | Cancel-before-dispatch | enqueue rồi cancelQueued | dispatch | `cancelQueued` gỡ khỏi hàng đợi; không dispatch | Unit | P1 |
| TR-004 ⚠️ | Resume snapshot round-trip | `saveProgressSnapshot` | `loadInFlight` | Đọc lại `bytes/total/retry/realUrl`; index partial `state IN (0,1,2)` đúng | Unit | P0 |
| TR-005 ⚠️ | UPSERT giữ progress_json | upsertTask đổi state | đọc lại | progress_json KHÔNG bị clobber khi đổi state (ADR D12; pin ATM-0271) | Unit | P0 |
| TR-006 | Migrate legacy JSON | Có JSON cũ | migrateLegacyUsingDb | Chuyển vào SQLite; không mất task | Unit | P2 |
| TR-007 | SpeedMeter | markProgress chuỗi | speed()/eta() | Rolling window đúng; chia 0 an toàn; eta "—" khi chưa đủ mẫu | Unit | P2 |

---

## KHU VỰC RỦI RO CAO CẦN TEST TRƯỚC (theo phân tích code)

1. 🔴 **Sync — lỗi tạo subfolder bị nuốt → cache poisoned** (`SY-202`).
   `ensureSubdirsThenEnqueue` (SyncService.cpp:744-768): khi `api->createFolderInPath` lỗi (network), code chỉ
   `qDebug() << "(treating as created)"` rồi **vẫn `createdOk.insert(rel)`**. Cache `m_createdSubdirs` đánh dấu
   "đã tạo" cho thư mục thực ra chưa tồn tại → upload tiếp theo trỏ path không có → fail; lần scan sau cache hit
   nên KHÔNG thử tạo lại → file kẹt Failed. Cần phân biệt "already exists" (ok) vs lỗi thật (không cache).

2. 🔴 **Sync — file 0 byte không bao giờ đồng bộ, âm thầm** (`SY-003`).
   `SyncScanner.cpp:133` bỏ qua mọi `fi.size()==0`. Người dùng kỳ vọng "đồng bộ folder" = mọi file; file rỗng
   (placeholder, `.gitkeep`, log mới tạo) biến mất khỏi backup mà không cảnh báo. Cần quyết định: sync file rỗng
   hay ít nhất tài liệu hóa.

3. 🔴 **Sync — sửa file <1s cùng kích thước không được phát hiện** (`SY-009`).
   Diff dùng `prev.mtime == sf.mtime` với `mtime` ở **giây** (SyncService.cpp:636-637, SyncScanner.cpp:149).
   File bị ghi đè cùng size trong cùng giây → coi "không đổi" → bản mới không được upload. Cân nhắc so thêm
   nano-giây hoặc hash khi size bằng.

4. 🔴 **Sync — file kẹt state Uploading khi task chết** (`SY-104`).
   `applyScanResult` `continue` khi `prev.state==Uploading` (SyncService.cpp:640). Nếu task upload chết bất
   thường (crash/kill) mà `onUploadFinished` không bao giờ chạy → entry kẹt Uploading vĩnh viễn → scan luôn bỏ
   qua → file không bao giờ được thử lại. Cần cơ chế "stale Uploading" reconcile lúc khởi động.

5. 🔴 **Upload — file shrink giữa chừng có thể gửi chunk thiếu** (`UL-107`).
   Seek bắt được khi `fromTarget` vượt EOF, nhưng nếu file co lại NHẸ (offset vẫn hợp lệ, đuôi mất),
   read callback đọc < `chunkLen` trong khi `CURLOPT_POSTFIELDSIZE_LARGE=chunkLen` → chunk thiếu byte. Nên
   re-stat size trước mỗi chunk và abort nếu khác snapshot.

6. 🟠 **Download — server bỏ qua resume / Range** (`DL-107`, `DL-008`).
   Phải chắc chắn khi server trả 200 thay vì 206/partial: single-segment xóa partial restart 0; multi-segment
   set `rangeBroken` → xóa+fallback. Sai ở đây = file corrupt (nối byte vào sai offset).

7. 🟠 **Resume sau kill-app (download/upload)** (`DL-304`, `DL-305`, `UL-304`, `TR-004`, `TR-005`).
   Snapshot 5s + journal 1s; khôi phục về Paused; upload giữ realUrl. Verify không mất/đúp byte, progress_json
   không bị clobber khi đổi state.

8. 🟠 **Symlink/junction cycle & cây lớn** (`SY-007`, `SY-013`, `SY-306`).
   Cycle guard canonicalPath; BFS iterative tránh stack overflow; watcher path cap dựa timer bù.

9. 🟡 **deleteAfterUpload remove fail im lặng** (`SY-404`).
   File không xóa được (lock/read-only) → vẫn Synced, không báo → user tưởng đã dọn.

10. 🟡 **Shutdown drain** (`DL-309`, `UL-309`, `SY-409`).
    Chờ ≤2000ms/thread, không terminate; timer/pool dừng sạch.

---

## TỔNG KẾT SỐ CASE THEO NHÓM

| Phần | Nhóm | Số case | P0 | P1 | P2 |
|---|---|---|---|---|---|
| A Download | D1 chính/multi-segment | 11 | 5 | 6 | 0 |
| A Download | D2 validate/lỗi | 10 | 5 | 5 | 0 |
| A Download | D3 edge/filesystem | 13 | 4 | 6 | 3 |
| A Download | D4 pause/kill | 12 | 7 | 4 | 1 |
| B Upload | U1 chính/chunked | 9 | 4 | 4 | 1 |
| B Upload | U2 validate/lỗi | 11 | 5 | 5 | 1 |
| B Upload | U3 edge/staging | 6 | 1 | 2 | 3 |
| B Upload | U4 pause/kill | 10 | 5 | 4 | 1 |
| C Sync | S1 scanner | 15 | 5 | 6 | 4 |
| C Sync | S2 diff/state | 10 | 6 | 3 | 1 |
| C Sync | S3 subdir | 5 | 1 | 3 | 1 |
| C Sync | S4 watcher/timer | 10 | 1 | 6 | 3 |
| C Sync | S5 kill/delete | 9 | 3 | 4 | 2 |
| C Sync | S6 repository | 6 | 0 | 2 | 4 |
| D Chung | TR orchestr/resume | 7 | 2 | 2 | 3 |
| **Tổng** | | **144** | **54** | **62** | **28** |

Case gắn cờ ⚠️ (rủi ro dễ ra bug): **47**. Trong đó 🔴 (nghi nghiêm trọng — mất dữ liệu / corrupt): **8**.

---

## PHẦN TỰ ĐỘNG HÓA ĐƯỢC (không cần mạng)

Engine download/upload nói chuyện trực tiếp với CDN/Fshare qua libcurl → phần lớn là **Manual**. Phần có thể
viết QtTest chạy CI ngay:

- **SyncScanner** (S1, một phần S2 diff logic): filesystem thuần dưới `QTemporaryDir` → `test_sync_scan_robustness.cpp`.
- **FileNameSanitizer** (DL-206/207/208/212): đã có `test_filename_sanitizer`; bổ sung case traversal nếu thiếu.
- **FshareUrl** (DL-109): đã có `test_fshare_url`.
- **Resume persistence** (TR-004/005): đã có `test_transfer_history_db`, `test_history_repository`.
- **Budget/Priority/Orchestrator** (TR-001..003): đã có `test_budget_manager`, `test_priority_scheduler`,
  `test_transfer_orchestrator`.
- **SyncRepository** (S6): đã có `test_sync_repository`.

Phần Manual cần checklist QA thủ công với tài khoản Fshare thật (kill-app, đĩa đầy, mất mạng, file lớn).
