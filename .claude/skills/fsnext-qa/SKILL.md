---
name: fsnext-qa
description: Generate, run, and maintain GUI/functional QA for FsNext (Qt6/QML Fshare desktop, Windows). Use ALWAYS when the user wants to test the app through its UI like a human tester, create or update test cases (whole app, or per module / screen / function), run automated UI-screen tests, log bugs with status flags for a separate fix process, or re-test + update test cases after a feature change / "dev done" / unit-tests-passed notification. Covers the qa/ catalog structure, the ui-driver (PrintWindow+SendInput) automation workflow, the bug lifecycle, and regression-after-update recommendations. Keep this skill in sync when QA structure or app modules change.
---

# FsNext QA — UI automation & test-case lifecycle

Bạn (Claude) đóng vai QA engineer: **sinh test case → chạy automation UI trên giao
diện thật → ghi bug có cờ trạng thái → khuyến nghị**. Mọi tài sản nằm trong `qa/`.
Đọc `qa/README.md` (cấu trúc + tooling matrix) trước khi làm.

## 0. Tài sản & quy ước (đọc trước)
- `qa/README.md` — cấu trúc, 6 loại test, vòng đời bug, tooling.
- `qa/automation/ui-driver/fsnext-ui.ps1` — thư viện lái UI (dot-source rồi dùng `Fs-*`).
- `qa/automation/ui-driver/GUIDE.md` — step-by-step + bản đồ toạ độ + sự cố.
- `qa/testcases/<module>/{functional,scenario,validation,ui,chaos}.md` — catalog.
- `qa/templates/*` — mẫu testcase/bug/run.
- `qa/reports/bugs/VLT-BUG-####.md` + `qa/reports/BUG-INDEX.md` — bug có front-matter `status`.
- `qa/runs/<ngày-module-rN>/` — kết quả + `evidence/` (screenshot/log).
- ID: `<MOD>-<LOẠI>-###` (LOẠI ∈ FN/SC/VAL/UI/CHAOS/CHAOS-MK). MOD: VLT (vault),
  HOME, MYFILES, FAV, DL, UL, SYNC, SET, AUTH, TRAY.

## 1. Module của FsNext cần phủ
Auth/Login · Trang chủ (Home + search) · My Files (file manager cloud) · Yêu thích ·
Tải xuống · Tải lên · Đồng bộ (Sync) · **Vault (E2EE)** · Cài đặt · Tray/HUD.

## 2. WORKFLOW A — Sinh / cập nhật test case cho 1 module
1. Đọc code module (viewmodel + QML page) + spec liên quan để hiểu function/màn hình.
2. Viết case vào `qa/testcases/<mod>/<loại>.md` theo `qa/templates/testcase-template.md`
   (9 cột: ID·Tiêu đề·Tiền điều kiện·Bước·Kết quả kỳ vọng·Tool·Ưu tiên·Auto?·Bug).
3. Phân loại đúng (xem README §3): function/scenario/validation/ui/chaos+monkey.
   Mỗi case nêu CẢ "phải xảy ra" lẫn "KHÔNG được xảy ra" (fail-closed).
4. Cột **Auto?**: `yes(<qttest_target>)` nếu đã có test tự động, ngược lại `no`.
5. Cờ ⚠️/🔴 cho case rủi ro cao (security/fail-closed/fuzz). Giữ bảng Traceability cuối file.
6. Toàn-app = lặp cho mọi module; mỗi module 1 thư mục con để retest độc lập.

## 3. WORKFLOW B — Chạy automation UI test (như tester thật)
> CHỈ chạy khi user yêu cầu chạy thật (tốn thời gian + cần app/đăng nhập). Mặc định
> dùng PowerShell. Quy trình + toạ độ ở `GUIDE.md`.
1. Tiền đề: `if (Fs-Locked)` → DỪNG (yêu cầu user mở khóa + tắt auto-lock). Build nếu cần.
2. `. qa/automation/ui-driver/fsnext-ui.ps1`; tạo `qa/runs/<ngày-mod-rN>/evidence`.
3. `Fs-Launch` → `Fs-Login <acc-test>` (account test, KHÔNG account thật).
4. Mỗi bước: `Fs-Front` → `Fs-Click x y $true` → `Start-Sleep` → `Fs-Grab evidence\NN-buoc.png`
   → **Read ảnh** đối chiếu expected → PASS/FAIL.
5. Native file/folder picker: `Fs-GrabScreen` + `Fs-ClickScreen` (KHÔNG `Fs-Grab`).
6. Sau hành động rủi ro: `Fs-CrashWatch`. Toạ độ nút chưa biết: chụp + ước lượng (GUIDE §6).
7. Ghi `qa/runs/.../results.md` (map case→PASS/FAIL→bug). Mỗi FAIL → Workflow C.
**Bẫy (đã gặp thật):** màn khóa chặn input; cửa sổ login đôi khi không maximize (kiểm
`$global:FsRect`~1936×1048 trước click); click đầu bước hay bị nuốt (`$warm=$true`);
app có thể thoát giữa chừng (relaunch); DPI phải 100%; Vault settings ở QSettings
scope `FPT\FshareNext`. Nếu flaky kéo dài: báo user, đề xuất làm tay/qmltest cho case đó.

## 4. WORKFLOW C — Ghi bug có cờ cho process fix khác
- 1 bug = 1 file `qa/reports/bugs/<MOD>-BUG-####.md` từ `qa/templates/bug-template.md`,
  front-matter `status: NEW`, `severity`, `found_by`, `found_run`, `assignee: ""`.
- Cập nhật `qa/reports/BUG-INDEX.md` (dashboard).
- Process fix lọc `status: NEW`/`TRIAGED` chưa có `assignee` → claim → FIXED + `fix_commit`.
  Có thể bắn mỗi bug thành 1 task fix nền (spawn task, worktree riêng).

## 5. WORKFLOW D — Cập nhật sau khi đổi code / "dev done" (QUAN TRỌNG)
Kích hoạt khi user báo: "dev done / unit test xong / vừa update chức năng X".
1. **Xác định thay đổi:** `git log`/`git diff` từ commit QA gần nhất (hoặc user nêu).
   Map file đổi → module → các case bị ảnh hưởng (dùng cột traceability + grep ID).
2. **Cập nhật test case:** thêm case cho hành vi mới; sửa case lệch hợp đồng mới;
   đánh dấu case lỗi-thời (~~strikethrough~~ + lý do). Cập nhật cột Auto?/Bug.
3. **Khuyến nghị test** (output cho user, dạng bảng): module ảnh hưởng · case cần
   chạy lại (regression) · case UI nên lái lại · mức rủi ro · ước lượng.
4. **Đồng bộ skill này + git:** nếu cấu trúc QA/module đổi → cập nhật chính SKILL.md
   này; commit `qa/` + `.claude/skills/fsnext-qa/` CÙNG đợt push code (skill versioned
   theo code). Sau push, nhắc user: "đã cập nhật test case + skill; muốn tôi chạy UI
   automation cho <module> không?".
5. **Re-verify bug:** bug `status: FIXED` trong BUG-INDEX → chạy lại case `found_by`
   → CLOSED/REOPENED, cập nhật report.

## 6. Khuyến nghị (format output chuẩn cho user)
Luôn kết thúc 1 đợt bằng bảng tóm tắt: số case theo loại · PASS/FAIL · bug mới (id+severity)
· case Auto? còn `no` đáng tự-động-hoá · module/luồng nên test tiếp · rủi ro chặn-release.

## 7. Nguyên tắc
- Tự động hoá tối đa (QtTest/qmltest vào CI); "lái mù" UI hợp khám phá/đợt có người xem.
- Bằng chứng đính kèm mọi run. Không bịa kết quả.
- Account TEST riêng; không đụng dữ liệu thật. Dọn artifacts test (fixtures/registry/
  screenshots tạm) sau đợt, trừ khi user muốn giữ.
- Mỗi module = thư mục riêng → retest độc lập, không ảnh hưởng nhau.
