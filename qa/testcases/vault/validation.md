# Vault — Validation test cases

> Loại: **validation** (parser `.fshenc` untrusted, tamper/toàn vẹn, edge bảo mật filename/path-traversal). Tổng: **50 case**.
> Cờ ⚠️ ở Tiêu đề = case rủi ro cao (security/fail-closed/fuzz/path-traversal); 🔴 = lỗ hổng nghi ngờ cao.

| ID | Tiêu đề | Tiền điều kiện | Bước | Kết quả kỳ vọng | Tool | Ưu tiên | Auto? | Bug |
|---|---|---|---|---|---|---|---|---|
| VLT-VAL-001 | Parser magic sai | Dựng header 146B thủ công | 1) 4 byte đầu = `"XENC"` 2) `parseFixedHeader` | `InvalidFormat`; `isFencFile`→false | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-002 | Parser version 0x00 | Magic đúng | 1) `VERSION=0x00` 2) parse | `UnsupportedVersion` | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-003 | Parser version 0x02 tương lai | Magic đúng | 1) `VERSION=0x02` 2) parse | `UnsupportedVersion` (không cố parse) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-004 | Parser algoId 0x00 | Version=0x01 | 1) `ALGO_ID=0x00` 2) parse | `UnsupportedVersion` | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-005 | Parser algoId 0x03 chưa định nghĩa | Version=0x01 | 1) `ALGO_ID=0x03` 2) parse | `UnsupportedVersion` | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-006 ⚠️ | Parser buffer ngắn 145 B | — | 1) Buffer 145 B (< FixedHeaderSize 146) 2) parse | `InvalidFormat`, KHÔNG đọc tràn (không ASan fault) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-007 | Parser buffer rỗng 0 B | — | 1) Buffer 0 B 2) `decryptBuffer`/`readHeader` | `InvalidFormat`; không crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-008 | Parser magic ngắn 3 B | — | 1) Buffer 3 B 2) `hasMagic` | false, `InvalidFormat` | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-009 ⚠️ | Parser FILENAME_LEN over-read | Header hợp lệ | 1) `FILENAME_LEN=0xFFFF`, file chỉ 146 B 2) readHeader/decryptStream | `InvalidFormat` (readExact fail); KHÔNG over-read 65535 B | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-010 ⚠️ | Parser FILENAME_LEN > cap 4096 | — | 1) `FILENAME_LEN=5000` (> MaxFilenameLen) 2) parseFixedHeader | `InvalidFormat` ngay (cap phòng thủ trước khi cấp phát) | QtTest | P0 | yes(test_vault_robustness) | VLT-BUG-0001 |
| VLT-VAL-011 | Parser FILENAME_LEN biên 4096 | — | 1) `FILENAME_LEN=4096` + đủ 4096 B filename 2) parse | Parse Ok (đúng biên `MaxFilenameLen`) | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-012 ⚠️ | Parser AES originalSize khổng lồ | AES file | 1) `ALGO_ID=0x02`, `ORIGINAL_SIZE=0xFFFFFFFFFFFFFFFF` 2) decryptStream | `InvalidFormat` (vượt `kAesMaxPlain` 64 MiB) trước khi cấp phát → không OOM/crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-013 ⚠️ | Parser AES size > cap 64 MiB | AES file | 1) `ORIGINAL_SIZE = 64 MiB + 1` 2) decrypt | `InvalidFormat` (reject, không alloc 64MB+) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-014 | Parser AES size biên 64 MiB | AES file | 1) `ORIGINAL_SIZE = đúng 64 MiB` với ct đủ dài 2) decrypt | Qua kiểm tra cap; tiếp tục giải mã/AEAD verify | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-015 ⚠️ | Parser chunkSize=0 | XChaCha file | 1) `ALGO_ID=0x01`, `CHUNK_SIZE=0` 2) decrypt | `InvalidFormat` (< kChunkMin 1024); KHÔNG chia 0 / loop vô hạn | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-016 ⚠️ | Parser chunkSize quá lớn | XChaCha file | 1) `CHUNK_SIZE=0x7FFFFFFF` (> kChunkMax 8 MiB) 2) decrypt | `InvalidFormat` (không cấp phát vector 2 GB) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-017 | Parser chunkSize biên | XChaCha file | 1) `CHUNK_SIZE=1024` rồi `=8 MiB` 2) parse | Cả hai qua kiểm tra biên (kChunkMin/kChunkMax inclusive) | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-018 | Parser chunk len byte ngoài biên | XChaCha file | 1) Body chunk `len:4` khai báo `clen < ABYTES(17)` hoặc `clen > maxCt` 2) parse | `InvalidFormat` (chặn dưới-tràn và trên-tràn trước readExact) | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-019 | Parser file toàn 0x00 | — | 1) File 200 B toàn byte 0x00 2) parse | Magic mismatch → `InvalidFormat`; không crash | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-020 | Parser file ngẫu nhiên | — | 1) File 500 B random 2) parse | `InvalidFormat`/`UnsupportedVersion`; không crash | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-021 ⚠️ | Parser FUZZ body | `.fshenc` hợp lệ; ASan | 1) Fuzz 10.000 vòng: lật/cắt/chèn byte 2) `decryptBuffer` | MỌI vòng trả enum lỗi (không bao giờ `Ok` sai); 0 crash / 0 over-read / 0 OOM | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-022 ⚠️ | Parser FUZZ header-only | ASan | 1) Fuzz 5.000 vòng trên 146B header random | Không crash; enum lỗi; theo dõi FILENAME_LEN/chunkSize/originalSize | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-023 | Parser truncated header mid-field | — | 1) File 100 B (cắt giữa header) 2) parse | `InvalidFormat`; readExact thất bại sạch | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-024 | Tamper FLAGS → Tampered | `.fshenc` AES + XChaCha hợp lệ; khóa K | 1) Lật 1 bit trong `FLAGS` 2) decrypt | `Tampered` (AAD = toàn header); không output | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-025 | Tamper KDF_SALT | `.fshenc` hợp lệ | 1) Lật byte trong `KDF_SALT` 2) decrypt | `Tampered` | QtTest | P1 | no | — |
| VLT-VAL-026 | Tamper WRAP_NONCE → WrongKey | `.fshenc` hợp lệ | 1) Lật byte trong `WRAP_NONCE` 2) decrypt | `WrongKey` (unwrap DEK fail); KHÔNG output | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-027 | Tamper WRAPPED_DEK → WrongKey | `.fshenc` hợp lệ | 1) Lật byte trong `WRAPPED_DEK` 2) decrypt | `WrongKey` (secretbox open fail) | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-028 | Tamper BODY_HEADER → Tampered | XChaCha file | 1) Lật byte trong `BODY_HEADER` 2) decrypt | `Tampered` (init_pull hoặc pull đầu fail) | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-029 | Tamper ORIGINAL_SIZE | `.fshenc` hợp lệ | 1) Sửa `ORIGINAL_SIZE` (AES nhỏ hơn thực) 2) decrypt | `Tampered`; nếu vượt 64 MiB → `InvalidFormat` | QtTest | P1 | no | — |
| VLT-VAL-030 | Tamper CHUNK_SIZE hợp lệ khác | XChaCha file | 1) Sửa `CHUNK_SIZE` 256K→512K 2) decrypt | `Tampered` (AAD chunk đầu đổi → pull đầu fail); không output | QtTest | P1 | no | — |
| VLT-VAL-031 | Tamper FILENAME byte | `.fshenc` hợp lệ | 1) Sửa 1 byte trong filename 2) decrypt | `Tampered` (filename trong AAD) | QtTest | P1 | no | — |
| VLT-VAL-032 | Tamper ciphertext AES | AES file | 1) Lật 1 byte trong vùng ciphertext 2) decrypt | `Tampered`; không output partial | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-033 | Tamper GCM tag | AES file | 1) Lật 1 byte trong 16-byte GCM tag 2) decrypt | `Tampered` | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-034 | Tamper ciphertext XChaCha | XChaCha file | 1) Lật 1 byte trong ciphertext 1 chunk 2) decrypt | `Tampered` tại chunk đó; dừng ngay, không ghi tiếp | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-035 ⚠️ | Đảo thứ tự chunk | XChaCha ≥3 chunk | 1) Hoán đổi chunk 2 và 3 (kèm `len:4`) 2) decrypt | `Tampered` (secretstream ràng buộc thứ tự); không output đầy đủ | QtTest | P0 | no | — |
| VLT-VAL-036 ⚠️ | Truncation trước TAG_FINAL | XChaCha ≥2 chunk | 1) Cắt cụt stream trước chunk TAG_FINAL 2) decrypt | `Tampered` (`sawFinal==false`); KHÔNG coi hợp lệ dù chunk trước verify | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-037 ⚠️ | Truncation giữa chunk | XChaCha file | 1) Cắt cụt giữa ciphertext 1 chunk 2) decrypt | `InvalidFormat`; không output | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-038 | Truncation AES body | AES file | 1) Xóa vài byte cuối ciphertext 2) decrypt | `InvalidFormat` hoặc `Tampered`; không output | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-039 | Wrong-key decrypt | `.fshenc` hợp lệ | 1) Decrypt bằng khóa khác | `WrongKey` (unwrap DEK fail); 0 byte output | QtTest | P0 | yes(test_encryption_engine) | — |
| VLT-VAL-040 | Tamper chunk len lệch | XChaCha file | 1) Sửa `len:4` chunk đầu (vẫn trong biên) 2) decrypt | `InvalidFormat` hoặc `Tampered`; không output | QtTest | P1 | no | — |
| VLT-VAL-041 | Append rác sau body | XChaCha/AES file | 1) Thêm byte rác sau TAG_FINAL / sau ct AES 2) decrypt | AES: rác bỏ qua → `Ok`. XChaCha: sau `sawFinal` break → `Ok`. Không treo | QtTest | P2 | no | — |
| VLT-VAL-042 🔴 | Path traversal filename | `.fshenc` header `filename="..\\..\\..\\Users\\x\\evil.exe"` | 1) `decryptAndOpen(fshenc)` | Phải sanitize basename / từ chối; KHÔNG ghi ngoài tmpDir, KHÔNG ghi đè file tùy ý | QtTest | P0 | yes(test_vault_robustness) | VLT-BUG-0002 |
| VLT-VAL-043 🔴 | Absolute path filename | header `filename="C:\\Windows\\System32\\drivers\\etc\\hosts"` | 1) `decryptAndOpen` | Chỉ lấy basename; KHÔNG giải thành đường dẫn tuyệt đối lạ | QtTest | P0 | yes(test_vault_robustness) | — |
| VLT-VAL-044 ⚠️ | Filename rỗng fallback | `.fshenc` với `FILENAME_LEN=0` | 1) `refreshFiles`/`decryptAndOpen` | Fallback `completeBaseName()` của `.fshenc`; không crash; tên hợp lý | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-045 | Filename rất dài 4096 B | filename 4096 B (max) | 1) `readHeader` 2) hiển thị 3) decrypt-to-temp | Parse Ok; UI cắt hiển thị; OS path-length (>260 Windows) không crash | QtTest | P2 | yes(test_vault_robustness) | — |
| VLT-VAL-046 | Filename unicode VI | filename "báo cáo tài chính.docx" (UTF-8) | 1) encrypt 2) readHeader 3) decryptAndOpen | Tên round-trip đúng UTF-8; mở file đúng tên tiếng Việt | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-047 | File không đuôi | encrypt file "README" (no ext) | 1) round-trip | `completeBaseName`/header xử lý OK; decrypt mở được | QtTest | P2 | no | — |
| VLT-VAL-048 ⚠️ | Ký tự đặc biệt filename | filename chứa `: * ? " < > \|` (cấm trên Windows FS) | 1) `decryptAndOpen` | Tạo temp với ký tự cấm → write fail → `IoError`, fail-closed xóa; KHÔNG crash; kiểm sanitize | QtTest | P1 | yes(test_vault_robustness) | — |
| VLT-VAL-049 | Filename có newline/null | filename chứa `\n` hoặc byte 0x00 giữa chuỗi | 1) readHeader | Chuỗi giữ nguyên bytes (assign theo len); hiển thị an toàn (không cắt tại NUL) | QtTest | P2 | no | — |
| VLT-VAL-050 ⚠️ | Plain-folder filename traversal | file nguồn tên bình thường | 1) `copyToPlainFolder` | Dùng `fi.fileName()` (đã basename) → an toàn; xác nhận không ghi ngoài plainDir | QtTest | P2 | no | — |

## Traceability (ID mới ↔ ID nguồn)

| ID mới | Nguồn | ID mới | Nguồn | ID mới | Nguồn |
|---|---|---|---|---|---|
| VLT-VAL-001 | VLT-101 | VLT-VAL-018 | VLT-118 | VLT-VAL-035 | VLT-212 |
| VLT-VAL-002 | VLT-102 | VLT-VAL-019 | VLT-119 | VLT-VAL-036 | VLT-213 |
| VLT-VAL-003 | VLT-103 | VLT-VAL-020 | VLT-120 | VLT-VAL-037 | VLT-214 |
| VLT-VAL-004 | VLT-104 | VLT-VAL-021 | VLT-121 | VLT-VAL-038 | VLT-215 |
| VLT-VAL-005 | VLT-105 | VLT-VAL-022 | VLT-122 | VLT-VAL-039 | VLT-216 |
| VLT-VAL-006 | VLT-106 | VLT-VAL-023 | VLT-123 | VLT-VAL-040 | VLT-217 |
| VLT-VAL-007 | VLT-107 | VLT-VAL-024 | VLT-201 | VLT-VAL-041 | VLT-218 |
| VLT-VAL-008 | VLT-108 | VLT-VAL-025 | VLT-202 | VLT-VAL-042 | VLT-901 |
| VLT-VAL-009 | VLT-109 | VLT-VAL-026 | VLT-203 | VLT-VAL-043 | VLT-902 |
| VLT-VAL-010 | VLT-110 | VLT-VAL-027 | VLT-204 | VLT-VAL-044 | VLT-903 |
| VLT-VAL-011 | VLT-111 | VLT-VAL-028 | VLT-205 | VLT-VAL-045 | VLT-904 |
| VLT-VAL-012 | VLT-112 | VLT-VAL-029 | VLT-206 | VLT-VAL-046 | VLT-905 |
| VLT-VAL-013 | VLT-113 | VLT-VAL-030 | VLT-207 | VLT-VAL-047 | VLT-906 |
| VLT-VAL-014 | VLT-114 | VLT-VAL-031 | VLT-208 | VLT-VAL-048 | VLT-907 |
| VLT-VAL-015 | VLT-115 | VLT-VAL-032 | VLT-209 | VLT-VAL-049 | VLT-908 |
| VLT-VAL-016 | VLT-116 | VLT-VAL-033 | VLT-210 | VLT-VAL-050 | VLT-909 |
| VLT-VAL-017 | VLT-117 | VLT-VAL-034 | VLT-211 | | |
