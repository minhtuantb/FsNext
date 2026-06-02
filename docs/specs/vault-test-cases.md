# BỘ TEST CASE TOÀN DIỆN — Vault / E2EE (FsNext)

| Trường | Giá trị |
|---|---|
| Phạm vi | Encryption Engine + Key Management (Vault), `.fshenc`, `VaultManager`, `VaultViewModel` |
| Module dưới test | `src/core/crypto/*`, `src/core/services/VaultManager.{h,cpp}`, `src/viewmodels/VaultViewModel.{h,cpp}` |
| Spec nguồn | `encryption-plan.md`, `encryption-ui-brief.md`, `encryption-progress.md` |
| Loại test | **Unit** (QtTest, crypto core/manager), **Integration** (VM async + services), **Manual** (UI/kill-app/đĩa đầy) |
| Ưu tiên | **P0** chặn release (an toàn dữ liệu / fail-closed / parser untrusted), **P1** quan trọng, **P2** nên có |
| Cập nhật | 2026-06-02 |

> **Cách đọc:** Cờ ⚠️ = case nghi ngờ dễ lộ bug (đã phân tích code, xem mục cuối "Khu vực rủi ro cao").
> Ký hiệu kích thước: 1 MiB = 1.048.576 B (ngưỡng AES↔XChaCha); cap AES decrypt = 64 MiB; chunk hợp lệ 1 KiB–8 MiB.

---

## NHÓM 1 — Crypto round-trip (`EncryptionEngine`)

Tiền điều kiện chung: `crypto::init()` đã chạy; có một `Key` 32B hợp lệ (vault-mode hoặc `fromBytes`). Trừ khi nói khác, dùng `encryptBuffer`/`decryptBuffer` hoặc `encryptFile`/`decryptFile`.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-001 | Round-trip | Máy có AES-NI (`aes256GcmAvailable()==true`) | Encrypt buffer 100 B → decrypt lại | Plaintext khớp byte-for-byte; header `algoId==0x02` (AES single-shot), `chunkSize==0`, flag `FlagSingleShot` set | Unit | P0 |
| VLT-002 | Round-trip | AES-NI | Encrypt file 1 KiB ảnh nhị phân ngẫu nhiên → decrypt | Khớp; `originalSize==1024` | Unit | P0 |
| VLT-003 | Round-trip | — | Encrypt buffer 5 MiB ngẫu nhiên → decrypt | Khớp; `algoId==0x01` (XChaCha secretstream), `chunkSize==256 KiB` | Unit | P0 |
| VLT-004 | Round-trip | — | Encrypt file 1.5 GiB (hoặc giả lập kích thước >1 GiB) → decrypt; theo dõi RAM | Khớp; `chunkSize==1 MiB`; RAM ổn định < ~100 MB (streaming) | Integration | P1 |
| VLT-005 | Round-trip | AES-NI | `EncryptOptions.forceXChaCha=true`, file 100 B → encrypt → decrypt | `algoId==0x01` dù file nhỏ; round-trip khớp (kiểm "Luôn ưu tiên tương thích") | Unit | P0 |
| VLT-006 | Round-trip | — | Encrypt buffer **0 byte** (n=0) → decrypt | `originalSize==0`; decrypt trả buffer rỗng, `Ok`; không crash. (AES: ct = 16B tag; XChaCha nếu forced: 1 chunk TAG_FINAL) | Unit | P0 |
| VLT-007 ⚠️ | Round-trip boundary | AES-NI | Encrypt file **đúng 1 MiB (1048576 B)** → decrypt | Chọn AES (`<= SmallFileThreshold`), `algoId==0x02`; round-trip khớp | Unit | P0 |
| VLT-008 ⚠️ | Round-trip boundary | AES-NI | Encrypt file **1 MiB + 1 (1048577 B)** → decrypt | Chọn XChaCha (`> threshold`), `algoId==0x01`, `chunkSize==256 KiB`; khớp | Unit | P0 |
| VLT-009 | Round-trip boundary | — | Encrypt file **đúng 1 GiB** rồi **1 GiB + 1** → so `chunkSize` | ≤1 GiB → 256 KiB; >1 GiB → 1 MiB (ranh giới `LargeFileThreshold`) | Unit | P1 |
| VLT-010 | Round-trip non-determinism | — | Encrypt cùng nội dung + cùng khóa **2 lần** → so ciphertext | Hai `.fshenc` KHÁC nhau (DEK + nonce + stream header random/ file); cả hai decrypt khớp | Unit | P1 |
| VLT-011 | Round-trip fallback | Giả lập `aes256gcm_is_available()==0` (môi trường không AES-NI) | Encrypt file 100 B (không force) → decrypt | Dùng XChaCha cho file nhỏ (`algoId==0x01`); round-trip khớp 100% | Unit | P1 |
| VLT-012 | Round-trip | — | Round-trip 1000 file kích thước ngẫu nhiên 0 B–4 MiB (cả AES & XChaCha lẫn lộn) | Tất cả khớp; chọn nhánh đúng theo size | Unit | P1 |
| VLT-013 | Round-trip file API | — | `encryptFile` → `decryptFile` ảnh + docx thật; so hash plaintext | Hash khớp; file mở được bằng app gốc | Integration | P1 |
| VLT-014 | Round-trip multi-chunk | — | Encrypt file = đúng bội số nguyên của chunk (vd 512 KiB = 2×256 KiB) | Chunk cuối vẫn TAG_FINAL (kể cả khi remaining==0 sau chunk đầy); decrypt khớp, `sawFinal==true` | Unit | P1 |
| VLT-015 | Round-trip header echo | — | Encrypt với `storedFilename` rỗng từ path `C:\a\b\report.docx` | Header `filename=="report.docx"` (basename); decrypt trả đúng tên | Unit | P2 |

---

## NHÓM 2 — Validate dữ liệu / parser `.fshenc` untrusted (QUAN TRỌNG)

Tiền điều kiện chung: dựng buffer/file `.fshenc` thủ công (hex), gọi `parseFixedHeader` / `readHeader` / `decryptBuffer`. Mục tiêu: **không over-read, không crash, trả lỗi enum sạch**.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-101 | Parser magic | — | Header 146B nhưng 4 byte đầu = `"XENC"` | `parseFixedHeader` → `InvalidFormat`; `isFencFile`→false | Unit | P0 |
| VLT-102 | Parser version | — | Magic đúng, `VERSION=0x00` | `UnsupportedVersion` | Unit | P0 |
| VLT-103 | Parser version | — | Magic đúng, `VERSION=0x02` (tương lai) | `UnsupportedVersion` (không cố parse) | Unit | P0 |
| VLT-104 | Parser algoId | — | Version=0x01, `ALGO_ID=0x00` | `UnsupportedVersion` (parser dùng mã này cho algo lạ) | Unit | P0 |
| VLT-105 | Parser algoId | — | `ALGO_ID=0x03` (chưa định nghĩa) | `UnsupportedVersion` | Unit | P0 |
| VLT-106 ⚠️ | Parser len ngắn | — | Buffer dài **145 B** (< FixedHeaderSize 146) | `InvalidFormat`, KHÔNG đọc tràn (không AddressSanitizer fault) | Unit | P0 |
| VLT-107 | Parser len 0 | — | Buffer **rỗng (0 B)** | `decryptBuffer`/`readHeader` → `InvalidFormat`; không crash | Unit | P0 |
| VLT-108 | Parser magic ngắn | — | Buffer chỉ **3 B** | `hasMagic`→false, `InvalidFormat` | Unit | P0 |
| VLT-109 ⚠️ | Parser FILENAME_LEN | — | Header hợp lệ, `FILENAME_LEN=0xFFFF` nhưng file chỉ có 146 B (không có filename theo sau) | `readHeader`/`decryptStream` → `InvalidFormat` (readExact thất bại); **KHÔNG over-read 65535 B** | Unit | P0 |
| VLT-110 ⚠️ | Parser FILENAME_LEN cap | — | `FILENAME_LEN=5000` (> MaxFilenameLen 4096) | `parseFixedHeader` → `InvalidFormat` ngay (cap phòng thủ trước khi cấp phát) | Unit | P0 |
| VLT-111 | Parser FILENAME_LEN biên | — | `FILENAME_LEN=4096` + đủ 4096 B filename theo sau | Parse Ok (đúng biên `MaxFilenameLen`) | Unit | P1 |
| VLT-112 ⚠️ | Parser AES size khổng lồ | AES file | `ALGO_ID=0x02`, `ORIGINAL_SIZE=0xFFFFFFFFFFFFFFFF` | `decryptStream` → `InvalidFormat` (vượt `kAesMaxPlain` 64 MiB) **trước khi** cấp phát vector → không OOM/crash | Unit | P0 |
| VLT-113 ⚠️ | Parser AES size > cap | AES file | `ORIGINAL_SIZE = 64 MiB + 1` | `InvalidFormat` (reject, không alloc 64MB+) | Unit | P0 |
| VLT-114 | Parser AES size biên | AES file | `ORIGINAL_SIZE = đúng 64 MiB` với ct đủ dài | Qua kiểm tra cap (không reject ở bước size); tiếp tục giải mã/AEAD verify | Unit | P1 |
| VLT-115 ⚠️ | Parser chunkSize=0 | XChaCha file | `ALGO_ID=0x01`, `CHUNK_SIZE=0` | `InvalidFormat` (< kChunkMin 1024) — KHÔNG chia 0 / loop vô hạn | Unit | P0 |
| VLT-116 ⚠️ | Parser chunkSize lớn | XChaCha file | `CHUNK_SIZE=0x7FFFFFFF` (> kChunkMax 8 MiB) | `InvalidFormat` (không cấp phát vector 2 GB) | Unit | P0 |
| VLT-117 | Parser chunkSize biên | XChaCha file | `CHUNK_SIZE=1024` rồi `=8 MiB` | Cả hai qua kiểm tra biên (kChunkMin/kChunkMax inclusive) | Unit | P1 |
| VLT-118 | Parser chunk len byte | XChaCha file | Body chunk `len:4` khai báo `clen < ABYTES(17)` hoặc `clen > maxCt` | `InvalidFormat` (chặn cả dưới-tràn và trên-tràn trước readExact) | Unit | P0 |
| VLT-119 | Parser file toàn 0x00 | — | File 200 B toàn byte 0x00 | Magic mismatch → `InvalidFormat`; không crash | Unit | P0 |
| VLT-120 | Parser file ngẫu nhiên | — | File 500 B ngẫu nhiên | `InvalidFormat`/`UnsupportedVersion`; không crash | Unit | P1 |
| VLT-121 ⚠️ | Parser FUZZ | — | Fuzz N=10.000 vòng: lấy `.fshenc` hợp lệ, lật/cắt/chèn byte ngẫu nhiên, gọi `decryptBuffer` | MỌI vòng trả enum lỗi (không bao giờ `Ok` với output sai); **0 crash / 0 over-read / 0 OOM** (chạy dưới ASan) | Unit | P0 |
| VLT-122 ⚠️ | Parser FUZZ header-only | — | Fuzz 5.000 vòng chỉ trên 146B header (random hoàn toàn) | Không crash; chỉ ra enum lỗi; đặc biệt theo dõi nhánh FILENAME_LEN/chunkSize/originalSize | Unit | P0 |
| VLT-123 | Parser truncated header mid-field | — | File 100 B (cắt giữa header) | `InvalidFormat`; readExact thất bại sạch | Unit | P1 |

---

## NHÓM 3 — Tamper / toàn vẹn (integrity)

Tiền điều kiện chung: tạo `.fshenc` hợp lệ với khóa K; sửa 1 vị trí; decrypt bằng K → kỳ vọng lỗi, **không output**.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-201 | Tamper header | AES + XChaCha | Lật 1 bit trong `FLAGS` (giữ algoId/version hợp lệ) | Decrypt → `Tampered` (AAD = toàn header, lật bất kỳ field → AEAD fail); không output | Unit | P0 |
| VLT-202 | Tamper header | — | Lật byte trong `KDF_SALT` (vault-mode = 0, vẫn là AAD) | `Tampered` | Unit | P1 |
| VLT-203 | Tamper header | — | Lật byte trong `WRAP_NONCE` | `WrongKey` (unwrap DEK secretbox thất bại) — KHÔNG output | Unit | P0 |
| VLT-204 | Tamper header | — | Lật byte trong `WRAPPED_DEK` | `WrongKey` (secretbox open fail) | Unit | P0 |
| VLT-205 | Tamper header | — | Lật byte trong `BODY_HEADER` (XChaCha stream header) | `Tampered` (init_pull hoặc pull đầu fail) | Unit | P0 |
| VLT-206 | Tamper header | — | Sửa `ORIGINAL_SIZE` (AES: nhỏ hơn thực) | `Tampered` (AAD đổi → AES decrypt fail); nếu vượt 64 MiB → `InvalidFormat` | Unit | P1 |
| VLT-207 | Tamper header | — | Sửa `CHUNK_SIZE` (XChaCha) sang giá trị hợp lệ khác (vd 256K→512K) | `Tampered` (AAD chunk đầu đổi → pull đầu fail); không output | Unit | P1 |
| VLT-208 | Tamper header | — | Sửa `FILENAME` (1 byte trong tên) | `Tampered` (filename nằm trong AAD) | Unit | P1 |
| VLT-209 | Tamper ciphertext | AES | Lật 1 byte trong vùng ciphertext AES | `Tampered`; không output partial | Unit | P0 |
| VLT-210 | Tamper tag | AES | Lật 1 byte trong 16-byte GCM tag (cuối body) | `Tampered` | Unit | P0 |
| VLT-211 | Tamper ciphertext | XChaCha | Lật 1 byte trong ciphertext của 1 chunk | `Tampered` tại chunk đó; dừng ngay, không ghi tiếp | Unit | P0 |
| VLT-212 ⚠️ | Đảo chunk | XChaCha ≥3 chunk | Hoán đổi vị trí chunk thứ 2 và 3 (kể cả `len:4` đi kèm) | `Tampered` (secretstream ràng buộc thứ tự); không output đầy đủ | Unit | P0 |
| VLT-213 ⚠️ | Truncation | XChaCha ≥2 chunk | Cắt cụt stream **trước** chunk có TAG_FINAL (xóa chunk cuối) | `Tampered` (`sawFinal==false` → trả Tampered); KHÔNG coi là hợp lệ dù các chunk trước verify | Unit | P0 |
| VLT-214 ⚠️ | Truncation giữa chunk | XChaCha | Cắt cụt giữa ciphertext 1 chunk (readExact thiếu) | `InvalidFormat`; không output | Unit | P0 |
| VLT-215 | Truncation AES body | AES | Xóa vài byte cuối ciphertext AES | `InvalidFormat` (readExact ct thiếu) hoặc `Tampered`; không output | Unit | P0 |
| VLT-216 | Tamper wrong-key | — | Decrypt `.fshenc` hợp lệ bằng **khóa khác** | `WrongKey` (unwrap DEK fail); 0 byte output | Unit | P0 |
| VLT-217 | Tamper chunk len | XChaCha | Sửa `len:4` của chunk đầu thành giá trị lệch (vẫn trong biên) | `InvalidFormat` hoặc `Tampered`; không output | Unit | P1 |
| VLT-218 | Append rác | XChaCha/AES | Thêm byte rác **sau** TAG_FINAL / sau ct AES | AES: rác bị bỏ qua (đọc đúng ctlen) → `Ok`. XChaCha: sau `sawFinal` break → `Ok`, rác bỏ qua. (Xác nhận hành vi không treo) | Unit | P2 |

---

## NHÓM 4 — Fail-closed / crash-safety / KILL APP GIỮA CHỪNG (QUAN TRỌNG)

Tiền điều kiện chung: vault unlocked (trừ case meta/blob). Nhiều case là **Manual** (kill process, đĩa đầy) — mô phỏng bằng file cắt cụt khi không kill được thật.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-301 ⚠️ | Fail-closed decrypt | `.fshenc` tamper (vd VLT-209) | `decryptFile(in,out,key)` ra đường dẫn `out` mới | Trả lỗi; file `out` **KHÔNG tồn tại** (xóa bản dở — spec §1.5, code: `fs::remove(out)` ở nhánh lỗi) | Unit | P0 |
| VLT-302 ⚠️ | Fail-closed decrypt wrong-key | `.fshenc` hợp lệ | `decryptFile` bằng khóa sai | `WrongKey`; `out` không tồn tại; KHÔNG để lại 0-byte/partial | Unit | P0 |
| VLT-303 ⚠️ | Fail-closed truncated | `.fshenc` cắt cụt trước TAG_FINAL (mô phỏng app bị kill khi đang ghi) | `decryptFile` | `Tampered`; `out` bị xóa; không crash; không partial plaintext | Unit | P0 |
| VLT-304 ⚠️ | Fail-closed encrypt IoError | Đường dẫn `out` không ghi được (thư mục read-only / ổ không tồn tại) | `encryptFile(in, out, key)` | `IoError`; KHÔNG để lại `.fshenc` dở tại out (code xóa ở nhánh lỗi) | Manual/Integration | P0 |
| VLT-305 ⚠️ | Fail-closed disk-full | Đĩa/quota gần đầy (giả lập ổ nhỏ) khi encrypt file lớn | Encrypt file > dung lượng trống | `IoError` khi `out.write`/flush fail; `.fshenc` dở bị xóa; app không crash | Manual | P0 |
| VLT-306 ⚠️ | Meta corrupt truncated | `.vault-meta.json` bị cắt cụt giữa chừng (kill khi đang ghi) | `setVaultDir` → `tryAutoUnlock`/`unlock` | `VaultMetadata::load` → `InvalidFormat`; VM xử lý graceful (vaultExists có thể true vì file tồn tại nhưng load fail → unlock trả lỗi); KHÔNG crash | Integration | P0 |
| VLT-307 ⚠️ | Meta JSON hỏng | `.vault-meta.json` chứa JSON không hợp lệ / thiếu `wrappedMasterKey` | `unlock(pass)` | `ensureMetaLoaded` false → `unlock` trả `InvalidFormat`; không crash | Unit | P0 |
| VLT-308 ⚠️ | Meta base64 sai độ dài | `wrappedMasterKey` base64 decode ≠ 48B (hoặc `wrapNonce` ≠ 24B) | load meta | `unb64` trả false → `InvalidFormat`; không cấp phát/đọc sai | Unit | P0 |
| VLT-309 ⚠️ | Atomic write integrity | Meta đang lưu (`save`) bị ngắt sau khi ghi `.tmp` nhưng trước rename | Khởi động lại, load meta | Meta cũ còn nguyên (rename atomic; `.tmp` dở không thay thế bản tốt); nếu chỉ có `.tmp` → vault coi như chưa có hoặc giữ bản cũ — KHÔNG hỏng | Manual | P0 |
| VLT-310 ⚠️ | Blob DPAPI hỏng | L2; `.vault-key.dpapi` bị cắt/hỏng | `tryAutoUnlock()` | `SecureStore::decrypt` trả ≠ 32B → `tryAutoUnlock` **false**; không crash; UI rơi về DLG-UNLOCK | Integration | P0 |
| VLT-311 ⚠️ | Blob plain hỏng | L3; `.vault-key.plain` ≠ 32B | `tryAutoUnlock()` | false; không crash | Unit | P1 |
| VLT-312 ⚠️ | Trạng thái lệch: có meta thiếu blob | L2 có meta nhưng XÓA `.vault-key.dpapi` | `tryAutoUnlock()` | false (open fail) → vault Locked → cần passphrase; không crash | Integration | P0 |
| VLT-313 ⚠️ | Trạng thái lệch: có blob thiếu meta | Có `.vault-key.dpapi` nhưng XÓA `.vault-meta.json` | `setVaultDir` → state | `vaultExists==false` (chỉ dựa meta) → state NoVault; `tryAutoUnlock` false (ensureMetaLoaded fail) | Integration | P1 |
| VLT-314 ⚠️ | Kill batch encrypt | Batch nhiều file đang encrypt | Gọi `cancelBatch()` (mô phỏng kill) giữa chừng | File đã xong = `.fshenc` HỢP LỆ (decrypt được); file đang dở KHÔNG nằm trong vault (encryptFile xóa output dở); file chưa làm → `lastFailed_` (retry được) | Integration | P0 |
| VLT-315 ⚠️ | Kill decrypt-to-temp | `decryptAndOpen` đang chạy | App đóng/lock trong khi đang giải mã | Bản giải mã tạm dở: decryptFile fail→xóa; nếu đã xong & tracked → `sweepTemps(true)` khi `lock()`/destructor secure-delete | Integration | P0 |
| VLT-316 | Temp sót → secure-delete khi mở lại | Có temp tracked từ phiên trước (cùng VM sống) | `lock()` rồi mở lại | `lock()` gọi `sweepTemps(true)` → temp bị overwrite + xóa; signal `tempCleaned(n)` | Integration | P1 |
| VLT-317 ⚠️ | Temp sót sau crash thật | App crash để lại `%TEMP%/FsNextVault/*` (KHÔNG tracked trong VM mới) | Khởi động lại app | **GAP nghi ngờ:** VM mới không quét thư mục `FsNextVault` cũ → temp giải mã có thể tồn tại sau crash. Kiểm có cơ chế sweep startup không (nếu không → bug an toàn) | Manual | P0 |
| VLT-318 | Fail-closed buffer API | `decryptBuffer` tamper | Decrypt buffer hỏng | `out` được `clear()` đầu hàm; chỉ assign khi `Ok` → out rỗng khi lỗi | Unit | P1 |
| VLT-319 ⚠️ | Lock giữa op async | Đang `createVault`/`unlock` (busy_) | Gọi `lock()` | `lock()` return sớm khi `busy_` (không đụng VaultManager giữa worker) → không race/crash | Integration | P0 |
| VLT-320 | VM hủy giữa op | Async op đang chạy, VM bị destroy | Đóng app | `QPointer guard` chặn callback (`if(!guard) return`); `~VaultViewModel` sweep temp; pool drain ở main.cpp | Integration | P1 |

---

## NHÓM 5 — VaultManager lifecycle

Tiền điều kiện chung: `setVaultDir` tới thư mục tạm sạch.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-401 | Create | Thư mục rỗng | `createVault("Personal", pass, balanced, MediumDpapi)` | `Ok`; state Unlocked; `.vault-meta.json` + `.vault-key.dpapi` tồn tại; `masterKey().valid()` | Unit | P0 |
| VLT-402 | Create AlreadyExists | Đã có vault | `createVault(...)` lần 2 | `AlreadyExists`; meta cũ KHÔNG bị ghi đè | Unit | P0 |
| VLT-403 | Create no dir | `vaultDir_` rỗng | `createVault(...)` | `InternalError` | Unit | P1 |
| VLT-404 | Unlock đúng | Vault locked | `lock()` → `unlock(pass đúng)` | `Ok`; Unlocked; masterKey hợp lệ; decrypt file vault thành công | Unit | P0 |
| VLT-405 | Unlock sai | Vault locked | `unlock(pass sai)` | `WrongKey` (KDF ra KEK sai → unwrap fail); state vẫn Locked; không crash | Unit | P0 |
| VLT-406 | Unlock pass rỗng | Vault locked | `unlock("")` | KDF chạy, unwrap fail → `WrongKey` (không bypass) | Unit | P0 |
| VLT-407 ⚠️ | ChangePass sai pass cũ | Unlocked/locked | `changePassphrase(oldSai, new, kdf)` | `WrongKey`/`KdfFailed`; meta KHÔNG đổi; vault vẫn mở được bằng pass cũ | Unit | P0 |
| VLT-408 ⚠️ | ChangePass đúng | Vault có data | `changePassphrase(oldĐúng, new, kdf)` | `Ok`; unlock bằng `new` thành công; unlock bằng `old` **thất bại** (`WrongKey`) | Unit | P0 |
| VLT-409 ⚠️ | ChangePass — file cũ vẫn mở | Có `.fshenc` mã hóa trước đổi pass | Đổi pass → unlock bằng new → decrypt file cũ | File cũ decrypt OK (chỉ re-wrap Master Key, DEK/file không đổi — spec §6.1) | Unit | P0 |
| VLT-410 | ChangePass preset đổi | — | Đổi pass kèm `kdf=maximum` | Meta cập nhật `kdfOps/MemKb`; unlock new dùng KDF mới | Unit | P1 |
| VLT-411 | Export recovery đúng | Unlocked | `exportRecoveryKeyfile(path)` | `Ok`; file 32B tại path; meta `hasRecoveryKey==true` + `wrappedMasterKeyRecovery` | Unit | P0 |
| VLT-412 | Export khi locked | Vault locked | `exportRecoveryKeyfile(path)` | `NotUnlocked`; không tạo file | Unit | P0 |
| VLT-413 | Unlock bằng keyfile đúng | Đã export recovery; locked | `unlockWithKeyfile(path đúng)` | `Ok`; Unlocked; masterKey khớp masterKey gốc | Unit | P0 |
| VLT-414 | Unlock keyfile sai nội dung | Có recovery; keyfile 32B random khác | `unlockWithKeyfile(path)` | `WrongKey` (unwrap recovery fail); Locked | Unit | P0 |
| VLT-415 ⚠️ | Unlock keyfile sai kích thước | Có recovery; keyfile 31B / 33B / 0B | `unlockWithKeyfile(path)` | `InvalidFormat` (raw.size() != 32); KHÔNG dùng khóa lệch; không crash; zeroize raw | Unit | P0 |
| VLT-416 | Unlock keyfile không có recovery | Vault chưa export recovery | `unlockWithKeyfile(path)` | `InvalidFormat` (`!meta_.hasRecoveryKey`) | Unit | P1 |
| VLT-417 ⚠️ | DeleteVault | Có vault + có `.fshenc` trong thư mục | `deleteVault()` | `Ok`; `.vault-meta.json` + blob bị xóa; state NoVault; masterKey wiped; **file `.fshenc` KHÔNG bị xóa** (vẫn còn trên đĩa) | Unit | P0 |
| VLT-418 | AutoUnlock L2 | L2 sau create (blob tồn tại) | `lock()` → `tryAutoUnlock()` | true; Unlocked không cần passphrase | Integration | P0 |
| VLT-419 | AutoUnlock L3 | L3 (Convenience) | `lock()` → `tryAutoUnlock()` | true (đọc `.vault-key.plain`) | Unit | P1 |
| VLT-420 | AutoUnlock L1 | L1 (HighRamOnly) | create L1 → `lock()` → `tryAutoUnlock()` | **false** (L1 không persist blob); cần passphrase | Unit | P0 |
| VLT-421 | Auto-lock timer | Unlocked, `autoLockMinutes=1`, L2 | Không `noteActivity` trong >1 phút (giả lập QTimer) | VM tự `lock()`; state Locked; signal `lockStateChanged` | Integration | P1 |
| VLT-422 | Auto-lock reset | Unlocked | `noteActivity()` lặp trước timeout | Timer reset; không tự khóa | Integration | P2 |
| VLT-423 | Auto-lock tắt ở L3 | L3 unlocked | Chờ quá timeout | `armAutoLock` bỏ qua khi `storageLevel==3` → KHÔNG tự khóa (Convenience) | Unit | P1 |
| VLT-424 | autoLockMinutes persist | Unlocked | `setAutoLockMinutes(30)` | Ghi meta (`autoLockMin=30`); reload meta giữ giá trị | Unit | P2 |
| VLT-425 | Fingerprint ổn định | Hai vault khác master key | So `masterKey().fingerprint()` | Khác nhau; cùng key → cùng fingerprint (BLAKE2b-64 deterministic) | Unit | P2 |

---

## NHÓM 6 — Size-cap / DLG-LARGEFILE (`VaultViewModel`)

Tiền điều kiện chung: VM unlocked + `setServices(settings, uploader)`; điều khiển `maxEncryptMb`/`overLimitBehavior` qua SettingsService.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-501 | Cap im lặng | `maxEncryptMb=0` (không cap) → cap hiệu lực = 1 GiB | `addFiles([file 10 MB])` | Encrypt ngay (không emit `largeFilesPending`); `.fshenc` xuất hiện | Integration | P0 |
| VLT-502 ⚠️ | Over-cap policy ask | `maxEncryptMb=5` (5 MB), behavior=0 (ask) | `addFiles([file 10 MB])` | Emit `largeFilesPending` chứa file đó (path/name/sizeText/estText); chưa encrypt | Integration | P0 |
| VLT-503 | Resolve encrypt | Sau VLT-502 (pending) | `resolveLargeFiles("encrypt")` | File lớn được encrypt; `fileOpFinished("encrypt", ...)` denom đúng | Integration | P0 |
| VLT-504 ⚠️ | Resolve plain | Sau pending | `resolveLargeFiles("plain")` | File copy vào `<vault>/../FsNext Unencrypted` (NGOÀI vault); KHÔNG tạo `.fshenc`; KHÔNG nằm trong `vaultFiles` | Integration | P0 |
| VLT-505 | Resolve skip | Sau pending | `resolveLargeFiles("skip")` | File lớn bỏ qua; `fileOpFinished` ok=true, "0/skipped"; không file trong vault | Integration | P0 |
| VLT-506 | Policy skip trực tiếp | behavior=1 (skip) | `addFiles([file lớn + file nhỏ])` | File nhỏ encrypt; file lớn bỏ (không hỏi); denom = total+skipped | Integration | P1 |
| VLT-507 ⚠️ | Policy plain trực tiếp | behavior=2 | `addFiles([file lớn])` | Copy ra plain folder ngoài vault (không hỏi); KHÔNG `.fshenc` | Integration | P0 |
| VLT-508 | Mix small+large ask | behavior=0; 1 nhỏ + 1 lớn | `addFiles(...)` | `pendingSmall_` giữ file nhỏ; emit largeFilesPending cho file lớn; resolve("skip") → vẫn encrypt file nhỏ | Integration | P1 |
| VLT-509 | Clamp maxEncryptMb | — | `setMaxEncryptMb(1024)` rồi đọc lại; thử giá trị âm | Giá trị lưu/đọc qua SettingsService; (kiểm UI clamp 0/1GB/5GB/custom) | Integration | P2 |
| VLT-510 | Clamp overLimitBehavior | — | `setOverLimitBehavior(0/1/2)` | Map đúng ask/skip/plain; giá trị ngoài [0..2] xử lý an toàn (default ask) | Integration | P2 |
| VLT-511 | estText hợp lý | File lớn | Đọc `estText` trong largeFilesPending | Số giây ≥1, không âm, không chia 0 (ước ~200 MB/s) | Unit | P2 |
| VLT-512 ⚠️ | Plain folder trùng tên | Đã có file cùng tên trong plain folder | `resolveLargeFiles("plain")` 2 lần | Lần 2 thêm hậu tố ` (1)` (không ghi đè bản trước) | Integration | P1 |
| VLT-513 | Cap đúng biên | `maxEncryptMb=5`; file đúng 5 MB | `addFiles` | `fi.size() > cap` false → encrypt im lặng (biên inclusive cap) | Integration | P2 |

---

## NHÓM 7 — Secure-delete

Tiền điều kiện chung: VM unlocked; có `.fshenc` decrypt được.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-601 | trackTemp | — | `decryptAndOpen` thành công | File tạm tạo dưới `%TEMP%/FsNextVault/<uuid>/`; được push vào `temps_` | Integration | P1 |
| VLT-602 | Sweep TTL 30' | Temp tracked, tuổi <30' | Trigger `sweepTemps(false)` (timer 60s) | Temp <30' GIỮ lại (không xóa sớm) | Integration | P1 |
| VLT-603 ⚠️ | Sweep TTL hết hạn | Temp tracked, giả lập `createdMs` >30' trước | `sweepTemps(false)` | Temp bị secure-delete; signal `tempCleaned(1)`; thư mục `<uuid>` removeRecursively | Integration | P1 |
| VLT-604 ⚠️ | Sweep on lock | Temp tracked | `lock()` | `sweepTemps(true)` xóa hết bất kể tuổi; vault Locked không để lại bản giải mã | Integration | P0 |
| VLT-605 ⚠️ | Sweep on destructor | Temp tracked | Hủy VM | `~VaultViewModel` gọi `sweepTemps(true)`; temp bị xóa | Integration | P0 |
| VLT-606 | Overwrite trước unlink | Temp file đã biết offset/nội dung | `secureDeleteFile(path)` | File mở ReadWrite, ghi đè random toàn bộ size rồi `remove`; file không còn | Unit | P1 |
| VLT-607 | secure-delete file không tồn tại | path không tồn tại | `secureDeleteFile` | Không crash (return sớm khi `!f.exists()`) | Unit | P2 |
| VLT-608 ⚠️ | Auto-lock → temp sạch | Unlocked, có temp, auto-lock kích hoạt | Chờ auto-lock | Tự `lock()` → temp bị sweep; không bản giải mã sót | Integration | P1 |

---

## NHÓM 8 — DLG-BATCH

Tiền điều kiện chung: VM unlocked + uploader wired.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-701 | addFolder đệ quy | Thư mục có file ở nhiều cấp con | `addFolder(dir)` | Mọi file (Subdirectories) được encrypt; mỗi → `<basename>.fshenc` trong vault | Integration | P0 |
| VLT-702 | batchProgress | Batch ≥3 file | `addFilesUpload(...)` | `batchProgress(done,total,name)` emit tăng dần tới total; thứ tự done=1..N | Integration | P1 |
| VLT-703 | fileOpFinished tổng | Batch 5 file, 1 lỗi | Chạy batch (1 file input không tồn tại) | `fileOpFinished("encrypt", false, "4/5", firstErr)`; 4 file ok hợp lệ | Integration | P1 |
| VLT-704 ⚠️ | retryFailed | Batch xong còn `lastFailed_` | `retryFailed()` | Chỉ chạy lại file lỗi; nếu nguồn đã có lại → ok; `lastFailed_` clear trước khi chạy | Integration | P1 |
| VLT-705 ⚠️ | cancelBatch giữa chừng | Batch nhiều file đang chạy | `cancelBatch()` | Dừng giữa các file (atomic flag); file còn lại vào `failedInputs` (retry được); file đã xong hợp lệ | Integration | P0 |
| VLT-706 ⚠️ | Trùng tên basename (flat) | 2 file cùng tên `a.txt` ở 2 thư mục con | `addFolder` | **GAP nghi ngờ:** cả hai → `a.txt.fshenc` cùng vault → file thứ 2 GHI ĐÈ file thứ 1 (flat theo basename, không cảnh báo). Kiểm có mất dữ liệu im lặng không | Integration | P0 |
| VLT-707 | Batch khi locked giữa lô | Auto-lock kích hoạt giữa batch | — | Worker dùng `snapshotKey` (bản sao master key độc lập) → batch đang chạy vẫn hoàn tất dù vault auto-lock | Integration | P1 |
| VLT-708 | Empty folder | Thư mục rỗng | `addFolder(dir)` | Không làm gì (files rỗng → return); không emit lỗi | Integration | P2 |
| VLT-709 | Reentrancy guard | Batch đang chạy (`fileBusy_`) | Gọi `addFiles` lần 2 | Bị chặn (`beginEncrypt` return khi `fileBusy_`); không chạy chồng | Integration | P1 |
| VLT-710 | cancel rồi retry | Cancel batch → còn failed | `cancelBatch()` → `retryFailed()` | Các file bị hủy chạy lại được; flag cancel reset (`store(false)` đầu runEncryptBatch) | Integration | P1 |

---

## NHÓM 9 — Auto-upload

Tiền điều kiện chung: VM unlocked; `uploader_` = lambda ghi nhận (files, folder).

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-801 | Upload sau encrypt | autoUpload=true, folder="/" | `addFilesUpload([f], true, "/")` | Sau encrypt OK → `uploader_({out}, "/")` được gọi; emit `fileUploadQueued(name)` | Integration | P0 |
| VLT-802 | Không upload | autoUpload=false | `addFilesUpload([f], false, "")` | `uploader_` KHÔNG gọi; chỉ encrypt | Integration | P1 |
| VLT-803 | autoUploadDefault | `setAutoUploadDefault(true)`, folder cấu hình | `addFiles([f])` (quick-add) | Dùng default → upload tới `uploadFolder()` | Integration | P1 |
| VLT-804 | uploadFolder mặc định | `uploadFolder` chưa set | đọc `uploadFolder()` | Trả `"/"` (root) mặc định; `addFilesUpload(...,"")` chuẩn hóa về `"/"` | Unit | P1 |
| VLT-805 | Upload chỉ file ok | Batch 1 ok + 1 lỗi, autoUpload | Chạy | Chỉ file ok (`okOutputs`) được upload; file lỗi không | Integration | P1 |
| VLT-806 | Uploader chưa wire | `setServices(settings, nullptr)` hoặc uploader rỗng | `addFilesUpload(...,true,...)` | Không crash (check `if (autoUpload && uploader_)`) | Integration | P2 |

---

## NHÓM 10 — Edge / bảo mật (filename, path-traversal)

Tiền điều kiện chung: tạo `.fshenc` thủ công với filename header tùy ý, hoặc encrypt với `storedFilename` đặc biệt; sau đó `decryptAndOpen`/`readHeader`/`refreshFiles`.

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-901 ⚠️🔴 | Path traversal filename | `.fshenc` có header `filename="..\\..\\..\\Users\\x\\evil.exe"` | `decryptAndOpen(fshenc)` | **GAP nghi ngờ cao:** `decryptAndOpen` dựng `out = tmpDir + "/" + name` từ `h.filename` KHÔNG sanitize → có thể ghi NGOÀI tmpDir (path traversal). Kỳ vọng ĐÚNG: phải sanitize basename / từ chối. Hiện tại có thể GHI ĐÈ file tùy ý → **lỗi bảo mật** | Integration | P0 |
| VLT-902 ⚠️🔴 | Absolute path filename | header `filename="C:\\Windows\\System32\\drivers\\etc\\hosts"` | `decryptAndOpen` | `tmpDir + "/" + name` với name tuyệt đối → trên Windows có thể giải thành đường dẫn lạ. Kiểm có sanitize không (kỳ vọng: chỉ lấy basename) | Integration | P0 |
| VLT-903 ⚠️ | Filename rỗng | `.fshenc` với `FILENAME_LEN=0` | `refreshFiles`/`decryptAndOpen` | Fallback `completeBaseName()` của file `.fshenc`; không crash; tên hiển thị hợp lý | Integration | P1 |
| VLT-904 | Filename rất dài | filename 4096 B (max) | `readHeader` → hiển thị | Parse Ok; UI cắt hiển thị; decrypt-to-temp với tên dài — kiểm OS path-length (>260 trên Windows) không làm crash | Integration | P2 |
| VLT-905 | Filename unicode VI | filename "báo cáo tài chính.docx" (UTF-8) | encrypt → readHeader → decryptAndOpen | Tên round-trip đúng UTF-8; mở file đúng tên tiếng Việt | Integration | P1 |
| VLT-906 | File không đuôi | encrypt file "README" (no ext) | round-trip | `completeBaseName`/header xử lý OK; decrypt mở được | Integration | P2 |
| VLT-907 | Ký tự đặc biệt filename | filename chứa `: * ? " < > |` (hợp lệ trong header nhưng cấm trên Windows FS) | `decryptAndOpen` | **GAP nghi ngờ:** tạo temp `tmpDir/<name>` với ký tự cấm → write fail → `IoError`, fail-closed xóa; KHÔNG crash. Kiểm có sanitize tên file temp không | Integration | P1 |
| VLT-908 | Filename có newline/null | filename chứa `\n` hoặc byte 0x00 giữa chuỗi | readHeader | Chuỗi giữ nguyên bytes (assign theo len); hiển thị an toàn (không cắt tại NUL gây lệch) | Unit | P2 |
| VLT-909 ⚠️ | Plain-folder filename traversal | (liên quan §6) file nguồn tên bình thường | `copyToPlainFolder` | Dùng `fi.fileName()` (đã là basename) → an toàn; xác nhận không ghi ngoài plainDir | Integration | P2 |

---

## NHÓM 11 — i18n

| ID | Nhóm | Tiền điều kiện | Bước | Kết quả kỳ vọng | Loại | Ưu tiên |
|---|---|---|---|---|---|---|
| VLT-1001 | tr() C++ | — | Grep chuỗi hiển thị trong `VaultViewModel.cpp` (vd "Khóa khôi phục", "Vault") | Mọi chuỗi hiển thị bọc `tr()`; không hardcode trần | Unit/Manual | P1 |
| VLT-1002 | qsTr QML | — | Grep `qml/Fshare/Pages/Vault*.qml` + Components mới | Mọi label/nút/toast/lỗi bọc `qsTr()` | Manual | P1 |
| VLT-1003 ⚠️ | EN không unfinished | `src/i18n/fshare_en.ts` | Mở .ts, lọc `type="unfinished"` trên chuỗi Vault | 0 entry unfinished cho chuỗi Vault; (progress: 0 unfinished/1060) | Manual | P1 |
| VLT-1004 | Runtime switch | App chạy | Đổi ngôn ngữ → vào trang Vault | Toàn bộ UI Vault dịch EN; không còn chuỗi VI sót | Manual | P2 |
| VLT-1005 | lupdate sạch sau sửa | Sau thêm chuỗi mới | `update_translations` → lrelease | Không chuỗi mới bị bỏ; build chèn .qm; verify runtime | Manual | P2 |

---

## KHU VỰC RỦI RO CAO CẦN TEST TRƯỚC (theo phân tích code)

Xếp theo mức nghi ngờ ra bug, **làm trước khi release**:

1. 🔴 **Path traversal qua filename trong header** — `VLT-901`, `VLT-902`, `VLT-907`.
   `VaultViewModel::decryptAndOpen` dựng `out = tmpDir + "/" + name` với `name` lấy thẳng từ `h.filename` (header `.fshenc` — **untrusted nếu file đến từ cloud/người khác**), **không thấy sanitize basename**. Một `.fshenc` thù địch với `filename="..\\..\\evil"` hoặc đường dẫn tuyệt đối có thể khiến app ghi bản giải mã ra **ngoài** thư mục temp → ghi đè file tùy ý. Đây là lỗ hổng cao nhất; cần xác minh + (nếu thiếu) thêm sanitize `QFileInfo(name).fileName()`.

2. 🔴 **Temp giải mã sót sau crash thật** — `VLT-317`.
   `temps_` chỉ sống trong RAM của VM. Nếu app crash (không qua destructor/lock), bản giải mã trong `%TEMP%/FsNextVault/` **không được quét lại** ở lần khởi động sau (không thấy sweep-startup theo thư mục). Nguy cơ rò rỉ plaintext nhạy cảm.

3. 🟠 **Fuzz parser `.fshenc`** — `VLT-121`, `VLT-122`.
   Parser nhận input untrusted (file từ cloud). Cần fuzz dưới ASan để chắc không over-read/OOM ở các nhánh `FILENAME_LEN`, `originalSize` (AES cap), `chunkSize`, `clen` per-chunk. Code đã có cap phòng thủ — phải kiểm bằng fuzz thực tế.

4. 🟠 **Ghi đè im lặng do flat-basename trong batch** — `VLT-706`.
   `addFolder` đặt mỗi file thành `<basename>.fshenc` phẳng trong vault. Hai file cùng tên ở thư mục con khác nhau → file thứ hai ghi đè file đầu, **mất dữ liệu không cảnh báo**. (progress.md xác nhận "flat theo basename — có thể trùng tên".)

5. 🟠 **Fail-closed toàn diện** — `VLT-301`..`VLT-305`.
   Xác minh encrypt/decrypt KHÔNG để lại file dở ở mọi nhánh lỗi (tamper/wrong-key/truncated/IoError/đĩa đầy). Code có `fs::remove` ở nhánh lỗi của `encryptFile`/`decryptFile` — cần test cả đường đĩa-đầy/quyền-ghi.

6. 🟠 **Meta / blob hỏng & trạng thái lệch** — `VLT-306`..`VLT-313`.
   `vault-meta.json` cắt cụt (kill khi ghi), base64 sai độ dài, blob DPAPI/plain hỏng, có-meta-thiếu-blob và ngược lại. Phải load graceful + `tryAutoUnlock` trả false, không crash. Kiểm tính atomic của `VaultMetadata::save` (tmp→rename, có nhánh remove+retry trên Windows).

7. 🟠 **ChangePassphrase không phá vault khi sai pass cũ** — `VLT-407`..`VLT-409`.
   Sai pass cũ phải để meta nguyên vẹn (vault vẫn mở bằng pass cũ); đổi xong file `.fshenc` cũ vẫn mở được (chỉ re-wrap Master Key).

8. 🟡 **Kill giữa batch / decrypt-to-temp** — `VLT-314`, `VLT-315`, `VLT-319`.
   File đã xong hợp lệ; file dở không vào vault; `lock()` khi `busy_` return sớm tránh race với worker.

9. 🟡 **Boundary 1 MiB / 1 GiB chọn nhánh** — `VLT-007`, `VLT-008`, `VLT-009`.
   Sai off-by-one ở ngưỡng chọn AES↔XChaCha hoặc chunk 256K↔1M.

---

## TỔNG KẾT SỐ CASE THEO NHÓM

| Nhóm | Tên | Số case | P0 | P1 | P2 |
|---|---|---|---|---|---|
| 1 | Crypto round-trip | 15 | 7 | 6 | 2 |
| 2 | Validate / parser untrusted | 23 | 17 | 5 | 1 |
| 3 | Tamper / toàn vẹn | 18 | 12 | 5 | 1 |
| 4 | Fail-closed / crash-safety / kill-app | 20 | 14 | 5 | 1 |
| 5 | VaultManager lifecycle | 25 | 14 | 6 | 5 |
| 6 | Size-cap / DLG-LARGEFILE | 13 | 6 | 4 | 4 (gồm 1 cận P2) |
| 7 | Secure-delete | 8 | 2 | 4 | 2 |
| 8 | DLG-BATCH | 10 | 3 | 6 | 1 |
| 9 | Auto-upload | 6 | 1 | 4 | 1 |
| 10 | Edge / bảo mật filename | 9 | 2 | 3 | 4 |
| 11 | i18n | 5 | 0 | 3 | 2 |
| **Tổng** | | **152** | **78** | **51** | **23** |

Số case gắn cờ ⚠️ (rủi ro dễ ra bug): **41**. Trong đó 🔴 (lỗ hổng bảo mật nghi ngờ cao): **3** (VLT-901, VLT-902, VLT-317).
</content>
</invoke>
