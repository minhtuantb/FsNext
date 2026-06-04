# Trang chủ (Home + Search) — Chaos / Monkey test cases

> Loại: **CHAOS** (cực đoan/đối nghịch) + **CHAOS-MK** (monkey ngẫu nhiên số lượng
> lớn). Tiêu chí PASS: suy biến **graceful** — không crash/hang, fail-closed, không
> rò kết quả "ghost", thông báo rõ. Chạy bằng PowerShell + UI-driver + `Fs-CrashWatch`
> (Event Log `0xc0000005`/`0xc0000374` + `%APPDATA%\FPT\FsNext\fsnext.log`).
> Tổng: **9 case**.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| HOME-CHAOS-MK-001 | Fuzz keyword ngẫu nhiên vào classify | mock API/offline | 1) Ném 10k chuỗi ngẫu nhiên (độ dài 0–4096, unicode/control/emoji) vào `classify()` | KHÔNG crash/hang; mỗi input phân loại hợp lệ vào 1 trong 7 state; không leak bộ nhớ tăng vô hạn | QtTest+monkey | P1 | no | — |
| HOME-CHAOS-MK-002 | Gõ-xoá-gõ tốc độ cao (UI monkey) | Home, đăng nhập test | 1) Bắn chuỗi keystroke + backspace ngẫu nhiên vào ô search 60s (seed cố định) | process còn sống; overlay không kẹt spinner; không nhân đôi request; Event Log sạch crash | monkey+UI-driver | P1 | no | — |
| HOME-CHAOS-MK-003 | Click ngẫu nhiên toàn trang Home | Home | 1) 500 click toạ độ ngẫu nhiên trong vùng Home + phím ngẫu nhiên 2 phút | không crash; không kẹt overlay/dialog; quick-action/filter không vào state lạ | monkey+UI-driver | P2 | no | — |
| HOME-CHAOS-001 ⚠️ | Spam Enter liên tục khi đang search | có kết quả/đang tải | 1) Giữ Enter bắn submit ~20 lần/giây trên keyword hợp lệ | search single-flight (`runKeywordSearchNow` short-circuit cùng keyword); không chồng request; không crash | UI-driver | P1 | no | — |
| HOME-CHAOS-002 ⚠️ | Race: gõ keyword mới khi page-2 đang load | overlay đang loadMore | 1) Trigger `loadMore()` 2) Ngay lập tức gõ keyword mới | response page-2 cũ bị bỏ (seq guard); overlay chỉ hiện kết quả keyword mới; không append nhầm vào model mới | UI-driver/QtTest | P0 | no | — |
| HOME-CHAOS-003 | Mất mạng giữa lúc search | đang gõ keyword | 1) Ngắt mạng 2) Gõ keyword 3) Chờ | `resp.isError()` → soft-fail: model cũ giữ nguyên, hết spinner, không "ghost"; có thể hiện thông báo lỗi nhẹ; không crash | UI-driver/tay | P1 | no | — |
| HOME-CHAOS-004 | Session hết hạn (HTTP 201) khi search | token sắp hết | 1) Search khi session expired | đi qua silent-refresh single-flight; không loop refresh; kết quả về sau refresh hoặc lỗi mềm; không crash | tay | P2 | no | — |
| HOME-CHAOS-005 | Kill app khi search in-flight | đang gõ search | 1) `Stop-Process -Force FsNext` lúc QtConcurrent task đang chạy 2) Mở lại | QPointer guard → callback no-op khi VM huỷ; mở lại Home sạch, không state hỏng, không crash startup | tay+crash-watch | P1 | no | — |
| HOME-CHAOS-006 | Resize cửa sổ liên tục khi overlay mở | overlay có kết quả | 1) Kéo resize + đổi width qua/lại breakpoint 720px khi overlay đang mở | overlay bám đúng vị trí search pill (`mapToItem` re-eval); không lệch/chớp; greeting hero scale 28↔40 mượt | UI-driver | P2 | no | — |

## Traceability
- Race/async (bug-class #1): HOME-CHAOS-001/002/005 ↔ functional HOME-FN-019/023/024/026 + [[project-homesearch-clearresults-race]].
- Network/session: HOME-CHAOS-003/004 ↔ HOME-FN-027 (soft-fail) + single-flight refresh.
- Untrusted input: HOME-CHAOS-MK-001 ↔ validation HOME-VAL-019/020.
- Layout: HOME-CHAOS-006 ↔ HOME-UI-001.
