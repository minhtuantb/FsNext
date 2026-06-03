# Vault — Functional test cases

> Loại: **function** (crypto round-trip, VaultManager lifecycle, size-cap/policy logic, secure-delete TTL/sweep, uploader/default). Tổng: **65 case**.
> Cờ ⚠️ ở Tiêu đề = case rủi ro cao (security/fail-closed/fuzz/path-traversal).

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-FN-001 | Round-trip buffer 100 B (AES single-shot) | Máy có AES-NI (`aes256GcmAvailable()==true`); `crypto::init()` đã chạy; Key 32B | 1) Encrypt buffer 100 B 2) Decrypt lại | Plaintext khớp byte-for-byte; header `algoId==0x02`, `chunkSize==0`, flag `FlagSingleShot` set | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-002 | Round-trip file 1 KiB nhị phân ngẫu nhiên | AES-NI | 1) Encrypt file 1 KiB ảnh nhị phân random 2) Decrypt | Khớp; `originalSize==1024` | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-003 | Round-trip buffer 5 MiB (XChaCha) | crypto init; Key 32B | 1) Encrypt buffer 5 MiB random 2) Decrypt | Khớp; `algoId==0x01`, `chunkSize==256 KiB` | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-004 | Round-trip file >1 GiB streaming RAM ổn định | crypto init; Key 32B | 1) Encrypt file 1.5 GiB (hoặc giả lập >1 GiB) 2) Decrypt, theo dõi RAM | Khớp; `chunkSize==1 MiB`; RAM ổn định < ~100 MB | QtTest | P1 | no | — |
| VLT-FN-005 | Force XChaCha cho file nhỏ | AES-NI | 1) `EncryptOptions.forceXChaCha=true`, file 100 B 2) Encrypt 3) Decrypt | `algoId==0x01` dù file nhỏ; round-trip khớp | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-006 | Round-trip buffer 0 byte | crypto init; Key 32B | 1) Encrypt buffer 0 byte 2) Decrypt | `originalSize==0`; decrypt trả buffer rỗng, `Ok`; không crash | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-007 ⚠️ | Boundary đúng 1 MiB → chọn AES | AES-NI | 1) Encrypt file đúng 1 MiB (1048576 B) 2) Decrypt | Chọn AES (`<= SmallFileThreshold`), `algoId==0x02`; round-trip khớp | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-008 ⚠️ | Boundary 1 MiB+1 → chọn XChaCha | AES-NI | 1) Encrypt file 1048577 B 2) Decrypt | Chọn XChaCha (`> threshold`), `algoId==0x01`, `chunkSize==256 KiB`; khớp | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-FN-009 | Boundary 1 GiB → chọn chunkSize | crypto init; Key 32B | 1) Encrypt đúng 1 GiB rồi 1 GiB+1 2) So `chunkSize` | ≤1 GiB → 256 KiB; >1 GiB → 1 MiB (ranh `LargeFileThreshold`) | QtTest | P1 | no | — |
| VLT-FN-010 | Non-determinism: 2 lần encrypt khác ciphertext | crypto init; Key 32B | 1) Encrypt cùng nội dung + cùng khóa 2 lần 2) So ciphertext | Hai `.fshenc` KHÁC nhau; cả hai decrypt khớp | QtTest | P1 | yes(test_encryption_engine) | — |
| VLT-FN-011 | Fallback XChaCha khi không AES-NI | Giả lập `aes256gcm_is_available()==0` | 1) Encrypt file 100 B (không force) 2) Decrypt | Dùng XChaCha (`algoId==0x01`); round-trip khớp 100% | QtTest | P1 | no | — |
| VLT-FN-012 | Round-trip 1000 file random 0 B–4 MiB | crypto init; Key 32B | 1) Round-trip 1000 file kích thước random (AES & XChaCha lẫn lộn) | Tất cả khớp; chọn nhánh đúng theo size | QtTest | P1 | no | — |
| VLT-FN-013 | Round-trip file API ảnh + docx thật | crypto init; Key 32B | 1) `encryptFile` → `decryptFile` ảnh + docx 2) So hash plaintext | Hash khớp; file mở được bằng app gốc | QtTest | P1 | yes(test_encryption_engine) | — |
| VLT-FN-014 | Multi-chunk bội số nguyên của chunk | crypto init; Key 32B | 1) Encrypt file = đúng bội số chunk (vd 512 KiB) 2) Decrypt | Chunk cuối vẫn TAG_FINAL; decrypt khớp, `sawFinal==true` | QtTest | P1 | yes(test_encryption_engine) | — |
| VLT-FN-015 | Header echo basename từ path | crypto init; Key 32B | 1) Encrypt với `storedFilename` rỗng từ `C:\a\b\report.docx` 2) readHeader | Header `filename=="report.docx"`; decrypt trả đúng tên | QtTest | P2 | yes(test_encryption_engine) | — |
| VLT-FN-016 | Create vault thành công | Thư mục rỗng; `setVaultDir` | 1) `createVault("Personal", pass, balanced, MediumDpapi)` | `Ok`; state Unlocked; `.vault-meta.json` + `.vault-key.dpapi` tồn tại; `masterKey().valid()` | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-017 | Create AlreadyExists không ghi đè | Đã có vault | 1) `createVault(...)` lần 2 | `AlreadyExists`; meta cũ KHÔNG bị ghi đè | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-FN-018 | Create khi không có dir | `vaultDir_` rỗng | 1) `createVault(...)` | `InternalError` | QtTest | P1 | no | — |
| VLT-FN-019 | Unlock đúng passphrase | Vault locked | 1) `lock()` 2) `unlock(pass đúng)` | `Ok`; Unlocked; masterKey hợp lệ; decrypt file vault thành công | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-020 | Unlock sai passphrase | Vault locked | 1) `unlock(pass sai)` | `WrongKey`; state vẫn Locked; không crash | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-021 | Unlock passphrase rỗng | Vault locked | 1) `unlock("")` | KDF chạy, unwrap fail → `WrongKey` (không bypass) | QtTest | P0 | no | — |
| VLT-FN-022 ⚠️ | ChangePass sai pass cũ không phá vault | Unlocked/locked | 1) `changePassphrase(oldSai, new, kdf)` | `WrongKey`/`KdfFailed`; meta KHÔNG đổi; vault vẫn mở bằng pass cũ | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-FN-023 ⚠️ | ChangePass đúng — pass cũ vô hiệu | Vault có data | 1) `changePassphrase(oldĐúng, new, kdf)` | `Ok`; unlock bằng `new` OK; unlock bằng `old` thất bại (`WrongKey`) | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-024 ⚠️ | ChangePass — file cũ vẫn mở được | Có `.fshenc` mã trước đổi pass | 1) Đổi pass 2) unlock bằng new 3) decrypt file cũ | File cũ decrypt OK (chỉ re-wrap Master Key) | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-025 | ChangePass đổi preset KDF | Vault unlocked | 1) Đổi pass kèm `kdf=maximum` | Meta cập nhật `kdfOps/MemKb`; unlock new dùng KDF mới | QtTest | P1 | no | — |
| VLT-FN-026 | Export recovery keyfile đúng | Unlocked | 1) `exportRecoveryKeyfile(path)` | `Ok`; file 32B; meta `hasRecoveryKey==true` + `wrappedMasterKeyRecovery` | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-027 | Export recovery khi locked | Vault locked | 1) `exportRecoveryKeyfile(path)` | `NotUnlocked`; không tạo file | QtTest | P0 | no | — |
| VLT-FN-028 | Unlock bằng keyfile đúng | Đã export recovery; locked | 1) `unlockWithKeyfile(path đúng)` | `Ok`; Unlocked; masterKey khớp gốc | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-029 | Unlock keyfile sai nội dung | Có recovery; keyfile 32B random khác | 1) `unlockWithKeyfile(path)` | `WrongKey`; Locked | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-FN-030 ⚠️ | Unlock keyfile sai kích thước | Có recovery; keyfile 31B/33B/0B | 1) `unlockWithKeyfile(path)` | `InvalidFormat`; KHÔNG dùng khóa lệch; không crash; zeroize raw | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-FN-031 | Unlock keyfile khi không có recovery | Vault chưa export recovery | 1) `unlockWithKeyfile(path)` | `InvalidFormat` (`!meta_.hasRecoveryKey`) | QtTest | P1 | no | — |
| VLT-FN-032 ⚠️ | DeleteVault giữ lại .fshenc | Có vault + `.fshenc` trong thư mục | 1) `deleteVault()` | `Ok`; meta + blob bị xóa; state NoVault; masterKey wiped; file `.fshenc` KHÔNG bị xóa | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-FN-033 | AutoUnlock L2 (DPAPI) | L2 sau create (blob tồn tại) | 1) `lock()` 2) `tryAutoUnlock()` | true; Unlocked không cần passphrase | QtTest | P0 | yes(test_vault_manager) | — |
| VLT-FN-034 | AutoUnlock L3 (Convenience) | L3 | 1) `lock()` 2) `tryAutoUnlock()` | true (đọc `.vault-key.plain`) | QtTest | P1 | no | — |
| VLT-FN-035 | AutoUnlock L1 không persist | L1 (HighRamOnly) | 1) create L1 2) `lock()` 3) `tryAutoUnlock()` | false (L1 không persist blob); cần passphrase | QtTest | P0 | no | — |
| VLT-FN-036 | Auto-lock timer tự khóa | Unlocked, `autoLockMinutes=1`, L2 | 1) Không `noteActivity` >1 phút (giả lập QTimer) | VM tự `lock()`; state Locked; signal `lockStateChanged` | QtTest | P1 | no | — |
| VLT-FN-037 | Auto-lock reset bằng activity | Unlocked | 1) `noteActivity()` lặp trước timeout | Timer reset; không tự khóa | QtTest | P2 | no | — |
| VLT-FN-038 | Auto-lock tắt ở L3 | L3 unlocked | 1) Chờ quá timeout | `armAutoLock` bỏ qua khi `storageLevel==3` → KHÔNG tự khóa | QtTest | P1 | no | — |
| VLT-FN-039 | autoLockMinutes persist | Unlocked | 1) `setAutoLockMinutes(30)` 2) reload meta | Ghi meta (`autoLockMin=30`); reload giữ giá trị | QtTest | P2 | no | — |
| VLT-FN-040 | Fingerprint ổn định | Hai vault khác master key | 1) So `masterKey().fingerprint()` | Khác nhau; cùng key → cùng fingerprint (BLAKE2b-64 deterministic) | QtTest | P2 | no | — |
| VLT-FN-041 | Cap im lặng file dưới ngưỡng | `maxEncryptMb=0` → cap = 1 GiB; VM unlocked + `setServices` | 1) `addFiles([file 10 MB])` | Encrypt ngay (không emit `largeFilesPending`); `.fshenc` xuất hiện | QtTest | P0 | no | — |
| VLT-FN-042 ⚠️ | Over-cap policy ask emit pending | `maxEncryptMb=5`, behavior=0 (ask) | 1) `addFiles([file 10 MB])` | Emit `largeFilesPending` chứa file (path/name/sizeText/estText); chưa encrypt | QtTest | P0 | no | — |
| VLT-FN-043 | Resolve large → encrypt | Sau pending (VLT-FN-042) | 1) `resolveLargeFiles("encrypt")` | File lớn được encrypt; `fileOpFinished("encrypt", ...)` denom đúng | QtTest | P0 | no | — |
| VLT-FN-044 ⚠️ | Resolve large → plain ngoài vault | Sau pending | 1) `resolveLargeFiles("plain")` | File copy vào `<vault>/../FsNext Unencrypted`; KHÔNG tạo `.fshenc`; KHÔNG trong `vaultFiles` | QtTest | P0 | no | — |
| VLT-FN-045 | Resolve large → skip | Sau pending | 1) `resolveLargeFiles("skip")` | File lớn bỏ qua; `fileOpFinished` ok=true, "0/skipped"; không file trong vault | QtTest | P0 | no | — |
| VLT-FN-046 | Policy skip trực tiếp | behavior=1 (skip) | 1) `addFiles([file lớn + file nhỏ])` | File nhỏ encrypt; file lớn bỏ (không hỏi); denom = total+skipped | QtTest | P1 | no | — |
| VLT-FN-047 ⚠️ | Policy plain trực tiếp | behavior=2 | 1) `addFiles([file lớn])` | Copy ra plain folder ngoài vault (không hỏi); KHÔNG `.fshenc` | QtTest | P0 | no | — |
| VLT-FN-048 | Mix small+large ask giữ file nhỏ | behavior=0; 1 nhỏ + 1 lớn | 1) `addFiles(...)` 2) `resolveLargeFiles("skip")` | `pendingSmall_` giữ file nhỏ; emit largeFilesPending cho file lớn; resolve skip → vẫn encrypt file nhỏ | QtTest | P1 | no | — |
| VLT-FN-049 | Clamp maxEncryptMb | VM unlocked | 1) `setMaxEncryptMb(1024)` đọc lại 2) thử giá trị âm | Lưu/đọc qua SettingsService; UI clamp 0/1GB/5GB/custom | QtTest | P2 | no | — |
| VLT-FN-050 | Clamp overLimitBehavior | VM unlocked | 1) `setOverLimitBehavior(0/1/2)` | Map đúng ask/skip/plain; giá trị ngoài [0..2] default ask an toàn | QtTest | P2 | no | — |
| VLT-FN-051 | estText hợp lý không chia 0 | File lớn | 1) Đọc `estText` trong largeFilesPending | Số giây ≥1, không âm, không chia 0 (~200 MB/s) | QtTest | P2 | no | — |
| VLT-FN-052 ⚠️ | Plain folder trùng tên thêm hậu tố | Đã có file cùng tên trong plain folder | 1) `resolveLargeFiles("plain")` 2 lần | Lần 2 thêm hậu tố ` (1)` (không ghi đè bản trước) | QtTest | P1 | no | — |
| VLT-FN-053 | Cap đúng biên inclusive | `maxEncryptMb=5`; file đúng 5 MB | 1) `addFiles` | `fi.size() > cap` false → encrypt im lặng (biên inclusive cap) | QtTest | P2 | no | — |
| VLT-FN-054 | trackTemp khi decryptAndOpen | VM unlocked; có `.fshenc` decrypt được | 1) `decryptAndOpen` thành công | File tạm tạo dưới `%TEMP%/FsNextVault/<uuid>/`; được push vào `temps_` | QtTest | P1 | no | — |
| VLT-FN-055 | Sweep TTL <30' giữ lại | Temp tracked, tuổi <30' | 1) Trigger `sweepTemps(false)` (timer 60s) | Temp <30' GIỮ lại (không xóa sớm) | QtTest | P1 | no | — |
| VLT-FN-056 ⚠️ | Sweep TTL hết hạn secure-delete | Temp tracked, giả lập `createdMs` >30' | 1) `sweepTemps(false)` | Temp bị secure-delete; signal `tempCleaned(1)`; thư mục `<uuid>` removeRecursively | QtTest | P1 | no | — |
| VLT-FN-057 ⚠️ | Sweep on lock xóa hết | Temp tracked | 1) `lock()` | `sweepTemps(true)` xóa hết bất kể tuổi; vault Locked không để lại bản giải mã | QtTest | P0 | no | — |
| VLT-FN-058 ⚠️ | Sweep on destructor | Temp tracked | 1) Hủy VM | `~VaultViewModel` gọi `sweepTemps(true)`; temp bị xóa | QtTest | P0 | no | — |
| VLT-FN-059 | Overwrite trước unlink | Temp file đã biết nội dung | 1) `secureDeleteFile(path)` | File mở ReadWrite, ghi đè random toàn bộ size rồi `remove`; file không còn | QtTest | P1 | no | — |
| VLT-FN-060 | secure-delete file không tồn tại | path không tồn tại | 1) `secureDeleteFile` | Không crash (return sớm khi `!f.exists()`) | QtTest | P2 | no | — |
| VLT-FN-061 ⚠️ | Auto-lock → temp sạch | Unlocked, có temp, auto-lock kích hoạt | 1) Chờ auto-lock | Tự `lock()` → temp bị sweep; không bản giải mã sót | QtTest | P1 | no | — |
| VLT-FN-062 | Không upload khi autoUpload=false | VM unlocked; `uploader_` lambda ghi nhận | 1) `addFilesUpload([f], false, "")` | `uploader_` KHÔNG gọi; chỉ encrypt | QtTest | P1 | no | — |
| VLT-FN-063 | autoUploadDefault dùng folder cấu hình | `setAutoUploadDefault(true)`, folder cấu hình | 1) `addFiles([f])` (quick-add) | Dùng default → upload tới `uploadFolder()` | QtTest | P1 | no | — |
| VLT-FN-064 | uploadFolder mặc định "/" | `uploadFolder` chưa set | 1) đọc `uploadFolder()` | Trả `"/"`; `addFilesUpload(...,"")` chuẩn hóa về `"/"` | QtTest | P1 | no | — |
| VLT-FN-065 | Uploader chưa wire không crash | `setServices(settings, nullptr)` | 1) `addFilesUpload(...,true,...)` | Không crash (check `if (autoUpload && uploader_)`) | QtTest | P2 | no | — |

## Traceability (ID mới ↔ ID nguồn)

| ID mới | Nguồn | ID mới | Nguồn | ID mới | Nguồn |
|---|---|---|---|---|---|
| VLT-FN-001 | VLT-001 | VLT-FN-023 | VLT-408 | VLT-FN-045 | VLT-505 |
| VLT-FN-002 | VLT-002 | VLT-FN-024 | VLT-409 | VLT-FN-046 | VLT-506 |
| VLT-FN-003 | VLT-003 | VLT-FN-025 | VLT-410 | VLT-FN-047 | VLT-507 |
| VLT-FN-004 | VLT-004 | VLT-FN-026 | VLT-411 | VLT-FN-048 | VLT-508 |
| VLT-FN-005 | VLT-005 | VLT-FN-027 | VLT-412 | VLT-FN-049 | VLT-509 |
| VLT-FN-006 | VLT-006 | VLT-FN-028 | VLT-413 | VLT-FN-050 | VLT-510 |
| VLT-FN-007 | VLT-007 | VLT-FN-029 | VLT-414 | VLT-FN-051 | VLT-511 |
| VLT-FN-008 | VLT-008 | VLT-FN-030 | VLT-415 | VLT-FN-052 | VLT-512 |
| VLT-FN-009 | VLT-009 | VLT-FN-031 | VLT-416 | VLT-FN-053 | VLT-513 |
| VLT-FN-010 | VLT-010 | VLT-FN-032 | VLT-417 | VLT-FN-054 | VLT-601 |
| VLT-FN-011 | VLT-011 | VLT-FN-033 | VLT-418 | VLT-FN-055 | VLT-602 |
| VLT-FN-012 | VLT-012 | VLT-FN-034 | VLT-419 | VLT-FN-056 | VLT-603 |
| VLT-FN-013 | VLT-013 | VLT-FN-035 | VLT-420 | VLT-FN-057 | VLT-604 |
| VLT-FN-014 | VLT-014 | VLT-FN-036 | VLT-421 | VLT-FN-058 | VLT-605 |
| VLT-FN-015 | VLT-015 | VLT-FN-037 | VLT-422 | VLT-FN-059 | VLT-606 |
| VLT-FN-016 | VLT-401 | VLT-FN-038 | VLT-423 | VLT-FN-060 | VLT-607 |
| VLT-FN-017 | VLT-402 | VLT-FN-039 | VLT-424 | VLT-FN-061 | VLT-608 |
| VLT-FN-018 | VLT-403 | VLT-FN-040 | VLT-425 | VLT-FN-062 | VLT-802 |
| VLT-FN-019 | VLT-404 | VLT-FN-041 | VLT-501 | VLT-FN-063 | VLT-803 |
| VLT-FN-020 | VLT-405 | VLT-FN-042 | VLT-502 | VLT-FN-064 | VLT-804 |
| VLT-FN-021 | VLT-406 | VLT-FN-043 | VLT-503 | VLT-FN-065 | VLT-806 |
| VLT-FN-022 | VLT-407 | VLT-FN-044 | VLT-504 | | |
