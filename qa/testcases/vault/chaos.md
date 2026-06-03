# Vault — Chaos test cases

> Loại: **chaos** (fail-closed, crash-safety, kill-app giữa chừng, meta/blob hỏng, sweep-sau-crash). Tổng: **20 case** (0 monkey).
> Cờ ⚠️ ở Tiêu đề = case rủi ro cao (security/fail-closed/crash); 🔴 = lỗ hổng nghi ngờ cao.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-CHAOS-001 ⚠️ | Fail-closed decrypt tamper xóa output | `.fshenc` tamper (vd VLT-VAL-032); vault unlocked | 1) `decryptFile(in,out,key)` ra `out` mới | Trả lỗi; file `out` KHÔNG tồn tại (xóa bản dở, `fs::remove(out)`) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-002 ⚠️ | Fail-closed decrypt wrong-key | `.fshenc` hợp lệ | 1) `decryptFile` bằng khóa sai | `WrongKey`; `out` không tồn tại; KHÔNG để 0-byte/partial | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-003 ⚠️ | Fail-closed truncated (mô phỏng kill khi ghi) | `.fshenc` cắt cụt trước TAG_FINAL | 1) `decryptFile` | `Tampered`; `out` bị xóa; không crash; không partial plaintext | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-004 ⚠️ | Fail-closed encrypt IoError | `out` không ghi được (thư mục read-only / ổ không tồn tại) | 1) `encryptFile(in, out, key)` | `IoError`; KHÔNG để `.fshenc` dở tại out | UI-driver | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-005 ⚠️ | Fail-closed disk-full | Đĩa/quota gần đầy (giả lập ổ nhỏ); encrypt file lớn | 1) Encrypt file > dung lượng trống | `IoError` khi write/flush fail; `.fshenc` dở bị xóa; app không crash | tay | P0 | no | — |
| VLT-CHAOS-006 ⚠️ | Meta corrupt truncated | `.vault-meta.json` cắt cụt giữa chừng (kill khi ghi) | 1) `setVaultDir` 2) `tryAutoUnlock`/`unlock` | `VaultMetadata::load` → `InvalidFormat`; VM graceful; KHÔNG crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-007 ⚠️ | Meta JSON hỏng / thiếu wrappedMasterKey | `.vault-meta.json` JSON không hợp lệ / thiếu field | 1) `unlock(pass)` | `ensureMetaLoaded` false → `unlock` trả `InvalidFormat`; không crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-008 ⚠️ | Meta base64 sai độ dài | `wrappedMasterKey` decode ≠ 48B (hoặc `wrapNonce` ≠ 24B) | 1) load meta | `unb64` trả false → `InvalidFormat`; không cấp phát/đọc sai | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-009 ⚠️ | Atomic write integrity (ngắt trước rename) | Meta `save` ngắt sau ghi `.tmp` trước rename | 1) Khởi động lại 2) load meta | Meta cũ còn nguyên (rename atomic); `.tmp` dở không thay bản tốt; KHÔNG hỏng | tay | P0 | no | — |
| VLT-CHAOS-010 ⚠️ | Blob DPAPI hỏng | L2; `.vault-key.dpapi` cắt/hỏng | 1) `tryAutoUnlock()` | `SecureStore::decrypt` ≠ 32B → false; không crash; UI rơi về DLG-UNLOCK | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-011 ⚠️ | Blob plain hỏng | L3; `.vault-key.plain` ≠ 32B | 1) `tryAutoUnlock()` | false; không crash | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-CHAOS-012 ⚠️ | Trạng thái lệch: có meta thiếu blob | L2 có meta nhưng XÓA `.vault-key.dpapi` | 1) `tryAutoUnlock()` | false (open fail) → Locked → cần passphrase; không crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-CHAOS-013 ⚠️ | Trạng thái lệch: có blob thiếu meta | Có `.vault-key.dpapi` nhưng XÓA `.vault-meta.json` | 1) `setVaultDir` 2) đọc state | `vaultExists==false` → NoVault; `tryAutoUnlock` false (ensureMetaLoaded fail) | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-CHAOS-014 ⚠️ | Kill batch encrypt giữa chừng | Batch nhiều file đang encrypt | 1) `cancelBatch()` (mô phỏng kill) giữa chừng | File xong = `.fshenc` HỢP LỆ; file dở KHÔNG vào vault; file chưa làm → `lastFailed_` (retry được) | QtTest | P0 | no | — |
| VLT-CHAOS-015 ⚠️ | Kill decrypt-to-temp | `decryptAndOpen` đang chạy | 1) App đóng/lock khi đang giải mã | Bản dở: decryptFile fail→xóa; nếu xong & tracked → `sweepTemps(true)` khi `lock()`/destructor secure-delete | UI-driver | P0 | no | — |
| VLT-CHAOS-016 | Temp sót → secure-delete khi mở lại | Temp tracked từ phiên trước (cùng VM sống) | 1) `lock()` 2) mở lại | `lock()` gọi `sweepTemps(true)` → temp overwrite + xóa; signal `tempCleaned(n)` | QtTest | P1 | no | — |
| VLT-CHAOS-017 ⚠️🔴 | Temp sót sau crash thật | App crash để lại `%TEMP%/FsNextVault/*` (KHÔNG tracked) | 1) Khởi động lại app | Kỳ vọng: có sweep startup quét thư mục cũ. GAP nghi ngờ: VM mới không quét → temp giải mã tồn tại (bug an toàn) | tay | P0 | no | VLT-BUG-0003 |
| VLT-CHAOS-018 | Fail-closed buffer API | `decryptBuffer` tamper | 1) Decrypt buffer hỏng | `out` được `clear()` đầu hàm; chỉ assign khi `Ok` → out rỗng khi lỗi | QtTest | P1 | yes(test_encryption_engine) | — |
| VLT-CHAOS-019 ⚠️ | Lock giữa op async (busy) | Đang `createVault`/`unlock` (busy_) | 1) Gọi `lock()` | `lock()` return sớm khi `busy_` → không race/crash | QtTest | P0 | no | — |
| VLT-CHAOS-020 | VM hủy giữa op async | Async op đang chạy, VM bị destroy | 1) Đóng app | `QPointer guard` chặn callback; `~VaultViewModel` sweep temp; pool drain ở main.cpp | UI-driver | P1 | no | — |

## Traceability (ID mới ↔ ID nguồn)

| ID mới | Nguồn | ID mới | Nguồn |
|---|---|---|---|
| VLT-CHAOS-001 | VLT-301 | VLT-CHAOS-011 | VLT-311 |
| VLT-CHAOS-002 | VLT-302 | VLT-CHAOS-012 | VLT-312 |
| VLT-CHAOS-003 | VLT-303 | VLT-CHAOS-013 | VLT-313 |
| VLT-CHAOS-004 | VLT-304 | VLT-CHAOS-014 | VLT-314 |
| VLT-CHAOS-005 | VLT-305 | VLT-CHAOS-015 | VLT-315 |
| VLT-CHAOS-006 | VLT-306 | VLT-CHAOS-016 | VLT-316 |
| VLT-CHAOS-007 | VLT-307 | VLT-CHAOS-017 | VLT-317 |
| VLT-CHAOS-008 | VLT-308 | VLT-CHAOS-018 | VLT-318 |
| VLT-CHAOS-009 | VLT-309 | VLT-CHAOS-019 | VLT-319 |
| VLT-CHAOS-010 | VLT-310 | VLT-CHAOS-020 | VLT-320 |
