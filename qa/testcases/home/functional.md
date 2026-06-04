# Trang chủ (Home + Search) — Functional test cases

> Loại: **function**. Đối tượng: `HomeSearchViewModel` (`classify()` / `submit()` /
> `clearResults()` / `loadMore()` + máy trạng thái 7 nhánh + debounce + request-seq
> guard + paging async). Tham chiếu code: `src/viewmodels/HomeSearchViewModel.{h,cpp}`.
> Cờ ⚠️ = rủi ro cao (fail-closed bad-word / race async). Tổng: **27 case**.
>
> Hằng số: `kMinLength=3`, `kDebounceMs=250`, `kPerPage=30`.
> **Tự động hoá: TOÀN BỘ 27 case** qua `tests/test_home_search_viewmodel.cpp`
> (CTest: `HomeSearchViewModel` — **41 pass / 0 skip**). Phần async (FN-019..027,
> gồm 2 ⚠️ race) dùng `FakeSearchApi : IFshareApi` điều khiển được + `QTRY_*`.
> `searchFiles()` đã được promote vào `IFshareApi` (virtual); ctor VM nhận `IFshareApi*`.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| HOME-FN-001 | classify("") → Idle | VM khởi tạo | 1) `classify("")` | `state==Idle(0)`; hint/hitWord/keyword rỗng; `clearResults()` chạy (model rỗng, không searching) | QtTest | P1 | yes(home_search) | — |
| HOME-FN-002 | classify chỉ khoảng trắng → Idle | — | 1) `classify("   ")` | trimmed rỗng → `Idle`; không gọi API | QtTest | P1 | yes(home_search) | — |
| HOME-FN-003 | classify "ab" (2 ký tự) → TooShort | — | 1) `classify("ab")` | `state==TooShort(1)`; hint "Nhập tối thiểu 3 ký tự…"; `keyword=="ab"`; KHÔNG schedule search | QtTest | P1 | yes(home_search) | — |
| HOME-FN-004 | classify "abc" (đúng biên 3) → Keyword | filter sạch | 1) `classify("abc")` | `state==Keyword(5)`; hint "Nhấn Enter để tìm kiếm"; `scheduleKeywordSearch` chạy → `isSearching==true` | QtTest | P0 | yes(home_search) | — |
| HOME-FN-005 ⚠️ | classify keyword chứa bad-word → Blocked | filter nạp dict | 1) `classify("phim sex")` | `state==Blocked(6)`; hint "Từ khoá chứa nội dung không phù hợp"; `hitWord` != ""; **KHÔNG** schedule search; `clearResults` chạy | QtTest | P0 | yes(home_search) | — |
| HOME-FN-006 ⚠️ | submit Blocked không gọi API | như trên | 1) `submit("phim sex")` | trả `Blocked`; emit `rejectedBadWord(hit)`; KHÔNG emit route*; `searchFiles` KHÔNG được gọi | QtTest | P0 | yes(home_search) | — |
| HOME-FN-007 | submit TooShort phát rejectedTooShort | — | 1) `submit("ab")` | trả `TooShort`; emit `rejectedTooShort()`; không route | QtTest | P1 | yes(home_search) | — |
| HOME-FN-008 | submit Idle no-op | — | 1) `submit("")` | trả `Idle`; KHÔNG emit signal nào | QtTest | P2 | yes(home_search) | — |
| HOME-FN-009 | classify 1 link file → UrlFile | downloadVm wired | 1) `classify("https://www.fshare.vn/file/ABC123")` | `state==UrlFile(2)`; hint "Link file · Nhấn Enter…"; `keyword`==URL; `clearResults` chạy (bỏ keyword cũ) | QtTest | P1 | yes(home_search) | — |
| HOME-FN-010 | classify 1 link folder → UrlFolder | downloadVm | 1) `classify(".../folder/XYZ")` | `state==UrlFolder(3)`; hint "Link folder · Nhấn Enter để duyệt" | QtTest | P1 | yes(home_search) | — |
| HOME-FN-011 | classify ≥2 link → UrlMultiple | downloadVm | 1) Dán 2 URL fshare cách nhau xuống dòng/space | `state==UrlMultiple(4)`; hint "Phát hiện 2 link…"; `keyword`=newline-joined | QtTest | P1 | yes(home_search) | — |
| HOME-FN-012 | submit UrlFile emit routeFileUrl đúng 1 lần | UrlFile state | 1) `submit(url_file)` | emit `routeFileUrl(url)` đúng 1 lần; KHÔNG emit route khác | QtTest | P1 | yes(home_search) | — |
| HOME-FN-013 | submit UrlFolder/UrlMultiple route đúng | tương ứng | 1) submit folder-url; 2) submit 2-url | (1) emit `routeFolderUrl`; (2) emit `routeMultipleUrls(joined)`; mỗi cái đúng 1 lần | QtTest | P1 | yes(home_search) | — |
| HOME-FN-014 | URL bypass length + bad-word | — | 1) `classify("fshare.vn/file/x")` (URL ngắn/ hoặc chứa stem nhạy cảm) | Phân loại Url* — KHÔNG rơi vào TooShort/Blocked (URL check chạy trước) | QtTest | P2 | yes(home_search) | — |
| HOME-FN-015 | Quoted mode hợp lệ → Keyword chính xác | filter | 1) `classify("\"Lồng tiếng\"")` | `state==Keyword`; hint "Chế độ chính xác · Nhấn Enter để tìm"; `keyword`=="Lồng tiếng" (bỏ ngoặc); schedule search với phrase trần | QtTest | P1 | yes(home_search) | — |
| HOME-FN-016 ⚠️ | Quoted khớp CHÍNH XÁC bad-word vẫn Blocked | filter | 1) `classify("\"<bad-word>\"")` (inner đúng 1 entry dict) | `checkExactPhrase==Blocked` → `state==Blocked`; không bypass; không search | QtTest | P0 | yes(home_search) | — |
| HOME-FN-017 | Quoted inner < 3 ký tự → TooShort | — | 1) `classify("\"ab\"")` | inner=="ab" → `TooShort`; keyword=="ab" | QtTest | P2 | yes(home_search) | — |
| HOME-FN-018 | submit Keyword chạy search ngay (bỏ debounce) | filter, api mock | 1) `classify("abcd")` (timer đang chạy) 2) `submit("abcd")` | `m_debounceTimer` stop; `runKeywordSearchNow` gọi `searchFiles("abcd",1)` ngay; emit `routeKeyword` | QtTest | P1 | yes(home_search)* | — |
| HOME-FN-019 | Debounce gộp nhiều keystroke thành 1 fetch | FakeSearchApi đếm call | 1) `classify("ab")`..`classify("abcde")` liên tục < 250ms | Chỉ **1** `searchFiles` được gọi (cho "abcde") sau khi timer fire; không gọi cho mỗi keystroke | QtTest | P1 | yes(home_search) | — |
| HOME-FN-020 | Kết quả về → resetItems + cờ | FakeSearchApi trả 30 item | 1) search → callback | `resultsModel.count==30`; `hasResults==true`; `resultsForKeyword`==keyword; `hasMorePages==true` (==kPerPage); `isSearching==false` | QtTest | P1 | yes(home_search) | — |
| HOME-FN-021 | Kết quả < kPerPage → hasMorePages=false | FakeSearchApi trả 5 item | 1) search | `count==5`; `hasMorePages==false`; `noResultsHit==false` | QtTest | P1 | yes(home_search) | — |
| HOME-FN-022 | 0 kết quả → noResultsHit=true | FakeSearchApi trả [] | 1) search | `count==0`; `noResultsHit==true`; `hasResults==false`; KHÔNG kẹt `isSearching` | QtTest | P1 | yes(home_search) | — |
| HOME-FN-023 ⚠️ | Response cũ (stale) bị bỏ — không "ghost" | FakeSearchApi latency | 1) submit "aaa" (chậm) 2) submit "bbb" (bump seq) 3) response "aaa" về | response "aaa" thấy `mySeq != m_requestSeq` → **return sớm**; model chỉ phản ánh "bbb" | QtTest | P0 | yes(home_search) | — |
| HOME-FN-024 ⚠️ | clearResults bump seq — callback sau bị huỷ | FakeSearchApi latency | 1) submit "aaa" 2) `clearResults()` 3) response về | `clearResults` đã `++m_requestSeq` → callback discard; model rỗng; overlay không hồi sinh (regression [[project-homesearch-clearresults-race]]) | QtTest | P0 | yes(home_search) | — |
| HOME-FN-025 | loadMore append + tăng page | có results page1, hasMorePages | 1) `loadMore()` | `searchFiles(kw, page2, append=true)` → `mergeItems`; model 30→60; page 2 được yêu cầu | QtTest | P2 | yes(home_search) | — |
| HOME-FN-026 | loadMore no-op khi !hasMorePages / đang searching | — | 1) `loadMore()` khi `hasMorePages==false` hoặc `isSearching==true` | KHÔNG gọi `searchFiles`; trả về im lặng | QtTest | P2 | yes(home_search) | — |
| HOME-FN-027 | API error giữ nguyên model (soft-fail) | có results, api lần sau lỗi | 1) search lỗi (`resp.isError()`) | model CŨ giữ nguyên (không xoá); `isSearching==false`; không crash | QtTest | P1 | yes(home_search) | — |

## Traceability
- `classify()` máy trạng thái: HOME-FN-001..017 (đối chiếu enum `State` trong [HomeSearchViewModel.h](src/viewmodels/HomeSearchViewModel.h:48)).
- `submit()` routing: HOME-FN-006/007/008/012/013/018.
- Async/race (bug-class #1): HOME-FN-019/023/024 — gắn với [[project-homesearch-clearresults-race]] + pattern QPointer guard.
- Paging: HOME-FN-020/021/025/026; soft-fail: HOME-FN-027.
- UI hiển thị tương ứng ở `home/ui.md` (HOME-UI-002..010).
