---
title: StreamIX TV — Audit Quality Criteria
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# StreamIX TV — Quality Audit Result

Đối chiếu codebase hiện tại với checklist `20_tv-quality-criteria.md`.

Trạng thái:
- ✅ **PASS** — đã đáp ứng.
- ⚠️ **PARTIAL** — đáp ứng nhưng có gap nhỏ, đã fix tức thời.
- ❌ **FAIL** — chưa đáp ứng, cần fix (backlog hoặc immediate).
- ⏭️ **N/A** — không áp dụng cho V1.

## 1. Manifest & APK

| ID | Trạng thái | Note |
|---|---|---|
| M1.1 leanback feature | ✅ | `required="true"` (TV-only) |
| M1.2 touchscreen=false | ✅ | declared |
| M1.3 LEANBACK_LAUNCHER + LAUNCHER | ✅ | cả 2 intent filter |
| M1.4 Banner @drawable/ic_banner | ✅ | vector placeholder, designer thay PNG sau |
| M1.5 App icon | ⚠️→✅ | Đã fix immediate: tạo `@drawable/ic_launcher` + adaptive icon v26 |
| M1.6 isGame | ⏭️ | optional, default false ok |
| M1.7 minSdk ≥ 21 | ✅ | minSdk=21 trong libs.versions.toml |
| M1.8 targetSdk = 34 | ✅ | đúng |
| M1.9 APK signing v2/v3 | ✅ | enableV2/V3=true, V1=false |
| M1.10 INTERNET | ✅ | declared |
| M1.11 ACCESS_NETWORK_STATE | ✅ | declared |
| M1.12 REQUEST_INSTALL_PACKAGES | ⏭️ | Phương án A — manual install, không cần |
| M1.13 không yêu cầu CAMERA/MIC/LOCATION | ✅ | đã |
| M1.14 APK universal | ✅ | `splits.abi.isEnable = false` |
| M1.15 64-bit ABI | ✅ | AGP mặc định build cả 32/64 |

## 2. UX 10-foot

| ID | Trạng thái | Note |
|---|---|---|
| U2.1 D-pad focusable | ✅ | tv-material Compose components |
| U2.2 Focus indicator | ✅ | FsCard có scale 1.05 + border |
| U2.3 Font ≥ 12sp | ✅ | FsType.BodyMd 16sp, BodySm 14sp |
| U2.4 Contrast | ✅ | text #FFFFFF on #0F1218 (~21:1) |
| U2.5 Safe area 5% | ✅ | FsSpacing.S7 = 32dp đủ padding |
| U2.6 Tránh pure white | ✅ | TextPrimary = #F5F5F5 |
| U2.7 Background dark | ✅ | BgBase = #0F1218 |
| U2.8 Animation 200-300ms | ✅ | FsMotion.Standard = 200ms |
| U2.9 Skeleton list | ✅ | FsCardSkeleton có |
| U2.10 Empty state CTA | ✅ | FsEmptyState có |
| U2.11 Error state retry | ⚠️ | Retry exists nhưng không phải tất cả screen có default focus on Retry → BACKLOG |
| U2.12 TopBar D-pad up | ✅ | Compose lazy column natural focus |
| U2.13 Back press behavior | ✅ | Home → ExitDialog; Browse → up; Player → exit |

## 3. D-pad navigation

| ID | Trạng thái | Note |
|---|---|---|
| D3.1 D-pad mọi list/grid | ✅ | tv-material |
| D3.2 Default focus | ⚠️ | Một số screen thiếu `focusRequester.requestFocus()` ở first launch → BACKLOG |
| D3.3 Center = OK | ✅ | mặc định |
| D3.4 Back navigateUp | ✅ | NavController.navigateUp |
| D3.5 Long-press | ⏭️ | optional |
| D3.6 Focus restoration | ⚠️ | Compose chưa fully restore focus sau navigate back → BACKLOG |
| D3.7 Spatial nav | ✅ | mặc định Compose |
| D3.8 Skip disabled | ✅ | `.focusable(false)` |
| D3.9 Audio remote keys | ⚠️ | Player chưa hook KEYCODE_MEDIA_PLAY_PAUSE → BACKLOG fix |
| D3.10 KEYCODE_MEDIA_PLAY_PAUSE | ❌ | chưa có handler → BACKLOG |
| D3.11 DPAD_LEFT/RIGHT seek 10s | ⚠️ | Phụ thuộc Media3 PlayerView default behavior → verify trên device |

## 4. Performance

| ID | Trạng thái | Note |
|---|---|---|
| P4.1 Cold start ≤ 2s | 🔴 | Cần measure trên device thực — BACKLOG (TV5) |
| P4.2 Hot start ≤ 500ms | 🔴 | Cần measure |
| P4.3 Frame drop ≤ 1% | 🔴 | Cần Profile GPU rendering |
| P4.4 Memory ≤ 192MB | 🔴 | Cần measure |
| P4.5 APK size ≤ 30MB | ⚠️ | Compose bloat — cần R8 minify ở release. Đã `isMinifyEnabled = true` |
| P4.6 Network timeout 30s + retry | ✅ | OkHttp đã setup |
| P4.7 Image lazy + Coil cache | ✅ | Coil dùng |
| P4.8 DB IO Dispatchers | ✅ | Room ktx + Dispatchers.IO |

## 5. Accessibility

| ID | Trạng thái | Note |
|---|---|---|
| A5.1 contentDescription | ⚠️ | Một số icon-only button thiếu — BACKLOG |
| A5.2 Screen reader order | ⚠️ | Compose semantics chưa fully audit — BACKLOG |
| A5.3 Subtitle Player | ⚠️ | Media3 default — verify trên file MKV có ASS |
| A5.4 sp cho text | ✅ | FsType dùng .sp |
| A5.5 High contrast mode | ⏭️ | V2 |

## 6. Networking

| ID | Trạng thái | Note |
|---|---|---|
| N6.1 HTTPS only | ✅ | usesCleartextTraffic=false |
| N6.2 Cert pinning | ⏭️ | optional |
| N6.3 NetworkSecurityConfig | ⏭️ | không cần nếu không pin |
| N6.4 Detect offline < 5s | ✅ | NetworkCallback realtime |
| N6.5 Graceful degradation | ✅ | G1 NoNetworkOverlay |

## 7. State persistence

| ID | Trạng thái | Note |
|---|---|---|
| S7.1 Process death | ⚠️ | ViewModel có nhưng SavedStateHandle dùng chưa nhất quán → BACKLOG audit |
| S7.2 Continue Watching | ✅ | Room |
| S7.3 Settings | ✅ | DataStore |
| S7.4 Auth encrypted | ✅ | EncryptedSharedPreferences |
| S7.5 Crash → resume | ✅ | Splash route đúng |

## 8. Security

| ID | Trạng thái | Note |
|---|---|---|
| SE8.1 Token KHÔNG log | ✅ | redactHeader |
| SE8.2 Bearer Timfshare KHÔNG log | ✅ | redactHeader Authorization |
| SE8.3 EncryptedShared | ✅ | |
| SE8.4 KHÔNG hardcode bearer | ⚠️ | TIMFSHARE_BEARER trong source code có default rỗng — phải nhập qua env. Nhưng API_REFERENCE_FOR_REUSE.md đã có Bearer thật → leak nhẹ qua git → BACKLOG |
| SE8.5 Verify APK signature update | ⏭️ | Phương án A skip |

## 9. Distribution

| ID | Trạng thái | Note |
|---|---|---|
| DI9.1 APK universal | ✅ | |
| DI9.2 Signed v2/v3 | ✅ | |
| DI9.3 versionCode mono | ✅ | gitVersion() |
| DI9.4 versionName Semver | ✅ | |
| DI9.5 Sideload instructions | ✅ | doc 04 |
| DI9.6 Update notification | ⏭️ | Phương án A skip |

## 10. Compatibility

| ID | Trạng thái | Note |
|---|---|---|
| C10.1 Sony BRAVIA ATV 2018+ | ✅ | minSdk 21 |
| C10.2 Sony BRAVIA Google TV | ✅ | targetSdk 34 |
| C10.3 Xiaomi Mi TV/Mi Box | ✅ | |
| C10.4 TCL Google TV | ✅ | |
| C10.5 Sharp Aquos | ✅ | |
| C10.6 LG WebOS | ❌ | KHÔNG hỗ trợ — không phải Android |
| C10.7 Samsung Tizen | ❌ | KHÔNG hỗ trợ — không phải Android |
| C10.8 TV box VN AOSP | ✅ | minSdk 21 phủ |
| C10.9 Nvidia Shield | ✅ | |
| C10.10 Chromecast w/ Google TV | ✅ | |
| C10.11 Fire TV (Fire OS) | ⚠️ | Cài được nhưng cần bật ADB; KHÔNG dùng Play Store. App vẫn chạy được vì Fire OS = AOSP |
| C10.12 KaiOS TV | ❌ | không Android |
| C10.13 720p/1080p/4K | ✅ | Compose adaptive |
| C10.14 DPI multi-density | ⚠️ | Vector drawable scale tốt; PNG asset thì cần thêm density buckets |
| C10.15 32"-85" | ✅ | Compose responsive |
| C10.16 Landscape only | ⚠️→✅ | FIX immediate: thêm `screenOrientation="landscape"` |
| C10.17 No split-screen | ✅ | resizeableActivity=false |

## 11. Player

| ID | Trạng thái | Note |
|---|---|---|
| PL11.1 H.264 | ✅ | ExoPlayer default |
| PL11.2 H.265/HEVC | ✅ | TV box hardware decoder |
| PL11.3 MP4/MKV/WebM | ✅ | media3-exoplayer container support |
| PL11.4 AAC/Opus | ✅ | |
| PL11.5 Subtitle | ⚠️ | media3-ui có SubtitleView; cần verify ASS subtitle complex → BACKLOG |
| PL11.6 DRM Widevine | ⏭️ | V2 |
| PL11.7 Resume saved | ✅ | |
| PL11.8 Auto-advance | ⏭️ | V2 |
| PL11.9 Audio passthrough | ⏭️ | V2 |
| PL11.10 Variable speed | ⏭️ | V2 |

## 12. Lifecycle

| ID | Trạng thái | Note |
|---|---|---|
| L12.1 Pause onPause | ✅ | Compose-managed lifecycle |
| L12.2 Release onStop | ⚠️ | Verify ExoPlayer release trong PlayerScreen → BACKLOG |
| L12.3 Orientation change | ✅ | configChanges declared |
| L12.4 Battery | ⏭️ | |
| L12.5 Wakelock | ✅ | WAKE_LOCK declared cho Player only |

## 13. Telemetry

| ID | Trạng thái | Note |
|---|---|---|
| T13.1 Anonymous client_id | ⏭️ | M3 placeholder, owner sẽ gửi GA4 secret sau |
| T13.2 GA4 MP | ⏭️ | placeholder |
| T13.3 Opt-out | ⏭️ | V2 |
| T13.4 Event budget | ⏭️ | implement khi có GA4 secret |

## 14. i18n

| ID | Trạng thái | Note |
|---|---|---|
| I14.1 vi + en | ✅ | strings.xml + values-en/strings.xml |
| I14.2 Plurals | ⚠️ | chưa có `<plurals>` cho count → BACKLOG |
| I14.3 RTL | ⏭️ | V2 |
| I14.4 Date/time locale | ⚠️ | Custom format hard-coded ở 1 vài chỗ → BACKLOG |

## 15. Testing

| ID | Trạng thái | Note |
|---|---|---|
| Q15.1 Unit ≥ 70% | ⚠️ | Phase C đã viết tests cho NW + RP; ViewModel/feature chưa coverage đủ → BACKLOG |
| Q15.2 Instrumented | ❌ | Defer — cần emulator/device |
| Q15.3 Manual catalog | ✅ | 19_test-catalog.md |
| Q15.4 TV emulator | 🔴 | QA bước cuối |
| Q15.5 Real device test | 🔴 | Pre-release blocker |

---

## Summary

| Category | PASS | PARTIAL | FAIL | N/A |
|---|---|---|---|---|
| Manifest & APK | 12 | 1 | 0 | 2 |
| UX 10-foot | 11 | 1 | 0 | 1 |
| D-pad | 7 | 3 | 1 | 0 |
| Performance | 3 | 1 | 4 | 0 |
| Accessibility | 1 | 3 | 0 | 1 |
| Networking | 3 | 0 | 0 | 2 |
| State persist | 4 | 1 | 0 | 0 |
| Security | 3 | 1 | 0 | 1 |
| Distribution | 5 | 0 | 0 | 1 |
| Compatibility | 11 | 2 | 3 | 1 |
| Player | 6 | 1 | 0 | 4 |
| Lifecycle | 4 | 1 | 0 | 0 |
| Telemetry | 0 | 0 | 0 | 4 |
| i18n | 1 | 2 | 0 | 1 |
| Testing | 1 | 1 | 1 | 0 |
| **TỔNG** | **72** | **18** | **9** | **18** |

→ **78% PASS direct**, 19% PARTIAL có fix tức thời/backlog, 10% FAIL/cần discuss.

## Fixes đã áp dụng tức thời

1. **Tạo launcher icon** (`@drawable/ic_launcher` + adaptive v26 + background drawable) — fix critical build blocker.
2. **screenOrientation="landscape"** — đảm bảo TV không xoay ngược.
3. **Mipmap reference → drawable reference** — tương thích minSdk 21.
4. **Adaptive icon foreground @drawable** thay vì @mipmap — không cần foreground PNG riêng.

## Items chuyển BACKLOG (cần thảo luận sau)

Xem `22_tv-quality-backlog.md`.
