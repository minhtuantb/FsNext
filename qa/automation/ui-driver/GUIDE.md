# Hướng dẫn STEP-BY-STEP — Tự động hóa UI test FsNext (như 1 tester)

> Bạn đóng vai tester thao tác trên giao diện thật. Mọi lệnh chạy trong
> **PowerShell**. Thư viện lái UI: `qa/automation/ui-driver/fsnext-ui.ps1`.
> Áp dụng cho **mọi module** (Login, Trang chủ, My Files, Yêu thích, Tải xuống,
> Tải lên, Đồng bộ, Vault, Cài đặt, Tray/HUD).

---

## PHẦN 0 — Chuẩn bị (làm 1 lần mỗi máy / mỗi buổi)

**0.1 TẮT tự khóa & sleep màn hình** (BẮT BUỘC — màn khóa = secure desktop, chặn
hoàn toàn automation):
```powershell
# tắt sleep + tắt tắt-màn-hình khi cắm điện (0 = never)
powercfg /change monitor-timeout-ac 0
powercfg /change standby-timeout-ac 0
# Settings → Accounts → Sign-in options → "If you've been away..." = Never
# Settings → Personalization → Lock screen → Screen saver = (None)
```

**0.2 Kiểm tra DPI = 100%** (để toạ độ ảnh = toạ độ click, 1:1):
```powershell
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Screen]::PrimaryScreen.Bounds   # ghi nhớ Width x Height (vd 1920x1080)
```
Nếu Windows scaling ≠ 100% (Settings → Display → Scale), toạ độ sẽ lệch — đặt 100%
hoặc nhân hệ số khi đọc toạ độ.

**0.3 Build app** (nếu chưa):
```powershell
$env:VCPKG_ROOT = "D:\OneDrive - FPT Corporation\Work\Fshare\tool\vcpkg"
cmd /c "D:\Work\FsNext\scripts\build.bat"
```

**0.4 Tài khoản test** (đừng dùng tài khoản thật): `taikhoantestfshare@gmail.com` / `Test2024@`.

---

## PHẦN 1 — Từ vựng lệnh (nạp 1 lần đầu mỗi cửa sổ PowerShell)

```powershell
. D:\Work\FsNext\qa\automation\ui-driver\fsnext-ui.ps1
```

| Lệnh | Tác dụng | Ví dụ |
|---|---|---|
| `Fs-Launch` | Kill sạch + mở app + chờ cửa sổ (tránh race) | `Fs-Launch` |
| `Fs-Login $email $pass` | Điền + bấm Đăng nhập | `Fs-Login "taikhoantestfshare@gmail.com" "Test2024@"` |
| `Fs-Front` | Đưa lên trước + **maximize** + cache rect (LUÔN gọi trước khi click) | `Fs-Front \| Out-Null` |
| `Fs-Click x y [$warm]` | Click toạ độ **tương đối cửa sổ** (warm=true: click trung tính trước) | `Fs-Click 75 342 $true` |
| `Fs-ClickScreen x y` | Click toạ độ **màn hình tuyệt đối** (cho native dialog) | `Fs-ClickScreen 390 262` |
| `Fs-Type "..."` | Gõ phím (SendKeys). `^a`=Ctrl+A, `{DEL}`, `{ENTER}` | `Fs-Type "abc{ENTER}"` |
| `Fs-Grab path` | Chụp **cửa sổ FsNext** (kể cả bị che; KHÔNG bắt native dialog) | `Fs-Grab "D:\Work\FsNext\.ui.png"` |
| `Fs-GrabScreen path` | Chụp **toàn màn hình** (để định vị native dialog) | `Fs-GrabScreen "D:\Work\FsNext\.scr.png"` |
| `Fs-CrashWatch [phút]` | Liệt kê crash FsNext trong Event Log | `Fs-CrashWatch 10` |
| `Fs-Alive` / `Fs-Locked` | App còn chạy? / Màn hình đang khóa? | `Fs-Alive` |

> **Quy tắc vàng về toạ độ:** sau `Fs-Front` (maximize ⇒ ảnh ~1936×1048), **pixel
> trong ảnh `Fs-Grab` = chính tham số `Fs-Click`** (1:1). Muốn biết click ở đâu:
> mở `.ui.png`, nhìn pixel mục tiêu → dùng đúng số đó. Với native dialog: pixel
> trong ảnh `Fs-GrabScreen` = tham số `Fs-ClickScreen`.

---

## PHẦN 2 — Bắt đầu 1 SESSION làm việc (khung chuẩn)

```powershell
# (1) Nạp thư viện + biến session
. D:\Work\FsNext\qa\automation\ui-driver\fsnext-ui.ps1
$run = "D:\Work\FsNext\qa\runs\$(Get-Date -Format yyyy-MM-dd)-manual-r1"
$ev  = "$run\evidence"; New-Item -ItemType Directory -Force $ev | Out-Null
function Shot($n){ Fs-Front | Out-Null; Fs-Grab "$ev\$n.png"; "saved $n.png" }

# (2) Kiểm tiền đề
if (Fs-Locked) { "DỪNG: màn hình đang khóa — mở khóa + tắt auto-lock"; return }

# (3) Mở app + đăng nhập
Fs-Launch | Out-Null
Fs-Login "taikhoantestfshare@gmail.com" "Test2024@"
Shot "00-home"
```
Mở `qa\runs\...\evidence\00-home.png` để xác nhận đã vào Trang chủ. Từ đây mỗi
bước test = (Fs-Front → Fs-Click → Shot tên-bước → mở ảnh kiểm).

---

## PHẦN 3 — Chu trình 1 bước test (lặp cho mọi thao tác)

```powershell
Fs-Front | Out-Null                 # luôn đưa lên trước trước khi click
Fs-Click <x> <y> $true              # click đầu tiên của bước: warm=$true
Start-Sleep -Milliseconds 1000      # chờ UI phản hồi
Shot "10-buoc-X"                    # chụp bằng chứng
# → mở ảnh, đối chiếu Kết quả kỳ vọng của case → PASS/FAIL
```
- Click thứ 2,3… trong cùng bước: `$warm=$false`.
- Sau hành động rủi ro (mã hóa file lớn, kill app…): `Fs-CrashWatch 5`.

---

## PHẦN 4 — BẢN ĐỒ ĐIỀU HƯỚNG (toạ độ đã khám phá, cửa sổ maximize 1936×1048 @1080p)

> Nếu đổi độ phân giải/scale ⇒ phải khám phá lại (xem Phần 6). Sidebar dùng x≈75.

**Sidebar (mọi module):**
| Module | Lệnh |
|---|---|
| Trang chủ | `Fs-Click 75 112 $true` |
| My Files (quản lý file cloud) | `Fs-Click 75 146 $true` |
| Yêu thích | `Fs-Click 75 180 $true` |
| Tải xuống | `Fs-Click 75 227 $true` |
| Tải lên | `Fs-Click 75 261 $true` |
| Đồng bộ | `Fs-Click 75 295 $true` |
| Vault | `Fs-Click 75 342 $true` |
| User menu (góc dưới trái) | `Fs-Click 110 997 $true` |

**Login:** email `1230 556` · mật khẩu `1230 628` · Đăng nhập `1240 722`.

**Vault header:** Khóa ngay `1338 87` · Thêm file `1472 87` · Thêm thư mục `1622 87`
· Trợ giúp `1766 87` · menu "…" `1865 87`.
**Vault — menu "…":** Quản lý khóa `1745 143` · Đổi passphrase `1755 182` · Xuất
khóa khôi phục `1768 220` · Cài đặt Vault `1748 258` · Xóa Vault `1738 305`.
**Vault — nút "Mở" của file:** x≈`1838`, dòng đầu y≈`165`, mỗi dòng cách ~`68`.
**DLG-LARGEFILE:** Hủy `939 669` · Thêm không mã hóa `1062 669` · Mã hóa & thêm `1232 669`.

**Search/Tải lên ở Trang chủ:** ô search `700 91` · nút "Tải lên" `1835 91` · bánh răng (cài đặt) `1762 91`.

> Các module khác (My Files toolbar, Tải xuống/Tải lên danh sách, Đồng bộ, Cài
> đặt tabs…) chưa map sẵn — dùng Phần 6 để tự lấy toạ độ trong 30 giây.

---

## PHẦN 5 — SESSION MẪU theo module (copy-paste từng lệnh)

### 5.1 Vault — luồng đầy đủ (tham chiếu, đã chạy thật)
```powershell
. D:\Work\FsNext\qa\automation\ui-driver\fsnext-ui.ps1
$ev="D:\Work\FsNext\qa\runs\vault\evidence"; New-Item -Force -ItemType Directory $ev|Out-Null
function Shot($n){ Fs-Front|Out-Null; Fs-Grab "$ev\$n.png" }
Fs-Launch|Out-Null; Fs-Login "taikhoantestfshare@gmail.com" "Test2024@"
# vào Vault (nếu đã có vault + L2/DPAPI → tự mở khóa)
Fs-Front|Out-Null; Fs-Click 75 342 $true; Start-Sleep 1; Shot "vault-home"
# Thêm 1 file nhỏ → mã hóa (qua dialog OS)
Fs-Front|Out-Null; Fs-Click 1472 87 $true; Start-Sleep -Milliseconds 1800   # Thêm file
Fs-Type "D:\Work\FsNext\.vaulttest\note-small.txt"; Start-Sleep -Milliseconds 400
Fs-Type "{ENTER}"; Start-Sleep 3; Shot "vault-added-small"
# Mở (giải mã) file rồi Khóa ngay → secure-delete
Fs-Front|Out-Null; Fs-Click 1838 300 $true; Start-Sleep 3                    # Mở dòng 3
Fs-Front|Out-Null; Fs-Click 1338 87 $true; Start-Sleep 1; Shot "vault-locked"
# kiểm temp đã wipe
(Get-ChildItem "$env:TEMP\FsNextVault" -Recurse -File -EA SilentlyContinue).Count   # kỳ vọng 0
```

### 5.2 Native dialog (Thêm thư mục / chọn file / save) — kỹ thuật chung
```powershell
Fs-Front|Out-Null; Fs-Click 1622 87 $true; Start-Sleep -Milliseconds 2000    # mở dialog OS
Fs-Type "D:\Work\FsNext\.vaulttest\folder-batch"; Start-Sleep -Milliseconds 600
Fs-GrabScreen "D:\Work\FsNext\.scr.png"     # CHỤP TOÀN MÀN HÌNH để thấy dialog
# → mở .scr.png, đọc pixel nút "Select Folder" (vd 390,262) rồi:
Fs-ClickScreen 390 262; Start-Sleep 1
Fs-Front|Out-Null; Fs-Grab "$ev\batch-config.png"
```
> Mẹo: native dialog không bắt được bằng `Fs-Grab` (chỉ chụp cửa sổ chính) →
> **luôn dùng `Fs-GrabScreen` + `Fs-ClickScreen`** cho file/folder picker.

### 5.3 Đăng nhập / Trang chủ
```powershell
Fs-Launch|Out-Null; Fs-Front|Out-Null; Fs-Grab "$ev\login.png"   # màn login
Fs-Login "taikhoantestfshare@gmail.com" "Test2024@"; Shot "home"
# test ô search ở Trang chủ:
Fs-Front|Out-Null; Fs-Click 700 91 $true; Fs-Type "test{ENTER}"; Start-Sleep 2; Shot "home-search"
```

### 5.4 My Files / Tải xuống / Tải lên / Đồng bộ / Yêu thích / Cài đặt
Khung chung — chỉ đổi toạ độ sidebar + nút trong trang (lấy bằng Phần 6):
```powershell
Fs-Front|Out-Null; Fs-Click 75 146 $true; Start-Sleep 1; Shot "myfiles"      # My Files
Fs-Front|Out-Null; Fs-Click 75 227 $true; Start-Sleep 1; Shot "downloads"    # Tải xuống
Fs-Front|Out-Null; Fs-Click 75 261 $true; Start-Sleep 1; Shot "uploads"      # Tải lên
Fs-Front|Out-Null; Fs-Click 75 295 $true; Start-Sleep 1; Shot "sync"         # Đồng bộ
Fs-Front|Out-Null; Fs-Click 75 180 $true; Start-Sleep 1; Shot "favorites"    # Yêu thích
Fs-Front|Out-Null; Fs-Click 1762 91 $true; Start-Sleep 1; Shot "settings"    # bánh răng → Cài đặt
```
Trong mỗi trang: chụp → mở ảnh → đọc toạ độ nút cần test (Phần 6) → `Fs-Click` →
chụp lại → đối chiếu case.

---

## PHẦN 6 — TỰ KHÁM PHÁ TOẠ ĐỘ (30 giây, cho mọi nút chưa có trong bản đồ)

```powershell
Fs-Front | Out-Null
Fs-Grab "D:\Work\FsNext\.ui.png"     # ảnh = đúng kích thước cửa sổ (1936x1048)
```
1. Mở `.ui.png`. Ảnh CHÍNH LÀ hệ toạ độ click.
2. Đưa con trỏ tới tâm nút trong ảnh, đọc pixel (X,Y) — đa số trình xem ảnh hiện
   toạ độ; hoặc ước lượng theo tỉ lệ (nút giữa-ngang ≈ X 968; mép phải ≈ 1850).
3. Click: `Fs-Front|Out-Null; Fs-Click X Y $true` → chụp lại kiểm.
4. Với native dialog: thay bằng `Fs-GrabScreen` + `Fs-ClickScreen`.

> Lệnh tiện: hiện toạ độ con trỏ hiện tại (rê chuột tới nút rồi chạy):
```powershell
Add-Type -AssemblyName System.Windows.Forms
$p=[System.Windows.Forms.Cursor]::Position; "screen=($($p.X),$($p.Y))  window-rel=($($p.X-$global:FsRect.L),$($p.Y-$global:FsRect.T))"
```

---

## PHẦN 7 — GHI KẾT QUẢ + MỞ BUG (cuối mỗi case fail)

```powershell
# tạo nhanh file bug từ template
$id="VLT-BUG-0007"
Copy-Item "D:\Work\FsNext\qa\templates\bug-template.md" "D:\Work\FsNext\qa\reports\bugs\$id.md"
# rồi sửa front-matter (status: NEW, severity, found_by, found_run) + dán ảnh từ $ev
# cập nhật bảng trong qa\reports\BUG-INDEX.md (thêm 1 dòng status NEW)
```
Ghi pass/fail từng case vào `qa\runs\...\results.md` (mẫu: `qa\templates\run-template.md`).

---

## PHẦN 8 — SỰ CỐ & KHẮC PHỤC (đã gặp thật)

| Triệu chứng | Nguyên nhân | Khắc phục |
|---|---|---|
| Ảnh ra **lock screen** / `Fs-Locked`=true | Màn hình khóa (secure desktop) | Mở khóa + Phần 0.1 tắt auto-lock |
| Click **trượt**, text vào sai ô | Cửa sổ **không maximize** lúc click | `Fs-Front` lại + kiểm `$global:FsRect` (phải ~1936×1048) trước khi click |
| Click **đầu tiên không ăn** | Cú input đầu bị nuốt sau khi đưa cửa sổ lên trước | Dùng `$warm=$true` cho click đầu của mỗi bước |
| `Fs-Grab` lỗi null / `alive=False` | App đã thoát | `Fs-Launch` lại + `Fs-Login`; chạy `Fs-CrashWatch 15` xem có crash |
| Native dialog không bắt được bằng `Fs-Grab` | PrintWindow chỉ chụp cửa sổ chính | Dùng `Fs-GrabScreen` + `Fs-ClickScreen` |
| Toạ độ lệch một nửa | DPI scaling ≠ 100% | Đặt scale 100% (Phần 0.2) hoặc nhân hệ số |
| App **tự thoát giữa lúc sync** | (đã fix CURLSH) — nếu tái diễn | `Fs-CrashWatch` lấy mã lỗi → mở bug |

---

## PHẦN 9 — GIỚI HẠN (khi nào KHÔNG nên "lái mù")
- Luồng nhiều bước + native dialog (file/folder picker) → **dễ flaky**; cân nhắc
  **làm tay** hoặc viết **qmltest** (mock ViewModel) cho regression lặp lại.
- "Lái mù" hợp để **khám phá / chạy 1 đợt có người ngồi xem**; KHÔNG thay thế
  test tự động trong CI. Phân vai: logic→QtTest, UI-component→qmltest, end-to-end→
  UI-driver/tay (xem `qa/README.md §3`).
