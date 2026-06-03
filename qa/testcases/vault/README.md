# Vault — test case catalogs

Catalog theo loại (xem `qa/README.md §3`). Mỗi file dùng `qa/templates/testcase-template.md`.

| File | Loại | Tiền tố ID | Tool chính | Số case |
|---|---|---|---|---|
| functional.md | function | VLT-FN-### | QtTest / qmltest | 65 |
| scenario.md | ngữ cảnh/kịch bản | VLT-SC-### | UI-driver / tay | 12 |
| validation.md | validate dữ liệu/biên | VLT-VAL-### | QtTest data-row | 50 |
| ui.md | UI screen | VLT-UI-### | qmltest / UI-driver | 5 |
| chaos.md | monkey/crazy/kill-app | VLT-CHAOS-##, VLT-CHAOS-MK-## | harness + crash-watch | 20 |
| **Tổng** | | | | **152** |

> **Nguồn hạt giống:** `qa/testcases/vault/seed.md` (152 case đã soạn) là kho
> case ban đầu — chuyển dần vào các file theo loại ở đây để gắn cột Tool/Auto/Bug
> và truy vết. Test tự động hiện có: `tests/test_encryption_engine.cpp`,
> `tests/test_vault_manager.cpp`, `tests/test_vault_robustness.cpp`,
> `tests/test_vault_viewmodel.cpp`.
