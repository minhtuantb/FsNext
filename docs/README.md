# docs/ — Chỉ mục tài liệu FsNext

Điểm vào: [`/CLAUDE.md`](../CLAUDE.md) (guide vận hành cho dev/Claude). Dưới đây là các tài liệu còn hiệu lực,
phân theo mục đích. Tài liệu legacy v5.3.0 và snapshot kế hoạch tháng 4 đã được xóa (còn trong git history).

## Tài liệu sống (cập nhật khi code đổi)
| File | Dùng để |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Kiến trúc chính xác theo code: layer, DI, transfer/auth/cache/sync, threading, QML. |
| [ASSESSMENT.md](ASSESSMENT.md) | Đánh giá kiến trúc + khuyến nghị (P1–P3), trạng thái sức khỏe codebase. |
| [BACKLOG.md](BACKLOG.md) | Backlog đang mở (P1–P3), việc đã xong, feature request (2FA...), tech debt (IFshareApi). |

## Reference theo chủ đề
| File | Dùng để |
|---|---|
| [transfer-hud-ux-spec.md](transfer-hud-ux-spec.md) | Spec HUD/tray/mini-window (taskbar progress, lifecycle). |
| [account_external_links.md](account_external_links.md) | Quyết định redirect sang fshare.vn cho password/2FA/VIP. |
| [module_ui_inventory.xlsx](module_ui_inventory.xlsx) | Inventory các module/màn hình UI. |

## Test cases (dùng cho hồi quy / verify)
| File | Dùng để |
|---|---|
| [download_test_cases.md](download_test_cases.md) | Ma trận test download (link parse, folder recursion, filename, concurrency). |
| [upload_test_cases.md](upload_test_cases.md) | Ma trận test upload (filename, size boundary, concurrency). |

## Runbook (khẩn cấp / vận hành)
| File | Dùng để |
|---|---|
| [runbook_login_400.md](runbook_login_400.md) | Chẩn đoán step-by-step khi `/api/user/login` trả HTTP 400 (app_key/UA rotation). |

## Crash audit (reference debug — cần đóng findings, xem ASSESSMENT §P1)
| File | Dùng để |
|---|---|
| [CRASH_AUDIT.md](CRASH_AUDIT.md) | 47 findings rủi ro crash toàn app. |
| [FILE_MANAGER_CRASH_AUDIT.md](FILE_MANAGER_CRASH_AUDIT.md) | Crash chuyên sâu khu vực file manager. |

## ADR — Architecture Decision Records (lý do thiết kế, authoritative)
| File | Dùng để |
|---|---|
| [decisions/001_architecture_pattern.md](decisions/001_architecture_pattern.md) | Vì sao chọn MVVM + Clean Architecture. |
| [decisions/002_tech_stack.md](decisions/002_tech_stack.md) | Vì sao Qt6 QML, libcurl, jsoncpp, CMake, vcpkg. |
| [decisions/003_upgrade_decisions.md](decisions/003_upgrade_decisions.md) | 13 quyết định nâng cấp D1–D13. |
