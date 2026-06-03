# BUG INDEX — Vault/E2EE (dashboard)

> Bảng tổng trạng thái. Mỗi bug có file chi tiết ở `qa/reports/bugs/`.
> Process fix lọc cột **Status = NEW/TRIAGED** chưa có **Assignee** để pick.
> Cập nhật cột Status + ngày mỗi khi đổi trạng thái.

| ID | Severity | Status | Area | Found by | Fix commit | Title |
|---|---|---|---|---|---|---|
| VLT-BUG-0001 | MEDIUM | CLOSED | EncryptionEngine | VLT-VAL-061 | e971fed | Filename >4096 ký tự: mã hóa được nhưng không mở lại được |
| VLT-BUG-0002 | HIGH | CLOSED | VaultViewModel | VLT-VAL-062 (review) | e971fed | Path traversal qua filename `.fshenc` không tin cậy trong decryptAndOpen |
| VLT-BUG-0003 | MEDIUM | CLOSED | VaultViewModel | VLT-CHAOS-031 | e971fed | Bản giải mã tạm sót sau crash cứng (thiếu sweep startup) |
| VLT-BUG-0004 | MEDIUM | CLOSED | VaultViewModel | VLT-CHAOS-032 | e971fed | Trùng basename ghi đè im lặng khi mã hóa thư mục |
| VLT-BUG-0005 | HIGH | CLOSED | VaultViewModel | VLT-SC-002 (UI) | fcfc78b | `setVaultDir` không Q_INVOKABLE → wizard kẹt "Đang tạo…" |
| VLT-BUG-0006 | CRITICAL | CLOSED | HttpClient | (sync crash, Event Log) | 0c4f14b | CURLSH thiếu lock callbacks → heap corruption (0xc0000374) khi sync |

## Thống kê
- Tổng: 6 · CLOSED: 6 · NEW/đang xử lý: 0
- Theo severity: CRITICAL 1 · HIGH 2 · MEDIUM 3 · LOW 0

## Quy ước
- **Status**: NEW → TRIAGED → FIXING → FIXED → VERIFYING → CLOSED (hoặc WONTFIX/REOPENED).
- File bug đầy đủ + nhật ký trạng thái: `qa/reports/bugs/VLT-BUG-####.md`.
- Bug CRITICAL/HIGH chưa CLOSED ⇒ chặn release.
