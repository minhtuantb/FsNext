# Sync vs Auto Upload — Phân tích & Đề xuất Refactor

**Tác giả:** Code review, 2026-05-14
**Phạm vi:** `qml/Fshare/Pages/{SyncPage,AutoUploadPage}.qml`, các dialog watch-folder dùng chung, `SyncViewModel`, `SyncService`

---

## 1. TL;DR

**`AutoUploadPage` là DEAD CODE.** Trên surface có vẻ trùng lặp với `SyncPage`, nhưng đào sâu thì nó **không reachable** từ runtime — chưa bao giờ được wire-up:

- ViewModel `autoUploadViewModel` mà page tham chiếu **không tồn tại** trong C++ (AppContext chỉ đăng ký `syncViewModel`).
- Không có Loader / Component / nav nào trong `Main.qml` route tới `AutoUploadPage`.
- Page dùng API hoàn toàn khác với `SyncViewModel` thực tế: `enabled`, `totalPending`, `pauseFolder`, `rescanFolder`, `retryFolder`, `activity`, `updateFolderSettings`…

**Story bên dưới**, dựa trên `docs/auto-upload-ui-design.md` (943 dòng spec):

1. Team **thiết kế feature "Đồng bộ tự động" trước** với ambition lớn (6 trạng thái folder, activity feed, master toggle, per-folder speed limit, dry-run preview, sidebar badge…).
2. Build **UI (AutoUploadPage + 3 dialog)** theo spec.
3. Lúc implement backend, **scope co lại đáng kể** → ra `SyncService` (max 5 folder, recursive cứng, speed limit cứng 5MB/s, skip-list cứng, không có pause/retry/activity log).
4. Để có gì show được, dựng `SyncPage` mới đơn giản hơn (comment ở đầu file thừa nhận: *"Matches handoff … **adapted for the one-way backup model the VM actually implements**"*).
5. AutoUploadPage không xoá → trở thành mồ chôn design.

→ **Kết quả hiện tại:** Code rotting + 3 dialog đã build có nhiều field UI **không persist được** (ignore patterns, watch subfolders, speed limit per folder — backend bỏ qua).

---

## 2. Tài liệu chứng minh

### 2.1 AppContext không đăng ký autoUploadViewModel

```cpp
// src/app/AppContext.cpp:194
ctx->setContextProperty(QStringLiteral("syncViewModel"), m_syncVM.get());
// (Không có dòng nào set "autoUploadViewModel")
```

```bash
$ rg 'autoUploadViewModel' src/
# Zero matches in src/. Only AutoUploadPage.qml references it.
```

### 2.2 Main.qml route currentPage=2 → SyncPage

```qml
// qml/Main.qml:393
Component { id: syncPageComp; SyncPage {} }

// qml/Main.qml:303-307
Loader {
    active: currentPage === 2
    sourceComponent: active ? syncPageComp : null   // ← SyncPage, không phải AutoUploadPage
}
```

AutoUploadPage **chỉ xuất hiện 1 lần** ngoài chính nó: `qml/Fshare/Pages/qmldir:9` — chỉ là registration trong qmldir, không có code nào instantiate.

### 2.3 API surface mismatch giữa AutoUploadPage và SyncViewModel hiện có

| AutoUploadPage gọi (theo design) | SyncViewModel hiện có | Trạng thái |
|---|---|---|
| `autoUploadViewModel.enabled` (master toggle) | ❌ | Vắng |
| `autoUploadViewModel.totalPending` | ❌ | Vắng |
| `autoUploadViewModel.folders.get(i).status` (enum 6 states) | Chỉ có `enabled` bool | Vắng |
| `pauseFolder(id)` | ❌ (chỉ có `setEnabled(id, false)` — semantically khác) | Vắng |
| `resumeFolder(id)` | ❌ | Vắng |
| `rescanFolder(id)` | ✓ `scanNow(id)` | OK (đổi tên) |
| `retryFolder(id)` | ❌ | Vắng |
| `activity` ListModel | ❌ | Vắng |
| `clearActivity()` | ❌ | Vắng |
| `addFolder(localPath, remoteFolderId, watchSub, deleteAfter, ignorePatterns)` | `addFolder(localPath)` — 1 param | One-shot config gone |
| `updateFolderSettings(id, watchSub, deleteAfter, ignorePatterns, speedLimitKBps)` | ❌ (chỉ có `setEnabled`/`setDeleteAfterUpload` rời rạc) | Vắng |
| Signal `previewReady`, `batchCompleted`, `folderMissing` | ❌ (có `folderSynced`, `folderMissing` ở SyncService nhưng chưa expose qua VM) | Vắng signal |

### 2.4 SyncService đã hardcode các thứ design định cho user tuỳ chỉnh

```cpp
// src/core/services/SyncService.h:44-47
static constexpr int    kMaxFolders     = 5;
static constexpr int64_t kSpeedLimitBps  = 5LL * 1024 * 1024;       // 5 MB/s — UI không edit được
static constexpr int64_t kMaxFileSize    = 1024LL * 1024 * 1024;    // 1 GB hardcoded
static constexpr int    kRescanIntervalMs = 5 * 60 * 1000;          // 5 min hardcoded
```

→ Field "Giới hạn tốc độ tải lên" trong `WatchFolderSettingsDialog` **không persist** — user save xong service vẫn dùng 5 MB/s.
→ Field "Bỏ qua file theo mẫu" (ignore patterns) cũng không persist — service dùng skip-list cứng `.tmp, Thumbs.db, .DS_Store, desktop.ini, ~$*, dotfiles, .git, node_modules, …`.
→ Field "Theo dõi thư mục con" cũng không persist — service luôn recursive.

### 2.5 3 dialog "shared" thực ra chỉ phục vụ AutoUpload design

| Dialog | Field UI | Persist được? |
|---|---|---|
| AddWatchFolderDialog Step 1 | Local path, Remote folder picker | Local ✓ — Remote ❌ (service tự suy từ folder name) |
| AddWatchFolderDialog Step 2 | watchSubfolders toggle | ❌ |
| AddWatchFolderDialog Step 2 | deleteAfterUpload toggle | ✓ |
| AddWatchFolderDialog Step 2 | ignorePatterns text | ❌ |
| WatchFolderSettingsDialog | watchSubfolders | ❌ |
| WatchFolderSettingsDialog | deleteAfterUpload | ✓ |
| WatchFolderSettingsDialog | ignorePatterns | ❌ |
| WatchFolderSettingsDialog | speedLimitMode + KBps | ❌ |

→ **5/8 field UI là decoration** — bấm Lưu xong service bỏ qua. Đây là **UX lừa dối** nguy hiểm hơn nhiều so với việc thiếu feature.

---

## 3. Ba phương án refactor

### Phương án A — "Cắt mọi thứ về scope đã ship"
**Triết lý:** Code phải khớp behavior. Cắt UI trùng và xoá field UI lừa user.

**Phạm vi:**
- Xoá `AutoUploadPage.qml`, remove khỏi `qmldir`.
- Archive `docs/auto-upload-ui-design.md` → `docs/decisions/auto-upload-design-v1-deferred.md` với header "DEFERRED — feature backlog cho v2".
- Sửa `AddWatchFolderDialog`: bỏ Step 2 hoàn toàn, chỉ giữ Step 1 (local + remote folder), add với deleteAfterUpload mặc định false; user chỉnh ở settings dialog sau.
- Sửa `WatchFolderSettingsDialog`: chỉ còn 1 toggle `deleteAfterUpload`. Bỏ watchSubfolders, ignorePatterns, speedLimit. Đổi title thành "Cài đặt nhỏ".
- Hoặc gọn hơn: **xoá luôn WatchFolderSettingsDialog**, di chuyển toggle `deleteAfterUpload` vào toolbar active folder (vốn đã có sẵn ở SyncPage line 13).

**Effort:** **1-2 ngày** (chủ yếu là xoá code & test).

**Trade-off:**
- (+) Hết "feature lừa", code/UI khớp 1-1 với backend.
- (+) Maintenance đơn giản nhất.
- (–) Mất tham vọng UX. User cần 6 trạng thái + activity feed + ignore patterns sẽ phải đợi v2.
- (–) Bỏ phí công sức đã đầu tư vào AutoUploadPage design.

### Phương án B — "Catch up backend tới design"
**Triết lý:** Spec design đã đủ chi tiết, build cho hết.

**Phạm vi:**
- Mở rộng `SyncService`:
  - Thay `kSpeedLimitBps` constant → per-folder field trong `SyncFolder` model + repository column.
  - Thay skip-list cứng → per-folder `ignorePatterns` field (vẫn merge với system defaults).
  - Add `watchSubfolders` per-folder + đổi scan logic (non-recursive option).
  - Add status enum (Idle/Scanning/Active/Paused/Error/Disabled), expose qua model role.
  - Add pause/resume per folder (khác enable/disable: pause giữ enabled=true nhưng tạm hoãn upload).
  - Add `previewFolder(path, watchSub, ignorePatterns)` dry-run.
  - Add `clearActivity()` + persistent activity log trong `SyncRepository`.
  - Add master enabled toggle (global pause, không destroy state).
- Tạo `AutoUploadViewModel` thay thế SyncViewModel theo spec line 696-748.
- Migrate `SyncPage` → `AutoUploadPage` (rename + adopt richer UI). Xoá SyncPage cũ.
- Đăng ký `autoUploadViewModel` trong AppContext.

**Effort:** **3-5 tuần** engineering.
- 1 tuần: schema migration SyncRepository (column mới, status enum)
- 1 tuần: SyncService extensions + tests
- 1 tuần: AutoUploadViewModel + ListModels
- 1 tuần: UI polish (FsWatchCard 6 states, FsActivityRow, animations)
- 0.5 tuần: i18n, accessibility, QA

**Trade-off:**
- (+) Đạt vision design, sản phẩm có thể compete với Google Drive Backup.
- (+) Activity feed + pause/resume + speed limit là feature thật sự valuable.
- (–) Chi phí lớn — 1 quý đội kỹ thuật.
- (–) Schema migration có rủi ro với user đang dùng.
- (–) "Big bang refactor" — review/test khó.

### Phương án C — "Pragmatic middle" ⭐ (Khuyến nghị)
**Triết lý:** Cắt nợ kỹ thuật **ngay**, mở rộng có chọn lọc theo phase.

#### Phase 1 — Dọn nợ (1 ngày, ship ngay sprint này)

| Việc | File | Ghi chú |
|---|---|---|
| 1.1. Xoá `AutoUploadPage.qml` | `qml/Fshare/Pages/AutoUploadPage.qml` | Dead code |
| 1.2. Bỏ registration | `qml/Fshare/Pages/qmldir` (xoá dòng 9 `AutoUploadPage 1.0 …`) | |
| 1.3. Sửa `AddWatchFolderDialog` Step 2 | `qml/Fshare/Dialogs/AddWatchFolderDialog.qml` | Comment out / disable 3 field không persist (watchSubfolders, ignorePatterns); GIỮ deleteAfterUpload |
| 1.4. Sửa `WatchFolderSettingsDialog` | `qml/Fshare/Dialogs/WatchFolderSettingsDialog.qml` | Disable speed limit + watchSubfolders + ignorePatterns; chỉ giữ deleteAfterUpload |
| 1.5. Cập nhật module Excel | `docs/module_ui_inventory.xlsx` | Xoá rows Auto Upload (8 element); cập nhật Sync element list cho khớp |
| 1.6. Archive design doc | `docs/auto-upload-ui-design.md` → `docs/decisions/auto-upload-v1-deferred.md` | Add header "STATUS: DEFERRED. UI built ahead of backend; treat as roadmap for Sync v2." |

**Risk:** Rất thấp. Code AutoUploadPage không reachable nên xoá không phá gì. Disable field dialog → user mất ảo tưởng nhưng product behavior đúng hơn.

**Verification:**
1. Rebuild → app vẫn launch.
2. Vào Đồng bộ → add folder → mở settings → save → assert backend không log warning về field unknown.
3. `grep -r autoUpload src/ qml/` → expect 0 matches.

#### Phase 2 — Nâng giá trị nhất, rẻ nhất (1-2 sprint, tuỳ chọn)

Chọn các feature có cost/value tốt từ design:

| # | Feature | Why high-value | Effort |
|---|---|---|---|
| 2.1 | Master enabled toggle (global pause) | Cần khi user đi du lịch, không muốn upload qua mobile data | S (½ ngày) |
| 2.2 | Activity log (last 50 ops, in-memory + persisted trong SyncRepository) | User mất niềm tin khi không thấy "đã upload N file" | M (2 ngày) |
| 2.3 | Sidebar badge `pendingCount` | Affordance gọi user về Sync page khi có hoạt động | S (½ ngày) |
| 2.4 | Per-folder progress + status enum (chỉ 4 state: Idle / Scanning / Uploading / Error) | Folder card có "đang làm gì" rõ ràng — UX nâng cấp đáng kể | M (3 ngày) |
| 2.5 | Dry-run preview "Sẽ quét N file (M GB)" trước khi confirm add | Trust + user biết hậu quả | S (1 ngày) |

→ Total ~7-8 ngày = 1 sprint dồn, hoặc rải 2 sprint xen với việc khác.

#### Phase 3 — Polish (chỉ làm khi cần đua tính năng)

| Feature | Effort |
|---|---|
| Pause/Resume per folder (semantically khác enable) | L (1 tuần — cần modify SyncService scheduling) |
| Per-folder speed limit | M (3 ngày — schema + scheduling integration) |
| Watch subfolders toggle | M (2 ngày — scan logic branch) |
| User-editable ignore patterns | S (1 ngày — merge với system defaults) |
| Retry failed batch | S (½ ngày) |
| Pause All / Resume All | S (½ ngày, dependency 2.1 + 3.1) |

**Trade-off Phase C:**
- (+) Ship-able theo phase, validate user value trước khi đầu tư sâu.
- (+) Phase 1 (dọn nợ) ship ngay → đỡ rủi ro mỗi sprint UI/UX bị đụng vào AutoUploadPage tưởng còn sống.
- (+) Phase 2 có 5 feature cost thấp value cao — không cần làm hết, pick 2-3 cái.
- (–) Vẫn cần tích luỹ Phase 3 nếu muốn close hết design gap.

---

## 4. Khuyến nghị cụ thể

### Bây giờ (1 ngày)
Làm Phase 1 toàn bộ. Tôi có thể thực thi ngay nếu được approve — đã xác minh không có side-effect.

### Sprint tới (1-2 tuần)
Pick từ Phase 2: **2.1 (Master toggle) + 2.3 (Sidebar badge) + 2.5 (Dry-run preview)** — tổng 2 ngày engineering, tăng trust signal đáng kể.

### Quý sau
Tổ chức 1 RFC team: dùng số liệu thực tế (analytics: bao nhiêu user dùng Sync? add bao nhiêu folder? rớt sau lần upload thứ N không?) để quyết Phase 3 và 2.2/2.4 có ROI không. Nếu Sync dưới 10% user dùng, đừng đổ thêm tuần engineering.

---

## 5. Rủi ro cần track

| Rủi ro | Mitigation |
|---|---|
| Schema migration Phase 3 phá user data | Bump schema version, ghi migration script. Test với DB của QA có nhiều folder watch. |
| Disable field trong dialog (Phase 1) làm user complaint "feature mất" | Đổi UI thành "Sẽ có ở phiên bản sau" thay vì xoá hẳn — minh bạch. |
| Khi pause/resume implement (Phase 3), tương tác với BudgetManager + TransferOrchestrator phức tạp | Spike 1 ngày trước khi commit; có thể dùng pattern setPriority(Background) thay vì pause thật. |
| Activity log (Phase 2.2) blow up DB | Cap 1000 row, rotate ra log file sau đó. |
| Pause All khi đang upload file lớn (Phase 3.6) | TransferService đã có `pauseAll()` (orchestrator scope) — reuse pattern. |

---

## 6. Phụ lục — Lệnh exec Phase 1

```powershell
# Backup
$ts = Get-Date -Format "yyyyMMdd-HHmmss"
Copy-Item "D:\Work\FsNext\qml\Fshare\Pages\AutoUploadPage.qml" "D:\Work\FsNext\qml\Fshare\Pages\AutoUploadPage.qml.bak-$ts"

# Delete
Remove-Item "D:\Work\FsNext\qml\Fshare\Pages\AutoUploadPage.qml"

# Edit qmldir, dialogs, archive design doc — sau khi review
```

**File sẽ chỉnh sau khi approve:**
- ✏ `qml/Fshare/Pages/qmldir` (xoá 1 dòng)
- ✏ `qml/Fshare/Dialogs/AddWatchFolderDialog.qml` (disable 3 field Step 2)
- ✏ `qml/Fshare/Dialogs/WatchFolderSettingsDialog.qml` (đơn giản hoá form)
- 🗑 `qml/Fshare/Pages/AutoUploadPage.qml` (xoá)
- 🚚 `docs/auto-upload-ui-design.md` → `docs/decisions/auto-upload-v1-deferred.md` (+ header)
- ✏ `docs/module_ui_inventory.xlsx` (update Module Summary)
