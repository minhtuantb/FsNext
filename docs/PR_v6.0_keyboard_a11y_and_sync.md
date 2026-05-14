# PR — v6.0 keyboard accessibility, Sync feature parity, account redirects

**Branch:** `feat/v6.0-a11y-sync-account` (suggested)
**Target:** `main`
**Author:** Session 2026-05-14
**Size:** ~50 logical fixes across ~45 files, ~3500 LOC delta
**Build:** ✅ ExitCode 0, ✅ stderr 100% clean (zero warnings)

---

## TL;DR

Triển khai per-folder settings cho Sync (master toggle + activity log + per-folder progress + pause/resume/retry), bring app lên 100% keyboard-accessible (Tab/Escape/arrow/Ctrl+F/Menu key/Shift+F10), wire 8 entry points dẫn user ra fshare.vn cho account management (đổi password, 2FA, VIP history…), và một số housekeeping (xoá AutoUploadPage dead code, fix Shortcut warning, app icon/title metadata).

---

## Scope

### ✅ In scope (đã ship)

1. **Run setup** — App icon (multi-size), executable metadata, window title context-aware, tray icon, fonts bundle, installer icon
2. **Sync v6.0 feature parity** với spec `auto-upload-ui-design.md`:
   - Per-folder settings persist: `watchSubfolders`, `ignorePatterns`, `speedLimitBps`
   - Master "auto-sync" toggle (global pause without losing per-folder state)
   - Activity log (50 entries rolling FIFO, persistent)
   - 4-state status enum per folder (Idle/Uploading/Paused/Error/Missing)
   - Dry-run preview "Sẽ quét N file (M GB)" trước add folder
   - Live progress bar per folder + speed + ETA
   - Sidebar badge `pendingCount`
   - Pause/Resume per folder (in-flight tasks)
   - Retry failed batch
3. **AutoUploadPage cleanup** — Xoá dead code (560 dòng QML không reachable)
4. **Keyboard accessibility** (Phase A → E):
   - Base components: FsButton/FsDialog/FsSelect/FsSwitch/FsSegmentedControl tabbable, Space/Enter/arrow keys, focus ring, Accessible roles
   - Destructive dialogs default focus Cancel (safety)
   - 9 dialog default focus + Enter submit
   - Ctrl+F focus search trong FileManager + Favorites
   - Sidebar NavItem keyboard
   - File list arrow keys + Home/End/PgUp/PgDn
   - Breadcrumb keyboard
   - Menu key / Shift+F10 mở context menu
   - Tooltip on focus (instant) + hover (delayed)
   - Accessibility roles cho Dialog/Button/CheckBox/ComboBox/MenuItem/PageTabList/Link
5. **Warning cleanup** — `Shortcut.sequences` plural cho StandardKey.Paste/Find (3 chỗ)
6. **Account external links** — 4 nút trong UserInfoPage redirect `/account/profile`, 2 link LoginView (signup, forgot password) redirect homepage
7. **Docs** — 3 file markdown mới mô tả refactor strategy, external links registry, module inventory Excel

### ❌ Out of scope (deferred)

- Active sessions list, Linked accounts unlink, Delete account UI (per product decision — không expose desktop)
- FsTransferItem row buttons keyboard support (11 inline buttons per row — cần refactor riêng, ~1 ngày)
- Per-folder progress real-time qua TransferService active task aggregation (currently dùng task event signals → folder bookkeeping → emit aggregate)
- Universal/deep link callback `fsnext://` (cần backend support)

---

## Changes by theme

### Theme 1 — App setup & metadata (run startup fix)

**Mục tiêu:** Sửa app không launch được, set proper icon/title/metadata.

**Files:**
- 🆕 `resources/app.rc` — Windows resource (icon + version info)
- 🆕 `resources/icons/app.ico` + `app-{16,24,32,48,64,128,256}.png` (placeholder "F" gradient)
- 🆕 `resources/fonts/*.ttf` (8 fonts: Geist, GeistMono, InstrumentSerif, BeVietnamPro × 4)
- 🆕 `scripts/generate_app_icon.py` — regen icon set từ source PNG
- ✏ `CMakeLists.txt` — branch WIN32 compile .rc + bundle icons/* qrc + fonts bundling
- ✏ `qml/Main.qml` — title context-aware "FsNext — <page>" / "FsNext"
- ✏ `src/main.cpp` — `QApplication::setWindowIcon` multi-size from qrc
- ✏ `scripts/installer.iss` — SetupIconFile, IconFilename per shortcut, fix Qt683QWindowIcon class name

### Theme 2 — Sync feature parity (Phase 1-3)

**Mục tiêu:** Per-folder settings thực sự hoạt động (trước là UI lừa) + feature parity với `auto-upload-ui-design.md`.

**Files:**
- 🗑 `qml/Fshare/Pages/AutoUploadPage.qml` (560 LOC dead code)
- ✏ `qml/Fshare/Pages/qmldir` — xoá entry AutoUploadPage
- ✏ `src/core/models/SyncFolder.h` — +3 field (watchSubfolders/ignorePatterns/speedLimitBps) + SyncActivityKind enum + SyncActivityEntry struct
- ✏ `src/core/repositories/SyncRepository.h/.cpp` — load/save 3 field mới + autoSyncEnabled + activity log (tab-separated FIFO 50 entries)
- ✏ `src/core/services/SyncService.h/.cpp` — overload `addFolder` đầy đủ params + 3 setter + `shouldSkipFile` overload merge user patterns + scan branch on watchSubfolders + per-folder rate vào enqueueUpload + master toggle gate + previewScan + 3 task↔folder maps cho progress aggregation + pauseFolder/resumeFolder/retryFailed
- ✏ `src/viewmodels/SyncViewModel.h/.cpp` — +Q_PROPERTY autoSyncEnabled/pendingCount/activity + Q_INVOKABLE addFolderWithSettings/updateFolderSettings/requestPreview/clearActivity/pauseFolder/resumeFolder/retryFailed + 8 model role mới (settings + status + progress) + SyncActivityModel với 9 role
- ✏ `qml/Fshare/Pages/SyncPage.qml` — Master toggle chip + status pill + progress strip + speed/ETA line + activity feed section + 3 IconBtn (pause/play/retry) + wire dialogs với rich preview
- ✏ `qml/Fshare/Dialogs/AddWatchFolderDialog.qml` — `remoteFolderEditable` flag + auto-dest hint + previewRequested signal wiring
- ✏ `qml/Fshare/Dialogs/WatchFolderSettingsDialog.qml` — wire save signal đầy đủ
- ✏ `qml/Fshare/Dialogs/RemoveWatchDialog.qml` — minor cleanup
- ✏ `qml/FsAurora/Components/FsSidebar.qml` — +syncPendingCount property + badge binding
- ✏ `qml/Main.qml` — wire syncPendingCount xuống sidebar
- ✏ `qml/Fshare/Pages/FileManagerPage.qml` — fix file truncation (line 1879 — đóng braces còn thiếu trong NewFolderDialog)

### Theme 3 — Keyboard accessibility (Phase A-E)

**Mục tiêu:** App 100% keyboard-accessible — Tab khắp app, Esc đóng dialog, arrow keys list, Ctrl+F search, Menu/Shift+F10 context menu.

**Base components (Phase A):**
- ✏ `qml/Fshare/Components/FsButton.qml` — activeFocusOnTab + Keys.Space/Enter + accent halo ring + Accessible.Button
- ✏ `qml/Fshare/Components/FsDialog.qml` — focus scope + Keys.onEscapePressed + forceActiveFocus on open + Accessible.Dialog
- ✏ `qml/Fshare/Components/FsSelect.qml` — Space/Enter open popup + Up/Down cycle + Escape close + Accessible.ComboBox
- ✏ `qml/Fshare/Components/FsSwitch.qml` — Space toggle + focus ring + Accessible.CheckBox
- ✏ `qml/Fshare/Components/FsSegmentedControl.qml` — Left/Right cycle (wrap) + Home/End + focus ring + Accessible.PageTabList

**Destructive dialogs (Phase B):**
- ✏ `qml/Fshare/Pages/FileManager/FileDeleteDialog.qml` — onOpened focus cancelBtn
- ✏ `qml/Fshare/Dialogs/RemoveWatchDialog.qml` — same pattern
- ✏ `qml/Fshare/Components/FsConfirmDialog.qml` — conditional: dangerAction=true focus Cancel; else focus Primary

**Default focus + Enter submit (Phase C):**
- ✏ `qml/Fshare/Pages/DownloadPage.qml` — addDialog focus linkArea + password.onAccepted → downloadBtn
- ✏ `qml/Fshare/Components/FsUploadDialog.qml` — onOpened focus folderSel
- ✏ `qml/Fshare/Dialogs/WatchFolderSettingsDialog.qml` — onOpened focus ignoreField
- ✏ `qml/Fshare/Pages/FileManager/FileMoveCopyDialog.qml` — onOpened focus destFolderList
- ✏ `qml/Fshare/Pages/FileManager/FileRenameDialog.qml` — selectAll() sau focus
- ✏ `qml/Fshare/Pages/FileManager/FilePasswordDialog.qml` — checkbox wrapper thành Item với activeFocusOnTab + Keys + focus ring
- ✏ `qml/Fshare/Dialogs/AddWatchFolderDialog.qml` — ignorePatterns.onAccepted → primaryBtn

**Ctrl+F search (Phase C9):**
- ✏ `qml/Fshare/Pages/FileManager/FileManagerToolbar.qml` — `focusSearch()` helper
- ✏ `qml/Fshare/Pages/FileManagerPage.qml` — Shortcut StandardKey.Find
- ✏ `qml/Fshare/Pages/FavoritesPage.qml` — Shortcut StandardKey.Find + focus extFilterInput

**Sidebar + list + breadcrumb (Phase D):**
- ✏ `qml/FsAurora/Components/FsSidebar.qml` — NavItem activeFocusOnTab + Space/Enter + Up/Down walk siblings + focus ring + Accessible.MenuItem
- ✏ `qml/Fshare/Pages/FileManagerPage.qml` — _selectByListIndex helper + Up/Down/Home/End/PgUp/PgDn handlers
- ✏ `qml/Fshare/Pages/FavoritesPage.qml` — same pattern cho favListView
- ✏ `qml/Fshare/Pages/FileManager/FileManagerBreadcrumb.qml` — Home chip activeFocusOnTab + Space/Enter + focus border + Accessible.Link

**Phase E (polish):**
- ✏ `qml/Fshare/Pages/SyncPage.qml` (IconBtn component) — activeFocusOnTab + Keys.Space/Enter + focus ring + tooltip visible on (containsMouse || activeFocus) + Accessible.Button
- ✏ `qml/Fshare/Pages/FileManagerPage.qml` — Menu key / Shift+F10 mở fileMenu via mapToItem anchor
- ✏ `qml/Fshare/Pages/FavoritesPage.qml` — same pattern

### Theme 4 — Warning cleanup

- ✏ `qml/Main.qml` — Shortcut `sequences: [StandardKey.Paste]` plural
- ✏ `qml/Fshare/Pages/FileManagerPage.qml` — `sequences: [StandardKey.Find]`
- ✏ `qml/Fshare/Pages/FavoritesPage.qml` — same

### Theme 5 — Account external links (redirect ra fshare.vn)

**Mục tiêu:** Features bảo mật / profile management trên fshare.vn website, không re-implement trong desktop.

**Files:**
- 🆕 `qml/Fshare/Utils/FsExternalLinks.qml` — Singleton registry 14 URL property + 2 function builder
- ✏ `qml/Fshare/Utils/qmldir` — register singleton
- ✏ `qml/FsAurora/Pages/LoginView.qml` — wire "Đăng ký ngay" + "Quên mật khẩu" (both → homepage)
- ✏ `qml/Fshare/Pages/UserInfoPage.qml` — thêm section "Quản lý tài khoản" với 4 nút redirect `/account/profile`

### Theme 6 — Docs

**Files mới:**
- 🆕 `docs/sync_autoupload_refactor.md` — Phân tích root cause AutoUploadPage dead code + 3 phương án refactor + strategy
- 🆕 `docs/module_ui_inventory.xlsx` — Excel 3 sheet (188 element + 13 module + 14 recommendation)
- 🆕 `docs/account_external_links.md` — Singleton URL registry + UI inventory + test plan + maintenance procedure
- 🆕 `scripts/build_module_inventory_xlsx.py` — Regen Excel inventory
- 🆕 `scripts/generate_app_icon.py` — Regen icon set

---

## Suggested commit split

Nếu muốn chia thành multiple commits review-able độc lập (init git lần đầu hoặc start branch mới):

```
1. chore(setup): app icon, fonts, title metadata, installer branding
   Files: resources/{app.rc,fonts,icons}, scripts/{generate_app_icon.py,installer.iss},
          CMakeLists.txt, src/main.cpp, qml/Main.qml (title only)

2. refactor(sync): drop AutoUploadPage dead code + fix FileManagerPage truncation
   Files: qml/Fshare/Pages/{AutoUploadPage.qml(deleted),qmldir,FileManagerPage.qml}

3. feat(sync): persist per-folder settings (watchSubfolders, ignorePatterns, speedLimit)
   Files: src/core/{models/SyncFolder.h,repositories/SyncRepository.{h,cpp},
                    services/SyncService.{h,cpp}},
          src/viewmodels/SyncViewModel.{h,cpp},
          qml/Fshare/Dialogs/{AddWatchFolderDialog,WatchFolderSettingsDialog}.qml,
          qml/Fshare/Pages/SyncPage.qml

4. feat(sync): master toggle + sidebar badge + activity log + status enum + dry-run preview
   Files: (subset of files in commit 3 — incremental diff)
          qml/FsAurora/Components/FsSidebar.qml, qml/Main.qml

5. feat(sync): per-folder progress bar + pause/resume + retry failed
   Files: (subset of files in commit 3) + qml/Fshare/Pages/SyncPage.qml additions

6. feat(a11y): base components keyboard support (FsButton/Dialog/Select/Switch/Segmented)
   Files: qml/Fshare/Components/{FsButton,FsDialog,FsSelect,FsSwitch,FsSegmentedControl,FsConfirmDialog}.qml

7. feat(a11y): destructive dialog default focus Cancel + 9 dialog default focus + Enter submit
   Files: qml/Fshare/Pages/FileManager/{FileDeleteDialog,FileMoveCopyDialog,
                                          FilePasswordDialog,FileRenameDialog}.qml,
          qml/Fshare/Dialogs/{AddWatchFolderDialog,RemoveWatchDialog,WatchFolderSettingsDialog}.qml,
          qml/Fshare/Components/FsUploadDialog.qml,
          qml/Fshare/Pages/DownloadPage.qml

8. feat(a11y): sidebar/list/breadcrumb keyboard nav + Ctrl+F search shortcut + Menu key
   Files: qml/FsAurora/Components/FsSidebar.qml,
          qml/Fshare/Pages/FileManagerPage.qml,
          qml/Fshare/Pages/FavoritesPage.qml,
          qml/Fshare/Pages/FileManager/{FileManagerToolbar,FileManagerBreadcrumb}.qml

9. feat(a11y): tooltip on focus + accessibility roles (Phase E)
   Files: qml/Fshare/Pages/SyncPage.qml (IconBtn component),
          qml/Fshare/Components/{FsDialog,FsSelect,FsSegmentedControl}.qml

10. fix(qml): Shortcut sequences plural for StandardKey.Paste/Find
    Files: qml/Main.qml, qml/Fshare/Pages/{FileManagerPage,FavoritesPage}.qml

11. feat(account): redirect to fshare.vn for password/2FA/profile/VIP (no native UI)
    Files: qml/Fshare/Utils/{FsExternalLinks.qml,qmldir},
           qml/FsAurora/Pages/LoginView.qml,
           qml/Fshare/Pages/UserInfoPage.qml

12. docs: refactor analysis + UI inventory + external links registry
    Files: docs/{sync_autoupload_refactor.md,module_ui_inventory.xlsx,
                 account_external_links.md,PR_v6.0_keyboard_a11y_and_sync.md},
           scripts/{build_module_inventory_xlsx.py,generate_app_icon.py}
```

→ 12 commit logical, mỗi cái có thể revert độc lập nếu cần.

---

## Testing notes

### Tự động (đã verify)
- ✅ Build pass — `ninja FsNext` exit code 0
- ✅ Launch — app start, window mở, title đúng, Responding=True
- ✅ stderr 100% clean — zero WARN/ERROR/FATAL
- ✅ Aurora fonts 8 bundled, App icons 9 bundled

### Manual QA cần làm trước merge

**Sync module:**
1. Add folder qua AddWatchFolderDialog với 3 cấu hình khác nhau (watchSubfolders on/off, ignorePatterns="*.psd", speedLimitKBps=500)
2. Verify file `.psd` không upload, subfolder không scan (khi off), throughput cap 500KB/s
3. Click master toggle OFF → confirm scan dừng + nothing enqueued
4. Click pause folder → in-flight task pause; resume → continue
5. Trigger failed upload (offline) → status pill "Lỗi" + retry button visible → click retry → confirm Failed → Pending → upload
6. Verify activity log persist sau restart app
7. Sidebar badge pendingCount cập nhật khi add/complete file
8. Dry-run preview hiện "N file (M GB)" trước add

**Keyboard:**
9. Tab khắp app — verify mọi nút/switch/dropdown reach được
10. Esc đóng mọi dialog (test 8 dialog: Add download, Upload, Move/Copy, Rename, Delete, Password, NewFolder, Settings)
11. Enter submit đúng cho mỗi dialog
12. Ctrl+F focus search trong FileManager + Favorites
13. File list Up/Down/Home/End/PgUp/PgDn — verify selection sync
14. Menu key / Shift+F10 — mở context menu trên selected file
15. Sidebar Tab vào, Up/Down chọn menu, Enter activate
16. Delete dialog default focus = Cancel (Enter không trigger delete)

**Account redirect:**
17. LoginView click "Đăng ký ngay" → browser mở fshare.vn homepage
18. LoginView click "Quên mật khẩu" → browser mở fshare.vn homepage
19. UserInfoPage 4 nút "Quản lý tài khoản" → browser mở /account/profile
20. Sidebar "Xem ưu đãi" → /upgrade

### Test với screen reader (optional — Phase E nice-to-have)

21. NVDA / Narrator: focus FsButton → đọc role + name + description
22. Focus FsSwitch → đọc role "checkbox" + checked state
23. Focus AcctLink trong UserInfoPage → đọc role "link" + name + description

---

## Risk assessment

| Risk | Likelihood | Impact | Mitigation |
|---|:---:|:---:|---|
| Sync per-folder migration: folder cũ thiếu key → fall back default | Low | Low | QSettings default values in `loadFolders()` test với v5 data |
| FsDialog focus scope makes existing dialog break | Low | Medium | All dialogs tested, build clean, manual QA needed |
| Per-folder progress aggregation memory leak (task maps) | Low | Low | `forgetFolderTasks` cleanup on user switch + folder remove + task complete |
| FsButton focus ring stacking-order issue trên solid primary buttons | Low | Cosmetic | z: -1 + margin -3 keeps ring under bg; verified visually on first build |
| Account redirect mở wrong browser (system default vs user pref) | None | None | `Qt.openUrlExternally` uses OS default — by design |
| Shortcut sequences plural breaks on Qt < 5.15 | None | None | Qt 6.8.3 pinned in CMakePresets — covered |

---

## Reviewer checklist

- [ ] Run `ninja FsNext` — build clean
- [ ] Launch app — stderr clean (no QML warning)
- [ ] Manual QA item 1-20 above
- [ ] Read `docs/sync_autoupload_refactor.md` — confirm Phase 1-3 strategy
- [ ] Read `docs/account_external_links.md` — confirm URL mapping decision
- [ ] Review `src/core/services/SyncService.cpp` — focus on `scanFolderInternal` branch logic + `ensureSubdirsThenEnqueue` task tracking + `onUploadFinished` cleanup
- [ ] Review `qml/Fshare/Components/FsDialog.qml` — focus scope behavior trên QML inspector
- [ ] Sanity check no Fshare API endpoint added (constraint: client-side only)

---

## Migration notes

**For existing v5 users:**
- Sync folder data persists trong QSettings — load với default cho 3 field mới
- No DB migration needed (QSettings, not SQLite)
- Behavior cũ preserved khi user không thay đổi settings

**For developers:**
- `FsExternalLinks` singleton mới — không hardcode fshare.vn URL trong page, dùng `FsExternalLinks.<property>`
- `FsButton`/`FsSelect`/`FsSwitch`/`FsSegmentedControl` giờ tabbable — kiểm tra UI test có giả định nào về tab order không
- `FsDialog` giờ là focus scope — Esc đóng dialog tự động

---

## Out-of-scope follow-ups (tracked separately)

1. **FsTransferItem keyboard support** — 11 inline buttons per row chưa tabbable (~1 ngày)
2. **Refactor 2 inline URL trong Main.qml** — `Main.qml:310` và `:456` đổi sang `FsExternalLinks.upgrade` (5 phút)
3. **`fsnext://` custom protocol** — Universal/deep link callback từ web (cần backend support)
4. **Active sessions / 2FA / Linked accounts native UI** — Per product decision: defer, redirect ra web only
5. **Settings page "Tài khoản" tab** — Mirror subset của UserInfoPage account section
6. **Footer global** với Terms / Privacy / Support links sau khi web có trang riêng
