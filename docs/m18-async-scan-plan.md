# M18 — Đưa quét thư mục sync ra background thread (kế hoạch thực thi)

> Tài liệu bàn giao cho một session làm việc khác. Mục tiêu: bỏ chặn UI khi `SyncService` quét folder nằm trên
> network mount, **không** viết lại lõi sync. Nguồn gốc: `docs/CRASH_AUDIT.md` M18 (đã hoãn ở đợt P2 2026-05-29).
> Mức ưu tiên: **P2 (perf/robustness)**. Ước lượng: **0.5–1 ngày** + test.

---

## 1. Vấn đề & tác động

`SyncService::scanFolderInternal()` ([src/core/services/SyncService.cpp:598](../src/core/services/SyncService.cpp))
chạy **trên main thread** và thực hiện một vòng BFS duyệt cây thư mục cục bộ bằng API filesystem **blocking**:
`QDir::entryInfoList`, `QFileInfo::canonicalFilePath/isDir/lastModified/size`.

- **Local SSD/HDD**: nhanh (ms) → không ai để ý.
- **Network mount (SMB/NFS), folder rất lớn/sâu, hoặc ổ ngủ (spun-down)**: vòng walk có thể block **1–30 giây** →
  **UI đơ** (main thread bận), spinner đứng hình, click không phản hồi.

Bị kích hoạt khá thường xuyên vì scan chạy ở **nhiều entry point** (xem §3): thêm folder, "sync now", file-watcher
fire, timer rescan 5 phút, login, retry. Mỗi lần đều block main nếu là network mount.

> Đây là vấn đề **perf/UX, KHÔNG phải crash**. Đó là lý do hoãn — nhưng đáng làm vì sync folder trên ổ mạng là
> use-case hợp lệ (NAS, ổ ánh xạ công ty).

---

## 2. Vì sao đã hoãn (bối cảnh quyết định)

Audit gốc mô tả M18 như "chuyển scan async = refactor lõi sync rủi ro". **Sau khi đọc kỹ, phạm vi hẹp hơn nhiều**:
- **Phần mạng ĐÃ async rồi**: `ensureSubdirsThenEnqueue()` ([SyncService.cpp:770](../src/core/services/SyncService.cpp))
  đã chạy `createFolderInPath` trên `QtConcurrent::run` và marshal kết quả về main thread bằng
  `QMetaObject::invokeMethod` để enqueue upload. ⇒ **không phải đụng phần đó.**
- **Chỉ duy nhất vòng walk filesystem** (dòng 627–726) là blocking-on-main cần tách ra.
- Đã có **tiền lệ walk thuần**: `previewScan()` ([SyncService.cpp:398](../src/core/services/SyncService.cpp)) là
  một hàm `const` duyệt y hệt (cùng skip-rule + cycle-guard `canonicalFilePath`) và trả về `PreviewResult` — chứng
  minh vòng walk **tách rời được** khỏi state.

Rủi ro thực: vòng walk hiện **xen kẽ đọc filesystem VỚI mutate shared state** (`m_files`, `m_repo`, `m_createdSubdirs`,
emit signal) — phải tách cẩn thận: *đọc FS ở background, mutate state ở main*.

---

## 3. Luồng hiện tại (chính xác)

`scanFolderInternal(folder, prio)` làm tuần tự trên main thread:

| Bước | Dòng | Việc | Thread an toàn? |
|---|---|---|---|
| A | 600–607 | guard (autoSync/enabled/rootDir exists) + emit `folderMissing` | main |
| B | 627–726 | **BFS walk** thư mục → tích lũy `pending` (file cần upload), `newRelDirs` (subdir mới), `seen`, đồng thời **ghi `map`/`m_repo->saveFile`** cho từng file (oversized/uploading) | ⚠ **walk blocking + mutate state** |
| C | 728–736 | đánh dấu file biến mất = `Missing` (đọc/ghi `map`/`m_repo`) | main-only |
| D | 738–746 | cập nhật `lastScanAt`, `emit folderFilesChanged`, `rebuildWatcher()` | main-only |
| E | 748–767 | sort `newRelDirs`, tính `needsRoot`, gọi `ensureSubdirsThenEnqueue` | main (rồi async) |

**Callers (11 chỗ, đều fire-and-forget, không đọc state ngay sau)** — an toàn để biến scan thành async:
`loadFoldersForUser` (144), `addFolder` (247), `setFolderEnabled` (327), `setWatchSubfolders` (350),
`setIgnorePatterns` (363), `scanFolder` (385/396), `retryFailed` (578), `rescanAll` (593), watcher
`directoryChanged` → (xem 59), timer `rescanAll` (63), login (954/964).

**Shared state mà walk đang chạm (PHẢI ở main thread):**
`m_files[folderId]` (`map`), `m_repo` (SyncRepository — SQLite, không thread-safe ở đây),
`m_createdSubdirs[folderId]` (`cache`, chỉ đọc trong walk), `findFolder`, `m_watcher`, mọi `emit`.

**Chưa có guard chống scan chồng nhau** (vì scan đang đồng bộ). Async sẽ tạo khả năng overlap → cần guard (§5.4).

---

## 4. Thiết kế đề xuất

**Nguyên tắc: tách "đọc filesystem" (background) khỏi "diff + mutate state + enqueue" (main).**

### 4.1 Cấu trúc dữ liệu trung gian (POD, copy được qua thread)
```cpp
// Kết quả walk thuần — KHÔNG tham chiếu state của SyncService.
struct ScannedFile {
    QString relPath;     // forward-slash, relative localPath
    QString absPath;
    QString relDir;      // "" nếu ở sync root
    qint64  size = 0;
    qint64  mtime = 0;   // secs since epoch
    bool    oversized = false;   // size > kMaxFileSize
};
struct ScanResult {
    QVector<ScannedFile> files;     // mọi file hợp lệ (đã qua skip-list, size>0)
    QSet<QString>        seenRel;   // relPath đã thấy (để tính Missing)
    bool                 rootExists = true;  // rootDir.exists() tại thời điểm walk
};
```

### 4.2 Hàm walk thuần (chạy background)
Tách phần BFS (dòng 627–726) thành **hàm static/free** `scanFilesystem(folder snapshot) -> ScanResult`:
- Nhận **bản copy** các trường cần của `SyncFolder` (`localPath`, `watchSubfolders`, `ignorePatterns`,
  `fshareFolderName`) — KHÔNG giữ con trỏ `SyncService`.
- Chỉ đọc filesystem, áp `shouldSkipFile/shouldSkipDir` (chuyển 3 hàm này thành `static` hoặc nhận skip-list params),
  cycle-guard `canonicalFilePath` (giữ nguyên), gom vào `ScanResult`.
- **Không** đụng `map`/`m_repo`/emit. (Giống `previewScan` nhưng trả metadata đầy đủ thay vì chỉ count.)
- *Tip*: refactor `previewScan` để dùng chung lõi walk này (DRY — hiện 2 bản walk song song dễ lệch).

### 4.3 scanFolderInternal mới (điều phối)
```cpp
void SyncService::scanFolderInternal(const SyncFolder &folder, TransferPriority prio) {
    if (!m_autoSyncEnabled || !folder.enabled) return;
    if (markScanInFlight(folder.id) == false) return;        // §5.4 guard
    // snapshot các trường folder cần cho walk:
    const auto snap = makeScanSnapshot(folder);              // POD copy
    QPointer<SyncService> guard(this);
    QtConcurrent::run([guard, snap, folderId=folder.id, prio]() {
        ScanResult r = scanFilesystem(snap);                 // BLOCKING, nhưng off-main
        if (!guard) return;
        QMetaObject::invokeMethod(guard.data(), [guard, folderId, prio, r]() {
            if (auto *s = guard.data()) s->applyScanResult(folderId, prio, r);
        }, Qt::QueuedConnection);
    });
}
```

### 4.4 applyScanResult (chạy main — chứa logic cũ bước B-diff + C + D + E)
- Re-validate: folder còn tồn tại? `m_userId` còn? `m_autoSyncEnabled`/`enabled`? (có thể đã đổi khi walk chạy)
  → nếu không, `clearScanInFlight(folderId)` rồi return.
- Nếu `!r.rootExists`: `emit folderMissing` + return (+ clear guard).
- **Diff + mutate `map`/`m_repo`** từ `r.files` (port nguyên logic dòng 674–724: oversized→Failed,
  unchanged Synced→skip, Uploading→skip, else→Uploading + saveFile + thêm vào `pending`; tính `newRelDirs` từ
  `cache`).
- Mark Missing từ `r.seenRel` (port 728–736).
- `lastScanAt`, `emit folderFilesChanged`, `rebuildWatcher` (port 738–746).
- `ensureSubdirsThenEnqueue(...)` (giữ nguyên — đã async).
- `clearScanInFlight(folderId)`.

---

## 5. Edge cases (BẮT BUỘC xử lý)

1. **Reentrancy / overlap**: watcher hoặc timer fire khi scan của cùng folder đang chạy background. → §5.4 guard
   per-folder; nếu đang in-flight, **set cờ "dirty"** và scan lại MỘT lần sau khi xong (coalesce) — đừng bỏ hẳn
   (mất thay đổi) cũng đừng chồng N scan.
2. **Folder bị remove / user logout / autoSync tắt giữa lúc walk chạy**: `applyScanResult` re-check
   `findFolderConst(folderId)` + `m_userId` + `m_autoSyncEnabled` (giống guard đã có ở
   [SyncService.cpp:835](../src/core/services/SyncService.cpp)). Nếu fail → drop kết quả, clear guard.
3. **Folder bị sửa config (watchSubfolders/ignorePatterns/speedLimit) giữa lúc walk**: vì snapshot lúc bắt đầu,
   kết quả phản ánh config CŨ; lần scan kế (config setter đều gọi scanFolderInternal) sẽ áp config mới. Chấp nhận
   được (giống value-snapshot của TransferTask). Ghi rõ trong comment.
4. **`m_repo` (SQLite) phải ở main**: tuyệt đối KHÔNG gọi `m_repo->saveFile` trong worker. Mọi save ở `applyScanResult`.
5. **`m_createdSubdirs` cache**: chỉ đọc/ghi ở main (`applyScanResult`). Worker không chạm.
6. **Thread pool shutdown**: lambda đã guard bằng `QPointer<SyncService>`; `main.cpp` đã có
   `QThreadPool::waitForDone(5000)` lúc thoát → an toàn.
7. **rebuildWatcher** phải chạy SAU diff (ở main) — giữ thứ tự cũ.
8. **shouldSkipFile/Dir** hiện là method instance dùng `kSystemFileSkip` static + user patterns → chuyển thành
   `static` thuần (chúng không đọc member nào) để gọi từ `scanFilesystem` background. Kiểm: chúng chỉ dùng hằng
   static + tham số → OK để static-hoá.

## 5.4 Guard reentrancy (đề xuất cụ thể)
```cpp
QSet<QString> m_scanInFlight;     // folderId đang quét background
QSet<QString> m_scanDirty;        // folderId nhận yêu cầu scan khi đang in-flight
bool markScanInFlight(const QString &id) {   // return false nếu đã in-flight (đã set dirty)
    if (m_scanInFlight.contains(id)) { m_scanDirty.insert(id); return false; }
    m_scanInFlight.insert(id); return true;
}
void clearScanInFlight(const QString &id) {  // gọi cuối applyScanResult
    m_scanInFlight.remove(id);
    if (m_scanDirty.remove(id)) {            // có yêu cầu dồn → quét lại 1 lần
        if (auto *f = findFolderConst(id)) scanFolderInternal(*f, TransferPriority::Background);
    }
}
```
Tất cả truy cập `m_scanInFlight/m_scanDirty` ở main thread (markScanInFlight đầu scanFolderInternal,
clear ở applyScanResult) → không cần mutex.

---

## 6. Các file & thay đổi

- `src/core/services/SyncService.h`: thêm struct `ScannedFile`/`ScanResult` (hoặc trong .cpp), khai báo
  `scanFilesystem` (static), `applyScanResult`, `makeScanSnapshot`, guard members + helpers; static-hoá
  `shouldSkipFile/shouldSkipDir`.
- `src/core/services/SyncService.cpp`: tách walk (627–726) → `scanFilesystem`; viết `applyScanResult` (port
  diff/missing/watcher/enqueue); viết lại `scanFolderInternal` thành điều phối async; (tùy chọn) refactor
  `previewScan` dùng chung lõi walk.
- Callers: **không đổi** (chữ ký `scanFolderInternal` giữ nguyên; chỉ hành vi từ sync → async, fire-and-forget).

---

## 7. Test

- **Unit (`tests/test_sync_scan.cpp` mới)** cho `scanFilesystem` thuần (không cần network/DB):
  dựng `QTemporaryDir` với cây file (gồm: file thường, file 0-byte, file > kMaxFileSize giả lập bằng cờ, thư mục
  skip `.git/node_modules`, subdir sâu, symlink/junction cycle nếu test được trên Windows). Assert `ScanResult`:
  đúng số file, đúng `relPath/relDir`, skip đúng, cycle không treo, `oversized` đúng. → bổ sung vào
  `tests/CMakeLists.txt` (link Qt6::Core + Test, KHÔNG cần curl).
- **Reentrancy**: test `markScanInFlight/clearScanInFlight` coalesce (gọi scan 3 lần liên tiếp → 1 scan + 1 dirty
  rescan). Có thể tách guard logic ra hàm thuần để test.
- **Manual (bắt buộc)**: tạo sync folder trên một **network share/ổ ánh xạ chậm** (hoặc giả lập độ trễ), quét →
  UI **không đơ** (kéo cửa sổ/click mượt trong lúc quét); file vẫn lên hàng đợi đúng. Đối chiếu folder local: hành
  vi không đổi.
- **Regression**: chạy full `ctest`; smoke app (add folder, sync now, đổi ignore patterns, retry, để timer rescan).

---

## 8. Verification & rollback

- Build: `& cmd.exe /c "scripts\build.bat"` (qua PowerShell) → `[PASS]`; `ctest --test-dir build -C Release` xanh.
- Smoke: chạy `output/FsNext.exe`, thao tác sync, soi `%APPDATA%/FPT/FsNext/fsnext.log` không có lỗi/đơ.
- **Rollback**: thay đổi gói gọn trong `SyncService.{h,cpp}`; nếu phát sinh lỗi sync, revert commit là đủ (không
  đụng schema DB, không đụng API/engine).

## 9. Ngoài phạm vi / lựa chọn đã cân nhắc
- **Không** đổi `QFileSystemWatcher` → một cơ chế khác (giữ nguyên; chỉ rebuild ở main như cũ).
- **Không** gộp scan đa-folder vào một worker (giữ per-folder để guard + hủy đơn giản).
- Đã loại phương án "bọc cả scan trong QtConcurrent rồi mutate state từ worker" — sai (m_repo/m_files/watcher là
  main-only). Phương án đúng là tách walk-thuần như §4.

## 10. Định nghĩa hoàn thành (DoD)
- [x] `scanFilesystem` thuần + `applyScanResult` + guard reentrancy; `scanFolderInternal` async; callers không đổi.
      → walk + skip-list + parse tách hẳn ra TU mới **`src/core/services/SyncScanner.{h,cpp}`** (free functions, chỉ
      phụ thuộc Qt6::Core) thay vì static member trong SyncService — lý do: §7 yêu cầu test link Qt6::Core/Test mà
      KHÔNG kéo curl/moc; nếu để trong SyncService.cpp thì test buộc phải link cả FshareApi/TransferService/SQLite.
- [x] `previewScan` dùng chung lõi walk (gọi thẳng `scanFilesystem`, loại oversized khỏi count — đúng hành vi cũ).
- [x] `test_sync_scan` xanh + full ctest xanh (12/12).
- [~] Manual: chưa verify trên network mount thật (môi trường này chưa đăng nhập + không có ổ mạng). Smoke local:
      app khởi động/thoát sạch, log không lỗi. Walk-correctness phủ bởi unit test + giữ nguyên logic diff.
- [x] Cập nhật `docs/CRASH_AUDIT.md` (M18 → ✅) + `docs/BACKLOG.md`.
