---
id: VLT-BUG-0007
title: VaultWizard binding loop on "height" (qml:134) — spam WARN liên tục
status: CLOSED
severity: MEDIUM
area: VaultWizard.qml:133-135
found_by: monkey-chaos-r1 (Burst 2 — chuyển sang Vault giữa lúc fetch)
found_run: 2026-06-03-monkey-chaos-r1
assignee: QA-dev
fix_commit: fa2b01d
verified_run: 2026-06-03-monkey-chaos-r1
created: 2026-06-03
updated: 2026-06-03
---

## Mô tả
QML báo `Binding loop detected for property "height"` tại `VaultWizard.qml:134` mỗi lần mở
wizard (account chưa có vault → click Vault hiện VaultWizard). Lặp rất nhiều → spam log WARN,
re-eval layout liên tục (perf), che lấp lỗi thật trong log. Không crash.

## Repro
1. Tài khoản CHƯA có vault.
2. Click sidebar **Vault** (mở VaultWizard) — lặp lại vài lần.
- **Expected:** wizard layout ổn định, 0 binding-loop warning.
- **Actual:** `[WARN] VaultWizard.qml:134 QML QQuickRectangle: Binding loop detected for property "height"` (cặp đôi, lặp).
- **Bằng chứng:** runs/2026-06-03-monkey-chaos-r1 (log trích trong results.md).

## Phân tích (QA)
- [VaultWizard.qml:133-135](../../../qml/Fshare/Pages/VaultWizard.qml):
  ```
  height: Math.min(implicitH, parent.height - AuroraTheme.sp8)        // 134
  readonly property int implicitH: stepperBox.height + bodyFlick.contentH + footer.height + 2  // 135
  ```
  `height` ⟵ `implicitH` ⟵ tổng chiều cao con; mà con nằm trong `ColumnLayout { anchors.fill: parent }`
  → chiều cao con phụ thuộc `height` của box ⇒ vòng lặp. **Fix gợi ý:** phá vòng — cho ColumnLayout
  dùng `implicitHeight` thay vì fill, hoặc tính `implicitH` từ implicitHeight của con (không phụ thuộc
  height đã layout), hoặc đặt height cố định theo step thay vì Math.min phụ thuộc lẫn nhau.

## Fix (process fix điền)
- Thay đổi:
- Commit:

## Verify (QA điền sau khi fix)
- Chạy lại: mở VaultWizard nhiều lần, kiểm 0 binding-loop warning + layout đúng.
- Run:

## Fix
- `implicitH` dùng `stepperBox.Layout.preferredHeight` + `footer.Layout.preferredHeight` (intrinsic) thay cho
  `.height` (đã layout) → phá vòng phụ thuộc height↔con. Commit **fa2b01d**.

## Verify
- Rebuild + mở VaultWizard 5 lần: **0 binding-loop warning** trong log (evidence 15-verify-vault.png).

## Nhật ký trạng thái
- 2026-06-03 16:42 — NEW (QA, run 2026-06-03-monkey-chaos-r1)
- 2026-06-03 17:05 — FIXED + CLOSED (fa2b01d, verify run monkey-chaos-r1)
