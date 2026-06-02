# SPEC KỸ THUẬT — Encryption Engine + Key Management (FsNext)

| Trường | Giá trị |
|---|---|
| Module | Encryption Engine + Key Management (Vault) — **độc lập server** |
| Nền tảng | Windows x86_64, C++17, Qt6, vcpkg |
| Crypto lib | **libsodium** (thêm mới) |
| Đuôi file mã hóa | **`.fshenc`** (magic nội bộ: `FENC`) |
| Tài liệu UI kèm theo | `docs/specs/encryption-ui-brief.md` |
| Trạng thái | Draft để code & review |

> Tài liệu này là spec để **code**. Tài liệu UI/UX riêng ở `encryption-ui-brief.md`. Hai tài liệu phải đồng bộ.

---

## 1. NGUYÊN TẮC
1. **Crypto core thuần** (`src/core/crypto/`): chỉ libsodium + STL, KHÔNG Qt → unit-test & CLI dùng lại được.
2. **Tách lớp**: Engine không biết server. Upload `.fshenc` tái dùng transfer engine sẵn có.
3. **Crash-safe & streaming**: file lớn xử lý theo chunk, RAM bị chặn (< ~vài MB) bất kể file 10GB+.
4. **Mỗi file tự mô tả**: `.fshenc` ghi `ALGO_ID`, KDF params, nonce → đổi thuật toán/đổi máy không hỏng file cũ.
5. **Fail-closed crypto**: sai khóa / file bị can thiệp → báo lỗi, **không** xuất output một phần.

---

## 2. THUẬT TOÁN MÃ HÓA — QUYẾT ĐỊNH

### 2.1 Lựa chọn & lý do
**Phương án LAI đã chốt:** AES cho file nhỏ (single-shot), XChaCha cho streaming.

| Vai trò | Thuật toán | Library | Áp dụng |
|---|---|---|---|
| **File nhỏ (≤ 1 MiB)** | **AES-256-GCM single-shot** | `crypto_aead_aes256gcm_*` | 1 lần gọi AEAD, nạp trọn vào RAM. Nhanh nhất nhờ AES-NI. Chỉ dùng khi `crypto_aead_aes256gcm_is_available()==1`. |
| **File lớn (> 1 MiB) / streaming** | **XChaCha20-Poly1305 secretstream** | `crypto_secretstream_*` | API streaming đã kiểm chứng của libsodium — KHÔNG tự dựng khung. RAM chặn ~1 chunk. |
| **Máy không có AES-NI** | XChaCha20-Poly1305 | `crypto_secretstream_*` | Dùng cho MỌI kích thước (kể cả file nhỏ) khi `aes256gcm_is_available()==0`. |
| KDF (passphrase→KEK) | Argon2id | `crypto_pwhash` | 3 mức cost (xem §6). |
| Wrap DEK | XChaCha20-Poly1305 secretbox | `crypto_secretbox_easy` | Wrap DEK bằng KEK/Master Key. |
| Random | CSPRNG | `randombytes_buf` | Qua `BCryptGenRandom` trên Windows. |
| Hash (tùy chọn) | BLAKE3 | BLAKE3 lib | Chỉ nếu cần checksum plaintext; mặc định bỏ (AEAD đã đủ). |
| Wipe RAM | `sodium_memzero`, `sodium_malloc` | libsodium | `SecureBytes`. |

`ALGO_ID` trong header `.fshenc`:
- `0x01` = XChaCha20-Poly1305 secretstream (streaming) — file lớn & mọi file trên máy không AES-NI
- `0x02` = AES-256-GCM single-shot — file nhỏ (≤ 1 MiB) trên máy có AES-NI

**Quy tắc chọn (encrypt):**
```
if (size <= 1 MiB && aes256gcm_is_available())   → ALGO 0x02 (AES single-shot)
else                                             → ALGO 0x01 (XChaCha secretstream)
```
Decrypt: đọc `ALGO_ID` từ header, giải mã đúng thuật toán đã ghi (file tự mô tả).

### 2.2 Khởi tạo & giới hạn khả chuyển (ĐÃ CHẤP NHẬN)
- Gọi `sodium_init()` một lần lúc khởi động (trong `AppContext::init`).
- Lưu cờ `aesAvailable = crypto_aead_aes256gcm_is_available()` (expose qua `VaultViewModel.algoAvailable`).
- **Giới hạn đã chấp nhận:** file ALGO `0x02` (AES) tạo trên máy có AES-NI **có thể KHÔNG giải mã được** trên máy KHÔNG có AES-NI (libsodium yêu cầu AES-NI cho `crypto_aead_aes256gcm`). Vì AES chỉ dùng cho **file nhỏ ≤ 1 MiB**, ảnh hưởng giới hạn ở nhóm file này; **toàn bộ file lớn (XChaCha) giải mã được trên mọi máy**.
- Khi gặp file `0x02` mà `aesAvailable==0` → báo lỗi rõ ràng (FSE-102 biến thể): "File này cần CPU hỗ trợ AES để mở. Hãy mở trên máy đã tạo nó." — KHÔNG crash, KHÔNG xuất partial.
- Máy không AES-NI: encrypt LUÔN dùng XChaCha (`0x01`) cho mọi kích thước → file tạo ra khả chuyển 100%.

### 2.3 AES-256-GCM single-shot (file nhỏ ≤ 1 MiB)
Đơn giản, không tự dựng khung chunk:
- **DEK**: 32 byte random / file (`randombytes_buf`).
- **Nonce 12 byte** random / file (`randombytes_buf`), lưu trong header (trường `AEAD_NONCE`, xem §7). Vì DEK duy nhất/ file → 1 lần dùng (key, nonce) duy nhất, không reuse.
- **Mã hóa 1 lần**: `crypto_aead_aes256gcm_encrypt(plaintext_toàn_file, AAD=header, nonce, DEK)` → ciphertext + 16-byte tag.
- **Giải mã 1 lần**: `crypto_aead_aes256gcm_decrypt(...)`; tag fail → lỗi, không xuất output.
- RAM ≈ 2× kích thước file (≤ ~2 MB) — chấp nhận được cho file nhỏ. Vượt ngưỡng 1 MiB → chuyển §2.4.
- **KHÔNG cần crypto review riêng** (dùng API one-shot chuẩn của libsodium).

### 2.4 XChaCha20-Poly1305 secretstream (file lớn > 1 MiB & streaming) — MÔ TẢ CHI TIẾT
Dùng nguyên `crypto_secretstream_xchacha20poly1305_*` của libsodium — **construction đã được kiểm chứng**, KHÔNG tự dựng. Cơ chế:

**Khởi tạo (encrypt):**
- `DEK` 32 byte random / file.
- `crypto_secretstream_..._init_push(&state, header, DEK)` → sinh **`STREAM_HEADER` 24 byte** (lưu vào header `.fshenc`). State giữ nonce nội bộ + khóa con, libsodium tự tăng nonce và tự rekey giữa các chunk — **không bao giờ reuse nonce**, lập trình viên không phải quản lý nonce.

**Vòng lặp chunk (encrypt):**
```
đọc CHUNK byte plaintext (256KB hoặc 1MB — xem §3)
tag = (chunk cuối) ? TAG_FINAL : TAG_MESSAGE
crypto_secretstream_..._push(&state, out, &outlen, in, inlen, AAD, AAD_len, tag)
ghi [outlen:4][ciphertext(inlen + 17 byte)]    // 17 = 16 Poly1305 tag + 1 tag byte
```
- AAD của chunk ĐẦU = toàn bộ header `.fshenc` (chống tráo header). Các chunk sau AAD rỗng (đã được chuỗi hóa nội bộ).
- Mỗi chunk được xác thực độc lập; thứ tự chunk bị ràng buộc bởi state (đảo/chèn chunk → fail).

**Giải mã (decrypt):**
- `init_pull(&state, STREAM_HEADER, DEK)`; nếu header hỏng → lỗi.
- Lặp `pull(...)`: trả về plaintext + `tag`. `pull` fail → **IntegrityError**, dừng ngay, không xuất phần còn lại.
- Kết thúc hợp lệ **chỉ khi** gặp chunk có `TAG_FINAL`. Stream hết mà chưa thấy `TAG_FINAL` → **lỗi cắt cụt (truncation)**.

**Lợi ích:** nonce 192-bit + auto-rekey → an toàn tuyệt đối với file khổng lồ; RAM chặn ~1 chunk; chống cắt cụt/đảo chunk/tráo header **sẵn trong API**, không cần code crypto thủ công.

> Ghi chú: file nhỏ trên máy KHÔNG có AES-NI cũng đi đường này (secretstream xử lý tốt 1 chunk duy nhất với TAG_FINAL).

---

## 3. CHIẾN LƯỢC THEO DUNG LƯỢNG FILE

Không đổi *thuật toán* theo size; đổi **chế độ** và **kích thước chunk** (đây mới là yếu tố ảnh hưởng tốc độ/RAM/overhead).

| Dung lượng file | Thuật toán (máy có AES-NI) | Chế độ | Chunk | Lý do |
|---|---|---|---|---|
| ≤ **1 MiB** | **AES-256-GCM** (0x02) | Single-shot in-memory | — | Nhanh nhất, overhead 1 tag, vừa RAM. |
| **1 MiB – 1 GiB** | XChaCha20 (0x01) | secretstream | **256 KiB** | Cân bằng overhead/RAM, độ mịn resume hợp lý. |
| **> 1 GiB** | XChaCha20 (0x01) | secretstream | **1 MiB** | Ít tag + ít syscall → throughput cao; RAM chặn ~1 chunk. |
| **> 100 GiB** (edge) | XChaCha20 (0x01) | secretstream | 1–4 MiB | Như trên; secretstream tự rekey, an toàn. |

**Ngưỡng "file nhỏ" = 1 MiB (1.048.576 byte)** — cấu hình được ở Settings nâng cao. Trên máy KHÔNG có AES-NI: mọi kích thước đều dùng XChaCha20 (file ≤1 MiB = secretstream 1 chunk).

**Overhead tag** (16 byte/chunk): chunk 64KB → 0.024%; 256KB → 0.006%; 1MB → 0.0015%. → Chunk lớn giảm overhead & tăng tốc, đánh đổi RAM + độ mịn resume. Ngưỡng trên là **mặc định**, cho phép override trong Settings nâng cao (256KB–4MB).

**Lưu ý GCM limit**: mỗi chunk là một lần gọi GCM với nonce riêng (≤ chunk size), nằm xa giới hạn an toàn của GCM. Tổng số chunk/ file giới hạn bởi counter 64-bit → an toàn tuyệt đối.

**Đo & nghiệm thu hiệu năng** (theo NFR spec gốc §E): encrypt 1GB < 8s, 10GB RAM < 100MB, throughput AES > 500 MB/s khi có AES-NI.

---

### 3.1 Chính sách theo dung lượng phía NGƯỜI DÙNG (khác với chế độ crypto §3)
> Phân biệt: §3 là chế độ crypto nội bộ (tự động, ẩn). Phần này là *chính sách hành vi* hiển thị cho người dùng theo mốc quen thuộc.

| Mốc | Hành vi mặc định | Crypto (nội bộ) |
|---|---|---|
| **< 100 MB** | Mã hóa ngay, im lặng (~tức thì→vài giây), không cảnh báo | AES single-shot (≤1MiB) / XChaCha stream |
| **100 MB – 1 GB** | Mã hóa + progress nổi bật + ước tính thời gian & dung lượng đĩa cần thêm; không chặn | XChaCha stream |
| **> 1 GB** | **Hỏi xác nhận** (`DLG-LARGEFILE`): ước tính thời gian + cần thêm ~bằng dung lượng file (cho `.fshenc`, và lần nữa khi giải mã để xem) + lưu ý "phải giải mã toàn bộ mới mở được — không xem trực tiếp". Chọn: Mã hóa / Thêm không mã hóa / Hủy | XChaCha stream |

**Setting `vault_max_encrypt_size`** (Settings → Vault):
- "Tự động mã hóa file đến:" `[Không giới hạn (mặc định) | 1 GB | 5 GB | Tùy chỉnh]`.
- "Khi file vượt ngưỡng:" `[Hỏi mỗi lần (mặc định) | Bỏ qua, không thêm vào Vault | Thêm vào thư mục thường — KHÔNG mã hóa]`.

**⚠ QUY TẮC AN TOÀN BẮT BUỘC:** file KHÔNG mã hóa **không bao giờ** nằm trong Vault dưới dạng "đã bảo vệ". Nếu người dùng chọn không mã hóa → file vào thư mục thường (ngoài Vault) hoặc bị bỏ qua, có nhãn rõ. Tránh cảm giác an toàn giả.

**Cân nhắc kỹ thuật (vì sao bỏ qua file lớn là chấp nhận được):** nút cổ chai là I/O đĩa (không phải crypto); cần gấp đôi đĩa tạm; xem file lớn phải giải mã toàn bộ ra temp trước (không stream-view) rồi secure-delete; media lớn thường ít nhạy cảm.

**Khuyến nghị:** mặc định **mã hóa mọi kích thước** (giữ cam kết E2EE), chỉ **hỏi khi > 1 GB**. Cho phép bật cap trong Settings cho ai muốn bỏ qua file lớn. KHÔNG đặt mặc định "bỏ qua file lớn".

---

## 4. CHẾ ĐỘ CHỌN ĐỐI TƯỢNG (selection modes)
Engine nhận **một danh sách file** (`std::vector<Path>`); mọi entry point quy về danh sách này:
1. **1 file** — chọn 1 file qua dialog OS, hoặc kéo-thả 1 file.
2. **Nhiều file** — multi-select trong dialog, hoặc kéo-thả nhiều file.
3. **1 thư mục** — duyệt đệ quy, giữ cấu trúc thư mục con (tùy chọn), áp filter ignore.

Pipeline xử lý chung (cho cả 3): resolve danh sách → (tùy chọn) lọc ignore → encrypt/decrypt từng file (song song có giới hạn slot) → (tùy chọn) upload Fshare → tổng kết thành công/lỗi. Lỗi 1 file **không** dừng cả lô; cuối lô cho "Thử lại file lỗi".

---

## 5. CRYPTO CORE — FILE & LỚP (`src/core/crypto/`)

| File | Trách nhiệm |
|---|---|
| `SecureBytes.h/.cpp` | Buffer key `sodium_malloc`/`sodium_free` (mlock + guard page), zero khi hủy; cấm copy, cho move. |
| `KdfParams.h` | struct {algo, salt[16], ops, mem_kb, parallel}; 3 preset cost. |
| `Key.h/.cpp` | `fromBytes(32)`, `fromPassphrase(pass, KdfParams)` (Argon2id). Bọc `SecureBytes`. Fingerprint (BLAKE3 của khóa, rút gọn) để hiển thị/đối chiếu. |
| `FencFormat.h/.cpp` | Hằng số (MAGIC, VERSION, ALGO_ID, CHUNK presets), struct `FencHeader`, serialize/parse header LE, validate. |
| `AeadCipher.h/.cpp` | Trừu tượng AEAD: `Aes256GcmCipher` (chunked framing §2.3) + `XChaChaStreamCipher` (secretstream). Engine chọn theo `ALGO_ID`. |
| `EncryptionEngine.h/.cpp` | `encryptFile/decryptFile`, `encryptBuffer/decryptBuffer`, `readHeader`, `isFencFile`. Tự chọn single-shot vs chunked theo §3. Báo lỗi qua enum. |
| `CryptoError.h` | enum khớp mã FSE-100..104. |

---

## 6. KEY MANAGEMENT (`src/core/crypto/`)

### 6.1 Phân cấp khóa (mô hình hợp nhất)
```
passphrase ──Argon2id(salt,ops,mem,par)──► KEK(32B)
KEK ──secretbox wrap/unwrap──► Master Key(32B, SecureBytes, mlock)   [vault-mode]
Master Key ──wrap──► DEK(32B random / file) ──AEAD──► ciphertext
(standalone-mode: KEK wrap thẳng DEK; KDF nằm trong header .fshenc)
```
Đổi passphrase = re-wrap **1** Master Key trong `vault-meta.json` (không đụng file `.fshenc`).

### 6.2 Argon2id — 3 preset cost (lộ ở UI "độ mạnh khóa")
| Preset | ops | mem | Mô tả UI |
|---|---|---|---|
| Cân bằng (mặc định) | 3 | 64 MB | "Mở khóa nhanh, đủ an toàn" |
| Mạnh | 4 | 256 MB | "An toàn hơn, mở khóa chậm hơn chút" |
| Tối đa | 4 | 1 GB | "Bảo mật cao nhất, chậm nhất, tốn RAM" |

### 6.3 File & lớp
| File | Trách nhiệm |
|---|---|
| `VaultManager.h/.cpp` | create/unlock/lock/changePassphrase/exportKeyfile/importKeyfile; giữ Master Key (SecureBytes); signal `lockStateChanged`; auto-lock timer. |
| `VaultMetadata.h/.cpp` | đọc/ghi `vault-meta.json` (jsoncpp), ghi atomic (write-temp→rename). |
| `KeyStore.h/.cpp` | lưu Master Key theo 3 cấp (L1 RAM / **L2 DPAPI = mặc định** / L3 plain). L2 tied to user+máy → blob mất khi cài lại Windows; passphrase vẫn là recovery cuối cùng. |
| `KeyRegistry.h/.cpp` | **danh sách khóa đã dùng** (cho trang Quản lý khóa): nhãn, fingerprint, loại, ngày tạo/dùng cuối, trạng thái sao lưu. **Lưu mã hóa**; reveal/copy khóa thật yêu cầu vault unlocked. |

### 6.4 Mở rộng `SecureStore` (sẵn có)
Hiện hardcode mô tả `"FsNext OAuth refresh token"` ([SecureStore.cpp:19](../../src/core/util/SecureStore.cpp)). Thêm overload nhận description/entropy riêng để blob master-key và blob token tách biệt. Tương thích ngược.

---

## 7. FORMAT `.fshenc` (đã sửa lỗi spec gốc)
```
Off   Size  Field
0x00  4     MAGIC "FENC"
0x04  1     VERSION 0x01
0x05  1     ALGO_ID  0x01=XChaCha20 secretstream | 0x02=AES-256-GCM single-shot
0x06  2     FLAGS    bit0 filename-enc · bit1 vault-mode · bit2 single-shot · bit3 standalone
0x08  16    KDF_SALT          (standalone-mode; vault-mode = 0)
0x18  4     KDF_OPS
0x1C  4     KDF_MEM_KB
0x20  1     KDF_PARALLEL
0x21  3     RESERVED
0x24  24    WRAP_NONCE        (secretbox nonce dùng để wrap DEK; cả 2 chế độ)
0x3C  48    WRAPPED_DEK       (DEK32 + secretbox MAC16)   ← 48B
0x6C  24    BODY_HEADER       (XChaCha: secretstream header 24B; AES: 12B GCM nonce + 12B zero)
0x84  8     ORIGINAL_SIZE     uint64 LE
0x8C  4     CHUNK_SIZE        uint32 LE (chunk plaintext; 0 nếu single-shot)
0x90  2     FILENAME_LEN
0x92  N     FILENAME (UTF-8, mã hóa nếu FLAGS bit0)
─────  vùng 0x00..0x92+N được bind làm AAD (qua BLAKE3) ─────
…     var   BODY:
            • AES single-shot (0x02): [ciphertext(ORIGINAL_SIZE) + 16 tag]
            • XChaCha secretstream (0x01): lặp [len:4][ct(CHUNK + 17)], chunk cuối TAG_FINAL
```
(Bỏ `FILE_HASH_PLAINTEXT` ở EOF — AEAD + final-marker đã chống giả mạo/cắt cụt.)

---

## 8. TÍCH HỢP FsNext
- `VaultManager` + `KeyRegistry` khởi tạo trong `AppContext::init()` (sau SettingsService), `unique_ptr`.
- `VaultViewModel` (mới) — Q_PROPERTY `isUnlocked/lockState/algoAvailable`; Q_INVOKABLE `createVault/unlock/lock/encrypt(list)/decryptToTemp/changePassphrase/batchEncryptFolder`.
- `KeyManagerViewModel` (mới) — bind danh sách khóa, copy fingerprint, reveal/copy key (confirm + auto-clear clipboard 30s), import/export.
- Thao tác crypto chạy off-main (`QtConcurrent::run` + `QPointer` guard + `if(!guard) return;`), marshal kết quả về main.
- Upload `.fshenc` tái dùng `UploadViewModel`/transfer engine (toggle "tự tải lên").
- Tray: mục Khóa/Mở vault + badge khóa.
- Mọi chuỗi `qsTr()`/`tr()` (skill `fsnext-i18n`).

---

## 9. CLI (tùy chọn) — `fshare-encrypt.exe` / `fshare-decrypt.exe`
Target CMake riêng, link crypto core + libsodium + CLI11 (KHÔNG Qt). Dùng verify format bằng hex-dump. Hỗ trợ chọn 1 file / nhiều file / `--recursive` thư mục; `--algo aes|xchacha`.

---

## 10. BUILD
- `vcpkg install libsodium` (triplet `x64-windows-static-md` + `x64-mingw-dynamic`).
- CMake: `find_package(unofficial-sodium CONFIG REQUIRED)` → link `unofficial-sodium::sodium`.
- Thêm nhóm `src/core/crypto/*` vào `FSNEXT_SOURCES`/`FSNEXT_HEADERS` ([CMakeLists.txt:81-252](../../CMakeLists.txt)).
- Giữ C++17, QtTest, jsoncpp, Qt-logging. Chỉ thêm libsodium (+CLI11 nếu làm CLI).

---

## 11. TEST PLAN (QtTest)
- Round-trip encrypt→decrypt identical: 1000 file random size, **cả AES và XChaCha**.
- Chọn nhánh đúng theo §3: ≤1 MiB → AES single-shot (0x02); >1 MiB → XChaCha secretstream (0x01).
- Sai khóa → lỗi, không partial output.
- Tamper 1 byte header / ciphertext / đảo chunk / cắt cụt chunk cuối → IntegrityError.
- Re-encrypt cùng nội dung cùng khóa → ciphertext khác (random DEK/nonce).
- 10GB stream → RAM < 100MB.
- Malformed `.fshenc` → graceful error (tiền đề **fuzzing** parser — untrusted input).
- Fallback: giả lập `aes256gcm_is_available()==0` → engine dùng XChaCha cho mọi kích thước, vẫn round-trip.
- File AES (0x02) + giả lập máy không AES-NI → báo lỗi rõ ràng, KHÔNG crash, KHÔNG partial output.
- Vault: create/unlock/wrong-pass/changePass/auto-lock/import-export/recovery.
- KeyRegistry: thêm/xóa/đối chiếu fingerprint; copy key auto-clear clipboard.
- Target mới: `test_encryption_engine`, `test_aead_cipher`, `test_vault_manager`, `test_key_registry` vào [tests/CMakeLists.txt](../../tests/CMakeLists.txt).

---

## 12. LỘ TRÌNH (phased)
| Phase | Nội dung |
|---|---|
| E0 | libsodium vào build; `sodium_init`; `SecureBytes`; khung test |
| E1 | `FencFormat` + `AeadCipher` (AES chunked + XChaCha) + `EncryptionEngine` + chiến lược theo size (§3) |
| E2 | CLI + hex-dump verify (nếu chốt làm) |
| E3 | `VaultManager`/`VaultMetadata`/`KeyStore`/`KeyRegistry` + auto-lock |
| E4 | `VaultViewModel`/`KeyManagerViewModel` + wiring AppContext + tray |
| E5 | UI: intro, wizard, unlock, vault page, key manager, batch, settings + i18n |
| E6 | Tích hợp upload/download `.fshenc` + batch tự tải lên |

---

## 13. QUYẾT ĐỊNH
1. ✅ **CHỐT:** AES-256-GCM single-shot cho file ≤ **1 MiB**; XChaCha20-Poly1305 secretstream cho phần còn lại. → **Không** phải tự dựng khung AES → bỏ rủi ro crypto-review nặng.
2. ✅ **CHẤP NHẬN:** file AES (0x02) có thể không mở được trên máy không AES-NI; báo lỗi rõ ràng, không crash. File lớn (XChaCha) luôn khả chuyển.
3. **Crypto review (nhẹ):** vẫn nên review *cách dùng* API (wrap DEK, KDF, key handling, parse header untrusted) — KHÔNG còn khung crypto tự dựng cần audit nặng.
4. ✅ **CHỐT — Mô hình khóa:** mô hình **hợp nhất qua Master Key** (§6.1). Vault dùng Master Key trung gian; standalone/CLI wrap thẳng passphrase. 1 format, phân biệt bằng FLAGS.
5. ✅ **CHỐT — Cấp lưu khóa mặc định = L2 (DPAPI):** tự mở theo phiên Windows, không hỏi passphrase mỗi lần; vẫn auto-lock khi đóng app + timeout. Wizard vẫn cho chọn L1/L3.
   - ⚠ **HỆ QUẢ phải truyền đạt:** DPAPI tied to **user+máy**. Nếu cài lại Windows / đổi máy / hỏng profile → blob DPAPI mất → **vẫn cần passphrase để mở lại**. Vì vậy **passphrase backup vẫn bắt buộc** kể cả ở L2 — L2 chỉ là tiện lợi hằng ngày, KHÔNG thay thế việc nhớ/sao lưu passphrase. Màn cảnh báo & intro phải nêu rõ.
6. ✅ **CHỐT — Số vault:** ship **1 vault trước**, nhưng schema/`vault-meta.json` (`vault_id`) + `KeyRegistry` thiết kế hỗ trợ **N vault** ngay → thêm multi-vault sau KHÔNG cần migrate.
