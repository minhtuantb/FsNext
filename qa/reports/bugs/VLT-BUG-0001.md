---
id: VLT-BUG-0001
title: Filename >4096 ký tự — mã hóa được nhưng không giải mã lại được
status: CLOSED
severity: MEDIUM
area: EncryptionEngine / FencFormat
found_by: VLT-VAL-061
found_run: 2026-06-02-vault-r1
assignee: claude
fix_commit: e971fed
verified_run: 2026-06-02-vault-r1
created: 2026-06-02
updated: 2026-06-02
---

## Mô tả
Khi tên file lưu trong header `.fshenc` dài hơn `fenc::MaxFilenameLen` (4096),
lúc encrypt ghi nguyên (FILENAME_LEN là uint16, chứa được tới 65535) nhưng lúc
parse `parseFixedHeader` từ chối `filenameLen > 4096` → file tạo được nhưng KHÔNG
bao giờ giải mã lại được (mất dữ liệu kiểu "tạo được, mở không được").

## Repro
1. `encryptBuffer` với `name` dài 4196 ký tự → trả `Ok`.
2. `decryptBuffer` cùng buffer → trả `InvalidFormat`.
- **Expected:** hoặc reject ở encrypt, hoặc round-trip được.
- **Actual:** enc=Ok, dec=InvalidFormat (created-but-unreadable).
- **Bằng chứng:** `test_vault_robustness::filenameOverMaxLenAsymmetry` FAIL.

## Fix (process fix)
- Clamp tên file lưu trữ về `MaxFilenameLen` ngay lúc encrypt (filename chỉ là
  metadata hiển thị; nội dung vẫn mã hóa đủ) → luôn parse lại được.
- Commit: e971fed (`EncryptionEngine` encryptStream).

## Verify
- Chạy lại `filenameOverMaxLenAsymmetry` → PASS. ctest 40/40.

## Nhật ký trạng thái
- 2026-06-02 — NEW (QA, run vault-r1, test fail)
- 2026-06-02 — FIXING → FIXED (commit e971fed)
- 2026-06-02 — CLOSED (regression PASS)
