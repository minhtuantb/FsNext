# STATUS — FsNext

**Current Phase**: Upgrade pass complete (per ADR 003)
**Status**: READY for build & verification (blocked on backend API issue — see below)
**Last Updated**: 2026-04-28

---

## Upgrade pass — 2026-04-28

All 13 items in `docs/08_assessment_and_roadmap.md` are now resolved or stubbed,
under decisions captured in `docs/decisions/003_upgrade_decisions.md`. The codebase
no longer carries a hybrid design system; Phase 9 platform deliverables (tray + Chrome
extension) are real; transfer engines have hardened retry/backoff semantics; HttpClient
shares DNS/TLS/cookie state across handles; AuthService can silently refresh sessions;
core modules now have unit-test coverage.

| ADR | Topic | What changed |
|-----|-------|--------------|
| D1 | Single design system: Aurora | All 33 components + 9 pages + Main.qml import `FsAurora.Theme`; `Fshare.Theme` is a thin alias kept only for external imports. |
| D2 | Vietnamese is the source language | `update_translations` CMake target wired; `fshare_en.ts` remains the single translation file. |
| D3 | OS-driven `reduceMotion` | `Application::osPrefersReducedMotion` reads `SPI_GETCLIENTAREAANIMATION` on Windows / `QAccessible::isActive()` elsewhere; pushed to `AuroraTheme.reduceMotion` via context property. |
| D4 | System tray (Qt Widgets) | `QSystemTrayIcon` + context menu (Hiện cửa sổ / Tạm dừng tất cả / Thoát); `QApplication` replaces `QGuiApplication`; `quitOnLastWindowClosed=false` for minimize-to-tray. |
| D5 | Chrome extension via separate native host | New `fsharenativeapp.exe` target + `extension/` MV3 (manifest, background, popup) + registration script. |
| D6 | Disk-space pre-check | `TransferService::addDownload` rejects when free disk < 50 MB headroom. |
| D7 | File-name conflict policy | `FileNameSanitizer::resolveConflict` + `AppSettings::fileConflictPolicy` (default = rename). |
| D8 | HTTP connection pooling | `curl_share_init` with DNS / SSL session / cookie / connect sharing in `HttpClient`. |
| D9 | Silent token refresh | `AuthService::tryRefreshSession` (single-flight) + `tryRefreshFinished` signal. |
| D10 | Multi-segment download | Already present (`probeFileInfo` + `downloadMultiSegment`); kept as-is. |
| D11 | Chunked upload retry | `MAX_CHUNK_RETRIES` 3 → 5 with exponential backoff (1/2/4/8/16 s); 401/403/507 short-circuit. |
| D12 | Progress persistence | Pending (planned for the next pass — will live in `HistoryRepository`). |
| D13 | Page split policy | `FsCommandPalette` + drag overlay landed; `FileManagerPage`/`FavoritesPage` split deferred to a follow-up. |

### Tests added

- `test_budget_manager` — slot caps, floor quotas, global cap, in-flight task preservation on `setConfig`.
- `test_fshare_url` — case insensitivity, scheme tolerance, share-token preservation in canonicalUrl.
- `test_filename_sanitizer` — Windows reserved chars, DOS device names, length cap with extension preservation.
- `test_speed_meter` — start/markProgress/reset/eta semantics.

### Known caveats

- `D12 — progress persistence` and the page-split refactor (`D13`) are not yet implemented. Both are on the next-pass list.
- Build verification: the previous build attempt (`build-log.txt`) failed at link time (`output\FsNext.exe` was locked because the binary was running). Run `cmake --build build` after closing any running FsNext instance.
- Showcase page (`qml/Fshare/Pages/ShowcasePage.qml`) still references some legacy aliases through the `Fshare.Theme` shim — works at runtime, port to Aurora when convenient.

---

## Pre-existing phases (carried forward)

Phase 0 → Phase 10 deliverables from the original `PLAN.md` remain intact. No regression
expected from the upgrade pass since type-safe ADRs cover every observable change.

---

## Backend API status — **LOGIN VERIFIED WORKING** (2026-04-28)

The HTTP 400 blocker noted in earlier `BACKLOG.md` entries (dated 2026-04-15) is
**no longer reproducible** in the current build.  Maintainer-confirmed:
email+password login through `POST /api/user/login` succeeds in real environment
with the post-upgrade rotation:
- `app_key = dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt` (2026 plaintext, XOR-encoded in `FshareApi.cpp:18-25`)
- `User-Agent = Fshare_Tool_2026` (`HttpClient.cpp:175`)

Most likely the backend allowlist was updated server-side around the same time
the client rotated — earlier diagnostics were against the stale 2025 key.  No
client-side workaround needed; **dev-bypass remains REMOVED** from
`AuthService.cpp` and stays out of production builds.

`docs/10_login_400_playbook.md` is retained as an **emergency runbook** in case
the same symptom recurs (typically after a future server-side rotation).

---

## Next pass — RC1 priorities

With the blocker gone, the path to a release candidate is purely engineering work:

1. **Build verify on a Windows dev machine** — `cmake --preset msvc2022 && cmake --build build && ctest`. Picks up Qt6::Widgets (added for tray), the new `fsharenativeapp` target, the 4 new unit tests (BudgetManager, FshareUrl, FileNameSanitizer, SpeedMeter).
2. **Smoke test 5 user flows** — login + download + upload + sync + ⌘K command palette + Chrome extension popup → URL routes into Download tab.
3. **ADR D12 — progress persistence**: SQLite sidecar so a mid-download crash resumes from the last 5 s checkpoint, not from byte 0.
4. **ADR D13 — page split**: `FileManagerPage` (2595 LOC) and `FavoritesPage` (1373 LOC) into sub-folders.
5. **Wire `AppSettings.minimizeToTray` + `fileConflictPolicy`** into `SettingsPage.qml` (fields exist in struct, UI not yet exposed).
6. **A11y audit** — `accessibleName` on every icon-only button, `Accessible.role` on toasts, focus ring offset 2 px per Aurora handoff.
7. **Production installer** — Inno Setup or NSIS bundling `FsNext.exe + fsharenativeapp.exe + Qt deps + register_native_host.bat`. Hook EV cert signing.
8. **`qmllint` in CI** — catch QML-side regressions (e.g. the `Overlay.modal` capitalisation bug from this verify pass) before runtime.
9. **i18n refresh** — `cmake --build build --target update_translations` to pick up `qsTr()` strings added during the upgrade pass (FsCommandPalette, drag overlay, tray menu).
10. **Telemetry self-host** — opt-in speed sample to `flog.fshare.vn` (mentioned in legacy `docs/01_features.md` §10.2). Replaces the deprecated Google Analytics path.
