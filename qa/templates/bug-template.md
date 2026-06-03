---
id: VLT-BUG-0000
title: <mô tả bug ngắn gọn>
status: NEW            # NEW → TRIAGED → FIXING → FIXED → VERIFYING → CLOSED | WONTFIX | REOPENED
severity: MEDIUM       # CRITICAL | HIGH | MEDIUM | LOW
area: <module/file>    # vd VaultViewModel, EncryptionEngine, FencFormat
found_by: VLT-XX-000   # test case id phát hiện
found_run: YYYY-MM-DD-<module>-r<n>
assignee: ""           # process fix điền tên để "claim" bug
fix_commit: ""         # SHA commit khi status=FIXED
verified_run: ""       # thư mục run xác nhận khi VERIFYING/CLOSED
created: YYYY-MM-DD
updated: YYYY-MM-DD
---

## Mô tả
<điều gì sai, vì sao là lỗi>

## Repro (các bước tái hiện)
1. ...
2. ...
- **Expected:** ...
- **Actual:** ...
- **Bằng chứng:** runs/<run>/evidence/<file> (screenshot/log/exit-code)

## Phân tích (tuỳ chọn — QA điền nếu biết)
<nghi vấn vị trí file:dòng, cơ chế>

## Fix (process fix điền)
- Thay đổi: <mô tả>
- Commit: <SHA>

## Verify (QA điền sau khi fix)
- Chạy lại case `found_by` + regression: <kết quả>
- Run: <thư mục>

## Nhật ký trạng thái
- YYYY-MM-DD HH:MM — NEW (QA, run …)
- YYYY-MM-DD HH:MM — FIXING (assignee …)
- YYYY-MM-DD HH:MM — FIXED (commit …)
- YYYY-MM-DD HH:MM — CLOSED (verify run …)
