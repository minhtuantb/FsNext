# Decision 003: Upgrade decisions (auto-resolved)

**Date**: 2026-04-28
**Status**: Accepted (auto-resolved by maintainer in absence of explicit user choice)
**Supersedes**: parts of 001, 002 still apply.

## Context

`docs/08_assessment_and_roadmap.md` lists 13 open decisions blocking the upgrade.
This ADR records what each one was resolved to and why, so future readers do not
have to re-derive the rationale.

## Decisions

### D1 — Single design system: **Aurora**
Drop `Fshare.Theme` entirely; every component imports `FsAurora.Theme` directly.
Token aliasing in `FshareColors.qml` is a transition shim that disappears once
all 33 components are ported.

**Why**: handoff explicitly targets Aurora; keeping two systems doubles the
maintenance surface (`heightInput` 32 vs 40, `bg2/bg3` only in Fshare) and
forces `Main.qml` to mirror `isDark` in two places.

### D2 — Vietnamese is the source language
Keep `qsTr("Tải xuống")` as-is. Generate `fshare_en.ts` via `lupdate` and translate
fully. Do not introduce a `fshare_vi.ts` (Qt treats source = vi as default).

**Why**: rewriting 470+ `qsTr` sites to English source is a multi-day task with
no functional benefit. The team writes Vietnamese first; translators write the
English `<translation>` once.

### D3 — `reduceMotion` driven by OS
Wire `Application::initAccessibility()` to read the OS "Reduce motion" / "Animations"
setting at startup AND on `QEvent::ApplicationStateChange`. Push to
`AuroraTheme.reduceMotion` via context property `osPrefersReducedMotion`.

**Why**: Aurora theme already gates every `Behavior` on this property. Without OS
sync the toggle does nothing.

### D4 — System tray on Qt Widgets
Add `Qt6::Widgets` to CMake (not just Quick) so we can use `QSystemTrayIcon`.
Tray runs on main thread; click → `showWindowRequested()`; context menu has
"Hiện cửa sổ / Tạm dừng tất cả / Thoát".

**Why**: `QSystemTrayIcon` lives in QtWidgets — there's no QML-only equivalent.
Adding QtWidgets adds ~3 MB to bundle but is cheap given we already deploy ~100 MB
of Qt runtime.

### D5 — Chrome extension via separate native-messaging host
Build a second target `fsharenativeapp.exe` (small CLI). Manifest registered at
`HKCU\Software\Google\Chrome\NativeMessagingHosts\com.fshare.tool`. Host reads
4-byte little-endian length prefix + JSON, forwards via `QLocalSocket` to the
running `FsNext.exe` single-instance. Extension is MV3, popup → `chrome.runtime.sendNativeMessage`.

**Why**: keeping native messaging in-process means launching the full Qt app for
every Chrome message — 2-second cold start. Separate host is ~200 KB, instant.

### D6 — Disk-space pre-check policy
Hard-block `addDownload` when `freeDiskSpace(savePath) < expectedSize * 1.1`
(10% headroom for filesystem overhead and other writes). Toast user with
required vs available bytes. Folder downloads check the **largest** known file,
not the sum (avoids false positives for huge folders that download serially).

**Why**: 10% is the legacy heuristic; partial-file cleanup on disk-full is
hard and a pre-check is the cheap way out.

### D7 — File-name conflict default: **rename "(n)"**
Default action is `Rename` — append ` (1)`, ` (2)` until unique. User can change
the default in Settings → Download → "Khi trùng tên" with options:
- `Đổi tên (1)` (default)
- `Ghi đè`
- `Bỏ qua`
- `Hỏi mỗi lần`

When `Hỏi mỗi lần` is selected, prompt has a "Áp dụng cho cả hàng đợi" checkbox
that sets `_sessionConflictAction` until the queue empties.

**Why**: rename is non-destructive and matches what most browsers do. Hidden
overwrite is the worst-feeling failure mode.

### D8 — HTTP connection pooling: CURL share + per-host easy-handle reuse
Add `curl_share_init()` with `CURL_LOCK_DATA_COOKIE | CONNECT | DNS | SSL_SESSION`
to `HttpClient`. Keep an `unordered_map<host, CURL*>` of idle handles, reuse on
next call to same host. Mutex-protected; bounded LRU at 8 entries.

**Why**: each FshareApi call currently spends ~200 ms on TLS handshake. Browse
flows do 5+ list/get calls in a row → ~1 s of pure handshake.

### D9 — Token refresh: silent re-login with saved credential
On HTTP 201/202/401/403 mid-operation, `AuthService::refreshSession()` runs
`api->login(savedEmail, savedPassword)` (DPAPI-decrypted) WITHOUT touching UI.
On success, `FshareApi::setSession()` is updated and the original call is
retried once. On failure → existing `sessionExpired` flow.

**Why**: avoids the "everything died at 3 AM" experience for long-running syncs.
One retry only — repeated failure indicates server-side invalidation, not
clock-drift.

### D10 — Download segments: 4 default, settings range 1–16, fallback single on 206-no-support
`DownloadEngine::probeRangeSupport()` issues `HEAD` first; if `Accept-Ranges: bytes`
absent OR `Content-Length` missing, fall back to single-segment. `numSegments`
read from `SettingsService::downloadSegments()` per task at start.

**Why**: legacy app used 4 too. Below 1 MiB single is faster (TCP slow-start
dominates).

### D11 — Upload chunk size: 20 MiB, halve to 5 MiB minimum on per-chunk fail
Initial chunk = 20 MiB; on chunk failure with retryable error
(network/timeout, NOT auth/quota), retry up to 5 times with exponential backoff
(1s, 2s, 4s, 8s, 16s). After 5 fails halve chunk size and resume from current
offset. Hard floor 5 MiB. Auth/quota errors propagate immediately.

**Why**: fixed-100 retries (legacy) hides genuine network failure. Adaptive
sizing handles the "starts fast, then route flaps" case without burning
bandwidth on doomed retries.

### D12 — Progress persistence: per-task JSON sidecar in History DB
On `taskProgressChanged` flush a JSON snapshot per task to SQLite (same DB as
history) every 5 seconds (debounced). On startup, `TransferService::loadHistory`
also loads in-flight rows where `state IN (Active, Paused, Queued)` and resumes
or marks `Failed (recoverable)` based on whether the partial file still exists.

**Why**: Fshare downloads are big enough that crashing at 80% and starting over
is a real user complaint. JSON sidecar avoids touching `TransferTask` schema.

### D13 — Page split policy
Files > 800 LOC must split. `FileManagerPage.qml` (2595) and `FavoritesPage.qml`
(1373) get sub-folders: `qml/Fshare/Pages/FileManager/` and
`qml/Fshare/Pages/Favorites/`. Page root file becomes the layout shell;
toolbars, dialogs, panels, and context menus go in siblings under the same
qmldir.

**Why**: 2.5k-line files are merge-conflict magnets. 800 was chosen so most
existing pages stay single-file.

## Implementation order

Track in `TaskList`. Roadmap document `docs/08_assessment_and_roadmap.md`
(sprints 1–4) gives the timeline; this ADR locks the choices.
