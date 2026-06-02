# Vault / E2EE — BÁO CÁO BUG (QA đợt 1)

> Ngày: 2026-06-02. Phạm vi: tính năng Vault/E2EE (crypto core, EncryptionEngine,
> VaultManager, VaultViewModel).
> Phương pháp: bộ test tự động `tests/test_vault_robustness.cpp` (24 case
> edge/fuzz/crash-safety) + đọc code (đối chiếu `docs/specs/vault-test-cases.md`).

## 1. Tổng quan thực thi test

| Bộ test | Kết quả |
|---|---|
| `test_vault_robustness` (24 case mới) | 23 PASS · 1 FAIL → **BUG-VLT-02** |
| `test_encryption_engine` (cũ) | PASS |
| `test_vault_manager` (cũ) | PASS |
| `test_crypto_securebytes`, `test_vault_viewmodel` (cũ) | PASS |

**Phòng thủ đã xác nhận TỐT (PASS — không phải bug):** parser bác bỏ header
ngắn/magic sai/version-algo lạ/FILENAME_LEN khổng lồ/originalSize-AES khổng
lồ/chunkSize lỗi; fuzz 400 byte-flip không bao giờ giải mã sai; fuzz 300 vòng
rác không crash; truncation stream → lỗi; **fail-closed**: sai khóa/tamper/cắt
cụt khi decrypt-to-file **không để lại output dở**; encrypt path lỗi → không
.fshenc dở; 0-byte file; boundary 1 MiB±1; meta hỏng/cắt cụt → unlock thất bại
**graceful không crash**; deleteVault giữ nguyên `.fshenc`; autoUnlock với blob
thiếu/hỏng → false không crash; wrong-pass/wrong-old-pass không làm hỏng vault;
keyfile sai kích thước bị từ chối.

> Ghi chú QA: 2 case meta ban đầu báo FAIL do **lỗi test** (đoán sai tên file —
> đúng là `.vault-meta.json` có dấu chấm). Sau khi sửa path → PASS. Không phải
> bug sản phẩm; ngược lại khẳng định meta-corruption được xử lý an toàn.

## 2. Danh sách bug

### 🔴 BUG-VLT-01 — Path traversal qua filename không tin cậy (HIGH, bảo mật)
- **Nguồn:** đọc code + test `filenameTraversalRoundTripsVerbatim` (engine giữ
  nguyên filename verbatim).
- **Vị trí:** `src/viewmodels/VaultViewModel.cpp` — `decryptAndOpen()`:
  `const QString out = tmpDir + "/" + name;` với `name` lấy **thẳng** từ
  `h.filename` của header `.fshenc` (input KHÔNG tin cậy — file có thể đến từ
  cloud/người khác/đồng bộ). Không strip path component.
- **Tác động:** một `.fshenc` thù địch với `filename = "..\\..\\..\\evil.exe"`
  hoặc đường dẫn tuyệt đối khiến app ghi bản giải mã **ra ngoài** thư mục temp
  → có thể ghi đè file tùy ý của người dùng khi họ "Mở" file đó.
- **Repro:** tạo `.fshenc` với filename chứa `../`; double-click trong Vault →
  output rơi ngoài `%TEMP%/FsNextVault/...`.
- **Fix đề xuất:** `name = QFileInfo(name).fileName();` (chỉ lấy basename), kèm
  fallback nếu rỗng. Áp ở `decryptAndOpen` (và bất kỳ nơi nào dùng `h.filename`
  làm đường dẫn ghi).

### 🟠 BUG-VLT-02 — Filename > 4096 ký tự: mã hóa được nhưng KHÔNG mở lại được (MEDIUM, toàn vẹn dữ liệu)
- **Nguồn:** test `filenameOverMaxLenAsymmetry` **FAIL** — `enc=Ok dec=InvalidFormat`.
- **Vị trí:** `EncryptionEngine`/`FencFormat`: lúc encrypt, `FILENAME_LEN` là
  `uint16` (tối đa 65535) và filename được ghi nguyên; lúc parse,
  `parseFixedHeader` từ chối `filenameLen > fenc::MaxFilenameLen (4096)`. → bất
  đối xứng: tạo được file `.fshenc` mà không bao giờ giải mã lại được.
- **Tác động:** thấp xác suất (tên file 4096+ ký tự rất hiếm) nhưng là lỗi đúng
  đắn: dữ liệu "tạo được, mất luôn".
- **Fix đề xuất:** lúc encrypt, **clamp** filename lưu trữ về `MaxFilenameLen`
  (filename chỉ là metadata hiển thị; nội dung vẫn mã hóa đủ) → đảm bảo luôn
  parse lại được. (Tên thật <255 ký tự nên không bao giờ bị ảnh hưởng.)

### 🟠 BUG-VLT-03 — Bản giải mã tạm sót lại sau crash CỨNG (MEDIUM, riêng tư)
- **Nguồn:** đọc code.
- **Vị trí:** `VaultViewModel` — danh sách `temps_` chỉ nằm trong RAM; secure-delete
  chỉ chạy ở `lock()`, destructor, và sweep-timer khi app còn sống. Nếu app
  **crash** (không qua destructor/`lock()`), bản giải mã trong
  `%TEMP%/FsNextVault/<uuid>/...` **không được quét lại lúc khởi động**.
- **Tác động:** plaintext nhạy cảm rò rỉ trên đĩa sau crash.
- **Fix đề xuất:** lúc khởi tạo `VaultViewModel` (hoặc khi mở khóa), **quét
  thư mục gốc** `%TEMP%/FsNextVault/` và secure-delete mọi tàn dư từ phiên trước.

### 🟠 BUG-VLT-04 — Trùng basename ghi đè im lặng khi mã hóa nhiều file/thư mục (MEDIUM, mất dữ liệu)
- **Nguồn:** đọc code + `encryption-progress.md §5.6` (tự thừa nhận).
- **Vị trí:** `VaultViewModel::runEncryptBatch` — `out = outDir + "/" +
  fi.fileName() + ".fshenc"` (phẳng theo basename). Hai file trùng tên ở thư
  mục con khác nhau → file thứ hai **ghi đè** file đầu, không cảnh báo.
- **Tác động:** mất dữ liệu thầm lặng khi "Thêm thư mục" có file trùng tên.
- **Fix đề xuất:** làm **duy nhất** đường dẫn `.fshenc` đầu ra (thêm hậu tố
  `(n)`) khi đã tồn tại — chống ghi đè. (Giữ cấu trúc thư mục con là feature lớn
  hơn, tách sau.)

## 3. Mức độ & thứ tự fix
1. **BUG-VLT-01** (HIGH, bảo mật) — fix ngay.
2. **BUG-VLT-04** (mất dữ liệu) — fix ngay.
3. **BUG-VLT-03** (rò rỉ plaintext) — fix ngay.
4. **BUG-VLT-02** (toàn vẹn, xác suất thấp) — fix ngay (rẻ).

Tất cả đều fix gọn, rủi ro thấp.

## 4. ĐÃ FIX & VERIFY (2026-06-02)

| Bug | Fix | Vị trí | Verify |
|---|---|---|---|
| VLT-01 path-traversal | `name = QFileInfo(name).fileName()` + fallback rỗng/`.`/`..` | `VaultViewModel::decryptAndOpen` | code + engine test `filenameTraversalRoundTripsVerbatim` (xác nhận engine giữ verbatim → guard ở VM là cần) |
| VLT-02 filename >4096 | clamp filename lưu trữ về `MaxFilenameLen` lúc encrypt | `EncryptionEngine` (encryptStream) | **regression test `filenameOverMaxLenAsymmetry` → PASS** |
| VLT-03 temp sót sau crash | `sweepOrphanTemps()` quét `%TEMP%/FsNextVault/` lúc khởi tạo VM, secure-delete tàn dư | `VaultViewModel` ctor | smoke launch (khởi động sạch) + code |
| VLT-04 ghi đè trùng tên | đường dẫn `.fshenc` đầu ra làm duy nhất (` (n)`) khi đã tồn tại | `VaultViewModel::runEncryptBatch` | code + VaultViewModel test không regression |

**Kết quả test sau fix:** `ctest` toàn bộ **40/40 PASS** — gồm `VaultRobustness`
**24/24** (BUG-VLT-02 đã xanh) và `VaultViewModel` (không regression sau khi sửa
sanitize/collision/orphan-sweep). App build OK, smoke launch sạch (không lỗi log,
`sweepOrphanTemps` chạy lúc khởi động không sự cố).

> Còn chưa có regression TỰ ĐỘNG riêng cho VLT-01/03/04 ở tầng VaultViewModel
> (cần vault mở khóa + QtConcurrent + side-effect mở file OS → khó unit-test
> sạch). Đã verify bằng đọc code + build + smoke + engine-level test. Khuyến nghị
> bổ sung integration test cho decryptAndOpen-sanitize và batch-collision ở đợt sau.

