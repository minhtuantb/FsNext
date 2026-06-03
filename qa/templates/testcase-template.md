# <Module> — <Loại> test cases

> Loại: FN (function) | SC (scenario) | VAL (validation) | UI | CHAOS | CHAOS-MK (monkey).
> Mỗi case 1 dòng bảng; ID ổn định, không tái dùng số đã xoá.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-XX-001 | <ngắn gọn> | <state cần có> | 1) … 2) … | <expected + điều KHÔNG được xảy ra> | QtTest/qmltest/UI-driver/tay | P0/P1/P2 | yes/no | — |

<!--
Quy ước cột:
- Tool: nơi thực thi (xem qa/README §3).
- Auto?: đã có test tự động chưa (link target nếu có).
- Bug: điền VLT-BUG-#### khi case này fail.
- ⚠️ gắn ở Tiêu đề cho case rủi ro cao (security/fail-closed/fuzz).
-->
