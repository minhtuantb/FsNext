# docs/ — Chỉ mục tài liệu FsNext

Điểm vào: [`/CLAUDE.md`](../CLAUDE.md) (guide vận hành cho dev/Claude). Tài liệu được phân nhóm theo **vai trò
trong vòng đời phát triển**; tên file thống nhất `kebab-case`. Tài liệu **test/bug đã chuyển toàn bộ sang
[`/qa/`](../qa/README.md)** (một nguồn sự thật cho QA).

```
docs/
  architecture/   thiết kế & sức khỏe hệ thống (tài liệu sống)
  decisions/      ADR — quyết định kiến trúc (authoritative)
  specs/          đặc tả tính năng để code
  runbooks/       quy trình xử lý sự cố vận hành
  audits/         báo cáo rà soát (crash / i18n) — cần đóng findings
  reference/      tra cứu theo chủ đề
  project/        quản trị dự án (backlog)
```

## architecture/ — Kiến trúc & đánh giá (cập nhật khi code đổi)
| File | Dùng để |
|---|---|
| [architecture/overview.md](architecture/overview.md) | Kiến trúc chính xác theo code: layer, DI, transfer/auth/cache/sync, threading, QML. |
| [architecture/assessment.md](architecture/assessment.md) | Đánh giá kiến trúc + khuyến nghị (P1–P3), trạng thái sức khỏe codebase. |
| [architecture/design-system.md](architecture/design-system.md) | Quyết định QML: `Fshare.Components` chuẩn, `FsAurora.Theme` token; bảng ánh xạ atom. |

## decisions/ — ADR (lý do thiết kế, authoritative)
| File | Dùng để |
|---|---|
| [decisions/0001-architecture-pattern.md](decisions/0001-architecture-pattern.md) | Vì sao chọn MVVM + Clean Architecture. |
| [decisions/0002-tech-stack.md](decisions/0002-tech-stack.md) | Vì sao Qt6 QML, libcurl, jsoncpp, CMake, vcpkg. |
| [decisions/0003-upgrade-decisions.md](decisions/0003-upgrade-decisions.md) | 13 quyết định nâng cấp D1–D13. |

## specs/ — Đặc tả tính năng (để code)
| File | Dùng để |
|---|---|
| [specs/encryption/plan.md](specs/encryption/plan.md) | Spec kỹ thuật E2EE: thuật toán (`.fshenc`), key hierarchy, lộ trình E0–E6, §13 quyết định. |
| [specs/encryption/ui-brief.md](specs/encryption/ui-brief.md) | Brief UI/UX cho thiết kế: 19 màn, microcopy VI, flow, checklist. |
| [specs/encryption/progress.md](specs/encryption/progress.md) | Bàn giao/tiến độ module Vault: việc đã làm, build/test/run + env, cạm bẫy, tồn đọng. |
| [specs/transfer-hud.md](specs/transfer-hud.md) | Spec HUD/tray/mini-window (taskbar progress, lifecycle). |
| [specs/async-scan.md](specs/async-scan.md) | Kế hoạch thực thi: đưa quét folder sync ra background thread (M18). |

## runbooks/ — Vận hành / khẩn cấp
| File | Dùng để |
|---|---|
| [runbooks/login-400.md](runbooks/login-400.md) | Chẩn đoán step-by-step khi `/api/user/login` trả HTTP 400 (app_key/UA rotation). |

## audits/ — Rà soát (cần đóng findings, xem architecture/assessment §P1)
| File | Dùng để |
|---|---|
| [audits/crash-audit.md](audits/crash-audit.md) | 47 findings rủi ro crash toàn app. |
| [audits/file-manager-crash-audit.md](audits/file-manager-crash-audit.md) | Crash chuyên sâu khu vực file manager. |
| [audits/i18n-audit.md](audits/i18n-audit.md) | Rà soát i18n: độ phủ qsTr/tr, trạng thái bản dịch EN, khuyến nghị. |

## reference/ — Tra cứu theo chủ đề
| File | Dùng để |
|---|---|
| [reference/account-external-links.md](reference/account-external-links.md) | Quyết định redirect sang fshare.vn cho password/2FA/VIP. |
| [reference/module-ui-inventory.xlsx](reference/module-ui-inventory.xlsx) | Inventory các module/màn hình UI. |

## project/ — Quản trị dự án
| File | Dùng để |
|---|---|
| [project/backlog.md](project/backlog.md) | Backlog đang mở (P1–P3), việc đã xong, feature request (2FA…), tech debt (IFshareApi). |

## Test & QA → [`/qa/`](../qa/README.md)
Test cases, bug reports, plan/run đợt chạy, và automation UI đã hợp nhất về `qa/`. Logic test (QtTest/CTest)
vẫn ở [`/tests/`](../tests). Xem [qa/README.md](../qa/README.md) làm điểm vào.
