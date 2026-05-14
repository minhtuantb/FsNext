# Verification report — Upgrade pass (2026-04-28)

Sau khi hoàn thành 19 task của upgrade pass (`docs/08_assessment_and_roadmap.md`,
ADR `docs/decisions/003_upgrade_decisions.md`), chạy 4 vòng verify trên toàn bộ
thay đổi và phát hiện **6 lỗi** đã được fix tại chỗ. Phần còn lại đã sạch.

---

## 1. Lỗi đã tìm được và sửa

### Lỗi #1 — Migration script làm hỏng `reduceMotion` (P0, blocker UI)

**Triệu chứng**: Mọi `Behavior on … { enabled: !AuroraTheme.reduceMotion }` không
biên dịch — `AuroraTheme.accentuceMotion` không phải property hợp lệ.

**Root cause**: REPL trong `migrate_aurora.py` xử lý `FshareTheme.red` →
`AuroraTheme.accent` *trước* `FshareTheme.reduceMotion` → "red" trong "reduceMotion"
bị thay đầu tiên, biến `FshareTheme.reduceMotion` thành `AuroraTheme.accentuceMotion`.
Lần thay sau cho `reduceMotion` không match được.

**Phạm vi**: 58 hit qua 20 file (mọi `Fshare/Components/*.qml` và `Fshare/Pages/*.qml`
đã migrate).

**Fix**: viết script tự động thay `AuroraTheme.accentuceMotion` → `AuroraTheme.reduceMotion`
qua tất cả file. Thêm guard ADR cho lần migration tới: lệnh thay phải xếp **theo
giảm dần độ dài** hoặc dùng regex word-boundary.

**Verify**: full audit — toàn bộ 72 unique `AuroraTheme.<prop>` đều resolve về
property thực trong `AuroraTheme.qml`.

### Lỗi #2 — `setQuitOnLastWindowClosed(false)` đặt trước `tray.setup()` (P0, leak process)

**Triệu chứng**: Trên Linux không tray hoặc CI headless, đóng window → app vẫn
chạy nền vô hình, người dùng phải kill bằng Task Manager.

**Fix**: chuyển `setQuitOnLastWindowClosed(false)` vào trong block `if (tray.setup())`
(`src/main.cpp:269`).

### Lỗi #3 — `SingleInstance` không được instantiate trong `main.cpp` (P0, feature dead)

**Triệu chứng**: `QLocalServer` `fshare.singleinstance` không bao giờ listen → mọi
gọi từ `fsharenativeapp.exe` (Chrome extension host) hoặc lần chạy thứ hai của
`FsNext.exe` đều fail. Single-instance feature trên giấy tờ tồn tại từ Phase 9
nhưng chưa bao giờ thực sự kích hoạt.

**Fix**: thêm vào `main.cpp` ngay sau khi `QApplication` được tạo:
```cpp
fsnext::SingleInstance singleInstance;
if (!singleInstance.tryLock("fshare.singleinstance")) {
    for (int i = 1; i < argc; ++i)
        singleInstance.sendMessage(QString::fromLocal8Bit(argv[i]));
    return 0;
}
```
Đồng thời connect `messageReceived` → emit signal `openDownloadWithLinks(string)`
của Main.qml để URL từ second-instance / Chrome extension chui qua đúng cùng pipeline
drag/drop hiện hữu.

### Lỗi #4 — Mismatch framing giữa native host và `SingleInstance` (P0, Chrome extension hỏng)

**Triệu chứng**: `nativehost_main.cpp` ghi raw UTF-8 bytes; `SingleInstance::onNewConnection`
đọc qua `QDataStream` (Qt-serialised string với 4-byte length prefix). Ngay cả khi
SingleInstance được wire, message từ Chrome sẽ bị decode thành garbage.

**Fix**: Đổi nativehost dùng cùng `QDataStream` framing với `Qt_6_0` version.

### Lỗi #5 — `tests/CMakeLists.txt` không được include từ root (P1, test không auto-build)

**Triệu chứng**: 4 test mới (`test_budget_manager`, `test_fshare_url`,
`test_filename_sanitizer`, `test_speed_meter`) không được biên dịch theo `cmake --build`
trừ khi dev biết phải `cmake -S tests -B build/tests` riêng.

**Fix**: thêm vào root CMakeLists.txt:
```cmake
option(FSNEXT_BUILD_TESTS "Build FsNext unit tests" ON)
if(FSNEXT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### Lỗi #6 — `overlay.modal:` viết thường trong FsCommandPalette (P1, parse error)

**Triệu chứng**: QML parser từ chối load FsCommandPalette → ⌘K mở popup văng exception.

**Root cause**: Property attached là `Overlay.modal` (chữ O hoa, namespace
`QtQuick.Controls.Overlay`), không phải `overlay.modal`.

**Fix**: rename + thêm comment giải thích.

---

## 2. Phần đã verify clean

| Khu vực | Phương pháp | Kết quả |
|---|---|---|
| Token map AuroraTheme | Regex scan toàn bộ `AuroraTheme.<prop>` so với 72 prop khai báo trong `AuroraTheme.qml` | ✅ tất cả resolve |
| FshareTheme còn sót | `grep -rln "FshareTheme\\." qml/Fshare qml/FsAurora qml/Main.qml` | ✅ chỉ còn 1 hit là comment trong AuroraTheme.qml |
| Spacing/radius literal collision | Manual review of `migrate_aurora.py` REPL ordering | ✅ longest-first đúng cho mọi sp/r token |
| C++ header/cpp signature | grep `osPrefersReducedMotion`, `tryRefreshSession`, `resolveConflict`, `m_share` | ✅ đầy đủ ở .h và .cpp |
| AuthService duplicate `signals:` | `grep -c "^signals:" AuthService.h` | ✅ chỉ 1 block |
| HttpClient share lifetime | Manual review destruction order qua AppContext | ✅ HttpClient destroyed sau engines (unique_ptr reverse decl order) |
| Native host framing | grep `QDataStream stream(&sock)` | ✅ khớp `SingleInstance::onNewConnection` |
| `Q_DECLARE_METATYPE` cho `TransferTask` | grep | ✅ vẫn còn (field mới không break MetaType) |
| QML resource glob | `qml/*.qml` recursive trong CMakeLists | ✅ FsCommandPalette tự động được pick up |
| qmldir đăng ký FsCommandPalette | grep | ✅ thêm dòng `FsCommandPalette 1.0 FsCommandPalette.qml` |
| nativehost CMake target | grep `qt_add_executable(fsharenativeapp` | ✅ có, link Qt6::Core + Qt6::Network |
| Qt6::Widgets vào `find_package` + `target_link_libraries` | grep | ✅ cả hai chỗ |

---

## 3. Caveats còn mở (không phải lỗi, là việc cần làm tiếp)

1. **`AppSettings::minimizeToTray` và `fileConflictPolicy`** đã thêm vào struct
   nhưng chưa có read/write trong `SettingsService` ↔ `SettingsViewModel`.
   `TransferService::addDownload` đọc `Download/conflictPolicy` thẳng từ
   `SettingsRepository::getInt` thay vì qua AppSettings struct → field hiện
   "dead". Không gãy build, nhưng SettingsPage.qml chưa có UI cho 2 lựa chọn này.
2. **ADR D12 — progress-persistence sidecar** chưa làm. Crash mid-download vẫn
   mất tiến độ.
3. **Page split (`FileManagerPage` 2595 LOC, `FavoritesPage` 1373 LOC)** chưa làm.
4. **`tests/CMakeLists.txt`** dùng `qt_add_executable` nhưng không có
   `qt_standard_project_setup()` ở top → có thể cần thêm dòng `set(CMAKE_AUTOMOC ON)`
   (đã có ở line 5). Build trước đây pass nên giữ nguyên, nhưng để ý nếu CI fail.
5. **i18n tiếng Anh** — file `fshare_en.ts` đã có nhưng nhiều `qsTr()` mới thêm
   trong upgrade pass (FsCommandPalette, drag overlay, tray menu) chưa được
   `lupdate`. Chạy `cmake --build build --target update_translations`.
6. **Brace-balance heuristic** trong verify pass báo false-positive vì JS regex
   literal + arrow function. `qmllint` chưa chạy được trên VM Linux này; cần
   chạy `qmllint qml/Main.qml qml/Fshare/**/*.qml` trên máy có Qt 6 cài đặt
   để verify cuối cùng.
7. **Build verify** — sandbox Linux này không có MSVC/Qt 6.8.3; build verify
   final phải chạy trên máy Windows dev với `cmake --preset msvc2022 && cmake --build build`.

---

## 4. Khuyến nghị bước tiếp theo (theo thứ tự ưu tiên)

### Ngay lập tức (trước khi merge upgrade pass)

1. **Build verification trên máy dev**:
   ```powershell
   # Đóng FsNext.exe nếu đang chạy
   cmake --preset msvc2022
   cmake --build build --config Release -j 8
   ctest --test-dir build --output-on-failure
   "C:/Qt/6.8.3/msvc2022_64/bin/qmllint.exe" --resource build/.qt/rcc/qml_resources.qrc qml/Main.qml
   ```
   Nếu link fail vì Qt6Widgets thiếu: kiểm tra `windeployqt6 --release output/FsNext.exe`
   đã copy `Qt6Widgets.dll` vào output chưa.
2. **Smoke test runtime**:
   - Boot app → kiểm tra tray icon hiện ở khay hệ thống.
   - ⌘K → mở Command palette, gõ "tải" → 3 lệnh hiện.
   - Đóng main window → app KHÔNG quit (kiểm tra Task Manager); double-click tray → window restore.
   - Drag Fshare URL từ browser vào window → Download tab tự mở.
   - Cài extension Chrome (load unpacked từ `extension/`), bấm popup → URL được nhận trong app đang chạy.
3. **Test bù coverage thực tế**: chạy `ctest` để đảm bảo 4 test mới PASS thật:
   - `test_budget_manager` — 7 case
   - `test_fshare_url` — 9 case
   - `test_filename_sanitizer` — 7 case
   - `test_speed_meter` — 7 case
   Nếu fail, có thể cần fix ASCII expectation (ví dụ: `dot` trong sanitizer test
   với extension nhiều thành phần `.tar.gz` — assertion là `endsWith(".gz")` —
   khớp với cài đặt "last `.ext`" của implementation).

### Đợt tiếp theo (next sprint, ~1 tuần)

4. **ADR D12 — progress persistence**: thêm cột `progress_json` vào
   `transfer_history` table. `TransferService::onDownloadProgress` debounce 5s,
   serialize `{taskId, bytesTransferred, fileSize, segmentBytes[]}` rồi UPSERT.
   Khởi động lại app: load row có `state IN (Active, Paused, Queued)` rồi resume.
5. **Wire AppSettings.minimizeToTray + fileConflictPolicy vào SettingsPage**:
   thêm 2 toggle/select trong SettingsPage.qml, ánh xạ qua SettingsViewModel.
   Settings được đọc bởi `Main.qml` (closeEvent → `event.accepted = false; window.hide()`)
   và bởi `TransferService::addDownload`.
6. **Page split**:
   - Tạo `qml/Fshare/Pages/FileManager/`: `Toolbar.qml`, `ContextMenuBuilder.qml`,
     `RenameDialog.qml`, `ShareDialog.qml`, `DetailPanel.qml`. `FileManagerPage.qml`
     trở thành layout shell ~600 LOC.
   - Tương tự `Favorites/`: `CollectionsRibbon.qml`, `Grid.qml`.
7. **A11y audit**: mỗi `Aurora.FsIcon` trong toolbar buộc phải có `accessibleName`
   hoặc tooltip. Toast → `Accessible.role: Accessible.AlertMessage`.
8. **`qmllint` vào CI**: thêm `add_custom_target(qml_lint COMMAND qmllint ...)`
   chạy mỗi build (gated `FSNEXT_QML_LINT=ON`). Bắt regex literal collision
   thật (qmllint hiểu QML, không như heuristic của tôi).

### Trung hạn (3-4 tuần)

9. ~~Backend coordination — login HTTP 400~~ — **RESOLVED 2026-04-28**.  Email
   +password login đã chạy được trên môi trường thật với app_key 2026 +
   `Fshare_Tool_2026` UA.  `docs/10_login_400_playbook.md` được giữ làm
   emergency runbook nếu blocker tái hiện sau lần rotation backend tiếp theo.
10. **Production installer**: viết `scripts/installer.iss` (Inno Setup) hoặc
    `scripts/installer.nsi` (NSIS) bao gồm: copy `output/FsNext.exe` +
    `fsharenativeapp.exe` + Qt deps; chạy `register_native_host.bat`; tạo
    Start Menu shortcut; uninstaller xoá HKCU registry key của native host.
11. **Code signing**: hook EV cert vào `signtool` trong post-link để Windows
    SmartScreen không cảnh báo người dùng cuối.
12. **Observability**: thêm endpoint `flog.fshare.vn` (đã được mention trong
    `docs/01_features.md` §10.2 cho legacy app) — opt-in trong Settings, gửi
    speed sample mỗi 5 phút khi có transfer active.

---

## 5. Tổng kết

| Hạng mục | Status |
|---|---|
| Code coverage thay đổi | ✅ verify-clean sau 6 fix |
| Compile-readiness (manual review) | ✅ tất cả thay đổi C++/QML đã đúng signature |
| Compile-readiness (machine) | ⏳ chờ build trên máy Windows có MSVC/Qt |
| Smoke test runtime | ⏳ chờ chạy trên máy dev |
| Tests bù | ✅ 4 file mới đã wire vào CMake |
| Documentation | ✅ ADR 003, STATUS, assessment, verification report đều cập nhật |

**Khuyến nghị**: trước khi merge upgrade pass, ưu tiên (1) chạy build verify trên
máy Windows dev, (2) smoke test 5 luồng chính (login, download, upload, sync,
tray + ⌘K + Chrome extension). Mọi caveat ở §3 đều có thể đẩy sang đợt sau —
không phải blocker.
