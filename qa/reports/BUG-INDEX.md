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
| VLT-BUG-0007 | MEDIUM | CLOSED | VaultWizard.qml:134 | monkey-chaos-r1 | fa2b01d | Binding loop "height" trong VaultWizard → spam WARN liên tục |
| MYFILES-BUG-0001 | LOW | CLOSED | FsIcon callers | monkey-chaos-r1 | fa2b01d | Icon "+" không render — caller dùng name "+" nhưng asset là plus.svg |
| DL-BUG-0001 | MEDIUM | FIXED | FavoritesVM/DownloadVM | monkey-chaos-r1 | | Toast "Đã thêm vào tải về" giả khi addDownload từ chối im lặng (disk-full không phát signal). Fix: toast bám kết quả enqueue thật + nổi lý do từ chối. Chờ QA re-test live → CLOSED |

## Thống kê
- Tổng: 9 · CLOSED: 8 · FIXED (chờ verify live): 1 · NEW/đang xử lý: 0
- Theo severity: CRITICAL 1 · HIGH 2 · MEDIUM 5 · LOW 1
- FIXED chờ re-test: **DL-BUG-0001** (MEDIUM) — code-verified (build/lint/i18n/smoke PASS); cần QA bấm "Tải về"
  với tài khoản thật để chuyển CLOSED.

## Quy ước
- **Status**: NEW → TRIAGED → FIXING → FIXED → VERIFYING → CLOSED (hoặc WONTFIX/REOPENED).
- File bug đầy đủ + nhật ký trạng thái: `qa/reports/bugs/VLT-BUG-####.md`.
- Bug CRITICAL/HIGH chưa CLOSED ⇒ chặn release.
