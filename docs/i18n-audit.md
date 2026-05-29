# FsNext — i18n Audit (2026-05-29)

Kết quả rà soát quốc tế hóa (P2 trong `docs/ASSESSMENT.md`). Ngôn ngữ nguồn = **tiếng Việt** (chuỗi nguồn hiển
thị trực tiếp); ngôn ngữ đích duy nhất hiện có = **English** (`src/i18n/fshare_en.ts` → `output/translations/fshare_en.qm`).

## Số liệu

| Hạng mục | Giá trị |
|---|---|
| `qsTr(...)` trong QML | **728** (42/74 file) |
| `tr(...)` trong C++ | **140** |
| Source text trong catalog (sau `lupdate`) | **819** (44 mới + 775 cũ + 9 obsolete) |
| Đã dịch sang EN (finished) | **71** (~9%) |
| **Chưa dịch (unfinished)** | **748** (~91%) |
| Literal user-facing CHƯA wrap (ước lượng thô) | ~177 |
| └─ trong ShowcasePage dev-only (×2) | ~77 |
| └─ false-positive (ký tự `✓ ⌘ ✕ ☁`, default `"Button"`, HTML fragment) | phần lớn còn lại |
| └─ chuỗi production VI thật chưa wrap | nhỏ (rải rác) |

## Phát hiện chính

1. **Hạ tầng + wrap đã tốt**: phần lớn UI dùng `qsTr`/`tr`; catalog đầy đủ. Đã chạy `lupdate qml/ src/` để gom 44
   chuỗi mới wrap kể từ lần cập nhật .ts trước (27/05) — build `lrelease` ra `fshare_en.qm` OK.
2. **Nội dung dịch EN gần như rỗng (~91% unfinished)**: bản EN chủ yếu là *stub* — Qt fallback về chuỗi nguồn
   (tiếng Việt) cho mọi entry chưa dịch. ⇒ **chọn English trong app thực tế vẫn hiển thị tiếng Việt** ở hầu hết
   màn hình. Đây là rủi ro UX (người dùng tưởng có EN nhưng UI lẫn lộn VI/EN).
3. **Độ hở wrap còn lại nhỏ & tập trung**: gần nửa nằm ở `ShowcasePage` (dev-only, `isDevBuild`, không cần dịch);
   phần còn lại nhiều là false-positive (symbol/icon glyph/default component). Chuỗi production VI thật chưa wrap chỉ
   là số ít.

## Khuyến nghị (quyết định sản phẩm)

- **Hoặc cam kết EN**: giao 748 entry cho người dịch điền `fshare_en.ts`, đồng thời wrap nốt số ít chuỗi production
  còn sót → khi đó EN mới thực sự dùng được.
- **Hoặc tạm ẩn lựa chọn English** cho tới khi có nhu cầu thật → tránh UI nửa-VI-nửa-EN gây hiểu nhầm. (Hạ tầng giữ
  nguyên, chỉ ẩn entry "English" ở Settings.)
- **Quy ước duy trì** (bất kể chọn hướng nào): chạy `lupdate qml/ src/ -ts src/i18n/fshare_en.ts` trước mỗi release;
  code-review chặn chuỗi user-facing mới không bọc `qsTr`/`tr`. Không cần dịch ShowcasePage (dev-only).

> Việc điền nội dung dịch EN là tác vụ của người dịch (không nằm trong phạm vi tự động hóa — sinh bản dịch máy sẽ
> sai sắc thái). Đợt này chỉ làm phần kỹ thuật an toàn: refresh catalog + đo + khuyến nghị.
