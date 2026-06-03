# Vault / E2EE — TIẾN ĐỘ & BÀN GIAO

> Trạng thái: **Lõi + Phase 2 HOÀN THÀNH & verified** (backend + UI end-to-end; auto-upload/DLG-BATCH/DLG-LARGEFILE/secure-delete; EN 100%; crypto fix L1+M1). Tài liệu này để phiên khác tiếp nối ngay.
> Cập nhật: 2026-06-02 (Phase 2).

---

## 1. Tài liệu liên quan (đọc trước)
| File | Nội dung |
|---|---|
| `docs/specs/encryption/plan.md` | **Spec kỹ thuật để code**: thuật toán (AES single-shot ≤1MiB / XChaCha20 streaming), format `.fshenc`, key hierarchy, lộ trình E0–E6, §13 quyết định đã chốt. |
| `docs/specs/encryption/ui-brief.md` | **Brief UI/UX** gửi thiết kế: 19 màn, microcopy VI, flow, checklist. |
| `.uidrop/fshare4/src/vault-*.jsx` | Prototype React (tham chiếu, **gitignored** qua `.uidrop/`). KeyManager/Batch/Settings ở `vault-pages.jsx`. |
| `CLAUDE.md`, skill `fsnext-i18n`, `fsnext-run` | Quy ước build/i18n/run. |

## 2. Quyết định kiến trúc đã chốt
- **Thuật toán lai**: AES-256-GCM single-shot cho file **≤ 1 MiB** (máy có AES-NI); XChaCha20-Poly1305 secretstream cho phần còn lại / máy không AES-NI. Mỗi `.fshenc` ghi `ALGO_ID` → trộn lẫn an toàn. Chấp nhận: file AES không mở được trên máy không AES-NI.
- **Mô hình khóa hợp nhất**: passphrase → Argon2id → KEK → wrap **Master Key** → wrap DEK/ file. Đổi passphrase chỉ re-wrap Master Key.
- **Cấp lưu khóa mặc định = L2 (DPAPI)**; vẫn cần nhớ/sao lưu passphrase (blob DPAPI mất khi cài lại máy).
- **1 vault trước**, schema (`vault_id`) chừa chỗ N vault.
- **Bảo mật UI**: không hiện đuôi `.fshenc`; lỗi crypto mơ hồ; **chỉ copy vân tay**, KHÔNG lộ Master Key thô; Argon2id chạy **off-main** (không freeze).

## 3. Đã implement

### Backend (C++) — `src/core/crypto/` + `src/core/services/`
- `Crypto.{h,cpp}` (sodium_init, aes256GcmAvailable) · `SecureBytes` (mlock/zero, move-only).
- `Key`/`KdfParams` (Argon2id 3 preset, fingerprint) · `CryptoError`.
- `FencFormat` (header 146B, parse phòng thủ) · `EncryptionEngine` (encrypt/decrypt file+buffer, tự chọn theo size, streaming RAM-thấp).
- `VaultMetadata` (vault-meta.json atomic) · `KeyRegistry` (metadata khóa nhập).
- `services/VaultManager` (**plain class**, không QObject): create/unlock/unlockWithKeyfile/lock/changePassphrase/exportRecoveryKeyfile/deleteVault/tryAutoUnlock; L1/L2/L3.
- `SystemTray`: `setVaultState()` + mục menu Khóa/Mở khóa Vault.

### ViewModel — `src/viewmodels/VaultViewModel.{h,cpp}`
- Hợp đồng QML: `lockState/isUnlocked/vaultExists/vaultName/hasRecoveryKey/storageLevel/autoLockMinutes/aesAvailable/busy/vaultDir/vaultFiles/fileBusy/keys`.
- Q_INVOKABLE: `createVault/unlock/changePassphrase` (async off-main qua QtConcurrent + QPointer + snapshot khóa), `unlockWithKeyfile/lock/exportRecoveryKeyfile/deleteVault/tryAutoUnlock/noteActivity`, `addFiles/addFolder/decryptAndOpen`, `copyFingerprint/importKeyfile/removeKey`.
- Signals: `operationFinished(op,ok,err)`, `fileOpFinished`, `keysChanged`, …
- Auto-lock `QTimer` ở VM (không ở VaultManager → an toàn off-main).
- Wire AppContext (`m_vaultVM`, default dir `~/Fshare/Vault`, tryAutoUnlock startup) + `registerQml("vaultViewModel")` + accessor `vaultViewModel()`.

### UI (QML) — `qml/Fshare/Pages/Vault*.qml` + `qml/Fshare/Components/`
- `VaultPage` (router: empty/locked/unlocked/intro/keys/vsettings) + header thích ứng + danh sách file + kéo-thả + menu "…".
- `VaultIntroPage` (giới thiệu + miễn trừ trách nhiệm) · `VaultWizard` (WIZ-1..7, WIZ-4 cảnh báo 4 checkbox).
- Dialog: Unlock, ChangePass, Export, DeleteVault, Decrypt (trong VaultPage).
- Sub-view: KEY-MANAGER (vân tay, copy, nhập/xuất), SET-VAULT (auto-lock, cấp khóa, hành động, xóa).
- Component mới: `FsCallout`, `FsStrengthMeter`; mở rộng `FsTextField` (reveal/valid/leadingIcon/mono), `FsFileTypeIcon` (lock badge); 12 icon SVG (`qml/Fshare/Icons/`); token `sunk`.
- Route: `Pages.vault=9`, sidebar mục "Vault", Loader trong `Main.qml`, `navigateToVault()`.

### Tests (QtTest) — `tests/`
- `test_crypto_securebytes`, `test_encryption_engine` (AES+XChaCha, tamper, KDF, file round-trip), `test_vault_manager` (lifecycle, recovery, DPAPI, deleteVault), `test_vault_viewmodel` (async, busy-gate, fileEncryptList, keyManagerList).
- **Toàn bộ 39/39 PASS.**

## 4. Build / Test / Run (môi trường)
- **vcpkg classic mode**, `VCPKG_ROOT` **không persist** → set trước khi build:
  `D:\OneDrive - FPT Corporation\Work\Fshare\tool\vcpkg`. Install root deps: `D:/DevTools/vcpkg-inst` (đã có **libsodium 1.0.21** static `x64-windows-static-md`).
- Build: `set VCPKG_ROOT=... ; cmd /c scripts\build.bat` → `output/FsNext.exe`. (libsodium link qua `unofficial-sodium::sodium` + define `SODIUM_STATIC`.)
- Test: `ctest --test-dir build` (hoặc `-R Vault`).
- qmllint: `C:\Qt\6.8.3\msvc2022_64\bin\qmllint.exe -I qml <file>`.
- Log thật: `%APPDATA%\FPT\FsNext\fsnext.log` (KHÔNG phải `…\Fshare Tool\` — đó là binary cũ).
- i18n: `cmake --build build --target update_translations` (lupdate) → dịch `src/i18n/fshare_en.ts` → build chạy lrelease.

## 5. PHASE 2 — ĐÃ HOÀN THÀNH (2026-06-02, verified: build OK · 39/39 test · qmllint sạch · smoke EN)
1. ✅ **Tự tải `.fshenc` lên Fshare** + **DLG-BATCH** (config→progress→done + "Thử lại file lỗi" + Hủy).
   - `VaultViewModel`: `addFilesUpload`/`addFolderUpload(paths, autoUpload, cloudFolder)`, signals `batchProgress(done,total,name)` · `fileUploadQueued(name)`, `retryFailed()`, `cancelBatch()` (atomic flag, hủy giữa file).
   - **Decoupling:** VM KHÔNG phụ thuộc `UploadViewModel` (compile-time). Dùng `using Uploader = std::function<void(QStringList,QString)>` + `setServices(SettingsService*, Uploader)`; AppContext wire lambda → `uploadVM->addUpload(files, folder, "", "", false, false)`. Giữ test nhẹ + sạch MVVM.
   - **Đích cloud = PATH** (`"/"`=root), KHÔNG phải folderId số (FshareApi normalize). Mặc định `/` + cấu hình ở SET-VAULT / DLG-BATCH (FsTextField nhập path).
   - QML: `batchDialog` trong VaultPage (3 bước, FsProgressBar) + "Thêm thư mục" → mở config.
2. ✅ **DLG-LARGEFILE** + setting `max_encrypt_size` + hành vi vượt ngưỡng.
   - Settings: `vaultMaxEncryptMb`(0=không giới hạn) · `vaultOverLimitBehavior`(0 hỏi/1 bỏ/2 thư mục thường) · `vaultAutoUpload` · `vaultUploadFolder` (AppSettings + SettingsRepository `Vault/*` + SettingsService getter/setter; proxy qua Q_PROPERTY `maxEncryptMb/overLimitBehavior/autoUploadDefault/uploadFolder` trên VM).
   - `beginEncrypt` phân loại theo cap (cap đặt → cap đó; không → 1 GiB). Policy ask → `largeFilesPending(QVariantList)` → `largeFileDialog` (Mã hóa & thêm / Thêm không mã hóa / Hủy) → `resolveLargeFiles("encrypt"|"plain"|"skip")`. plain → copy ra `<vault>/../FsNext Unencrypted` (KHÔNG nằm trong vault — đúng §3.1).
   - SET-VAULT nhóm "MÃ HÓA": cap (segmented Không giới hạn/1GB/5GB) + behavior (Hỏi/Bỏ qua/Thư mục thường) + auto-upload (FsSwitch) + thư mục đích.
3. ✅ **Secure-delete** bản giải mã tạm: `VaultViewModel` track temp từ `decryptAndOpen`; sweep timer 60s/TTL 30' + force-sweep khi `lock()` + destructor; overwrite random rồi unlink (`secureDeleteFile`); signal `tempCleaned(n)` → toast "Đã xóa bản giải mã tạm an toàn".
4. ✅ **Dịch EN** TOÀN BỘ: `fshare_en.ts` **0 unfinished / 1060 finished** (lupdate +30 chuỗi mới, dịch 229 entry, lrelease OK, verify runtime EN qua màn login). App vẫn mặc định VI.
5. ✅ **Crypto review nhẹ** (`encryption-plan.md §13.3`): không Critical/High. Đã fix **L1** (engine tự xóa output dở khi encrypt/decrypt lỗi — fail-closed độc lập caller) + **M1** (zeroize QByteArray trung gian chứa Master Key: keyfile/DPAPI/plain trong `VaultManager`). Còn (Low, tùy chọn): fuzz test parser; mlock buffer plaintext AES.

### CÒN TỒN ĐỌNG (tùy chọn, ngoài phạm vi)
6. (Tùy chọn) multi-vault UI; mã hóa tên file (`encryptNames`); preserve cấu trúc thư mục khi mã hóa folder (hiện flat theo basename — có thể trùng tên); per-file status list trong DLG-BATCH (hiện overall progress bar); fuzz test parser (review §13 L2).

## 6. Cạm bẫy đã gặp (để phiên sau tránh)
- `FsButton` **không có** `iconRight`/variant `"text"`; variants: primary/secondary/ghost/danger/link/success. `FsSegmentedControl` dùng `options:[{value,label}]`+`selectedValue`+`selectionChanged`.
- `font.pixelSize` là **int** — gán thập phân (12.5) → "Invalid property assignment" làm type "unavailable".
- Delegate/nested component cần `pragma ComponentBehavior: Bound` + `required property` + qualify id để qmllint sạch.
- Context-property (`vaultViewModel`) luôn bị qmllint "Unqualified access" — chấp nhận (pattern toàn app).
- QML không compile lúc bundle → **smoke launch + đọc log** là cách bắt lỗi runtime (Type unavailable / Expected token).
