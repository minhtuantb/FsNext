# Trang chủ (Home + Search) — Validation / biên test cases

> Loại: **validation** (data-driven). Đối tượng: input phân loại của
> `HomeSearchViewModel::classify()` + `BadWordFilter` (`check` / `checkExactPhrase`)
> + `DownloadViewModel::extractFshareLinks()`. Phủ biên độ dài, ngoặc kép, dấu
> tiếng Việt, URL biến thể, untrusted/giant input. Cờ ⚠️ = fail-closed bảo mật.
> Tổng: **20 case**. **Tự động hoá:** `tests/test_home_search_viewmodel.cpp`
> (CTest: `HomeSearchViewModel`) phủ toàn bộ → `Auto? = yes(home_search)`.
> `*` = phủ một phần (xem ghi chú dưới bảng).

| ID | Tiêu đề | Tiền điều kiện | Bước (input) | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| HOME-VAL-001 | Biên độ dài 0 | — | `""` | `Idle` | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-002 | Biên 1 ký tự | — | `"a"` | `TooShort` | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-003 | Biên 2 ký tự | — | `"ab"` | `TooShort` | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-004 | Biên đúng 3 (min) | filter sạch | `"abc"` | `Keyword` (≥ kMinLength) | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-005 | Trim trước khi đếm độ dài | — | `"  a  "` (1 ký tự thực) | `TooShort` (trim → "a") | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-006 | Trim đủ dài sau cắt khoảng trắng | filter | `"  abc  "` | `Keyword`; keyword=="abc" | QtTest | P2 | yes(home_search) | — |
| HOME-VAL-007 ⚠️ | Bad-word đơn token | dict | `"sex movie"` (chứa 1 entry) | `Blocked`; hitWord != "" | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-008 ⚠️ | Bad-word đa token (n-gram) | dict | `"phim sex hd"` | `Blocked` (n-gram 1..3 bắt "phim sex") | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-009 ⚠️ | Bad-word bỏ dấu tiếng Việt | dict | dạng không dấu của entry có dấu (`"tucxau"` ↔ `"tụcxấu"`) | `Blocked` (m_stripped khớp) | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-010 | False-positive an toàn ("essex") | dict | `"essex"` | `Keyword` (word-boundary, KHÔNG chặn substring "sex") | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-011 | Quoted hợp lệ → phrase trần | filter | `"\"Lồng tiếng\""` | `Keyword` chính xác (checkExactPhrase Clean); keyword bỏ ngoặc | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-012 ⚠️ | Quoted KHỚP đúng bad-word → vẫn chặn | dict | `"\"sex\""` (inner == entry) | `Blocked` (quote ≠ bypass) | QtTest | P0 | yes(home_search) | — |
| HOME-VAL-013 | Quoted inner ngắn | — | `"\"ab\""` (inner < 3) | `TooShort`; keyword=="ab". (sub-case `""` rỗng chưa phủ) | QtTest | P2 | yes(home_search)* | — |
| HOME-VAL-014 | Một dấu ngoặc không kích hoạt quoted | filter | `"\"abc"` (chỉ mở ngoặc) | đi nhánh keyword thường (không quoted) → `Keyword` | QtTest | P2 | yes(home_search) | — |
| HOME-VAL-015 | URL file biến thể (https / www) | downloadVm | `https://fshare.vn/file/X`, `https://www.fshare.vn/file/X` | đều `UrlFile`. (regex YÊU CẦU scheme `https?://` — dạng "bare" `fshare.vn/...` KHÔNG nhận) | QtTest | P1 | yes(home_search)* | — |
| HOME-VAL-016 | URL folder case-insensitive /FOLDER/ | downloadVm | `https://fshare.vn/FOLDER/abc` | `UrlFolder` (`contains(..., CaseInsensitive)`) | QtTest | P2 | yes(home_search) | — |
| HOME-VAL-017 | Text lẫn URL | downloadVm | `"xem phim https://fshare.vn/file/X nhe"` | extract chỉ lấy URL → `UrlFile` (URL detection ưu tiên) | QtTest | P2 | yes(home_search) | — |
| HOME-VAL-018 | Chuỗi giống-URL nhưng không phải fshare | downloadVm | `"https://google.com/file/x"` | extract rỗng → KHÔNG Url* (rơi về keyword path) | QtTest | P2 | yes(home_search) | — |
| HOME-VAL-019 ⚠️ | Input khổng lồ (100k ký tự) không hang/crash | — | chuỗi 100.000 ký tự | classify trả về kịp, không hang/crash → `Keyword` | QtTest | P1 | yes(home_search) | — |
| HOME-VAL-020 | Unicode/emoji/zero-width không phá classify | filter | emoji, ZWSP, RTL marks + token | không crash; state hợp lệ (Idle..Blocked) | QtTest | P2 | yes(home_search) | — |

> `*` HOME-VAL-013: mới phủ sub-case inner ngắn (`"ab"`); sub-case quoted rỗng `""` chưa có.
> `*` HOME-VAL-015: phủ https + www; dạng "bare" (không scheme) cố ý không hỗ trợ bởi regex.

## Traceability
- BadWordFilter: HOME-VAL-007..014 ↔ [BadWordFilter.h](src/core/util/BadWordFilter.h) (`check` n-gram vs `checkExactPhrase`).
- URL detection: HOME-VAL-015..018 ↔ `DownloadViewModel::extractFshareLinks`.
- Robustness/untrusted: HOME-VAL-019/020 (xem thêm chaos `HOME-CHAOS-MK-001`).
- Fail-closed bảo mật (⚠️): HOME-VAL-007/008/009/012/019 — bắt buộc xanh trước ship.
