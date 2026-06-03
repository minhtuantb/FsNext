# Run: 2026-06-03 — All-screens sweep — r1

- **Loại**: UI screen sweep (discovery, có người xem) qua ui-driver (PrintWindow+SendInput).
- **Build**: `output/FsNext.exe` rebuild 2026-06-03 15:41 (sau khi sửa SyncService/UploadEngine), `[PASS] Build successful`.
- **Môi trường**: 1920×1080, DPI 100%, cửa sổ maximize 1936×1048, auto-lock/sleep AC = off.
- **Account**: taikhoantestfshare@gmail.com (test), PromoPlus.
- **Phạm vi**: render + điều hướng cấp màn hình cho 6 module mới (auth/home/myfiles/favorites/settings+userinfo/tray-hud chưa sweep ở đợt này).
- **Crash-watch**: 0 crash (Event Log + alive=true suốt phiên).

## Kết quả

| Case | Mô tả | Evidence | Kết quả | Ghi chú |
|---|---|---|---|---|
| AUTH-UI-001 | Render màn login đầy đủ phần tử | 00-login.png | ✅ PASS | Đủ email/password/social Google-FB-Zalo/giữ đăng nhập/quên MK/footer TLS1.3·SRP |
| AUTH-UI-007 | Login thành công → vào Home | 01-home.png | ✅ PASS | Chuyển sang sidebar + HomePage |
| HOME-UI-001 | Render trang chủ | 01-home.png | ✅ PASS | Search/Tải lên/gear/greeting/4 quick-action/File gần đây + filter tabs |
| MYFILES-UI-001 | Render trang + toolbar | 02-myfiles.png | ✅ PASS | "Files · 12 mục · Thư mục gốc", Search/New Folder/Refresh/toggle list-grid, cột Name/Size, breadcrumb |
| MYFILES-UI-004 | Detail panel khi chưa chọn | 02-myfiles.png | ✅ PASS | "Chọn file hoặc thư mục để xem thông tin" |
| FAV-UI-001 | Render trang yêu thích | 03-favorites.png | ✅ PASS | "Yêu thích · 6 mục đã gắn sao", search, Làm mới, cột Tên/Kích thước, breadcrumb ❤ |
| FAV-UI-007 | Detail panel (empty) | 03-favorites.png | ✅ PASS | Panel empty state |
| SET-UI-011 | Render UserInfo | 06-userinfo.png | ✅ PASS | VIP card 52.1/300GB, QUYỀN LỢI (AES-256, 97000đ), thông tin cá nhân + badge "ĐÃ XÁC MINH", 3 cột storage |
| SET-UI-012 | Storage bar màu theo % | 06-userinfo.png | ✅ PASS | DL bảo đảm 100% → đỏ (danger), không bảo đảm 0.8% → xanh — đúng logic |
| SET-UI-001 | Render đủ section Settings | 07-settings.png | ✅ PASS | Chung (8 toggle), Khi tên file trùng (4 radio), Language |
| SET-UI-007 | File-conflict radio | 07-settings.png | ✅ PASS | "Đổi tên (1)" đang chọn (mặc định), 3 option còn lại |
| SET-UI-010 | Đổi ngôn ngữ runtime sang EN | 08→14 | ✅ PASS (retry) | Lần đầu inconclusive do click nhầm menu; retry qua segmented Vi/En trong Settings → toàn app dịch EN ngay (Home + Settings), không sót VI, switch về VN OK |

## Round 1b — tương tác sâu (cùng phiên)

| Case | Mô tả | Evidence | Kết quả | Ghi chú |
|---|---|---|---|---|
| SET-UI-010 | Switch EN runtime (retry) | 10,11,12 | ✅ PASS | "Hi, Test, shall we continue?" + Settings "Transfer Performance"… dịch đủ; không restart |
| SET-UI-003 | Proxy host/port gating | 09 | ✅ PASS | Proxy "Không dùng" → KHÔNG hiện host/port (chỉ hiện khi "Thủ công") |
| SET-UI-005 | NumberSpinner render + dải | 09,10 | ✅ PASS | 4 spinner (DL 2 [1–16], Luồng 4 [1–32], UL 2 [1–8], Tổng 10 [1–32]) hiện đúng |
| HOME-UI-002 | Search <3 ký tự → TooShort | 13 | ✅ PASS | Gõ "ab" → chip "Nhập tối thiểu 3 ký tự", không overlay, submit chặn |
| HOME-UI-003 | Keyword → overlay kết quả | 14 | ✅ PASS | Gõ "phim" → "KẾT QUẢ TÌM KIẾM · 4 mục", 4 thư mục khớp, border accent |
| MYFILES-UI-005 | Detail panel chọn 1 file | 15 | ✅ PASS | Panel Thông tin: Mở thư mục/THAO TÁC/CHI TIẾT/BẢO MẬT đầy đủ |
| MYFILES-UI-008 | Cờ bảo mật trong panel | 15 | ✅ PASS | Secure: Bật · Mật khẩu: Không · Direct link: Tắt |
| MYFILES-UI-006 | Multi-select bulk (Ctrl+A) | 16 | ⚠️ INCONCLUSIVE | Ctrl+A không đổi selection — nghi focus không ở list khi gửi phím (lái mù). KHÔNG kết luận bug; cần Ctrl+Click thật / qmltest |

## Bug mở: 0

Không phát hiện lỗi render/crash trong cả 2 round. 2 case INCONCLUSIVE là giới hạn lái-mù (đã giải quyết SET-UI-010; còn MYFILES-UI-006).

## Điều hướng đã học (bổ sung cho GUIDE)
- Cửa sổ **đăng nhập đã maximize** (1936×1048) — toạ độ login GUIDE dùng được; `$global:FsRect.W/H` để trống là quirk parse (L/T=-8 = maximized).
- **Bánh răng Cài đặt (1762 91) chỉ có trên Home** — ở trang khác phải vào Settings qua **menu tài khoản** (góc dưới trái 110 997): Thông tin tài khoản (~125 797) · Cài đặt (~95 839) · Language (~125 881) · Đăng xuất (~125 933).

## Khuyến nghị đợt sau
- Retry SET-UI-010 với toạ độ segmented Vi/En (cuộn Settings xuống section Language) để xác nhận i18n switch toàn app.
- Sweep tiếp các case tương tác sâu: MYFILES-UI-012 (xóa có xác nhận), MYFILES-UI-006 (multi-select), FAV-UI-005 (bỏ yêu thích), TRAY-UI-* (cần tạo transfer + minimize).
- Cân nhắc viết qmltest (mock VM) cho các case gating thuần (AUTH-UI-002, HOME-UI-002/005, SET-UI-003/004) để vào CI, đỡ lái mù.
