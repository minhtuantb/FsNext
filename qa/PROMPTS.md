# PROMPTS — Cách "đặt lệnh" cho Claude để chạy QA UI tự động (phiên sau)

> Các prompt dưới đây kích hoạt skill **`fsnext-qa`** (`.claude/skills/fsnext-qa/`).
> Copy-paste, thay phần `<...>`. Claude sẽ tự theo workflow trong skill + dùng
> `qa/` (catalog, ui-driver, bug report). KHÔNG cần bạn tự gõ PowerShell.

---

## A. Sinh test case (không chạy app)

**Toàn ứng dụng:**
> "Dùng skill fsnext-qa: sinh bộ test case **toàn bộ ứng dụng** theo từng module
> (Auth, Home, My Files, Yêu thích, Tải xuống, Tải lên, Đồng bộ, Vault, Cài đặt,
> Tray/HUD), chia theo loại (function/scenario/validation/ui/chaos) vào
> `qa/testcases/<module>/`. Xong báo bảng số case mỗi module/loại."

**Một module / màn hình mới:**
> "fsnext-qa: viết test case cho module **<tên>** (vd Tải lên) — phủ function,
> scenario, validate dữ liệu, UI screen, chaos/monkey. Ghi vào
> `qa/testcases/<module>/`. Đánh dấu case nào đã có test tự động."

---

## B. Chạy automation UI thật (cần app + tài khoản test + KHÔNG khóa màn hình)

**Một module (khuyến nghị — đỡ flaky hơn toàn app):**
> "fsnext-qa: chạy **UI automation test cho module <tên>**. Đăng nhập bằng account
> test `taikhoantestfshare@gmail.com` / `Test2024@`. Chụp bằng chứng từng bước vào
> `qa/runs/...`, đối chiếu test case, báo PASS/FAIL + mở bug cho mỗi FAIL. (Tôi đã
> tắt auto-lock màn hình.)"

**Toàn ứng dụng (theo đợt, có người ngồi xem):**
> "fsnext-qa: chạy UI automation **lần lượt từng module** của toàn app, mỗi module
> 1 thư mục run riêng, dừng + báo nếu app crash hoặc màn hình khóa."

**Một kịch bản cụ thể:**
> "fsnext-qa: lái UI kịch bản — tạo Vault → thêm file 6MB → DLG-LARGEFILE chọn
> 'Mã hóa & thêm' → mở file → Khóa ngay → kiểm secure-delete. Chụp từng bước."

> ⚠️ Trước khi chạy mục B: **tắt tự khóa/sleep màn hình** (xem GUIDE §0) và để máy
> không bị khóa trong lúc chạy.

---

## C. Sau khi dev cập nhật code (dev done / unit test xong)

> "fsnext-qa **Workflow D**: tôi vừa update chức năng **<tên>**, đã merge + unit
> test pass (commit/branch: `<sha/branch>`). Hãy: (1) xác định thay đổi so với lần
> QA trước, (2) **cập nhật lại test case** bị ảnh hưởng + thêm case cho hành vi
> mới, (3) đưa **khuyến nghị test** (case regression + luồng UI nên chạy lại +
> mức rủi ro), (4) cập nhật skill fsnext-qa nếu cấu trúc đổi, (5) commit `qa/` +
> skill cùng đợt. Sau đó hỏi tôi có chạy UI automation không."

**Biến thể ngắn (khi tôi báo bạn 'dev done'):**
> "fsnext-qa: code module <tên> vừa done — update test case + gợi ý cần test gì."

---

## D. Vòng đời bug

**Sau khi process fix sửa xong:**
> "fsnext-qa: các bug `status: FIXED` trong `qa/reports/BUG-INDEX.md` đã có commit
> fix — hãy **verify lại** (chạy case `found_by` + regression) rồi cập nhật trạng
> thái CLOSED/REOPENED trong report."

**Giao bug cho process fix nền:**
> "Bắn mỗi bug `status: NEW` trong BUG-INDEX thành 1 task fix nền (worktree riêng),
> set assignee, để tôi review sau."

---

## E. Bảo trì / đồng bộ

> "fsnext-qa: rà toàn bộ catalog `qa/testcases/`, đánh dấu case lỗi-thời so với code
> hiện tại, cập nhật cột Auto? (test nào đã tự động), và liệt kê case `Auto?: no`
> đáng viết QtTest/qmltest để vào CI."

---

## Mẹo đặt prompt
- Luôn nhắc cụm **"fsnext-qa"** hoặc **"UI automation test"** để kích hoạt skill.
- Nêu rõ **module** + **chạy thật hay chỉ sinh case** (mục A vs B).
- Mục B: xác nhận **đã tắt auto-lock** + **đồng ý dùng account test**.
- Sau mỗi đợt, Claude trả **bảng khuyến nghị** (xem skill §6) — dùng nó để quyết
  định đợt test tiếp.
