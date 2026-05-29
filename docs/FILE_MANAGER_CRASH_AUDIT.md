# Báo cáo Audit Crash — Chức năng File Manager / My Files

> ## ✅ CLOSURE — Đối chiếu lại với code ngày 2026-05-29
>
> | ID | Trạng thái | Ghi chú |
> |---|---|---|
> | FM-H1 | ✅ FIXED | `setSelected()` đã là `Q_INVOKABLE` trong `FileManagerViewModel.h` + `FavoritesViewModel.h` |
> | FM-H2 | ✅ FIXED | `FileCacheService` nhánh SingleShot đã dùng `QPointer guard` + `if (!guard) return` |
> | FM-H3 | ✅ FIXED | `FolderTreeModel::rebuildVisible` đã có `visited` set + `kMaxDepth` + cảnh báo cycle |
> | FM-M1/M2/M3, L1/L2 | ✅ NOT-A-BUG | Đã xác nhận lại đúng như mục 5 bên dưới |
> | FM-M4 | ⬜ OPEN | Context menu `mapToGlobal` vs `mapToItem` lệch tọa độ (UX, không crash) → [BACKLOG.md](BACKLOG.md) |
> | FM-M5 | ⬜ OPEN | `m_settingsInFlight` leak nếu request settings fail/timeout không emit signal → BACKLOG (data-consistency) |
>
> **Toàn bộ 3 finding HIGH (crash) đã được fix.** Phần điều tra gốc bên dưới giữ nguyên làm tham chiếu.

**Ngày:** 2026-05-28
**Phạm vi:** Hành vi crash app khi click vào file/folder trong trang File Manager (My Files).
**Phương pháp:** Đọc trực tiếp các file `.cpp/.h/.qml` liên quan + verify từng claim bằng Grep/Read, KHÔNG dựa thuần vào sub-agent (đã bị false positive cao trong audit trước).
**Tham chiếu:** Báo cáo tổng thể đã có ở [CRASH_AUDIT.md](CRASH_AUDIT.md) (47 findings, 2026-05-26). File này chỉ tập trung vào path **click file/folder** và **không lặp lại** các finding đã ghi ở đó.

---

## 1. Tóm tắt nguyên nhân khả năng cao

Khi user click vào file/folder trong File Manager, crash có thể xảy ra do **3 nhóm nguyên nhân chính (xếp theo xác suất giảm dần)**:

| # | Vùng | Triệu chứng người dùng | Mức |
|---|---|---|---|
| A | **QML gọi method C++ không tồn tại (`setSelected`)** trên `FileManagerViewModel` / `FavoritesViewModel` | App có thể không crash nhưng phun TypeError mỗi keypress / mouse click, **một số signal/binding bị dừng giữa chừng** → state QML và VM lệch nhau, hậu quả tiếp theo khó debug (vd. delete linkcode đã không còn highlight, copy link rỗng…) | **HIGH** |
| B | **Race khi click rapid 2 folder liên tiếp** trong cold-start (chưa có `m_userId`) → `SingleShotConnection` capture `[this]` không có `QPointer` | Crash chính xác lúc `FileService::fileListLoaded` emit về `FileCacheService` đã destruct (vd. user nhấp folder → ngay lập tức logout / quit) | **HIGH** |
| C | **FolderTreeModel::rebuildVisible** dùng lambda đệ quy DFS, không cap depth, không phát hiện cycle | Nếu server trả về metadata cây folder bị self-reference (parentId trỏ chính nó) hoặc rất sâu, stack overflow → crash đúng lúc người dùng click sidebar / nav sang folder mới | **HIGH** |

Ngoài ra còn một số rủi ro nhỏ hơn (xem mục 4).

---

## 2. Chuỗi sự kiện khi user click vào folder/file

```
QML click on folder row (FileManagerPage.qml:729-735, 816-822)
  → page.selectedFiles = [linkcode]
  → fileManagerViewModel.navigateTo(linkcode, name)
       ├─ m_history.append, m_histIdx++
       ├─ emit currentFolderChanged
       ├─ emit historyChanged
       └─ reloadCurrentFolder()
              └─ m_service->listFiles(fid, sortKey, sortAsc, typeFilter)
                     └─ FileCacheService::listFiles
                            ├─ m_currentFolderId = folderId
                            ├─ IF không có cache/userId:
                            │     connect(SingleShotConnection)  ← ⚠ B
                            │     m_service->listFiles(folderId)
                            │           └─ QtConcurrent::run → api->listFiles
                            │                 └─ invokeMethod main → emit fileListLoaded
                            ├─ emitCurrentFolderFromCache()        ← path bình thường
                            │     └─ m_db->queryFiles → emit fileListLoaded
                            └─ m_sync->enqueueFolder(...)
                                  └─ Orchestrator → dispatchReady → startCrawl
                                        └─ QtConcurrent::run → batch loop
                                              └─ invokeMethod main → upsertFiles → emit firstBatchSynced
                                                     └─ FileCacheService → emitCurrentFolderFromCache 2 lần nữa
                                                            └─ VM cập nhật FileListModel (digest skip duplicate)
```

`navigateTo` chạy đồng bộ → emit signal → reloadCurrentFolder → listFiles → trong cùng main thread một loạt callback fire. **Vấn đề:** mỗi click có thể fire 3–4 lần `emit fileListLoaded` cho cùng folderId (cache lần 1, firstBatchSynced, folderSynced) — VM có digest fingerprint để dedup, nhưng nếu digest CHỉ kiểm cho cùng folderId hiện tại và user navigate nhanh sang folder khác giữa chừng, có thể emit cho folder cũ trong khi UI đã ở folder mới (đã có check `if (folderId == self->m_currentFolderId)` tại FileCacheService.cpp:85, 92 — OK).

---

## 3. Findings ưu tiên — vùng File Manager

### FM-H1. ⚠ QML gọi `setSelected()` không tồn tại trong ViewModel (HIGH)

**File:** [FileManagerPage.qml:237](qml/Fshare/Pages/FileManagerPage.qml#L237), [FavoritesPage.qml:194](qml/Fshare/Pages/FavoritesPage.qml#L194)

```qml
function _selectByListIndex(idx) {
    ...
    if (fileManagerViewModel)
        fileManagerViewModel.setSelected([it.linkcode]);   // ← method KHÔNG tồn tại
}
```

**Xác minh:**
- `Grep("Q_INVOKABLE.*set[Ss]elect|void setSelected")` trên `src/` → **không match**.
- Header `FileManagerViewModel.h` chỉ có `toggleSelection`, `selectAll`, `clearSelection`. Không có `setSelected`.

**Hậu quả:**
- Trên Qt6, gọi method không tồn tại trên QObject từ QML JS → `TypeError: Property 'setSelected' of object FileManagerViewModel is not a function`.
- Trong context handler `Keys.onPressed` / `MouseArea.onClicked`, exception bị engine catch và in lên console. **Bản thân không crash app**, NHƯNG:
  1. Mỗi mũi tên Up/Down/Home/End/PageUp/PageDown user nhấn đều phun lỗi → log spam.
  2. Selection state ở C++ (`m_selected` trong VM) KHÔNG được đồng bộ với selection QML (`page.selectedFiles`). Các thao tác bulk dựa vào `selectedLinkcodes` từ VM (vd. Ctrl+A, deleteSelected) sẽ thấy selection rỗng dù QML đang highlight nhiều row → user click "Xoá" trong context menu lấy linkcode từ QML (OK), nhưng click toolbar "Xoá đã chọn" → VM thấy rỗng → no-op.

**Khắc phục — phương án 1 (recommended):** Thêm `Q_INVOKABLE` `setSelected(const QStringList &)` vào `FileManagerViewModel`:

```cpp
// FileManagerViewModel.h
Q_INVOKABLE void setSelected(const QStringList &linkcodes);

// FileManagerViewModel.cpp
void FileManagerViewModel::setSelected(const QStringList &linkcodes)
{
    QSet<QString> next(linkcodes.cbegin(), linkcodes.cend());
    if (next == m_selected) return;
    m_selected = std::move(next);
    emit selectionChanged();
}
```

**Phương án 2:** Bỏ dòng QML `fileManagerViewModel.setSelected(...)` nếu thực sự không cần đồng bộ — nhưng khi đó cần audit lại toàn bộ flow Ctrl+A / deleteSelected / selectedCount để chắc chắn không bị lệch.

### FM-H2. ⚠ `SingleShotConnection` capture `[this]` không có QPointer khi cache chưa sẵn sàng (HIGH)

**File:** [FileCacheService.cpp:287-291](src/core/services/FileCacheService.cpp#L287)

```cpp
if (!m_db->isOpen() || m_userId.isEmpty()) {
    connect(m_service, &FileService::fileListLoaded,
            this, [this](const QVector<FileItem> &files) {   // ← [this] raw
                emit fileListLoaded(files);
            }, Qt::SingleShotConnection);
    m_service->listFiles(folderId);
    return;
}
```

**Kịch bản crash:**
1. App cold start, user click vào File Manager trước khi OAuth refresh hoàn tất → `m_userId.isEmpty()` = true.
2. Code đi vào nhánh SingleShot, post async call.
3. User logout/quit trong khi response chưa về (1-3 giây).
4. `FileCacheService` destruct.
5. `FileService::fileListLoaded` emit (do `QtConcurrent::run` lambda hoàn tất sau cùng).
6. Qt dispatch lambda với `[this]` dangling → `emit this->fileListLoaded(...)` → **CRASH**.

Lambda `[this]` không có QPointer guard. `Qt::SingleShotConnection` chỉ disconnect SAU khi fire một lần — không bảo vệ khỏi việc receiver bị destroy trước khi fire.

**Khắc phục:**
```cpp
QPointer<FileCacheService> guard(this);
connect(m_service, &FileService::fileListLoaded,
        this, [guard](const QVector<FileItem> &files) {
            if (!guard) return;
            emit guard->fileListLoaded(files);
        }, Qt::SingleShotConnection);
```

**Lưu ý:** Pattern tương tự không tồn tại trong các nhánh QtConcurrent::run khác của file (loadFolderTree, searchFiles, getDownloadUrl) — chúng đã có QPointer. Chỉ duy nhất nhánh fallback SingleShot này thiếu.

### FM-H3. ⚠ `FolderTreeModel::rebuildVisible` đệ quy không cap, không cycle-detect (HIGH)

**File:** [FolderTreeModel.cpp:140-171](src/viewmodels/FolderTreeModel.cpp#L140), [FolderTreeModel.cpp:149-158](src/viewmodels/FolderTreeModel.cpp#L149)

```cpp
auto addSubtree = [&](auto &self, int idx, int depth) -> void {
    if (idx < 0 || idx >= m_all.size()) return;
    m_visible.append(idx);
    m_visibleDepths.append(depth);
    auto it = m_childrenOfParent.find(m_all[idx].linkcode);
    if (it == m_childrenOfParent.end()) return;
    for (int childIdx : it.value())
        self(self, childIdx, depth + 1);           // ← đệ quy DFS không cap
};
```

**Kịch bản crash:**
1. Server (hoặc cache DB bị tampered) trả về 1 folder có `linkcode == parentId` (self-loop): `{linkcode: "A", parentId: "A"}`.
2. `addSubtree("A", 0)` → `m_childrenOfParent["A"]` chứa idx của chính A → recurse → vô tận → **stack overflow → crash**.

Khác với `buildBreadcrumbPath` (line 179: `int safety = 100;`), `rebuildVisible` KHÔNG có cap. Mọi lần FolderTreeModel reset (sau folderTreeLoaded, sau folderSynced, sau API listFolders) đều chạy lại DFS này.

**Triggering từ click:** Click vào folder → `loadFolderTree` enqueue background sync → khi xong → `folderTreeLoaded` emit → VM gọi `folderTreeModel->resetFolders(folders)` → nếu folders có cycle → `rebuildVisible` → SO.

**Khắc phục:**
```cpp
auto addSubtree = [&, visited = QSet<QString>()](auto &self, int idx, int depth) mutable -> void {
    if (idx < 0 || idx >= m_all.size() || depth > 64) return;
    const QString &lc = m_all[idx].linkcode;
    if (visited.contains(lc)) {
        qWarning() << "[FolderTreeModel] cycle detected at" << lc;
        return;
    }
    visited.insert(lc);
    m_visible.append(idx);
    m_visibleDepths.append(depth);
    auto it = m_childrenOfParent.find(lc);
    if (it == m_childrenOfParent.end()) return;
    for (int childIdx : it.value())
        self(self, childIdx, depth + 1);
};
```

Hoặc đổi sang BFS với `QQueue` heap-allocated như đề xuất H9 trong CRASH_AUDIT.md (cùng pattern fix cho FolderExpander).

---

## 4. Findings phụ — mức trung bình / thấp

### FM-M1. `FileListModel::data()` an toàn nhưng `m_localPaths` snapshot có thể stale

**File:** [FileListModel.cpp:18-49](src/viewmodels/FileListModel.cpp#L18)

`m_localPaths.value(f.linkcode)` được đọc trong `data()` (gọi từ QML render). Cùng main thread → không race. Tuy nhiên `setLocalPaths()` thay thế toàn bộ `m_localPaths`, sau đó `emit dataChanged(index(0), index(size-1), ...)`. Nếu giữa lúc `setLocalPaths` chạy mà QML đang đọc `data()` cho row k, vì cùng thread duy nhất → không xảy ra. **OK, KHÔNG bug.**

### FM-M2. `enrichWithLocalPaths()` xóa local file trong khi đang iter QHash (verify lại)

**File:** [FileManagerViewModel.cpp:589-596](src/viewmodels/FileManagerViewModel.cpp#L589)

```cpp
for (auto it = paths.begin(); it != paths.end(); ) {
    if (!QFile::exists(it.value())) {
        m_service->removeLocalFile(it.key());    // gọi vào DB
        it = paths.erase(it);                    // erase từ local copy paths
    } else {
        ++it;
    }
}
```

`paths` là `QHash<QString, QString>` local (trả về từ `getLocalPaths`). `removeLocalFile(it.key())` đi vào DB (`m_db->removeLocalFile`) — KHÔNG sửa `paths`. `erase(it)` sửa `paths`. `it.key()` được dùng TRƯỚC `erase` nên ko UB. **OK.**

Tuy nhiên có rủi ro nhỏ: nếu một file lớn 10K rows mà 50% bị stale (đã xóa khỏi disk), 5000 lần `m_db->removeLocalFile` sync trên main thread → **block UI khi navigate sang folder lớn** (chưa crash, là perf).

**Khắc phục:** batch remove:
```cpp
QStringList toRemove;
for (auto it = paths.begin(); it != paths.end(); ) {
    if (!QFile::exists(it.value())) {
        toRemove << it.key();
        it = paths.erase(it);
    } else ++it;
}
if (!toRemove.isEmpty()) m_service->removeLocalFiles(toRemove); // thêm batch API
```

### FM-M3. `FileMoveCopyDialog` tham chiếu `folderTreeModel` raw

**File:** [FileManagerPage.qml:1678](qml/Fshare/Pages/FileManagerPage.qml#L1678)

```qml
FileMoveCopyDialog {
    folderTreeModel: fileManagerViewModel ? fileManagerViewModel.folderTreeModel : null
```

`folderTreeModel` là child của VM (QObject parent), expose dưới CONSTANT. An toàn vì VM lifetime ≥ dialog lifetime (cả 2 do AppContext quản lý). **OK, KHÔNG bug.**

### FM-M4. Right-click context menu — `mapToGlobal` vs `mapToItem` không nhất quán

**File:** [FsFileMediumCard.qml:164](qml/Fshare/Components/FsFileMediumCard.qml#L164), [FileManagerPage.qml:839](qml/Fshare/Pages/FileManagerPage.qml#L839)

```qml
// FsFileMediumCard.qml
onClicked: (mouse) => {
    if (mouse.button === Qt.RightButton) {
        const p = root.mapToGlobal(mouse.x, mouse.y);    // ← global tọa độ
        root.contextMenuRequested(p.x, p.y);
    }
}

// FileManagerPage.qml — receiver
onContextMenuRequested: (x, y) => {
    const pt = medDelegate.mapToItem(page, x, y);        // ← treat x,y as local-to-delegate (?)
    fileMenu.popup(pt.x, pt.y);
}
```

`mapToGlobal` trả về tọa độ MÀN HÌNH, nhưng `medDelegate.mapToItem(page, x, y)` mong x,y theo hệ delegate. Đây là **mismatch tính toán** → context menu popup sai vị trí (lệch theo offset của cửa sổ). Không crash, là UX bug.

**Khắc phục:** Trong FsFileMediumCard.qml dùng `mapToItem(null, ...)` hoặc đơn giản truyền raw `mouse.x, mouse.y` rồi receiver tự `mapToItem(page, x, y)` (đúng vì lúc đó x,y là local của delegate).

### FM-M5. `getFileInfo` qua context menu có thể double-fire khi click context "Đặt mật khẩu..." rồi cancel

**File:** [FileCacheService.cpp:444](src/core/services/FileCacheService.cpp#L444)

```cpp
void FileCacheService::getFileInfo(...) { ++m_settingsInFlight; m_service->getFileInfo(url); }
```

Pattern `++m_settingsInFlight` không bị balance nếu request fail timeout (FileService không emit operationComplete/Failed nào vì exception/timeout). Sau N lần thất bại, `m_settingsInFlight > 0` mãi → tiếp theo `onWriteOperationComplete` (do delete/rename) sẽ "ăn" event này (vì check `if (m_settingsInFlight > 0) --m_settingsInFlight; return;`) → write op KHÔNG trigger refresh → user thấy file đã rename nhưng list không cập nhật. **Không crash, là data inconsistency.**

**Khắc phục:** Thêm timeout/cap cho `m_settingsInFlight` hoặc dùng counter per-request-id thay vì global int.

### FM-L1. `FileListModel::setLocalPaths` emit dataChanged khi `m_items` rỗng

**File:** [FileListModel.cpp:218-223](src/viewmodels/FileListModel.cpp#L218)

```cpp
m_localPaths = paths;
if (!m_items.isEmpty())
    emit dataChanged(index(0), index(m_items.size() - 1), { LocalPathRole, IsDownloadedRole });
```

Đã có guard `!m_items.isEmpty()` → OK. **KHÔNG bug.**

### FM-L2. `FileListModel::data()` không lock — nhưng QAbstractListModel never crosses thread (OK)

`data()` chỉ được gọi từ QML render trên main thread. `resetItems` cũng chạy main thread. Không race.

---

## 5. Tài sản đã xác minh KHÔNG phải bug (chống false-positive)

| Sub-agent / linh cảm ban đầu | Sự thật |
|---|---|
| `FshareApi::listFiles` capture `[this, &folderUrl, ...]` → use-after-free khi retry | `executeAuthed` chạy `fn()` đồng bộ trong cùng stack frame của caller; `folderUrl` ref còn sống suốt (xem [FshareApi.h:106-131](src/core/api/FshareApi.h#L106)). **KHÔNG bug.** (Cũng đã ghi trong CRASH_AUDIT.md M2.) |
| `parseFileItem` trên item không phải Json::Object sẽ throw | JsonCpp `item.get("key", default)` trả về default khi key missing hoặc item là arrayValue/nullValue. KHÔNG throw, KHÔNG crash. |
| `m_db` (FileCacheDB) bị truy cập từ thread khác trong FileSyncWorker | Tất cả truy cập đi qua `QMetaObject::invokeMethod(guard, ...)` về main thread. **An toàn.** |
| `setSelected` không tồn tại → crash | Không crash hard, chỉ TypeError console + selection lệch (chuyển thành FM-H1). |
| `FsContextMenu.popup` với linkcode không còn (file đã bị xóa giữa chừng) | Action handler khi click sẽ gọi `m_service->deleteFiles(["dead_lc"])` → server 404 → operationFailed → toast lỗi. **KHÔNG crash.** |

---

## 6. Lộ trình khắc phục

### Sprint 1 (P0 — chặn crash thực sự, 1-2 ngày):

1. **FM-H1** — Thêm `Q_INVOKABLE setSelected()` vào `FileManagerViewModel` + `FavoritesViewModel`. Verify Ctrl+A và deleteSelected hoạt động đúng.
2. **FM-H2** — Thay `[this]` thành `[guard]` với QPointer trong FileCacheService.cpp:287. Audit grep toàn bộ codebase tìm pattern tương tự (`connect(.*, this, \[this\]`).
3. **FM-H3** — Thêm `visited` set + depth cap 64 vào `FolderTreeModel::rebuildVisible` lambda. Log warning khi phát hiện cycle. Lý tưởng nhất là sửa cả `FolderExpander::crawl` cùng pattern (đã có H9 trong CRASH_AUDIT.md).

### Sprint 2 (P1 — UX và data correctness, 2-3 ngày):

4. **FM-M4** — Sửa mismatch `mapToGlobal` vs `mapToItem` trong FsFileMediumCard context menu.
5. **FM-M5** — Refactor `m_settingsInFlight` thành counter có timeout, hoặc tốt hơn là phân tách signal cho settings ops khỏi write ops (tách `settingsComplete` / `settingsFailed`).
6. Thêm log + telemetry quanh `FileCacheService::emitCurrentFolderFromCache` và `enqueueFolder` để khi crash xảy ra ở user thực tế, log đủ để truy nguyên folderId / userId / call stack.

### Sprint 3 (P2 — defensive hygiene):

7. Audit toàn bộ ViewModel khác (HomeSearchViewModel, RemoteShareViewModel) tìm pattern `setSelected` hoặc tương tự gọi method không tồn tại từ QML.
8. Thêm CI check: viết script grep `QML.*\.setX\(` rồi cross-check với `Q_INVOKABLE` trong `.h` để bắt slot/method missing trước khi merge.
9. Tích hợp Crashpad/Sentry để khi user trên prod crash, có call stack đầy đủ thay vì phải đoán từ log file.

---

## 7. Test plan để verify (manual)

| Test case | Bước | Kỳ vọng sau fix |
|---|---|---|
| **A** (FM-H1) | Cold start → login → File Manager → arrow Down 5 lần → Ctrl+A → click "Xoá đã chọn" trên toolbar | Tất cả file trong folder đều bị xoá; console không có TypeError |
| **B** (FM-H2) | Cold start → ngay khi splash biến mất, click vào "My Files" trước khi sidebar load → quit app trong 1 giây | App quit clean, không crash report trong Event Viewer / log file |
| **C** (FM-H3) | Tạm tampered: edit DB cache (`filecache.db`) UPDATE files SET parent_id=linkcode WHERE linkcode='X'. Restart app → mở File Manager → click vào sidebar folder X | Console log "[FolderTreeModel] cycle detected at X", UI không crash, folder X hiển thị nhưng không expand được |
| **D** (FM-M4) | Chuyển view sang "Medium icons" → right-click vào card ở góc dưới phải | Context menu hiện đúng dưới chuột (không bị lệch ra ngoài cửa sổ) |
| **E** | Click vào folder có 10K+ file ngay sau khi login → ngay lập tức click sang folder khác | Không crash; folder mới hiển thị đúng list của nó (không bị files của folder cũ) |
| **F** | Click rapid 10 folder liên tiếp trong 2 giây | Không crash, log không có "QPointer null"; cuối cùng list khớp với folder cuối user click |

---

## 8. Khuyến nghị tổng thể

1. **Bug class quan trọng nhất trong codebase này** là **lambda capture raw `this` / raw pointer service** (đã được phản ánh trong CRASH_AUDIT.md H1, H7 + FM-H2 trên đây). Đề xuất ban hành nội bộ:
   > **Mọi lambda chạy async (QtConcurrent::run, SingleShotConnection, QTimer::singleShot) phải capture QPointer cho TẤT CẢ QObject pointer, không chỉ `this`.** Thêm vào `CLAUDE.md` / coding guide.

2. **Đệ quy không cap** (FM-H3 + H9 cũ) cần một sweep: grep `auto\s+\w+\s*=\s*\[&\]\s*\(auto\s*&` để tìm các generic recursive lambda khác.

3. **Method bridge QML ↔ C++** dễ bị thiếu khi rename/move. Đề xuất:
   - Mỗi VM kèm comment `// QML invokes: foo(), bar(), baz()` đầu file để self-document.
   - CI script kiểm `Q_INVOKABLE` matching giữa `.h` và call site trong `.qml`.

4. **Crash reporter** (đã đề trong CRASH_AUDIT.md mục 8) — ưu tiên cao. Hiện `terminateHandler` chỉ ghi log local; user crash không có cơ chế tự gửi report về dev.

5. **Tham chiếu thêm**:
   - [CRASH_AUDIT.md](CRASH_AUDIT.md) — 47 findings tổng thể, đặc biệt H1 (QtConcurrent + QPointer), H9 (đệ quy FolderExpander), M22 (waitForDone trong aboutToQuit).
   - [FileManagerViewModel.cpp:550-563](src/viewmodels/FileManagerViewModel.cpp#L550) (`reloadCurrentFolder`) — chain logic cốt lõi.
   - [FileCacheService.cpp:275-306](src/core/services/FileCacheService.cpp#L275) (`listFiles`) — choke point.

---

## 9. Phụ lục — File đã đọc trực tiếp

- `src/viewmodels/FileManagerViewModel.{cpp,h}` (601+243 LOC)
- `src/viewmodels/FileListModel.cpp` (240 LOC)
- `src/viewmodels/FolderTreeModel.cpp` (192 LOC)
- `src/core/services/FileCacheService.{cpp,h}` (534+175 LOC)
- `src/core/services/FileService.cpp` (223 LOC)
- `src/core/cache/FileSyncWorker.cpp` (291 LOC)
- `src/core/cache/FileCacheDB.cpp` (đọc parseJson, queryFiles, upsertFiles)
- `src/core/api/FshareApi.cpp` (parseJson, parseFileItem, listFiles)
- `src/core/api/FshareApi.h` (executeAuthed template)
- `src/app/AppContext.h` (destruct order)
- `qml/Fshare/Pages/FileManagerPage.qml` (1984 LOC — đọc phần lớn handlers)
- `qml/Fshare/Components/FsContextMenu.qml`
- `qml/Fshare/Components/FsFileMediumCard.qml`

**Tổng:** ~4500 LOC đã đọc trực tiếp + cross-reference với CRASH_AUDIT.md.
