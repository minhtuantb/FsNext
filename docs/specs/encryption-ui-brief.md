# YÊU CẦU THIẾT KẾ UI/UX — Tính năng Mã hóa & Vault (FsNext)

| Trường | Giá trị |
|---|---|
| Sản phẩm | FsNext — Fshare Desktop Client (Windows, Qt6/QML) |
| Phạm vi brief này | Module **Encryption Engine + Key Management (Vault)** — KHÔNG bao gồm Sync 2 chiều |
| Người nhận | Team Thiết kế (UI/UX) |
| Ngôn ngữ hiển thị | **Tiếng Việt (nguồn)** + English (bản dịch) |
| Nền tảng | Desktop Windows 10/11, app cửa sổ + system tray |
| Trạng thái | Draft để team thiết kế thực hiện |

---

## 0. BỐI CẢNH & MỤC TIÊU

FsNext đã là một desktop client trưởng thành (download/upload, file manager, tray, HUD). Tính năng mới: **Vault mã hóa đầu-cuối (E2EE)** — người dùng bỏ file vào Vault, app mã hóa thành `.fshenc` ở máy trước khi tải lên cloud; server **không đọc được** nội dung. Khóa giải mã chỉ nằm ở máy người dùng.

**3 thông điệp UX cốt lõi phải truyền tải xuyên suốt:**
1. **An toàn** — "Chỉ bạn mới mở được file của mình."
2. **Bạn là người giữ chìa khóa** — "Khóa nằm ở máy bạn; hãy bảo quản cẩn thận và giữ thêm một bản sao lưu ở nơi an toàn."
3. **Đơn giản** — bảo mật mạnh nhưng thao tác hằng ngày mượt (mở khóa một lần, dùng cả phiên).

> **Tông giọng (quan trọng):** trấn an và tôn trọng người dùng. Tránh lặp lại ngôn từ nặng nề kiểu "không thể đảo ngược"/"mất vĩnh viễn". Vẫn nói rõ sự thật, nhưng nhẹ nhàng và luôn kèm hướng dẫn phòng ngừa (lưu khóa nơi khác, sao lưu dữ liệu).

**Persona ưu tiên:** privacy-focused + doanh nghiệp lưu tài liệu nhạy cảm (hợp đồng, pháp lý).

---

## 1. NGUYÊN TẮC & DESIGN SYSTEM BẮT BUỘC

Thiết kế PHẢI dùng **design system Aurora** hiện hành của FsNext (không tạo token mới nếu chưa cần):

**Màu (token Aurora):**
- Accent chính: `#FF5B2E` (cam) · Accent phụ: `#FFAF1D` (vàng) · `#FF3D7F` (hồng)
- Semantic: success (xanh lá), warn (vàng), danger (đỏ), info (xanh dương) — mỗi cái có biến thể Soft (nền) và Tint10/15 (alpha)
- Light: nền `#F5F4F1` (kem), panel trắng, ink1–4 (chữ chính→placeholder)
- Dark: nền `#0E0E12`, panel `#17171E`
- Sidebar **luôn tối** (`#0E0E12`) ở cả 2 theme
- **Bắt buộc hỗ trợ Dark mode + Light mode.**

**Typography:** Geist (sans), Geist Mono (mono — dùng cho passphrase/keyfile/hex), Instrument Serif (tiêu đề lớn nếu cần điểm nhấn). Be Vietnam Pro fallback cho tiếng Việt.

**Component có sẵn phải tái dùng (KHÔNG vẽ lại từ đầu):** `FsButton`, `FsTextField`, `FsCard`, `FsBadge`, `FsSwitch`, `FsProgressBar`, `FsIcon`. Shell: `FsSidebar`, `FsPageHeader`, `FsScrollPage`, `FsToastHost`. Cửa sổ HUD mini đã có.

**Quy ước icon:** dùng bộ icon SVG hiện có, bổ sung icon khóa/mở khóa/vault/shield/key cùng style line-weight.

---

## 2. BẢN ĐỒ MÀN HÌNH (SITEMAP) — TỔNG QUAN SỐ LƯỢNG

**Tổng: 3 trang + 1 wizard 7 bước + 7 dialog modal + 3 surface phụ (tray/toast/badge).**

| # | ID | Loại | Tên màn hình | Modal? |
|---|----|----|---|---|
| 0 | `VAULT-INTRO` | Trang (page) | Giới thiệu & hướng dẫn nhanh (+ miễn trừ trách nhiệm) | Không |
| 1 | `VAULT-HOME` | Trang (page) | Trang Vault (danh sách file .fshenc) | Không |
| 2 | `WIZ-1` | Wizard step | Giới thiệu Vault | Modal/fullscreen |
| 3 | `WIZ-2` | Wizard step | Chọn phương thức bảo vệ | Modal |
| 4 | `WIZ-3` | Wizard step | Tạo passphrase | Modal |
| 5 | `WIZ-4` | Wizard step | ⚠ Cảnh báo mất khóa (4 checkbox) | Modal |
| 6 | `WIZ-5` | Wizard step | Tải khóa khôi phục (recovery keyfile) | Modal |
| 7 | `WIZ-6` | Wizard step | Đặt tên & vị trí Vault | Modal |
| 8 | `WIZ-7` | Wizard step | Hoàn tất | Modal |
| 9 | `DLG-UNLOCK` | Dialog | Mở khóa Vault | Modal |
| 10 | `DLG-CHGPASS` | Dialog | Đổi passphrase | Modal |
| 11 | `DLG-EXPORT` | Dialog | Xuất khóa khôi phục | Modal |
| 12 | `DLG-DECRYPT` | Dialog | Tiến trình giải mã & mở file | Modal/non-blocking |
| 13 | `DLG-DELVAULT` | Dialog | Xóa Vault (xác nhận nguy hiểm) | Modal |
| 14 | `DLG-LARGEFILE` | Dialog | Xác nhận mã hóa file lớn (>1 GB): thời gian/đĩa, chọn mã hóa/không/hủy | Modal |
| 15 | `KEY-MANAGER` | Trang (page) | Quản lý khóa (danh sách khóa đã dùng, copy/nhập/xuất) | Không |
| 16 | `DLG-BATCH` | Dialog/panel | Mã hóa/giải mã 1 file · nhiều file · cả thư mục + tự tải lên Fshare | Modal/non-blocking |
| 17 | `SET-VAULT` | Tab trong Settings | Cài đặt Vault | Không |
| 18 | `TRAY-VAULT` | Tray menu + badge | Mục tray + chỉ báo trạng thái | — |
| 19 | `TOASTS` | Notification | Bộ toast/Windows toast | — |

> **Tùy chọn (nếu chốt multi-vault):** thêm `VAULT-LIST` (danh sách nhiều vault Personal/Work). Brief này thiết kế **1 vault trước**, nhưng layout phải chừa chỗ mở rộng nhiều vault (xem §4.1 ghi chú).

---

## 3. CÁC FLOW CHÍNH (USER JOURNEY)

**Flow A — Tạo Vault lần đầu:**
`VAULT-HOME (empty, chưa có vault)` → bấm "Tạo Vault" → `WIZ-1 → WIZ-2 → WIZ-3 → WIZ-4 → WIZ-5 → WIZ-6 → WIZ-7` → `VAULT-HOME (unlocked, rỗng)`.

**Flow B — Mở khóa & dùng hằng ngày:**
Mở app / mở Vault khi đang khóa → `DLG-UNLOCK` → nhập passphrase → `VAULT-HOME (unlocked)`.

**Flow C — Mã hóa file:**
`VAULT-HOME` (unlocked) → "Thêm file" hoặc kéo-thả → chọn file → tiến trình mã hóa (inline trên item) → file `.fshenc` xuất hiện trong danh sách → (tùy chọn) tự upload.

**Flow D — Mở/giải mã file:**
`VAULT-HOME` → double-click item `.fshenc` → nếu khóa thì `DLG-UNLOCK` trước → `DLG-DECRYPT` (tiến trình) → mở bằng app mặc định OS → khi đóng → thông báo "đã xóa bản tạm an toàn".

**Flow E — Đổi passphrase:**
`SET-VAULT` → "Đổi passphrase" → `DLG-CHGPASS` → nhập cũ + mới → thành công toast.

**Flow F — Auto-lock:**
Unlocked → không thao tác 15' (cấu hình được) → tự khóa → badge tray đổi + toast "Vault đã tự khóa".

**Flow G — Mã hóa cả thư mục + tự tải lên:**
`VAULT-HOME` → "Thêm thư mục" (hoặc kéo-thả thư mục) → `DLG-BATCH` cấu hình (bật "Tự tải lên Fshare") → tiến trình từng file → tổng kết (thành công/lỗi) → file `.fshenc` xuất hiện trong Vault & trên cloud.

**Flow H — Quản lý/đối chiếu khóa:**
`VAULT-HOME` menu "…" → `KEY-MANAGER` → xem danh sách khóa (vân tay) → copy vân tay / nhập khóa mới / xuất khóa → banner nhắc sao lưu nơi an toàn khác.

---

## 4. ĐẶC TẢ TỪNG MÀN HÌNH (CHI TIẾT ĐẾN TỪNG ITEM)

### 4.0 `VAULT-INTRO` — Trang giới thiệu & hướng dẫn nhanh
Truy cập: tự hiện ở lần đầu (trước khi có vault) và luôn truy cập lại được qua nút "Giới thiệu / Trợ giúp" ở header trang Vault. Mục tiêu: **ngắn gọn nhưng đầy đủ** — người đọc 60 giây là hiểu và yên tâm.

- **Hero:** icon shield + tiêu đề "Vault — mã hóa đầu-cuối" + 1 câu định vị: "File của bạn được mã hóa ngay trên máy trước khi rời khỏi thiết bị."
- **Cách hoạt động (3–4 bước, dạng card icon + 1 dòng):**
  1. "Tạo Vault và đặt passphrase của riêng bạn."
  2. "Thêm file → FsNext mã hóa thành `.fshenc` ngay trên máy."
  3. "File mã hóa được tải lên Fshare — server không đọc được nội dung."
  4. "Mở khóa bằng passphrase để xem lại bất cứ lúc nào."
- **Câu hỏi nhanh (accordion 3–4 mục):** "Quên passphrase thì sao?", "Khóa của tôi lưu ở đâu?", "File `.fshenc` mở bằng gì?", "Mã hóa có làm chậm không?".
- **Khối MIỄN TRỪ TRÁCH NHIỆM (bắt buộc, nền info-soft, giọng nhẹ nhàng):**
  > "Toàn bộ quá trình mã hóa diễn ra ngay trên máy của bạn. FsNext không lưu giữ passphrase hay khóa của bạn ở bất kỳ đâu, nên chúng tôi không thể truy cập hay khôi phục nội dung trong Vault. Mã hóa giúp bảo vệ quyền riêng tư, **nhưng không thay thế cho việc sao lưu** — hãy luôn giữ thêm một bản sao lưu các file quan trọng ở nơi an toàn khác, phòng khi quên khóa, lỗi phần cứng hoặc file bị hỏng."
- **CTA:** "Tạo Vault" (nếu chưa có) hoặc "Đến trang Vault" (nếu đã có) + link "Đọc hướng dẫn đầy đủ".

---

### 4.1 `VAULT-HOME` — Trang Vault (truy cập từ sidebar, mục mới "Vault" có icon khóa)

Trang có **3 trạng thái** thiết kế riêng:

**(A) Chưa có Vault (empty/onboarding):**
- Header trang: tiêu đề "Vault" + phụ đề "Mã hóa đầu-cuối — chỉ bạn mới mở được."
- Khối hero giữa trang: icon shield lớn + tiêu đề "Bảo vệ tài liệu nhạy cảm của bạn" + đoạn mô tả 2 dòng.
- 3 bullet giá trị (icon + 1 dòng): "Mã hóa ngay trên máy" · "Server không đọc được" · "Bạn giữ chìa khóa".
- CTA chính: **`FsButton` primary "Tạo Vault"** → mở `WIZ-1`.
- Link phụ: "Tìm hiểu về mã hóa" (mở doc/help).

**(B) Có Vault nhưng đang KHÓA (locked):**
- Header: "Vault" + badge `FsBadge` màu warn "Đang khóa" + icon ổ khóa đóng.
- Khối giữa: icon khóa + "Vault đang khóa" + "Mở khóa để xem và quản lý file đã mã hóa."
- CTA: **`FsButton` "Mở khóa"** → `DLG-UNLOCK`.
- (Tùy chọn) danh sách file vẫn hiển thị tên mờ + icon khóa, KHÔNG mở được tới khi unlock.

**(C) Có Vault & ĐÃ MỞ (unlocked) — màn chính:**
- Header trang:
  - Trái: tên Vault (vd "Personal Vault") + `FsBadge` success "Đã mở" + icon ổ khóa mở.
  - Phải: nút **"Khóa ngay"** (icon khóa), nút **"Thêm file"** (primary, hỗ trợ chọn **1 hoặc nhiều file**), nút **"Thêm thư mục"** (→ `DLG-BATCH`), nút **"Trợ giúp"** (→ `VAULT-INTRO`), menu "…" (Quản lý khóa / Đổi passphrase / Xuất khóa / Cài đặt Vault / Xóa Vault).
  - Chỉ báo auto-lock: text nhỏ "Tự khóa sau 14:32" (đếm ngược, reset khi thao tác). Hover → tooltip giải thích.
- Thanh công cụ phụ: ô tìm kiếm (`FsTextField` có icon search), nút sort (Tên / Ngày / Kích thước), toggle xem List/Grid.
- **Vùng kéo-thả:** toàn bộ danh sách là drop zone; khi kéo file vào → overlay nét đứt + text "Thả vào đây để mã hóa & thêm vào Vault".
- **Danh sách file (item-level):** mỗi dòng gồm:
  | Item | Mô tả |
  |---|---|
  | Icon file | Theo loại file gốc (docx/jpg/…) + huy hiệu khóa nhỏ góc dưới |
  | Tên hiển thị | Tên gốc (vd `report.docx`) — KHÔNG hiện đuôi `.fshenc` cho người dùng |
  | Trạng thái | chip: "Đã mã hóa" / "Đang mã hóa…" (progress) / "Đã tải lên cloud" / "Chỉ ở máy" / "Lỗi" |
  | Kích thước | size file |
  | Sửa đổi lần cuối | ngày giờ |
  | Hành động (hover) | Mở (giải mã & xem) · Tải lên cloud · Lưu bản giải mã ra ngoài · Xóa khỏi Vault · "…" |
- **Trạng thái rỗng (unlocked, chưa có file):** illustration nhẹ + "Vault trống. Kéo file vào hoặc bấm Thêm file."
- **Ghi chú multi-vault (tương lai):** chừa chỗ cho dropdown chọn vault ở cạnh tiêu đề header (hiện ẩn nếu chỉ 1 vault).

---

### 4.2 Wizard tạo Vault — `WIZ-1 … WIZ-7`

**Khung chung của wizard (áp cho mọi step):**
- Thanh tiến trình 7 bước ở trên (stepper: chấm + nhãn, bước hiện tại nổi accent).
- Vùng nội dung giữa.
- Footer: nút "Quay lại" (trái) + nút "Tiếp tục"/"Hoàn tất" (phải, primary). Nút "Hủy" (text, góc).
- Đóng wizard giữa chừng → confirm "Hủy tạo Vault? Tiến trình sẽ không được lưu."

**`WIZ-1` — Giới thiệu Vault**
- Icon shield lớn + tiêu đề "Tạo Vault mã hóa".
- 3–4 dòng lợi ích (mã hóa trên máy, server mù, chỉ bạn giữ khóa, dùng được cả khi offline).
- Footer: "Bắt đầu".

**`WIZ-2` — Chọn phương thức bảo vệ**
- Tiêu đề: "Bạn muốn bảo vệ Vault bằng cách nào?"
- 3 card chọn (radio, dùng `FsCard` selectable):
  1. **Passphrase** (khuyến nghị) — "Một câu mật khẩu chỉ bạn biết."
  2. **Tệp khóa (keyfile)** — "Một file `.key` lưu ở USB/nơi an toàn."
  3. **Cả hai (2 lớp)** — "Passphrase + keyfile. Bảo mật cao nhất." (badge "Nâng cao")
- Mỗi card: icon + tên + 1 dòng mô tả + (badge khuyến nghị/nâng cao).
- Footer: "Tiếp tục" (disabled tới khi chọn).

**`WIZ-3` — Tạo passphrase** (hiện nếu chọn Passphrase/Cả hai)
- `FsTextField` "Passphrase" (masked, có nút hiện/ẩn 👁).
- **Strength meter** ngay dưới: thanh 4 mức (Yếu/Trung bình/Khá/Mạnh) đổi màu (danger→warn→info→success) + text gợi ý.
- Checklist yêu cầu (tick động khi đạt): "≥ 12 ký tự", "có chữ", "có số", "có ký hiệu".
- `FsTextField` "Nhập lại passphrase" + báo khớp/không khớp.
- Callout info: "FsNext không lưu passphrase ở đâu cả. Hãy ghi nhớ hoặc lưu vào trình quản lý mật khẩu."
- Footer: "Tiếp tục" (disabled tới khi đạt yêu cầu + khớp).

**`WIZ-4` — ⚠ CẢNH BÁO MẤT KHÓA (màn quan trọng nhất, an toàn dữ liệu)**
> Đây là màn bắt buộc theo đặc tả kỹ thuật. Thiết kế phải gây "chú ý nghiêm túc" nhưng không hù dọa kiểu spam.
- Khối cảnh báo nền danger-soft, viền danger, icon ⚠ lớn.
- Tiêu đề: "Đọc kỹ trước khi tiếp tục".
- Đoạn nội dung (giọng điềm tĩnh, không hù dọa, in đậm ý chính):
  - "Passphrase do bạn tự đặt và **chỉ mình bạn biết** — FsNext không lưu lại."
  - "Vì vậy nếu quên, chúng tôi **không thể khôi phục giúp bạn**, và file trong Vault sẽ không mở lại được."
  - "Đây cũng chính là điều làm nên sự riêng tư: không ai khác — kể cả Fshare — chạm được vào dữ liệu của bạn."
- Khối miễn trừ trách nhiệm rút gọn (nhắc lại từ `VAULT-INTRO`): "Mã hóa chạy trên máy bạn; FsNext không giữ khóa. Hãy luôn giữ thêm một bản sao lưu dữ liệu quan trọng ở nơi khác."
- Khối khuyến nghị (4 dòng, icon check):
  - "Lưu passphrase vào trình quản lý mật khẩu"
  - "In ra giấy, cất nơi an toàn"
  - "(Tùy chọn) cho một người tin cậy biết"
  - "Tải tệp khóa khôi phục ở bước sau"
- **4 checkbox xác nhận BẮT BUỘC** (mỗi cái 1 ý, KHÔNG gộp thành 1; câu chữ nhẹ nhàng):
  1. "Tôi hiểu Fshare không thể khôi phục passphrase giúp tôi."
  2. "Tôi hiểu nếu mất passphrase, các file trong Vault sẽ không mở lại được."
  3. "Tôi đã lưu passphrase ở nơi an toàn (hoặc sẽ làm ngay)."
  4. "Tôi đã đọc và hiểu các lưu ý trên."
- Nút **"Tôi hiểu, tiếp tục"** ở footer **DISABLED** tới khi cả 4 checkbox được tick. Khi disabled: tooltip "Hãy xác nhận cả 4 mục."
- Nút "Quay lại" luôn bật.

**`WIZ-5` — Tải khóa khôi phục (recovery keyfile)**
- Tiêu đề: "Tải tệp khóa khôi phục (khuyến nghị)".
- Mô tả: "Tệp này giúp bạn mở Vault nếu quên passphrase. Hãy lưu ở USB hoặc nơi tách biệt máy tính."
- Nút **"Tải tệp khóa (.key)"** → mở save dialog OS.
- Sau khi tải: trạng thái success "Đã lưu tệp khóa tại …" + nhắc "Đừng để chung với máy đang dùng."
- Cho phép **"Bỏ qua"** (text link) → confirm phụ "Bỏ qua khóa khôi phục? Bạn sẽ chỉ còn passphrase để mở Vault."
- Nếu `WIZ-2` chọn "Keyfile" → bước này là bắt buộc (không Bỏ qua được), và keyfile chính là phương thức mở khóa.

**`WIZ-6` — Đặt tên & vị trí Vault**
- `FsTextField` "Tên Vault" (mặc định "Personal Vault").
- Chọn thư mục Vault: hiển thị đường dẫn mặc định (vd `C:\Users\<name>\Fshare\Vault`) + nút "Đổi…".
- Toggle `FsSwitch`: "Tự xóa bản gốc sau khi mã hóa & tải lên" (mặc định TẮT, có tooltip giải thích rủi ro).
- Chọn cấp lưu khóa (radio, 3 mức — xem §4.6): **mặc định "Vừa — nhớ qua Windows (DPAPI), tự khóa khi đóng app"**. Kèm 1 dòng nhắc: "Dù chọn mức này, hãy vẫn ghi nhớ/sao lưu passphrase — cần khi đổi/cài lại máy."
- Footer: "Tạo Vault".

**`WIZ-7` — Hoàn tất**
- Icon success + "Vault đã sẵn sàng!".
- Tóm tắt: tên vault, vị trí, cấp bảo mật, đã/không có khóa khôi phục.
- Gợi ý hành động: nút "Thêm file đầu tiên" + "Đến trang Vault".

---

### 4.3 `DLG-UNLOCK` — Mở khóa Vault (modal nhỏ, ~420px rộng)
- Tiêu đề: "Mở khóa {tên Vault}" + icon ổ khóa.
- `FsTextField` "Passphrase" (masked, nút hiện/ẩn). Autofocus.
- (Nếu vault dùng keyfile) nút "Chọn tệp khóa…".
- Checkbox "Ghi nhớ trong phiên này" — kèm **cảnh báo bảo mật nhỏ** (icon info, màu warn): "Giảm an toàn — chỉ bật trên máy cá nhân."
- Vùng lỗi: khi sai → text danger "Giải mã thất bại. Kiểm tra lại passphrase." (KHÔNG nói rõ sai ở đâu — tránh lộ thông tin).
- Link "Quên passphrase?" → mở panel giải thích: nếu có keyfile → hướng dẫn dùng keyfile; nếu không → thông báo thẳng không thể khôi phục.
- Footer: "Hủy" + "Mở khóa" (primary, hiện spinner khi đang derive key — Argon2id mất ~0.5–1s, PHẢI có trạng thái loading, không để đơ).
- Trạng thái loading: nút "Mở khóa" → spinner + "Đang xác thực…".

---

### 4.4 `DLG-DECRYPT` — Tiến trình giải mã & mở file
- Có thể là toast/panel nhỏ không chặn, hoặc dialog nhỏ.
- Nội dung: tên file + `FsProgressBar` + "Đang giải mã…".
- Khi xong: tự mở bằng app mặc định OS → dialog chuyển thành thông báo "Đã mở {file}. Bản giải mã tạm sẽ tự xóa khi bạn đóng file."
- Nếu lỗi tampered: dialog danger "File có thể đã hỏng hoặc bị can thiệp. Không thể mở an toàn." + nút "Đóng".
- Khi đóng file gốc / hết 30': toast "Đã xóa bản giải mã tạm an toàn." (kèm ghi chú nhỏ về giới hạn SSD nếu hover help).

---

### 4.5 `DLG-CHGPASS` / `DLG-EXPORT` / `DLG-DELVAULT`

**`DLG-CHGPASS` — Đổi passphrase:**
- `FsTextField` "Passphrase hiện tại" (masked).
- `FsTextField` "Passphrase mới" + strength meter + checklist (như WIZ-3).
- `FsTextField` "Nhập lại passphrase mới".
- Callout: "File đã mã hóa vẫn dùng được bình thường, không cần mã hóa lại."
- Footer: "Hủy" + "Đổi passphrase" (loading spinner khi xử lý).
- Lỗi sai pass cũ: "Passphrase hiện tại không đúng."

**`DLG-EXPORT` — Xuất khóa khôi phục:**
- Yêu cầu nhập passphrase để xác nhận (chống người khác xuất khóa).
- Nút "Tải tệp khóa (.key)".
- Cảnh báo: "Bất kỳ ai có tệp này đều mở được Vault. Hãy bảo quản như chìa khóa nhà."

**`DLG-DELVAULT` — Xóa Vault (hành động nguy hiểm):**
- Nền danger, icon ⚠.
- Giải thích rõ điều gì xảy ra: "Xóa cấu hình Vault khỏi máy này. File `.fshenc` đã tải lên cloud vẫn còn nhưng sẽ KHÔNG mở được nếu bạn không còn khóa."
- Yêu cầu gõ tên Vault để xác nhận (pattern destructive confirm).
- Footer: "Hủy" + "Xóa Vault" (danger, disabled tới khi gõ đúng tên).

---

### 4.6 `SET-VAULT` — Tab "Vault" trong trang Settings hiện có
Thêm 1 tab "Vault" vào SettingsPage (đồng bộ style các tab Download/Upload/UI…). Các nhóm:

| Nhóm | Item | Control |
|---|---|---|
| Trạng thái | Tên vault, vị trí, trạng thái (Đã mở/Đang khóa) | text + badge |
| | "Khóa ngay" / "Mở khóa" | FsButton |
| Bảo mật | Cấp lưu khóa (3 mức) | radio group |
| | – Cao: nhập passphrase mỗi lần mở app (an toàn nhất) | |
| | – **Vừa: nhớ qua Windows (DPAPI), tự khóa khi đóng app** | **mặc định** |
| | – Tiện lợi: không tự khóa (CẢNH BÁO, cần xác nhận) | |
| | Tự khóa sau (phút) | slider/stepper (5/10/15/30/60) |
| | Phương pháp mã hóa (Nâng cao) | expander → §4.A (thuật toán + độ mạnh khóa) |
| Quản lý khóa | **Mở trang Quản lý khóa** | → `KEY-MANAGER` |
| | Đổi passphrase | → DLG-CHGPASS |
| | Xuất khóa khôi phục | → DLG-EXPORT |
| | Nhập khóa khôi phục (recovery) | file picker |
| Mã hóa | Tự xóa bản gốc sau khi tải lên | FsSwitch |
| | Mã hóa cả tên file (ẩn tên trên cloud) | FsSwitch (mặc định TẮT) |
| | Tự động mã hóa file đến | dropdown: Không giới hạn (mặc định) / 1 GB / 5 GB / Tùy chỉnh |
| | Khi file vượt ngưỡng | dropdown: Hỏi mỗi lần (mặc định) / Bỏ qua / Thêm thư mục thường (không mã hóa) |
| Nguy hiểm | Xóa Vault | → DLG-DELVAULT (nút danger) |

Khi chọn cấp "Tiện lợi" → bật confirm cảnh báo riêng.

---

### 4.A Lựa chọn phương pháp mã hóa (Algorithm & Security profile)
Người dùng thường KHÔNG cần chọn — mặc định đã an toàn. Chỉ lộ lựa chọn ở **mục Nâng cao** (expander trong `WIZ-6` và trong `SET-VAULT`). Danh sách *được kiểm soát*, kèm mô tả thân thiện (không thuật ngữ nặng):

**Triết lý:** người dùng KHÔNG chọn thuật toán cụ thể. App **tự chọn tối ưu**: AES-256-GCM cho file nhỏ (≤ 1 MiB, nhanh nhất nhờ AES-NI) và XChaCha20-Poly1305 cho file lớn/streaming (an toàn, tương thích rộng). UI chỉ phơi bày 1 lựa chọn "chế độ" + độ mạnh khóa.

| Nhóm | Lựa chọn | Nhãn hiển thị cho người dùng |
|---|---|---|
| Chế độ mã hóa | **Tự động** (mặc định) | "Khuyến nghị — FsNext chọn cách nhanh & an toàn nhất theo kích thước file" + badge "Mặc định" |
| | Luôn ưu tiên tương thích | "Dùng XChaCha20 cho mọi file — mở được trên mọi máy" (Nâng cao) |
| Độ mạnh khóa (Argon2id) | Cân bằng (mặc định) | "Mở khóa nhanh, đủ an toàn" |
| | Mạnh | "An toàn hơn, mở khóa chậm hơn một chút" |
| | Tối đa | "Bảo mật cao nhất, mở khóa chậm nhất" + cảnh báo tốn RAM |

- UI: radio/segmented control, mỗi mục 1 dòng mô tả. Mục mặc định highlight + badge "Khuyến nghị".
- **Tự tối ưu theo dung lượng (người dùng KHÔNG cần chỉnh):** file nhỏ mã hóa 1 lần trong RAM; file lớn chia chunk (256KB–1MB) để chặn bộ nhớ. Thiết kế chỉ cần 1 dòng info: "FsNext tự chọn cách mã hóa tối ưu theo kích thước file." Ô chỉnh ngưỡng/chunk để ở Nâng cao, ẩn mặc định.
- **Giới hạn cần truyền đạt nhẹ nhàng (chỉ ở mục Nâng cao/Trợ giúp):** "File nhỏ được mã hóa bằng AES (tăng tốc phần cứng). Một số máy rất cũ không hỗ trợ AES có thể không mở được các file này — chọn 'Luôn ưu tiên tương thích' nếu bạn cần mở trên nhiều máy khác nhau." (KHÔNG hiện cảnh báo này với người dùng thường.)
- Đổi chế độ chỉ áp dụng cho file MỚI; file cũ giữ nguyên (mỗi `.fshenc` tự ghi thuật toán).

---

### 4.B `KEY-MANAGER` — Trang Quản lý khóa
Truy cập: menu "…" trên `VAULT-HOME` hoặc tab `SET-VAULT`. Mục đích: xem các khóa đã/đang dùng, nhập khóa mới, sao chép an toàn, và nhắc sao lưu.

- **Header:** tiêu đề "Quản lý khóa" + nút "Nhập khóa…" (import keyfile) + nút "Tạo khóa mới" (nếu hỗ trợ nhiều khóa/standalone).
- **Banner an toàn (luôn hiện, nền warn-soft):** "Hãy giữ thêm một bản sao lưu khóa ở nơi an toàn, tách biệt với máy này (USB, két, trình quản lý mật khẩu). Mất khóa đồng nghĩa không mở được file đã mã hóa bằng khóa đó."
- **Danh sách khóa — mỗi item gồm:**
  | Item | Mô tả | Ghi chú bảo mật |
  |---|---|---|
  | Nhãn | tên do user đặt (vd "Personal Vault", "Khóa hợp đồng 2026") | |
  | Vân tay (fingerprint) | chuỗi rút gọn dạng mono (vd `9f3a…b21c`) để nhận diện | **KHÔNG hiển thị khóa thô** |
  | Loại | Vault / Keyfile / Recovery | |
  | Ngày tạo · lần dùng cuối | metadata | |
  | Trạng thái sao lưu | chip "Đã sao lưu" / "Chưa sao lưu" (nhắc nhở) | |
  | Hành động (hover) | **Sao chép vân tay** (mặc định) · Xuất khóa (.key) · Đổi nhãn · Xóa khỏi danh sách | |
- **Cơ chế copy an toàn (BẮT BUỘC thiết kế):**
  - Nút copy mặc định = copy **vân tay** (an toàn, để đối chiếu).
  - Muốn copy **khóa thật** → menu phụ "Sao chép khóa (nhạy cảm)" → mở confirm: "Khóa sẽ được sao vào clipboard và **tự xóa sau 30 giây**. Dán ngay vào nơi an toàn." → sau copy hiện đếm ngược + toast khi clipboard đã được dọn.
- **Nhập khóa (import):** chọn file `.key` hoặc dán chuỗi khóa → xác thực hợp lệ → đặt nhãn → thêm vào danh sách. Báo lỗi rõ nếu khóa sai định dạng.
- **Trạng thái rỗng:** "Chưa có khóa nào. Tạo Vault hoặc nhập khóa để bắt đầu."
- **Lưu trữ:** nhấn mạnh trong spec dev (không phải UI): danh sách khóa này phải được **mã hóa khi lưu** (không để khóa thô trên đĩa dạng thường); reveal/copy khóa thật yêu cầu vault đang mở khóa.

---

### 4.C `DLG-BATCH` — Mã hóa/Giải mã (1 file · nhiều file · cả thư mục) + tự tải lên Fshare
Truy cập (3 chế độ chọn — cùng đổ về 1 pipeline):
- **1 file:** nút "Thêm file" → dialog OS chọn 1 file, hoặc kéo-thả 1 file.
- **Nhiều file:** "Thêm file" với multi-select, hoặc kéo-thả nhiều file cùng lúc.
- **Cả thư mục:** nút "Thêm thư mục" → chọn thư mục, hoặc kéo-thả thư mục (đệ quy).

> Với 1 file đơn, có thể rút gọn thành thao tác inline trên `VAULT-HOME` (không cần mở dialog batch đầy đủ); dialog batch dùng khi ≥2 file hoặc thư mục.

- **Bước 1 — Chọn & cấu hình:**
  - Tóm tắt nguồn đã chọn: số file + tổng dung lượng ước tính (với thư mục: hiện đường dẫn gốc + số file đệ quy).
  - (Khi chọn nhiều file) danh sách file đã chọn, cho phép bỏ bớt từng file.
  - Chế độ: **Mã hóa** (mặc định) hoặc **Giải mã** (segmented control).
  - Toggle `FsSwitch`:
    - "Giữ nguyên cấu trúc thư mục con" (mặc định BẬT).
    - "Mã hóa cả tên file" (mặc định TẮT, tooltip giải thích).
    - "**Tự tải lên Fshare sau khi mã hóa**" (mặc định TẮT) → khi bật, hiện chọn thư mục đích trên cloud.
    - "Xóa bản gốc sau khi mã hóa & tải lên thành công" (mặc định TẮT, cảnh báo).
  - Lưu ý filter: cho phép loại trừ (vd bỏ qua file tạm, file ẩn) — tái dùng pattern ignore của Sync nếu có.
- **Bước 2 — Tiến trình (non-blocking, có thể thu nhỏ về HUD):**
  - `FsProgressBar` tổng + "Đã xử lý 12/340 file".
  - Danh sách cuộn: mỗi file 1 dòng trạng thái — Chờ / Đang mã hóa / Đang tải lên / Xong / Bỏ qua / **Lỗi** (kèm lý do).
  - Nút "Tạm dừng" / "Hủy" / "Thu nhỏ".
  - Khi có lỗi từng file → **không dừng cả lô**; cuối cùng tổng kết "335 thành công · 5 lỗi" + nút "Thử lại các file lỗi".
- **Bước 3 — Hoàn tất:** tóm tắt số file mã hóa, đã tải lên, dung lượng; nút "Mở trang Vault" / "Xem file trên cloud".
- **Edge cases cần thiết kế trạng thái:** file đang bị khóa bởi app khác, hết dung lượng đĩa, mất mạng giữa chừng (tải lên tạm dừng & tiếp tục), vault tự khóa giữa lô (yêu cầu mở lại để tiếp tục).

---

### 4.D `DLG-LARGEFILE` — Xác nhận mã hóa file lớn (>1 GB)
Hiện khi thêm file > 1 GB (ngưỡng cấu hình được). Mục tiêu: minh bạch chi phí, để người dùng quyết định.
- Tiêu đề: "Mã hóa file lớn?" + tên file + dung lượng.
- Thông tin (icon info, KHÔNG hù dọa):
  - "Ước tính thời gian mã hóa: ~{n} giây."
  - "Cần thêm khoảng {size} dung lượng đĩa tạm thời."
  - "Lưu ý: file lớn phải được giải mã toàn bộ trước khi xem (không xem trực tiếp)."
- 3 lựa chọn (nút):
  1. **"Mã hóa & thêm vào Vault"** (primary).
  2. **"Thêm không mã hóa"** → file vào **thư mục thường** (ngoài Vault), KHÔNG nằm trong Vault. Hiện rõ "File này sẽ KHÔNG được bảo vệ."
  3. "Hủy".
- Checkbox: "Áp dụng cho tất cả file lớn trong lượt này" (khi batch nhiều file lớn).
- Tôn trọng setting `max_encrypt_size`: nếu người dùng đã đặt "Bỏ qua" / "Thêm thư mục thường" thì KHÔNG hỏi lại (làm theo setting), trừ khi để "Hỏi mỗi lần".

---

### 4.7 `TRAY-VAULT` — System tray (bổ sung vào tray hiện có)
- **Badge trạng thái:** thêm chỉ báo khóa nhỏ lên icon tray khi Vault tồn tại — ổ khóa **mở** (success) khi unlocked, ổ khóa **đóng** (xám) khi locked.
- **Menu chuột phải** thêm mục: "Vault: Đã mở / Đang khóa" (header), "Khóa Vault" / "Mở khóa Vault", "Mở trang Vault".
- Tooltip tray cập nhật: "Vault: đang khóa" / "Vault: đã mở".

---

### 4.8 `TOASTS` — Thông báo (in-app FsToastHost + Windows toast)
Thiết kế template cho các sự kiện:
- success: "Đã mã hóa {file}", "Đã đổi passphrase", "Đã tải tệp khóa".
- info: "Vault đã tự khóa do không hoạt động."
- warn: "Còn 1 phút nữa Vault sẽ tự khóa." (tùy chọn, trước auto-lock).
- danger: "Giải mã thất bại — sai khóa", "File có thể bị can thiệp".
KHÔNG toast mỗi file routine (tránh spam). Mỗi toast: icon + tiêu đề + 1 dòng + (action nếu cần).

---

## 5. TRẠNG THÁI DÙNG CHUNG (phải thiết kế cho mọi màn liên quan)
Mỗi danh sách/hành động cần có đủ: **Empty · Loading · Error · Locked · Success.**
- **Loading derive-key:** Argon2id ~0.5–1s → luôn có spinner/skeleton, không freeze.
- **Loading mã hóa/giải mã file lớn:** progress bar có % + tốc độ + nút Hủy.
- **Error:** thông báo ngắn gọn, không lộ chi tiết crypto; có hành động khắc phục (Thử lại / Đóng).
- **Locked overlay:** khi vault khóa, nội dung nhạy cảm bị làm mờ + lớp phủ "Mở khóa để xem".

---

## 6. MICROCOPY & TONE (tiếng Việt)
- Tông: rõ ràng, trấn an, **không thuật ngữ kỹ thuật** với người dùng cuối (tránh "DEK/KEK/AEAD/Argon2id" trên UI; chỉ dùng "khóa", "passphrase", "mã hóa").
- Nhất quán: dùng "passphrase" (không lẫn "mật khẩu" trừ khi cần thân thiện) — **team thiết kế chốt 1 thuật ngữ và dùng xuyên suốt**.
- Mọi chuỗi sẽ được dịch sang English (app song ngữ) → cung cấp bản EN song song cho mỗi chuỗi trong file giao.
- Lỗi crypto luôn **mơ hồ có chủ đích** (không nói "sai byte thứ N", chỉ "Giải mã thất bại").

---

## 7. ICON CẦN BỔ SUNG (cùng style line hiện có)
Ổ khóa đóng, ổ khóa mở, shield, chìa khóa (key), tệp khóa (.key/document-key), huy hiệu khóa nhỏ (overlay góc file), đồng hồ auto-lock, cảnh báo (đã có), mắt hiện/ẩn (đã có).

---

## 8. RESPONSIVE / KÍCH THƯỚC
- App là cửa sổ desktop (không mobile). Hỗ trợ thay đổi kích thước cửa sổ; nội dung co giãn, danh sách scroll.
- Dialog modal: chiều rộng ~420–560px, căn giữa, có overlay tối.
- Wizard: ~640–760px rộng, chiều cao co theo nội dung, scroll khi tràn (đặc biệt WIZ-4 nhiều text).
- Hỗ trợ DPI scaling (125%/150%/175%).

---

## 9. ACCESSIBILITY & I18N
- Điều hướng bàn phím đầy đủ (Tab/Shift+Tab, Enter submit, Esc hủy/đóng dialog).
- Tương phản đạt WCAG AA (đặc biệt text cảnh báo trên nền màu).
- Trạng thái checkbox/nút disabled rõ ràng (không chỉ dựa vào màu).
- Mọi chuỗi đều dịch được (VI nguồn + EN). Tránh chữ trong ảnh.
- Strength meter & trạng thái có nhãn text (không chỉ màu) cho người mù màu.

---

## 10. DELIVERABLES MONG ĐỢI TỪ TEAM THIẾT KẾ
1. **Figma file** với toàn bộ màn hình/surface ở §2 (18 mục, gồm trang giới thiệu, quản lý khóa, batch thư mục), cả **Light + Dark**.
2. Tất cả **trạng thái** ở §5 cho từng màn (empty/loading/error/locked/success).
3. **Flow prototype** clickable cho Flow A (tạo vault) và Flow B–D (mở khóa, mã hóa, giải mã).
4. **Bảng microcopy VI + EN** (mọi label/nút/lỗi/toast).
5. Bộ **icon mới** (SVG) ở §7 đúng style hệ thống.
6. **Spec handoff**: spacing, token màu dùng (tham chiếu Aurora), kích thước, trạng thái component — để dev map về `Fshare.Components`.
7. Thiết kế **WIZ-4 (cảnh báo mất khóa)** là hạng mục ưu tiên review riêng (an toàn dữ liệu).

---

## 11. CHECKLIST NGHIỆM THU THIẾT KẾ
- [ ] Dùng đúng token Aurora, không hardcode màu/size lạ.
- [ ] Đủ Light + Dark cho mọi màn.
- [ ] WIZ-4: nút disabled tới khi tick đủ 4 checkbox; cảnh báo rõ, nghiêm túc, không spam.
- [ ] DLG-UNLOCK có trạng thái loading khi derive key (không freeze).
- [ ] Không hiện đuôi `.fshenc` cho người dùng; thay bằng huy hiệu khóa + tên gốc.
- [ ] Lỗi crypto mơ hồ có chủ đích, không lộ chi tiết.
- [ ] Tái dùng FsButton/FsTextField/FsCard/FsBadge/FsSwitch/FsProgressBar/FsIcon.
- [ ] Tray badge phản ánh đúng locked/unlocked.
- [ ] Microcopy VI + EN đầy đủ, thuật ngữ nhất quán.
- [ ] Keyboard nav + WCAG AA + DPI scaling.
- [ ] `VAULT-INTRO` có khối miễn trừ trách nhiệm + nhắc sao lưu, giọng nhẹ nhàng.
- [ ] `KEY-MANAGER`: chỉ hiện vân tay; copy khóa thật có confirm + tự xóa clipboard 30s; banner nhắc sao lưu khóa nơi khác.
- [ ] Phương pháp mã hóa chỉ ở mục Nâng cao, mặc định khuyến nghị; đổi thuật toán không ảnh hưởng file cũ.
- [ ] `DLG-BATCH`: hỗ trợ 3 chế độ chọn (1 file / nhiều file / cả thư mục); lỗi từng file không dừng cả lô; có "Thử lại các file lỗi"; toggle tự tải lên Fshare.
- [ ] Chế độ mã hóa mặc định "Tự động" (app chọn AES file nhỏ / XChaCha file lớn); người dùng không phải chọn thuật toán thô.
- [ ] File <100MB im lặng · 100MB–1GB có progress+ước tính · >1GB hiện `DLG-LARGEFILE`.
- [ ] File không mã hóa KHÔNG bao giờ nằm trong Vault như "đã bảo vệ" (vào thư mục thường hoặc bỏ qua, có nhãn rõ).
- [ ] Setting `max_encrypt_size` + hành vi khi vượt ngưỡng có trong SET-VAULT.
- [ ] Câu chữ cảnh báo đã làm mềm (không lặp "không thể đảo ngược"/"vĩnh viễn").

---

## 12. NGOÀI PHẠM VI BRIEF NÀY (đừng thiết kế ở đợt này)
- Sync 2 chiều / conflict UI.
- **Auto-watch** folder Vault (tự mã hóa nền khi OS phát hiện file mới) — KHÁC với `DLG-BATCH` (người dùng chủ động chọn thư mục để mã hóa, đã nằm TRONG phạm vi).
- Shell extension overlay icon trong Windows Explorer.
- File-association `.fshenc` (mở từ ngoài app).
- Multi-vault management UI đầy đủ (chỉ chừa chỗ mở rộng, chưa thiết kế chi tiết).
