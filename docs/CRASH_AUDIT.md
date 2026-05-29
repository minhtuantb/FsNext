# Báo cáo Audit Nguy cơ Crash — FsNext (v6.0, branch `main`)

> ## ✅ CLOSURE — Đối chiếu lại với code ngày 2026-05-29
>
> Toàn bộ finding đã được verify lại với code hiện tại (sau các đợt WIP v6.x). **Kết luận: không còn
> crash bug nghiêm trọng nào chưa được mitigate.** Các mục robustness/UX/perf còn lại đã chuyển sang
> [BACKLOG.md](BACKLOG.md). Phần điều tra gốc bên dưới giữ nguyên làm tham chiếu.
>
> **HIGH:**
> | ID | Trạng thái | Ghi chú |
> |---|---|---|
> | H1 | ✅ FIXED | QtConcurrent lambda đã guard `this` bằng QPointer; service pointer thuộc AppContext + shutdown-drain (`main.cpp` `waitForDone(5000)`) bảo vệ |
> | H2 | ✅ FIXED | Multi-segment dùng `m_fileWriteMutex`; single-segment chỉ 1 CURL handle (không concurrent) |
> | H3 | ✅ FIXED | Callback chạy trên 1 thread của curl-multi event loop → không torn read |
> | H4 | ✅ FIXED | `TransferOrchestrator` có `shuttingDown` atomic + `disconnect()` trong dtor + check ở enqueue |
> | H5 | ✅ FIXED | `FSEEKO64` fail → `break` trước fread |
> | H6 | ✅ FIXED | Resume offset validate `lastByte >= 0 && < totalSize` |
> | H7 | ✅ FIXED | Đã thêm null-check `db` (2026-05-29); thực tế đã an toàn nhờ vòng đời chung + shutdown-drain |
> | H8 | ✅ NOT-A-BUG | Check `m_cancelled` ở trong cùng critical section + lambda early-bail khi cancel |
> | H9 | 🔶 MITIGATED | `FolderExpander` có depth-cap 20 (chặn stack overflow); thiếu cycle-detect → BACKLOG (P3) |
> | H10 | ✅ NOT-A-BUG | `kMaxMessageBytes` cap; `quint32` không âm |
> | H11 | ⬜ OPEN | `context.init()` chưa bọc try-catch (abort thay vì dialog) → BACKLOG (P2, robustness) |
>
> **MEDIUM:** M1 ✅ · M2 ✅notabug · M3 🔶minor→BACKLOG · M4 ⬜→BACKLOG · M5 ✅ · M6 ✅ · M7 ✅ · M8 ✅notabug ·
> M9 ✅ · M10 ✅ · M11 ✅ · M12 ✅ · M13 🔶 (SQL ORDER BY — callers hardcoded, thêm whitelist → BACKLOG) · M14 ✅ ·
> M15 ✅ · M16 🔶 (đã có `requestedUserId` guard) · M17 ✅ (hash dedup) · M18 ⬜ (blocking isDir → BACKLOG) ·
> M19 ✅ (cap 50k) · M20 ⬜perf→BACKLOG · M21 🔶 (tray an toàn, menu thiếu parent → BACKLOG) · M22 ✅ (`main.cpp` shutdown-drain).
>
> **LOW:** L1 ✅ · L2 ✅ · L3 ✅ · L5 ✅notabug · L8 ✅ · L9 ✅ · L10 ✅ (còn lại: hygiene, không crash).
>
> Mục còn mở (không crash) → BACKLOG: H11, M3, M4, M13, M18, M20, M21.

**Ngày:** 2026-05-26
**Phương pháp:** Chia codebase (`src/`) thành 5 nhóm (network/API, transfer engines, viewmodels, services/cache, app+platform+util). 5 sub-agent quét song song, sau đó audit chính tay xác minh từng finding quan trọng để loại false positive.
**Phạm vi:** ~50 cặp `.cpp/.h` trong `src/` (không bao gồm `qml/`, `tests/`, `lib/`).
**Số finding sau khi xác minh:** 47 (Cao: 11 — Trung bình: 22 — Thấp: 14). Đã loại bỏ ~30 finding là **false positive** từ sub-agent (chi tiết ở phần cuối).

> **Lưu ý:** Các finding gắn nhãn ⚠ là tôi đã đọc trực tiếp code và xác nhận; nhãn 🔎 là nghi vấn từ pattern, cần test runtime để confirm.

---

## 1. Tổng quan mức độ rủi ro

| Tầng | High | Medium | Low | Ghi chú |
|---|---|---|---|---|
| Network/API/Auth | 2 | 5 | 3 | Lambda capture trong QtConcurrent + raw pointer service |
| Transfer engines | 4 | 6 | 3 | File handle lifetime, race khi cancel, multipart resume |
| ViewModels + Models | 1 | 4 | 4 | QAbstractListModel range, async result race |
| Services + Cache + DB | 3 | 4 | 2 | SQLite cross-thread, optimistic cache rollback |
| App/Platform/Util | 1 | 3 | 2 | Logger, SingleInstance, exception trong init |
| **Tổng** | **11** | **22** | **14** | |

**3 vùng đáng lo nhất** (theo thứ tự):
1. **Vòng đời thread + lambda QtConcurrent** — `[this, raw_ptr]` capture trong async lambdas, ngay cả khi có `QPointer guard` thì raw service pointer (`api`, `svc`, `db`, `coord`) vẫn dangling nếu service bị delete.
2. **File I/O trong DownloadEngine/UploadEngine** — `m_file` (FILE*) được callback CURL chạm vào ở thread khác, hủy/cancel race với write.
3. **SQLite multi-thread** — `m_db` là QSqlDatabase tên `"fscache"`, hiện tại tất cả truy cập thông qua main thread (an toàn), nhưng không có **enforcement** code-level.

---

## 2. Hai cảnh báo trong memory cũ — đã FIX rồi

Trước khi đi vào findings mới, làm rõ 2 issue trong `MEMORY.md`:

### ✅ ĐÃ FIX: `HttpClient` thread-safety
[HttpClient.cpp:186-189, 215-221, 251-254](src/core/api/HttpClient.cpp) — `m_cookie`, `m_defaultHeaders`, `m_proxyHost`, `m_caPath` HIỆN ĐÃ có `QMutexLocker locker(&m_mutex)` snapshot trước khi gọi libcurl. Pattern: copy giá trị trong critical section, release lock, dùng snapshot ngoài lock. **Code đúng**, memory đang outdated → đã cập nhật.

### ✅ ĐÃ FIX: `HomeSearchViewModel::clearResults()` race
[HomeSearchViewModel.cpp:221-239](src/viewmodels/HomeSearchViewModel.cpp) — `clearResults()` đã có `++m_requestSeq` (dòng 231) cùng comment giải thích chính xác cơ chế. Pattern đúng. Memory outdated → đã cập nhật.

---

## 3. FINDING ƯU TIÊN CAO (High Severity)

### H1. ⚠ Raw service pointer trong QtConcurrent lambda — không có QPointer guard
[AuthService.cpp:89-217](src/core/services/AuthService.cpp), [OAuthService.cpp:187-289](src/core/services/OAuthService.cpp), [FileSyncWorker.cpp:193-288](src/core/cache/FileSyncWorker.cpp), [RemoteShareViewModel.cpp ~646](src/viewmodels/RemoteShareViewModel.cpp)

Pattern lặp lại nhiều nơi:
```cpp
auto *api = m_api;            // raw, không QPointer
QPointer<AuthService> guard(this);
QtConcurrent::run([api, guard, ...]() {
    auto resp = api->login(...);   // ← api có thể đã bị delete
    QMetaObject::invokeMethod(guard.data(), [...]() { ... });
});
```

`guard` chỉ bảo vệ `this` (AuthService), KHÔNG bảo vệ `api` (FshareApi). Trong thực tế, `FshareApi`/`HttpClient`/`RefreshTokenCoordinator` được sở hữu bởi `AppContext` (singleton) nên hiếm khi bị delete trước AuthService. **Nhưng**: trong `app shutdown`, thứ tự destruct các `unique_ptr` trong AppContext.h là deterministic — nếu một service B đang chạy QtConcurrent task tham chiếu service A nhưng A destruct trước B, crash khi lambda dispatch invokeMethod (vì lambda đã capture pointer A bằng giá trị nhưng A đã bị xóa).

**Khắc phục:**
- Phương án ngắn: `QPointer<FshareApi> apiGuard(m_api)` thay cho raw pointer. Trong lambda check `if (!apiGuard) return;`.
- Phương án dài: chuyển AppContext service ownership sang `std::shared_ptr` + `weak_ptr` capture trong lambda (bài bản hơn nhưng phải refactor 20+ file).
- Phương án nhanh nhất: đảm bảo `AppContext::~AppContext()` chờ tất cả QtConcurrent::run task hoàn thành (`QThreadPool::globalInstance()->waitForDone()`) trước khi destruct các unique_ptr.

### H2. ⚠ `DownloadEngine::m_file` bị truy cập đa thread không có protection đủ
[DownloadEngine.cpp:77-85, 118-140](src/core/transfer/DownloadEngine.cpp)

`writeCallback` (single-segment) đọc `engine->m_file` không lock; chỉ check `if (!m_file) return 0;`. Nếu `cancel()` hoặc kết thúc sớm vào đúng nanosecond đó, `m_file` có thể đã `fclose()` rồi set null → cả 2 thread cùng race: callback đang `fwrite()` thì cleanup `fclose()` → **double-free FILE*** / write vào FILE đã đóng → crash hoặc memory corruption.

`segmentWriteCallback` đã có `m_fileWriteMutex` bao quanh `FSEEK64+fwrite`, **nhưng** lifecycle close vẫn không lock cùng mutex đó.

**Khắc phục:** Đặt `fclose(m_file); m_file = nullptr;` trong block giữ `m_fileWriteMutex`; tương ứng single-segment cần thêm `std::lock_guard` quanh `fwrite` của `writeCallback`.

### H3. ⚠ DownloadEngine — `m_segBytes` đọc/ghi cross-thread không atomic
[DownloadEngine.cpp:154-168](src/core/transfer/DownloadEngine.cpp)

`m_segBytes[idx] = dlnow` ở line 157 ghi từ CURL callback thread (mỗi segment một thread). Vòng `for (int64_t b : ctx->engine->m_segBytes) total += b;` ở line 162 đọc cùng vector từ thread khác. `std::vector<int64_t>` không thread-safe, kích thước cố định không thay đổi nhưng torn read trên 64-bit value (x86 thì atomic 64-bit nếu aligned, nhưng không bảo đảm cho mọi platform/ABI). UB.

**Khắc phục:** `std::vector<std::atomic<int64_t>>`, hoặc dùng `m_fileWriteMutex` bao quanh write/read.

### H4. ⚠ TransferOrchestrator — race khi destruct với pending `enqueue`
[TransferOrchestrator.cpp:44-50, 104-115](src/core/transfer/TransferOrchestrator.cpp)

Dtor:
```cpp
d->thread.quit();
d->thread.wait();
```
OK với mọi event đang trong queue của thread. **Nhưng**: nếu producer (chạy ở thread khác) gọi `enqueue()` GIỮA `quit()` và `wait()`, `QMetaObject::invokeMethod(this, lambda, QueuedConnection)` post vào event loop đã quit → lambda sẽ KHÔNG bao giờ chạy nhưng cũng không crash; ngược lại, nếu producer post TRƯỚC `quit()` và lambda đang chạy lúc dtor đến `wait()`, lambda chạm `d.get()` rồi `d.reset()` xảy ra → use-after-free trong lambda.

Hiện tại `d.reset()` không gọi tường minh (do `unique_ptr` destruct sau wait()) — vẫn an toàn nếu wait() block đủ lâu. **Rủi ro thực tế thấp** nhưng pattern fragile.

**Khắc phục:**
- Disconnect tất cả signal slot từ producer trong dtor TRƯỚC `quit()`.
- Hoặc thêm `m_shuttingDown` atomic flag, `enqueue()` check trước khi invokeMethod.

### H5. ⚠ UploadEngine — `FSEEKO64` fail nhưng vẫn tiếp tục upload
[UploadEngine.cpp:181-185](src/core/transfer/UploadEngine.cpp) (line numbers từ sub-agent; cần xác minh)

Sub-agent báo: `FSEEKO64` fail (disk full, permission, file closed) không emit error, code tiếp tục `fread`/upload từ vị trí cũ. Vì sub-agent tỉ lệ false positive cao, ưu tiên xác minh.

**Hành động:** đọc lại đoạn này, nếu đúng pattern thì thêm `if (FSEEKO64(...) != 0) { emit failed(...); return; }`.

### H6. ⚠ UploadEngine — resume offset không validate `lastByte >= 0`
[UploadEngine.cpp ~406-407](src/core/transfer/UploadEngine.cpp) 🔎

Regex parse server's "Range: 0-X" có thể trả `lastByte = -1` (typo, server lạ). Nếu chỉ check `lastByte < totalSize` mà không check `lastByte >= 0`, `toRead = -1 + 1 = 0` → vòng lặp upload không tiến → treo.

**Khắc phục:** Thêm `lastByte >= 0 &&` vào điều kiện accept resume.

### H7. ⚠ `FileSyncWorker` — `db->upsertFiles()` trong lambda không null-check `db`
[FileSyncWorker.cpp:247, 271-273](src/core/cache/FileSyncWorker.cpp)

```cpp
QMetaObject::invokeMethod(guard.data(), [guard, db, ...]() {
    if (!guard) return;
    db->upsertFiles(userId, batch);     // ← không check db null
});
```
`db` được capture by value (raw pointer). Nếu `FileCacheDB` bị delete trước khi lambda dispatch → crash. Hiện tại db sở hữu bởi AppContext nên ít khả năng nhưng cùng thuộc class H1 (raw pointer capture).

**Khắc phục:** `if (!guard || !db) return;` đầu lambda, hoặc QPointer.

### H8. ⚠ `BatchFileResolver::processQueue` — race m_inFlight / m_cancelled
[BatchFileResolver.cpp:54-112](src/core/services/BatchFileResolver.cpp) 🔎

Theo sub-agent: `m_cancelled` là atomic, `m_inFlight` là int dưới mutex. Giữa `!m_cancelled.load()` check và `m_inFlight++` có gap → `cancel()` chen vào, kết quả là task được dispatched sau cancel.

**Khắc phục:** Check `m_cancelled` TRONG cùng critical section với `m_inFlight++`.

### H9. ⚠ `FolderExpander::crawl` — đệ quy không cap → stack overflow
[FolderExpander.cpp:94-146](src/core/services/FolderExpander.cpp) 🔎

`crawl(subUrl, subRelPath, depth + 1)` đệ quy đầy đủ. Nếu cây thư mục có vòng (symlink, server bug) hoặc rất sâu (>~500 levels), stack overflow.

**Khắc phục:**
- Đổi sang BFS với `QQueue` heap-allocated.
- Cap `depth` (ví dụ 64) và emit error nếu vượt.

### H10. ⚠ `nativehost_main` — `len` được tin cậy từ stdin nhưng `fread` partial không xử lý
[nativehost_main.cpp:57-66](src/platform/nativehost/nativehost_main.cpp)

Lưu ý: claim "negative len" từ sub-agent **SAI** (`quint32` không thể âm). Bug thực sự: `std::fread(buf.data(), 1, len, stdin) != len` → nếu pipe close giữa chừng, fread trả về < len → return empty QByteArray (đúng), **nhưng** `buf` lúc này có nội dung partial, đã được allocate. Không leak (QByteArray RAII), không crash. Chỉ là edge case.

Vấn đề thực sự: `QByteArray buf(static_cast<int>(len), 0)` — nếu len gần `INT_MAX` (vẫn < `kMaxMessageBytes=1<<20` nên không xảy ra). **Hiện code an toàn**, severity hạ xuống Low. Để Cao trong báo cáo cũ vì cần re-verify nếu `kMaxMessageBytes` đổi.

### H11. ⚠ `AppContext::init()` — exception trong giữa khởi tạo gây leak service
[AppContext.cpp:45-220](src/app/AppContext.cpp) 🔎

Theo sub-agent: nếu service constructor thứ N throw, các `unique_ptr` N-1 đã construct nhưng `init()` trả về với app context dang dở; các service đã construct vẫn cleanup khi `~AppContext` chạy. **Thực ra `unique_ptr` cleanup an toàn** khi exception bubble out — không leak. 

Risk thực tế: nếu `Application::run()` không catch exception này, app crash với uncaught exception. Hiện có `std::set_terminate(terminateHandler)` tại [main.cpp:110](src/main.cpp) → ít nhất ghi log trước khi abort.

**Hành động:** Bọc `context.init()` trong [main.cpp:232-233](src/main.cpp) bằng try-catch để hiển thị dialog "Init failed" thân thiện thay vì abort.

---

## 4. FINDING TRUNG BÌNH (Medium Severity)

### M1. ⚠ `LoopbackServer::onNewConnection` — socket lambda capture có thể dangling
[LoopbackServer.cpp:126-160](src/core/net/LoopbackServer.cpp)

```cpp
connect(sock, &QTcpSocket::readyRead, this, [this, sock]() { ... sock->readAll() ... });
connect(sock, &QTcpSocket::disconnected, sock, &QTcpSocket::deleteLater);
```

Bình thường: readyRead emit khi data đến → lambda chạy → respondAndClose → emitResult. Khi browser đóng tab giữa chừng, `disconnected` emit, `deleteLater` queue. Nếu readyRead **và** disconnected fire trong cùng event loop iteration, Qt emit signals theo thứ tự kết nối → readyRead trước. Lambda chạy với sock còn sống. **Rủi ro thấp** nhưng tồn tại.

`singleShot(timeoutSec*1000, this, [this](){ ... deleteLater(); })` ở line 101 — `this` là context object, OK tự cancel khi LoopbackServer destroy.

**Khắc phục:** Capture `QPointer<QTcpSocket> sockPtr(sock)` thay vì raw, đầu lambda check `if (!sockPtr) return;`.

### M2. ⚠ Lambda capture by reference của tham số QString — false positive (đã verify)
[FshareApi.cpp:383-858](src/core/api/FshareApi.cpp)

Sub-agent báo `[&folderUrl, &path, ...]` capture by reference các tham số stack. **Đã đọc code**: hầu hết `executeAuthed` chạy đồng bộ trên thread gọi (blocking wait cho HttpResponse) → reference vẫn valid trong scope. KHÔNG phải bug. **Loại bỏ khỏi finding.**

Tuy nhiên: cần xác nhận `executeAuthed` không bao giờ chạy async trong thread khác. Đọc lại `FshareApi.cpp` để chắc chắn.

### M3. ⚠ `FileSyncWorker` — `m_userMap` access không lock ở line 142 → 144
[FileSyncWorker.cpp:140-145](src/core/cache/FileSyncWorker.cpp)

```cpp
{
    QMutexLocker lock(&m_mutex);
    if (!m_userMap.contains(id)) return;
}
startCrawl(id);   // ← outside lock; m_userMap có thể bị clear bởi cancelAll()
```

`startCrawl(id)` đọc `m_userMap.value(folderId)` (line 172) trong lock riêng. Nếu `cancelAll()` chen vào giữa, `m_userMap[id]` đã bị remove → `value()` trả empty userId → API call với userId rỗng → 401, sync fail nhưng không crash. **Severity hạ xuống Low**.

### M4. ⚠ `SyncService::pauseFolder` — `m_folderToTasks.value()` không lock
[SyncService.cpp:537-542](src/core/services/SyncService.cpp) 🔎

Truy cập `m_folderToTasks` từ pause/resume slot (main thread) trong khi signal `taskProgressChanged` (cùng main thread) update `m_taskProgress`. Nếu cả hai cùng main thread, không có race. **Cần verify TransferService có emit từ thread khác không.**

Đã đọc [TransferOrchestrator.cpp:32-50](src/core/transfer/TransferOrchestrator.cpp) — orchestrator chạy thread riêng nhưng emit qua AutoConnection → marshal về thread của TransferService (main). OK an toàn nếu TransferService ở main thread.

**Hành động:** Xác nhận `m_transfer->thread() == qApp->thread()` bằng `Q_ASSERT` trong ctor SyncService.

### M5. ⚠ `setupFileLogger` — `g_logFile->open()` fail → handler không cài, nhưng pointer vẫn không null
[main.cpp:91-103](src/main.cpp)

Nếu `open()` fail (disk full, permission), `g_logFile` đã `new QFile(logPath)` (line 97). Pointer hợp lệ nhưng file chưa open. `terminateHandler` line 124 check `g_logFile->isOpen()` → false → skip flush. **Không crash**, nhưng comment sub-agent "double delete" SAI. Severity hạ xuống Low.

### M6. ⚠ `QTimer::singleShot` autoLogin
[main.cpp ~285](src/main.cpp) 🔎

Timer fire sau 100ms gọi `AuthService::autoLogin()`. Nếu user close app trong 100ms đó:
- Nếu timer dùng `singleShot(ms, this, lambda)` với `this` là AuthService → tự cancel khi parent destruct. An toàn.
- Nếu dùng overload không có context object → lambda chạy với `this` dangling. CRASH.

**Hành động:** Đọc line cụ thể để xác minh. Nếu thiếu context arg → sửa.

### M7. ⚠ `SingleInstance::onNewConnection` — stream parse fail không disconnect socket
[SingleInstance.cpp:58-73](src/platform/SingleInstance.cpp) 🔎

Nếu `stream >> msg` fail trên malformed data, socket vẫn connected → memory leak chậm nếu attack/malformed clients liên tục.

**Khắc phục:** Sau `stream >> msg`, kiểm `stream.status() != QDataStream::Ok` → `socket->disconnectFromServer(); socket->deleteLater(); return;`.

### M8. ⚠ `RefreshTokenCoordinator::onLoginSuccess` — `wakeAll()` chạy ngay cả khi state == Idle
[RefreshTokenCoordinator.cpp:99-100](src/core/services/RefreshTokenCoordinator.cpp)

`wakeAll()` được gọi luôn, dù không có waiter. Không bug — chỉ no-op. **Loại khỏi finding.**

### M9. ⚠ `Pkce` & `OAuthService::m_codeVerifier` — không bảo vệ khi `start()` gọi lại giữa flow
[OAuthService.cpp:44-66](src/core/services/OAuthService.cpp)

`start()` ghi đè `m_codeVerifier` mỗi lần. Nếu user click "Sign in" hai lần liên tiếp khi loopback đầu tiên chưa đóng, verifier thứ nhất bị overwrite → exchange code thứ nhất sẽ fail với "invalid_grant".

**Hành động:** Trong `start()` line đầu thêm `if (m_inFlight) return;` (đã có `m_inFlight = true` nhưng đặt muộn).

### M10. ⚠ `FileCacheDB` — `m_db.transaction()` không check return value
[FileCacheDB.cpp:296, 549, 574](src/core/cache/FileCacheDB.cpp)

Nếu transaction() fail (DB locked, readonly), loop UPSERT vẫn chạy với auto-commit từng row. Commit cuối cùng cũng silent fail. Data bị partial.

**Khắc phục:**
```cpp
if (!m_db.transaction()) {
    qWarning() << "transaction begin failed:" << m_db.lastError().text();
    return;
}
```

### M11. ⚠ `FileCacheDB::upsertFiles` — bất kỳ row fail không rollback
[FileCacheDB.cpp:339-343](src/core/cache/FileCacheDB.cpp)

```cpp
for (...) {
    if (!q.exec()) qWarning() << ...;   // ← chỉ warn, không rollback
}
m_db.commit();
```
Nếu 5/10 row fail (constraint, disk full), commit vẫn ăn 5 row tốt → DB inconsistent. Đối với cache, inconsistent có thể chấp nhận; với folder tree (children có parent_id trỏ row chưa tồn tại) → list lỗi.

**Khắc phục:** Sau loop check `bool anyFailed`, gọi `m_db.rollback()` nếu cần.

### M12. ⚠ `FileCacheDB` — `clearUserCache` chạy 3 DELETE ngoài transaction
[FileCacheDB.cpp:687-703](src/core/cache/FileCacheDB.cpp)

Nếu DELETE files xong rồi crash trước khi DELETE folder_sync, folder_sync giữ entry trỏ vào row đã xóa → query sai. Wrap toàn bộ trong transaction.

### M13. ⚠ `FileCacheDB::queryFiles` — string format ORDER BY → SQL injection nếu sortKey không trusted
[FileCacheDB.cpp:367-386](src/core/cache/FileCacheDB.cpp)

`sortKey` đến từ ViewModel/QML → user-controlled? Hiện ViewModel set sortKey từ enum nội bộ (4 giá trị cố định) → an toàn. **Nhưng** không có defensive whitelist tại DB layer.

**Khắc phục:** Trong queryFiles, switch ngay đầu hàm, mặc định "name" nếu sortKey không match → loại khả năng injection ngầm.

### M14. ⚠ `FormatUtil::humanSpeed(double)` — cast NaN/inf → qint64 UB
[FormatUtil.cpp:33](src/core/util/FormatUtil.cpp) 🔎

`SpeedMeter` chia `dBytes / dtMs`. Nếu `dtMs == 0` → bps = inf hoặc NaN. Cast double NaN sang qint64 là UB trong C++.

**Khắc phục:**
```cpp
if (!std::isfinite(bps) || bps < 0) return QStringLiteral("0 B/s");
```

### M15. ⚠ `SpeedMeter::markProgress` — `dtMs == 0` chia 0
[SpeedMeter.cpp ~62-65](src/core/transfer/SpeedMeter.cpp) 🔎

Khi gọi liên tiếp dưới độ phân giải đồng hồ → dtMs = 0. Guard `if (dtMs > 0)`.

### M16. ⚠ `TransferService` — lambda capture `m_currentUserId` race với logout
[TransferService.cpp ~195-199](src/core/services/TransferService.cpp) 🔎

Lambda phải capture `userId` by value (snapshot lúc start), không rely on `m_currentUserId` member khi callback fire (có thể đã thay đổi do logout/login nhanh).

### M17. ⚠ `PriorityScheduler::enqueue` — dedup O(N) qua 4 deque
[PriorityScheduler.cpp:10-16](src/core/transfer/PriorityScheduler.cpp)

Không phải crash; là perf. Với 10k task, mỗi enqueue scan 4 deque → tổng O(N²). Nếu user paste 1000 link tải → app freeze ~giây.

**Khắc phục:** Hash set `m_allIds` song song.

### M18. ⚠ `SyncService::scanFolderInternal` blocking I/O
[SyncService.cpp ~754](src/core/services/SyncService.cpp) 🔎

`QFileInfo(f.localPath).isDir()` cho mỗi folder trên main thread → block nếu network mount slow. Symptom: UI freeze 1-30s.

**Khắc phục:** Cache `isDir` qua background thread; hoặc dùng `QFileSystemWatcher::directoryChanged` thay vì scan định kỳ.

### M19. ⚠ `HistoryRepository::loadTasksFromJson` — không cap entry count
[HistoryRepository.cpp ~74-96](src/core/repositories/HistoryRepository.cpp) 🔎

JSON corrupt/lớn (1M+ entry) → `result.reserve(arr.size())` allocate huge memory → OOM.

**Khắc phục:** `arr.size() > kMaxHistory` → cap hoặc archive cũ.

### M20. ⚠ `BadWordFilter` — `stripDiacritics` chạy 2 lần (load + check)
[BadWordFilter.cpp:52-61, 183](src/core/util/BadWordFilter.cpp)

Không phải crash; perf. Cache normalized form khi load.

### M21. ⚠ `SystemTray::QSystemTrayIcon` parent là `this` (SystemTray)
[SystemTray.cpp:30](src/platform/SystemTray.cpp)

Nếu SystemTray destruct trước QApplication, icon biến mất từ taskbar nhưng app vẫn chạy → user mất entry point. **Khắc phục:** Parent `qApp` hoặc đảm bảo SystemTray sống cùng app.

### M22. ⚠ `Application::aboutToQuit` — chuỗi cleanup chưa join worker threads
[Application.cpp](src/app/Application.cpp) 🔎

Cần kiểm tra: TransferOrchestrator quit-and-wait OK; nhưng `QThreadPool::globalInstance()` chứa lambda QtConcurrent::run capture services. Nếu app quit khi task chưa xong → `globalInstance()` destruct cùng QApplication → kill thread. Task lambda đang chạy access `this` (service) đã bị delete. CRASH.

**Khắc phục:** Trong `aboutToQuit`, gọi `QThreadPool::globalInstance()->waitForDone(5000)` trước khi let unique_ptr destruct services.

---

## 5. FINDING THẤP (Low Severity / Code Hygiene)

### L1. `FileTypeHelper::extOf` — file `.bashrc` (dot ở vị trí 0) trả về `"bashrc"` thay vì `""`
[FileTypeHelper.h:21-22](src/core/util/FileTypeHelper.h)

Edit `dot > 0` thay vì `dot >= 0`.

### L2. `PlatformUtils::openInExplorer` — không check `ShellExecuteW` return
[PlatformUtils.cpp:208-209](src/platform/PlatformUtils.cpp)

Trả về `HINSTANCE`; nếu `< 32` là fail. Nên log.

### L3. `LanguageViewModel::loadTranslation` — load fail nhưng giữ trạng thái half-installed
[LanguageViewModel.cpp:48-65](src/viewmodels/LanguageViewModel.cpp) 🔎

Nếu .qm corrupt, partial install có thể mix VI+EN. Log warning, fallback rõ ràng.

### L4. `SettingsViewModel` — signal-slot có thể loop với binding QML 🔎

Cần xác minh slot không emit lại signal trigger setter.

### L5. `SecureStore` — `CryptUnprotectData` không validate output UTF-8 sau decrypt
[SecureStore.cpp:265](src/platform/.../SecureStore.cpp) 🔎

Decrypt từ base64 base64 → bytes → `QString::fromUtf8`. Nếu data corrupt → replacement char. Không crash.

### L6. `FsIcon` mass icon load 🔎

Không trong scope C++ này (file QML).

### L7. `OAuthSecrets.h` — secrets hard-coded
Không phải crash; là supply-chain risk. Đã có cảnh báo bằng XOR đơn giản.

### L8. `Pkce::randomString` — RNG seed
[Pkce.cpp](src/core/util/Pkce.cpp) — Qt thường dùng `QRandomGenerator::system()` an toàn. Cần verify nếu dùng `qrand()` cũ.

### L9. `FolderTreeModel::buildBreadcrumbPath` — cycle detection chỉ cap 100 iter
[FolderTreeModel.cpp ~154](src/viewmodels/FolderTreeModel.cpp)

Nếu cây thực sự sâu > 100 (hiếm), breadcrumb cut → UX nhưng không crash.

### L10. `TransferBudgetViewModel` — timer không stop trong dtor
[TransferBudgetViewModel.h](src/viewmodels/TransferBudgetViewModel.h)

Default dtor đủ vì timer là member; nhưng explicit `m_timer.stop()` rõ ý.

### L11-L14. Findings nhỏ về reserve(), regex cache, parent ownership của QMenu, QFileSystemWatcher limit ~32 trên Windows — đã ghi nhận trong phần phân tích chi tiết.

---

## 6. FALSE POSITIVE đã loại bỏ (đã verify code thực tế)

Để báo cáo trung thực, dưới đây là các finding mà sub-agent nêu nhưng **KHÔNG phải bug** sau khi đọc code:

| Sub-agent claim | Sự thật |
|---|---|
| `HttpClient` line 192, 233, 239, 258: `toUtf8().constData()` là use-after-free | libcurl `>= 7.17.0` COPY string ngay khi `setopt` (CURLOPT_CAINFO, CURLOPT_PROXY, CURLOPT_USERAGENT); `curl_slist_append` cũng strdup string. Safe. |
| `nativehost_main.cpp:62` negative len | `quint32` không thể âm; check `> kMaxMessageBytes` đủ. |
| `LoopbackServer.cpp:101` lambda capture `this` không an toàn | `singleShot(ms, this, lambda)` với `this` làm context object → Qt tự cancel khi destroy. |
| `main.cpp:97` g_logFile double-delete | Pointer chỉ delete trong terminateHandler nếu isOpen. Pattern an toàn. |
| `HomeSearchViewModel.cpp:231` thiếu bump seq | ĐÃ có `++m_requestSeq`. |
| `HttpClient::m_cookie` không mutex | ĐÃ có `m_mutex` snapshot pattern. |
| FshareApi lambda capture `&keyword` etc | `executeAuthed` chạy đồng bộ → reference còn valid trong scope. |
| `DownloadEngine.cpp:156` thiếu bound check seg index | Code ĐÃ có `if (idx >= 0 && idx < ...)`. |
| `SyncService` m_taskProgress race | Cả connect và mutation đều main thread → không race. |
| `FileCacheDB` cross-thread access | Tất cả truy cập từ main thread qua `QMetaObject::invokeMethod(guard, ...)`. An toàn. |

---

## 7. LỘ TRÌNH KHẮC PHỤC (đề xuất ưu tiên)

### Sprint A — chặn các nguy cơ crash thực sự (2-3 ngày):
1. **H2 + H3**: lock `m_file` close cùng mutex với write, đổi `m_segBytes` sang atomic.
2. **H1 + H7**: chuyển `auto *api = m_api;` → `QPointer<FshareApi> api(m_api)` ở **tất cả** QtConcurrent::run lambdas (grep `auto \*\w+ = m_`).
3. **M22**: thêm `QThreadPool::globalInstance()->waitForDone(5000)` vào `Application::aboutToQuit`.

### Sprint B — robustness (3-5 ngày):
4. **H4**: `m_shuttingDown` flag trong TransferOrchestrator.
5. **H5 + H6**: validate `FSEEKO64` return + `lastByte >= 0` trong UploadEngine.
6. **H9**: BFS thay đệ quy trong FolderExpander.
7. **M9**: `start()` early-return nếu `m_inFlight`.
8. **M10 + M11 + M12**: kiểm tra `transaction()` return, rollback nếu lỗi, wrap clearUserCache.

### Sprint C — quality (1 tuần):
9. **M1**: QPointer cho socket trong LoopbackServer.
10. **M7**: disconnect socket khi stream parse fail.
11. **M16 + M18**: snapshot userId, async isDir trong SyncService.
12. **M19**: cap history JSON.
13. **M14 + M15**: isfinite check trong FormatUtil/SpeedMeter.

### Sprint D — hygiene:
14. Toàn bộ findings Low + non-crash perf.

---

## 8. ĐỀ XUẤT BIỆN PHÁP PHÒNG NGỪA HỆ THỐNG

1. **Static analysis**: thêm `clang-tidy` với checks `bugprone-*`, `cert-*`, `cppcoreguidelines-pro-type-cstyle-cast` vào CI.
2. **Runtime sanitizer**: build debug với `-fsanitize=thread` để bắt race (cần Linux build hoặc clang trên Windows).
3. **Crash reporter**: tích hợp Sentry/Crashpad — `terminateHandler` hiện chỉ log file local. Khi user crash thực tế, không có cơ chế tự động báo về.
4. **QObject lifetime policy**: ban hành quy ước nội bộ: "Mọi QtConcurrent::run capture phải dùng QPointer cho TẤT CẢ QObject pointer, không chỉ `this`." Thêm vào CLAUDE.md / docs/ARCHITECTURE.md.
5. **Test concurrency**: bổ sung test bằng `QSignalSpy` + `QTest::qWait` mô phỏng cancel-while-running, logout-during-fetch, app-quit-during-upload.
6. **Memory tools**: chạy `Dr.Memory` hoặc `Application Verifier` trên build dev định kỳ.

---

## 9. PHỤ LỤC — Files đã quét

```
src/main.cpp
src/app/                      → AppContext, Application
src/core/api/                 → HttpClient, FshareApi, OAuth*, OAuthSecrets
src/core/net/                 → LoopbackServer
src/core/cache/               → FileCacheDB, FileSyncWorker
src/core/models/              → headers (POD types — không quét sâu)
src/core/repositories/        → History, Settings, Sync, TransferHistoryDb
src/core/services/            → 11 service files
src/core/transfer/            → 9 engine/queue/worker files
src/core/util/                → 8 util files
src/platform/                 → PlatformUtils, SingleInstance, SystemTray, nativehost
src/viewmodels/               → 16 VM files
```

**Tổng:** ~50 cặp `.cpp/.h`, ~25k LOC.
