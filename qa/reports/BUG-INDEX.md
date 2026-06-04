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
| DL-BUG-0001 | MEDIUM | FIXED | FavoritesVM/DownloadVM | monkey-chaos-r1 | c0be2df | Toast "Đã thêm vào tải về" giả khi addDownload từ chối im lặng (disk-full không phát signal). Fix: toast bám kết quả enqueue thật + nổi lý do từ chối. Chờ QA re-test live → CLOSED |
| SYNC-BUG-0001 | LOW | NEW | SyncViewModel/SyncPage | SYNC-SC-017 | | File list folder active không tự refresh theo scan/upload nền (đứng yên tới khi scan-now/đổi folder); pill + activity vẫn cập nhật. UX freshness, không mất dữ liệu |
| DL-BUG-0002 | MEDIUM | NEW | DownloadVM/effectiveDownloadFolder | EDL-011 | | Dialog "Thêm tải xuống" mặc định "Thư mục lưu" = C:\Windows (system dir, không ghi được) → tải mặc định fail/cần admin. Nghi fallback về CWD. Cần xác minh root cause |
| AUTH-BUG-0001 | HIGH | NEW | AuthService/OAuth/RefreshTokenCoordinator/OAuthSecrets | edge-r1 (log) | | Silent refresh `/api/user/refreshToken` → **403 "Invalid app key!"** → session hết hạn/logout lặp lại ~30–40' khi đang dùng. Nghi app key refresh sai/không hợp lệ. Chặn tác vụ dài + QA tự động |
| HOME-BUG-0001 | MEDIUM | CLOSED | HomeSearchOverlay.qml (activateHighlighted) | HOME-UI-009 | | Overlay search: `↑↓`+Enter mở dòng highlight không chạy (đọc sai role offset +1 → lấy IdRole thay LinkcodeRole). Fix +1→+2/+2→+3. Verified run 2026-06-04-home-r1 |
| HOME-BUG-0002 | LOW | CLOSED | Main.qml ↔ DownloadPage (lazy Loader) | HOME-UI-011 | | Quick-action "Dán link & tải" không tự mở dialog Thêm URL (signal `openDownloadDialog` bị lỡ do Loader chưa instantiate). Fix: chuyển sang property-flag `pendingOpenAddDialog` (consume onCompleted+onChange). Verified run 2026-06-04-home-r1 |

## Thống kê
- Tổng: 14 · CLOSED: 10 · FIXED (chờ verify live): 1 · NEW/đang xử lý: 3
- Theo severity: CRITICAL 1 · HIGH 3 · MEDIUM 7 · LOW 3
- FIXED chờ re-test: **DL-BUG-0001** (MEDIUM) — code-verified (build/lint/i18n/smoke PASS); cần QA bấm "Tải về"
  với tài khoản thật để chuyển CLOSED.
- NEW chưa pick: **AUTH-BUG-0001** (HIGH — refresh "Invalid app key!", chặn release/QA dài) · **DL-BUG-0002**
  (MEDIUM, default save folder = C:\Windows) · **SYNC-BUG-0001** (LOW, freshness file-list).
- ⚠️ Bug HIGH chưa CLOSED chặn release: AUTH-BUG-0001 (silent-refresh hỏng).

## Quy ước
- **Status**: NEW → TRIAGED → FIXING → FIXED → VERIFYING → CLOSED (hoặc WONTFIX/REOPENED).
- File bug đầy đủ + nhật ký trạng thái: `qa/reports/bugs/VLT-BUG-####.md`.
- Bug CRITICAL/HIGH chưa CLOSED ⇒ chặn release.
