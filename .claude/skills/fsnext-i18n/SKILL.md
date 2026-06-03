---
name: fsnext-i18n
description: Make FsNext features bilingual (Vietnamese source + English). Use ALWAYS when adding or changing any user-facing text in FsNext — QML pages/components/dialogs, toasts, menus, C++ tr() strings (errors, notifications, tray) — and when updating/translating src/i18n/fshare_en.ts. Covers the qsTr/tr convention, lupdate/lrelease via Qt llvm-mingw, professional EN translation tone, and runtime language switching (LanguageViewModel).
---

# FsNext — Bilingual (song ngữ) convention

FsNext ships **two languages**: **Tiếng Việt = ngôn ngữ nguồn (mặc định)**, **English = tùy chọn**.
Người dùng đổi ngôn ngữ ở Settings hoặc ở user menu (sidebar) → đổi **runtime**, không cần restart, và **nhớ
qua các lần đăng nhập** (QSettings key `UI/language`, `"vi"`|`"en"`, mặc định `vi`).

> **Luật vàng:** MỌI chuỗi hiển thị cho người dùng PHẢI đi qua `qsTr(...)` (QML) hoặc `tr(...)`/`QObject::tr(...)`
> (C++). Chuỗi hardcode = không dịch được = bản English bị lẫn tiếng Việt. Đây là bug-class hay gặp nhất.

## Khi nào dùng skill này
- Thêm/sửa BẤT KỲ tính năng nào có text người dùng thấy (page, component, dialog, toast, menu, tooltip,
  placeholder, label; hoặc message lỗi/thông báo/tray trong C++).
- Khi cần cập nhật hoặc dịch `src/i18n/fshare_en.ts`.

## Quy trình bắt buộc cho mỗi tính năng mới

1. **Viết UI bằng tiếng Việt, bọc qsTr/tr ngay từ đầu.**
   ```qml
   text: qsTr("Đăng xuất")                       // ✅
   text: "Đăng xuất"                              // ❌ không bao giờ
   title: qsTr("Ngôn ngữ")
   placeholderText: qsTr("Tìm file, folder…")
   ```
   ```cpp
   emit error(tr("Không thể kết nối máy chủ"));   // ✅ (trong QObject)
   QObject::tr("Tải lên thất bại")                // ✅ (ngoài QObject)
   ```
   - Chuỗi ghép động: bọc từng mảnh tĩnh — `qsTr("Chào ") + name`. Với số nhiều/đếm dùng dạng có `%n`
     (`qsTr("%n file", "", count)`) để bản EN tách số ít/số nhiều.

2. **KHÔNG bọc:** danh từ riêng/nhãn hiệu (Fshare, FsNext, VIP, FPT, Google/Facebook/Zalo), placeholder kỹ thuật
   (`your@email.com`, `••••••••`), ký hiệu/emoji (`⌘K`, `✓`, `━━`), đơn vị thuần (GB/MB/TB), URL.

3. **Cập nhật catalog dịch** (sau khi thêm/sửa qsTr/tr):
   ```powershell
   $lupdate = "C:\Qt\6.8.3\llvm-mingw_64\bin\lupdate.exe"
   & $lupdate -silent -recursive qml src -ts src\i18n\fshare_en.ts
   ```
   lupdate thêm các chuỗi mới vào `fshare_en.ts` với `type="unfinished"` (giữ nguyên bản dịch cũ đã có).

4. **Dịch các entry `type="unfinished"` sang tiếng Anh** trong `src/i18n/fshare_en.ts`:
   - Tìm: `grep -n 'type="unfinished"' src/i18n/fshare_en.ts`
   - Điền `<translation>...</translation>` và **xóa** `type="unfinished"`.
   - Khối lớn (nhiều chục chuỗi) → giao 1 sub-agent dịch CẢ file (1 writer, tránh hỏng XML), rồi validate
     bằng lrelease. Khối nhỏ → tự Edit.

5. **Sinh .qm + build + verify:**
   ```powershell
   $lrelease = "C:\Qt\6.8.3\llvm-mingw_64\bin\lrelease.exe"
   & $lrelease src\i18n\fshare_en.ts -qm output\translations\fshare_en.qm   # phải in "0 unfinished"
   & cmd.exe /c "scripts\build.bat"                                          # POST_BUILD cũng tự chạy lrelease
   ```
   Verify EN: đặt `HKCU\Software\FPT\FsNext\UI\language = en`, chạy app, kiểm UI ra tiếng Anh (xong khôi phục `vi`).

## Giọng văn tiếng Anh (chuẩn khách hàng)
Trang trọng, **ngắn gọn, súc tích**, chuyên nghiệp như app thương mại (Dropbox/Drive). Không giải thích, không
lý do kỹ thuật. Thuật ngữ chuẩn: Sign in / Sign out / Account / Settings / Sync / Download / Upload / Folder /
File / Favorites / Retry / Cancel / Delete / Rename / Move / Copy. Nút = mệnh lệnh ngắn; tiêu đề = Title Case;
câu mô tả = Sentence case. Giữ nguyên placeholder `%1 %2 %n %L1` đúng vị trí ngữ nghĩa; escape `&`→`&amp;`.

## Audit chuỗi còn hardcode (chạy định kỳ / trước khi ship)
```bash
grep -rnE '(text|title|placeholderText|accentWord|subtitle|label|desc|hint|ToolTip\.text):\s*"[^"]*[àáảãạăâđèéêìíòóôơùúưỳýĐÀ]' qml --include=*.qml | grep -v 'qsTr'
```
In ra các chuỗi tiếng Việt CHƯA bọc qsTr → bọc lại theo bước 1. (Bỏ qua `ShowcasePage.qml` — gallery dev,
không ship cho người dùng.)

## Hạ tầng (đã có sẵn — không phải dựng lại)
- **`src/viewmodels/LanguageViewModel.{h,cpp}`** — single source of truth. Q_PROPERTY `language` (read/write),
  `displayName`, `availableLanguages`. Load `.qm` lúc khởi tạo (trước khi engine load QML); `setLanguage()`
  cài translator + persist + emit `translationReloadNeeded`.
- **`main.cpp`** nối `translationReloadNeeded` → `QQmlApplicationEngine::retranslate()` (đổi runtime).
- Context property QML: `languageViewModel` (dùng được ở mọi file QML). Cũng có ở Settings (FsSegmentedControl)
  và user menu sidebar (FsSidebar, mục "Ngôn ngữ" mở rộng inline).
- `.qm` build ra `output/translations/fshare_en.qm` (POST_BUILD lrelease, xem CMakeLists). Tiếng Việt = nguồn,
  KHÔNG cần `.qm`.

## Gotcha đã biết
- **Ngày/giờ tính trong QML** (HomePage `_greetingKicker`/`_relTime`: "THỨ HAI", "ngày trước"…) ĐÃ bọc qsTr →
  dịch được. Nhưng **C++ `FormatUtil` vẫn dùng `QLocale::system()`** cho một số format ngày tuyệt đối → bám locale
  HỆ ĐIỀU HÀNH, không theo toggle app. Muốn localize hẳn phần C++: suy `QLocale` từ `LanguageViewModel.language`.
- **Tiêu đề editorial** (`FsPageHeader` title + accentWord): tiếng Việt tách 2 từ ("Cài"/"đặt."); tiếng Anh 1 từ
  thì theo house style `FileManagerHeader` — `title:"Settings"` + `accentWord:"."` (đừng tách giữa từ → "Set"/"tings.").
- lupdate có thể auto-điền vài bản dịch `type="unfinished"` (heuristic trùng text) — vẫn phải review + xóa cờ.
- Đừng tự dịch chuỗi vào `<source>`; chỉ điền `<translation>`. Đừng đổi `<location>`/`<context>`/`<name>`.
