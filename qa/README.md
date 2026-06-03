# QA — Hệ thống kiểm định FsNext

> Sổ tay vận hành cho **1 người** (QA-dev) test chức năng + UI screen của FsNext
> (Qt6/QML/C++17, Windows). Bao trùm: viết test case → chạy → ghi report có
> **cờ trạng thái** để một **process fix khác** xử lý → fix xong **cập nhật lại
> report**. Tham chiếu lịch sử: `qa/testcases/vault/seed.md`,
> `qa/reports/vault-bug-report.md`.

---

## 0. Nguyên tắc

1. **Test case là source-of-truth**, có ID ổn định, truy vết được tới bug & tới
   commit fix (traceability 2 chiều).
2. **Mỗi bug = 1 file** có *front-matter* trạng thái → máy đọc được → process
   fix tự pick. Không gộp nhiều bug 1 file.
3. **Tự động hoá tối đa**; cái gì máy chạy lặp được thì viết QtTest/qmltest,
   cái gì cần mắt người/native-dialog thì runbook tay hoặc UI-driver.
4. **Bằng chứng đính kèm** mỗi lần chạy (screenshot/log/exit-code), không "tin lời".
5. **Fail-closed khi nghi ngờ**: test khẳng định cả *điều phải xảy ra* lẫn
   *điều KHÔNG được xảy ra* (vd: sai khoá → KHÔNG để lại output dở).

---

## 1. Yêu cầu kỹ thuật & môi trường

| Hạng mục | Công cụ | Ghi chú |
|---|---|---|
| Build | `scripts/build.bat` (Qt 6.8.3 msvc2022_64, vcpkg, Ninja) | `set VCPKG_ROOT=...` trước khi build |
| Unit/Integration (logic C++) | **QtTest** + **CTest** (`ctest --test-dir build`) | đã có ở `tests/` |
| UI component (logic QML) | **Qt Quick Test** (`qmltest`) + mock ViewModel | `tests/qml/` (xem §3.4) |
| Lint QML | `qmllint -I qml <file>` | bắt type/property sai |
| UI end-to-end (screen thật) | **UI-driver** `qa/automation/ui-driver/fsnext-ui.ps1` (PrintWindow + SendInput) hoặc **runbook tay** | native file/folder picker ⇒ ưu tiên tay |
| Crash detection | **Windows Event Log** (`Get-WinEvent` App, Level=Error) → `0xc0000374`=heap, `0xc0000005`=access-violation | + `%APPDATA%\FPT\FsNext\fsnext.log` |
| Monkey/Fuzz | harness ở `qa/automation/monkey/` (random bytes vào parser; random click/key vào UI) | |
| i18n | `lupdate`/`lrelease` (llvm-mingw) — 0 unfinished trước ship | |

**Cạm bẫy môi trường đã ghi nhận (đọc trước khi test UI):**
- Màn hình **khóa (LogonUI)** → secure desktop chặn HOÀN TOÀN automation. Tắt
  auto-lock/sleep khi chạy UI-driver.
- Cửa sổ login có thể **không maximize** → toạ độ lệch; luôn `Fs-Front` + verify
  rect trước khi click.
- Click **đầu tiên sau khi đưa cửa sổ lên trước hay bị nuốt** → dùng warm-up click.
- App cần **đăng nhập Fshare** mới vào được sidebar/Vault (Vault độc lập account
  nhưng UI gate sau login). Dùng account test riêng.
- Settings Vault đọc từ QSettings scope **`FPT\FshareNext`** (SettingsRepository),
  KHÁC scope mặc định `FPT\FsNext`.

---

## 2. Cấu trúc thư mục `qa/`

```
qa/
  README.md                     # sổ tay này (entry point) — ID/severity/status ở §4–§5
  manual-checklist.md           # checklist smoke thủ công
  test-inventory.xlsx           # inventory case (atomic items)
  conventions.md                # [kế hoạch] tách chi tiết ID/severity/traceability ra riêng
  templates/
    testcase-template.md        # mẫu 1 case
    bug-template.md             # mẫu 1 bug (front-matter trạng thái)
    run-template.md             # mẫu 1 lần chạy
  testcases/                    # CATALOG — source of truth, theo module/loại
    vault/
      functional.md             # VLT-FN-###   (theo function)
      scenario.md               # VLT-SC-###   (theo ngữ cảnh/kịch bản)
      validation.md             # VLT-VAL-###  (validate dữ liệu/biên)
      ui.md                     # VLT-UI-###   (UI screen)
      chaos.md                  # VLT-CHAOS-## (monkey/crazy/kill-app)
      seed.md                   # kho 152 case gốc đã soạn (nguồn hạt giống)
    download/ upload/ transfer-sync/   # functional.md theo module (đã chuyển từ docs/)
  plans/                        # chọn case nào cho 1 đợt chạy
    YYYY-MM-DD-<module>-r<n>.md
  runs/                         # KẾT QUẢ mỗi đợt chạy + bằng chứng
    YYYY-MM-DD-<module>-r<n>/
      results.md                # pass/fail từng case + link bug
      evidence/                 # screenshot, log, exit-code
  reports/
    bugs/
      VLT-BUG-0001.md           # 1 file / bug, có status front-matter
      ...
    BUG-INDEX.md                # bảng tổng (dashboard trạng thái)
    vault-bug-report.md         # báo cáo hợp nhất (lịch sử, đã chuyển từ docs/)
    transfer-sync-bug-report.md #   "
  automation/
    ui-driver/fsnext-ui.ps1     # thư viện lái UI (Front/Click/Type/Grab/crash-watch)
    monkey/                     # [kế hoạch] harness monkey/fuzz
  scripts/
    run-suite.ps1               # build + ctest + thu kết quả + sinh skeleton run
    new-case.ps1 / new-bug.ps1  # [kế hoạch] scaffold nhanh
```

Quan hệ với cây có sẵn: **logic test** vẫn nằm ở `tests/` (QtTest/CTest) — `qa/`
là lớp **điều phối + tài liệu + report + automation UI**, KHÔNG thay thế `tests/`.

---

## 3. Phân loại test — cách viết & chạy

Mỗi case gắn đúng **1 loại** (tiền tố ID). Tiêu chí chọn loại + cách thực thi:

### 3.1 Theo function (`VLT-FN-###`)
- **Là gì:** kiểm 1 hàm/Q_INVOKABLE/1 hành vi đơn lẻ làm đúng hợp đồng.
- **Viết:** mỗi case = input → action → expected (xác định, không mơ hồ).
- **Chạy bằng:** **QtTest** (C++ logic, vd `test_vault_manager`, `test_encryption_engine`)
  hoặc **qmltest** (UI logic). Ưu tiên đây vì lặp lại được + vào CI.

### 3.2 Theo ngữ cảnh / kịch bản (`VLT-SC-###`)
- **Là gì:** luồng end-to-end nhiều bước theo *user journey* (tạo vault → mã hóa
  → tải lên → khóa → mở lại → giải mã).
- **Viết:** liệt kê bước tuần tự + trạng thái kỳ vọng SAU mỗi bước + điều kiện
  rẽ nhánh.
- **Chạy bằng:** **UI-driver** (`fsnext-ui.ps1`) chụp ảnh từng bước, hoặc
  **runbook tay** nếu có native dialog. Lưu screenshot vào `runs/.../evidence/`.

### 3.3 Theo validate dữ liệu / biên (`VLT-VAL-###`)
- **Là gì:** input hợp lệ/không hợp lệ, biên, rỗng, tràn, sai định dạng, untrusted.
- **Viết:** dùng bảng (data-driven): cột input + expected-error. Phủ cả biên ±1.
- **Chạy bằng:** **QtTest data-row** (`QTest::addColumn`/`addRow`) — vd
  `test_vault_robustness` (parser `.fshenc`: magic/version/algo/filenameLen/
  originalSize/chunkSize, header ngắn, biên 1 MiB±1).

### 3.4 UI screen (`VLT-UI-###`)
- **Là gì:** màn hình render đúng + tương tác đúng (dialog hiện, nút bật/tắt,
  gating, chuyển trạng thái, i18n).
- **2 cách (chọn theo case):**
  - **qmltest** (khuyến nghị cho regression): file `tests/qml/tst_*.qml` với
    `TestCase` + `SignalSpy`, nạp component (vd `VaultPage`/`VaultWizard`) với
    **mock `vaultViewModel`** (QtObject giả), drive `mouseClick/keyClick`, assert
    binding/visible/enabled. → CI chạy headless, không cần login/mạng.
  - **UI-driver / runbook tay** cho luồng có native dialog (file/folder picker),
    login Fshare, hoặc cần "mắt thấy" thật. Bằng chứng = screenshot.

### 3.5 Monkey test (`VLT-CHAOS-MK-###`)
- **Là gì:** input/thao tác **ngẫu nhiên số lượng lớn** để tìm crash/hang/state lạ.
- **Data monkey:** ném N nghìn buffer ngẫu nhiên vào parser/giải mã → KHÔNG bao
  giờ ra plaintext, KHÔNG crash (đã có `fuzzRandomGarbage`, `fuzzSingleByteFlip`).
- **UI monkey:** harness `qa/automation/monkey/` bắn chuỗi click toạ độ ngẫu
  nhiên + phím ngẫu nhiên + điều hướng ngẫu nhiên trong M phút, **song song theo
  dõi**: process còn sống? Event Log có crash? log có lỗi? Seed cố định để repro.

### 3.6 Crazy / Chaos test (`VLT-CHAOS-###`)
- **Là gì:** tình huống cực đoan/đối nghịch, mô phỏng đời thực khắc nghiệt:
  - **Kill app giữa chừng**: `Stop-Process -Force` khi đang encrypt/decrypt/ghi
    meta → mở lại: file đã xong hợp lệ, file dở KHÔNG nằm trong vault, meta
    không hỏng, temp giải mã được dọn (sweep startup).
  - **Mất mạng / hết đĩa / file bị khóa bởi app khác** giữa thao tác.
  - **Đầu vào khổng lồ**: vault 10k file; file >10GB; tên file 4096+; unicode.
  - **Đua thao tác**: bấm nút liên tục, mở 2 dialog, encrypt+lock đồng thời.
  - **Trạng thái hỏng**: sửa `.fshenc`/`vault-meta.json` trên đĩa khi app đang đọc.
- **Tiêu chí PASS:** suy biến **graceful** — không mất dữ liệu, không crash,
  fail-closed, thông báo lỗi rõ.
- **Chạy bằng:** kịch bản PowerShell + UI-driver + crash-watch; một số mô phỏng
  được ở QtTest (cắt cụt file, meta hỏng — xem `test_vault_robustness`).

---

## 4. Quy ước ID & traceability

- **Test case:** `VLT-<LOẠI>-<###>` — LOẠI ∈ {FN, SC, VAL, UI, CHAOS, CHAOS-MK}.
  (Đổi `VLT` theo module: `SYNC-`, `DL-`, `UL-`…)
- **Bug:** `VLT-BUG-####`.
- **Run:** thư mục `runs/YYYY-MM-DD-<module>-r<n>/`.
- **2 chiều:** case ghi `bug:` khi fail; bug ghi `found_by:` (case id) + `fix_commit:`;
  run `results.md` map case → pass/fail → bug.

---

## 5. Vòng đời report & bug (cho process fix khác)

Mỗi bug là `reports/bugs/VLT-BUG-####.md` với **front-matter** máy đọc được:

```yaml
---
id: VLT-BUG-0007
title: <mô tả ngắn>
status: NEW          # NEW → TRIAGED → FIXING → FIXED → VERIFYING → CLOSED | WONTFIX | REOPENED
severity: HIGH       # CRITICAL | HIGH | MEDIUM | LOW
area: VaultViewModel
found_by: VLT-VAL-021
assignee: ""         # process fix set tên mình vào đây để "claim"
fix_commit: ""       # SHA khi FIXED
verified_run: ""     # thư mục run khi VERIFYING/CLOSED
---
```

**Luồng (1 người + 1 process fix tách biệt):**
1. **QA** chạy test → case fail → tạo `VLT-BUG-####.md` (`status: NEW`) + cập nhật
   `BUG-INDEX.md`.
2. **Process fix** (dev / agent / spawned task) `grep "status: NEW"` →
   set `TRIAGED`→`FIXING` + `assignee` (claim), sửa code, commit, set `FIXED` +
   `fix_commit`.
   - FsNext có sẵn cơ chế **spawn task fix nền** — mỗi bug NEW có thể bắn thành 1
     task riêng (worktree riêng) để không đụng phiên QA.
3. **QA** thấy `FIXED` → set `VERIFYING`, **chạy lại đúng case `found_by`** (+
   regression). Xanh → `CLOSED` + `verified_run`. Đỏ → `REOPENED` (kèm ghi chú).
4. **Cập nhật lại report**: mọi đổi trạng thái ghi vào file bug + `BUG-INDEX.md`
   (dashboard). `BUG-INDEX.md` luôn phản ánh hiện trạng.

> Cờ `status` + `assignee` chính là "đánh dấu để process khác tiến hành" mà bạn
> cần: process fix chỉ cần lọc `status: NEW|TRIAGED` chưa có `assignee`.

---

## 6. Quy trình 1 người vận hành (end-to-end)

```
[1] Viết/cập nhật case  → qa/testcases/<module>/<loại>.md   (dùng templates/)
[2] Lập plan đợt chạy   → qa/plans/YYYY-MM-DD-...md          (chọn case ID)
[3] Chạy:
      - logic:  qa/scripts/run-suite.ps1   (build + ctest + thu kết quả)
      - UI:     . qa/automation/ui-driver/fsnext-ui.ps1  → lái + chụp
      - monkey: qa/automation/monkey/*     (kèm crash-watch)
[4] Ghi kết quả        → qa/runs/.../results.md (+ evidence/)
[5] Mỗi fail → tạo bug → qa/reports/bugs/VLT-BUG-####.md (status: NEW) + BUG-INDEX
[6] Process fix xử lý các bug NEW (xem §5)
[7] Fix xong → QA verify lại case → update bug + BUG-INDEX (CLOSED/REOPENED)
```

Lặp lại theo đợt (round). `BUG-INDEX.md` là nơi nhìn nhanh "còn bao nhiêu bug,
trạng thái gì".

---

## 7. CI (khuyến nghị)
- Pre-merge: `qmllint` + `ctest` (FN/VAL/UI-qmltest) phải xanh.
- Nightly: monkey/fuzz N vòng + chaos subset (kill-app, meta-corrupt) + crash-watch.
- UI-driver end-to-end: bán tự động (cần login) — chạy thủ công theo đợt, lưu
  evidence vào `runs/`.
