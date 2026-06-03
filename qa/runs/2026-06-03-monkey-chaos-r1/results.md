# Run: 2026-06-03 — Monkey + Chaos (săn crash) — r1

- **Mục tiêu**: stress tìm crash — click liên tục, chuyển trang/gọi API không đợi phản hồi, kill app giữa chừng.
- **Loại**: CHAOS-MK (monkey) + CHAOS (crazy) qua ui-driver, có crash-watch (Event Log + fsnext.log).
- **Build**: output/FsNext.exe rebuild 2026-06-03 15:41. **Môi trường**: 1920×1080, DPI 100%, auto-lock off.
- **Account**: taikhoantestfshare@gmail.com. Seed RNG cố định mỗi burst để repro.

## Các burst & kết quả

| # | Kịch bản | Cường độ | alive | Crash | Log mới | Evidence |
|---|---|---|---|---|---|---|
| 1 | Chuyển trang sidebar ngẫu nhiên, không đợi load | 80× ~100ms | ✅ | ❌ 0 | +0 | 01 |
| 2 | Hammer "Refresh" My Files + nhảy trang giữa fetch | 50× ~70ms | ✅ | ❌ 0 | +8698 | 02 |
| 3 | Search spam (gõ/xóa từ khóa đổi liên tục, không đợi API) | 40 chu kỳ | ✅ | ❌ 0 | +6 | 03 |
| 4 | Vào/ra thư mục liên tục (Enter→Backspace) | 30× | ✅ | ❌ 0 | +319 | — |
| 5 | Monkey thuần: click ngẫu nhiên + phím ngẫu nhiên | 150× | ✅ (died=-1) | ❌ 0 | +528 | 04 |
| 6 | **CHAOS**: kill `-Force` giữa lúc refresh → relaunch | 1 cycle | ✅ phục hồi | ❌ 0 | +281 | 05 |

## Kết luận chính: 🟢 KHÔNG CRASH

- App **sống sót toàn bộ** mọi burst (page-switch dồn dập, API đồng thời, monkey, force-kill).
- **Chaos kill-mid-refresh → relaunch sạch**: 0 dòng corrupt/error/fatal/failed khi khởi động lại; cache DB
  + state nguyên vẹn; Home render đúng. Lớp async-lifetime + concurrent-API (CURLSH/cookie, đã fix ở 0c4f14b)
  giữ vững dưới stress.
- requestSeq cancel (HomeSearchVM) chịu được search-spam (+6 dòng, 0 lỗi).

## Bug phát hiện (KHÔNG crash, không chặn release): 2

| ID | Severity | Mô tả |
|---|---|---|
| [VLT-BUG-0007](../../reports/bugs/VLT-BUG-0007.md) | MEDIUM | VaultWizard.qml:134 binding loop "height" → spam WARN |
| [MYFILES-BUG-0001](../../reports/bugs/MYFILES-BUG-0001.md) | LOW | Icon "+" không render (caller dùng "+", asset là plus.svg) |

> Burst 2 nổ +8698 dòng log gần như toàn bộ là 2 WARN trên lặp lại — đáng sửa vì che lấp lỗi thật trong log.

## Chaos sâu hơn — kill giữa lúc DOWNLOAD (chưa hoàn tất + lộ finding)
Định chạy "kill giữa lúc tải file ghi đĩa+DB". Để chuẩn bị: đặt download-folder = .chaostest (qua registry,
đã khôi phục sau test) → Yêu thích → file rar 4GB → "Tải về".
- **Chặn lại bởi finding [DL-BUG-0001](../../reports/bugs/DL-BUG-0001.md)**: toast "Đã thêm vào tải về" hiện,
  nhưng task KHÔNG xuất hiện ở Tải xuống (Đang tải 0 + Lịch sử 0, không ghi file, không log download).
- ⚠️ CANDIDATE — có biến nhiễu (auto-download tắt, file secure=1, đổi folder qua registry, lái mù) → cần re-test
  có kiểm soát (xem bug). Vì download không chạy nên **không thực hiện được kill-mid-download** đợt này.
- Evidence: 07-fav-rar-selected, 08/10-downloads (0 file), 09-fav-after-taive (toast), 11-downloads-history.

## Khuyến nghị
- Sửa 2 bug WARN trên để log sạch (dễ phát hiện lỗi thật về sau).
- Đợt sau: chaos sâu hơn cần transfer thật đang chạy — kill giữa lúc DOWNLOAD/UPLOAD/SYNC (đang ghi file/DB)
  để test resume + toàn vẹn file dở; mở 2 dialog đồng thời; mất mạng giữa transfer. (Cần fixtures + account test.)
- Cân nhắc UI monkey harness cố định seed trong `qa/automation/monkey/` để chạy nightly + crash-watch tự động.
