# Post-blocker roadmap — đường thẳng tới RC1 và GA

**Ngày**: 2026-04-28
**Trigger**: Maintainer xác nhận `POST /api/user/login` hoạt động bình thường
trên môi trường thật. Backend blocker duy nhất ngoài tay frontend đã hết.
**Tham chiếu**: `STATUS.md`, `docs/08_assessment_and_roadmap.md`,
`docs/decisions/003_upgrade_decisions.md`, `docs/09_verification_report.md`,
`docs/10_login_400_playbook.md` (emergency runbook).

---

## 1. Tình hình hiện tại

| Lĩnh vực | Trạng thái |
|---|---|
| Architecture (MVVM + Clean) | ✅ stable, ~25k LOC, DI sạch |
| Design system (Aurora) | ✅ unified, FshareTheme là alias mỏng |
| Auth — OAuth (Google/Facebook/FPT ID) | ✅ working, refresh_token persist DPAPI |
| Auth — email + password | ✅ working (2026 app_key + UA) |
| Transfer engines (multi-segment DL, chunked UL retry) | ✅ implemented |
| Sync (file watcher, max 5 folders) | ✅ implemented |
| FileCache (SQLite + FTS5) | ✅ + 36 unit test PASS |
| Connection pooling (CURL share) | ✅ DNS/TLS/cookie/connect shared |
| Token refresh (silent re-login) | ✅ tryRefreshSession() + single-flight |
| System tray | ✅ Qt6::Widgets + 3-action menu + balloon |
| Chrome extension native messaging | ✅ host binary + MV3 extension + register script |
| Disk-space pre-check + conflict policy | ✅ logic, UI chưa có toggle |
| Single-instance | ✅ wired, second exe forwards argv |
| Command palette (⌘K) | ✅ FsCommandPalette + 10 lệnh |
| Crash handler + log rotation | ✅ |
| i18n vi/en | ✅ pipeline có lupdate target |
| Tests | ✅ 36 cũ + 30 mới (BudgetManager/FshareUrl/Sanitizer/SpeedMeter) |

**Chưa làm** (không phải blocker, là việc còn lại trên đường tới GA):
- ADR D12 — progress persistence trên crash
- ADR D13 — page split (FileManagerPage 2595 LOC, FavoritesPage 1373)
- Wire AppSettings.minimizeToTray + fileConflictPolicy vào SettingsPage
- A11y audit
- Production installer + code signing
- qmllint trong CI
- Telemetry self-host (thay Google Analytics legacy)

---

## 2. Lộ trình 4 mốc — RC1, RC2, GA, post-GA

### Mốc 1 — RC1 candidate (1 tuần)

**Tiêu chí**: build sạch trên Windows dev, smoke test 5 luồng pass, no known crash.

| # | Task | Effort | Owner | Acceptance |
|---|---|---|---|---|
| M1.1 | Build verify trên Windows dev (`cmake --preset msvc2022 && cmake --build build && ctest`) | 30 phút | Maintainer | exit 0, ≥ 60 test PASS, output/FsNext.exe + output/fsharenativeapp.exe |
| M1.2 | qmllint pass cho mọi file qml/ | 1 giờ | FE | không error, warnings được triage |
| M1.3 | Smoke test 5 luồng (login, download 50MB, upload 20MB, sync 1 folder, tray show/hide + ⌘K + Chrome ext popup) | 2 giờ | QA / maintainer | tất cả luồng từ đầu tới cuối không crash, không stuck |
| M1.4 | Run `cmake --build build --target update_translations` rồi review diff `fshare_en.ts` | 1 giờ | FE | mọi `qsTr()` mới (FsCommandPalette / drag overlay / tray menu) có entry tiếng Anh chấp nhận được |
| M1.5 | Cập nhật `REPORT.md` + tag git `v6.0.0-rc1` | 30 phút | Maintainer | tag pushed |

### Mốc 2 — RC2 (2-3 tuần sau RC1) — đóng debt nguy hiểm

| # | Task | Effort | Acceptance |
|---|---|---|---|
| M2.1 | **ADR D12 — progress persistence** | 1.5 ngày | crash mid-download 50% file → restart → resume từ ~50%, không byte 0 |
| M2.2 | Wire `AppSettings.minimizeToTray` + `fileConflictPolicy` vào SettingsPage QML | 1 ngày | toggle/select hoạt động, persist sau restart |
| M2.3 | A11y audit pass 1 — `accessibleName` cho mọi `Aurora.FsIcon` icon-only button (ước tính ~80 chỗ) | 1 ngày | NVDA / Narrator đọc được mọi action |
| M2.4 | A11y audit pass 2 — `Accessible.role` cho FsToast / FsDialog / focus ring 2 px theo handoff | 1 ngày | screen reader announce toast khi xuất hiện |
| M2.5 | qmllint vào CI (`add_custom_target(qml_lint)` + GitHub Action / Azure pipeline) | 4 giờ | mỗi PR có job qml_lint chạy |
| M2.6 | Bù test: viết test cho `FshareApi::login` happy + 405 path; `AuthService::tryRefreshSession` single-flight | 1 ngày | tổng test ≥ 90 case PASS |

### Mốc 3 — GA candidate (4-5 tuần sau RC1)

| # | Task | Effort | Acceptance |
|---|---|---|---|
| M3.1 | **ADR D13 — page split** FileManagerPage → 5 sub-component (Toolbar, ContextMenuBuilder, RenameDialog, ShareDialog, DetailPanel); FavoritesPage → 2 sub-component | 3 ngày | mỗi file ≤ 800 LOC, behaviour bit-identical |
| M3.2 | Production installer (Inno Setup) — bundle `FsNext.exe` + `fsharenativeapp.exe` + Qt deps + cài Start Menu shortcut + chạy `register_native_host.bat` post-install + uninstaller xoá HKCU registry | 2 ngày | `FsNextSetup.exe` chạy clean trên Win 10/11 fresh, install + run + uninstall không vết |
| M3.3 | Code signing với EV cert | 0.5 ngày | SmartScreen không cảnh báo trên fresh download |
| M3.4 | Telemetry opt-in (thay Google Analytics legacy) — speed sample tới `flog.fshare.vn` mỗi 5 phút khi có active transfer | 1.5 ngày | settings có toggle "Gửi thống kê tốc độ ẩn danh"; payload kích thước < 500 byte/sample |
| M3.5 | Tài liệu user — `README.md` cấp 1: cài đặt, hướng dẫn 5 luồng, FAQ về extension Chrome + tray | 1 ngày | reviewer ngoài (PM/người dùng test) đọc và làm theo được |

### Mốc 4 — Post-GA (tuần 6+)

| # | Task | Lý do |
|---|---|---|
| M4.1 | macOS port (Application::osPrefersReducedMotion stub đã sẵn) | 30% user mac |
| M4.2 | Drag-overlay polish — bounce animation khi qua inner UploadPage drop zone | UX wow |
| M4.3 | RSS auto-download (theo `docs/01_features.md` §7) | 5% legacy user, không cấp bách |
| M4.4 | Subtitle search (legacy §8) | Same |
| M4.5 | Auto-update with delta patches | Quality of life |
| M4.6 | Reverse-engineer scoring (analytics local) cho file recommended | Theo dashboard handoff |

---

## 3. Tuần này (24 → 28 Apr 2026) — concrete to-do

Trong 5 ngày làm việc tới, ưu tiên **cao nhất** (cứ làm đúng thứ tự sẽ ra RC1):

**Thứ Hai (M1.1 + M1.2)**
- Sáng: trên máy dev Windows, đóng mọi `FsNext.exe` đang chạy (build trước fail vì exe locked), `cmake --preset msvc2022 && cmake --build build --config Release`. Kỳ vọng: link OK, output có 2 binary.
- Chiều: `qmllint qml/Main.qml qml/Fshare/Pages/*.qml qml/Fshare/Components/*.qml qml/FsAurora/**/*.qml` — fix mọi lỗi (chủ yếu sẽ là warning-cấp).

**Thứ Ba (M1.3 + M1.4)**
- Sáng: `ctest --test-dir build --output-on-failure`. 4 test mới phải PASS, 36 test cũ vẫn PASS.
- Chiều: chạy app thật, kiểm 5 luồng theo M1.3. Bug nhặt được → log Asana / GitHub issue, không block tag RC1 trừ khi crashing.

**Thứ Tư (M1.4 i18n + M1.5 tag)**
- Sáng: `cmake --build build --target update_translations`. Review diff `fshare_en.ts` — dịch các string mới (~30-50 entry).
- Chiều: Git tag `v6.0.0-rc1`. Cập nhật `REPORT.md`. Notify stakeholder.

**Thứ Năm — Thứ Sáu (M2.1 progress persistence)**
- Đây là technical debt P0 cho RC2 — bắt đầu sớm để có buffer.
- Schema: thêm cột `progress_json TEXT` vào table `transfer_history`. Format `{"taskId","bytesTransferred","fileSize","segmentBytes":[...]}`. Debounce 5s qua `QTimer::singleShot` trong `TransferService::onProgress`.
- Test: `test_transfer_resume.cpp` — start download, kill app, restart, expect resume.

---

## 4. Risk register

| Rủi ro | Xác suất | Impact | Mitigation |
|---|---|---|---|
| Backend rotation app_key tiếp theo (cuối 2026 / đầu 2027) | Medium | Login hỏng | `docs/10_login_400_playbook.md` đã sẵn. Chuẩn bị: có 1 alert / cron poll tìm spike HTTP 400 từ telemetry M3.4. |
| Qt 6.8 → 6.9 LTS upgrade trong năm | Medium | Một số API deprecated | Dependency lock trong `CMakePresets.json`, upgrade theo lịch riêng, có nhánh test |
| Windows 12 release | Low (2026) | API tray / WAR có thể đổi | Chỉ áp dụng nếu MS thật sự đổi `SystemParametersInfo`. Application::osPrefersReducedMotion đã có fallback. |
| Vendored libs (curl/openssl/jsoncpp) CVE | Medium | Security patch cấp bách | vcpkg auto pin SHA, mỗi tháng review `vcpkg upgrade --dry-run` |
| Chrome extension MV3 policy thay đổi (Google deprecate native messaging) | Low (Google đã commit MV3 keep) | Chrome integration hỏng | Fallback: drag-drop URL + ⌘V paste vẫn hoạt động độc lập với extension |
| GitHub / GitLab Actions free tier hết hạn | Low | CI fail | Self-host runner sẵn |

---

## 5. Definition of done — Bảng kiểm cuối cùng cho GA

Một bản FsNext v6.0 RTM được coi là "done" khi đồng thời:

- [ ] M1.x + M2.x + M3.x toàn pass.
- [ ] 0 known crash trong 7 ngày soak test trên 3 máy Windows khác nhau.
- [ ] Login email-password + 3 OAuth provider tất cả pass smoke ngày tag.
- [ ] Download multi-segment file > 1 GB → MD5 khớp với server.
- [ ] Upload chunked file > 100 MB qua mạng "khó" (4G hotspot) → khôi phục được sau disconnect 30s.
- [ ] Sync 1 folder local → cloud, thêm/sửa/xoá → cloud reflect trong 5 phút (debounce + scan).
- [ ] Window minimize-to-tray, double-click tray restore → state persist.
- [ ] Chrome extension popup gửi URL khi app đóng → app khởi động và load URL.
- [ ] App khởi động trên máy 4 GB RAM trong < 4 giây.
- [ ] Bộ nhớ steady-state idle < 250 MB (no transfer).
- [ ] Bundle installer < 80 MB (qua compression).
- [ ] Smartscreen không cảnh báo (cần code sign).
- [ ] User documentation đầy đủ ngôn ngữ vi + en.

---

## 6. Câu hỏi cần maintainer trả lời tuần này

1. **EV cert đã có chưa?** Nếu chưa → request từ ops team / mua mới (chi phí ~$500/năm).
2. **Hosting telemetry endpoint** `flog.fshare.vn` — vẫn còn / đã deprecated? Nếu deprecated, target mới là gì? Hoặc có thể self-host endpoint khác?
3. **CI/CD pipeline** — đang dùng gì? GitHub Actions / Azure / Jenkins / on-prem? Job đầu tiên chỉ cần: configure → build → test → upload artifact.
4. **App Store distribution** (Microsoft Store, Mac App Store khi có port) — có kế hoạch không? Quyết định ảnh hưởng tới installer & code-sign requirements.
5. **Translation budget** — có translator trả tiền hay tự team dịch en.ts? ~470 string × 30s/string ≈ 4 giờ tự dịch.
6. **Telemetry opt-in mặc định ON hay OFF?** Quyết định privacy + GDPR — đề xuất OFF (an toàn hơn).
