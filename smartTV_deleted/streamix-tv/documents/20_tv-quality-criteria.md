---
title: StreamIX TV — Tiêu chí chất lượng TV App
version: 1.0
date: 2026-05-06
owner: tuandm30@fpt.com
---

# Tiêu chí chất lượng cho ứng dụng TV (Reference checklist)

Tài liệu này tổng hợp **tất cả tiêu chí** một ứng dụng TV cần đáp ứng. Dùng làm
master checklist để audit StreamIX TV (file kế tiếp `21_tv-quality-audit.md`).

Nguồn:
- Google Android TV App Quality (`developer.android.com/quality/tv`).
- Android Compatibility Definition Document (CDD) cho TV devices.
- Leanback design principles (10-foot UI guidelines).
- AOSP TV reference: Sony BRAVIA, Xiaomi Mi Box, Nvidia Shield, AirTV.

## 1. Manifest & APK

| ID | Yêu cầu | Note |
|---|---|---|
| M1.1 | Declare `<uses-feature android:name="android.software.leanback" android:required="false">` | Required false để cài được trên cả phone (nếu cần debug); true nếu chỉ TV. |
| M1.2 | Declare `<uses-feature android:name="android.hardware.touchscreen" android:required="false">` | TV không có touch — phải false. |
| M1.3 | Activity intent filter `LAUNCHER` + `LEANBACK_LAUNCHER` | LEANBACK_LAUNCHER bắt buộc cho ATV launcher. |
| M1.4 | Banner image `android:banner` 320×180 dp xhdpi | Hiển thị trên ATV launcher. |
| M1.5 | App icon 512×512 PNG | Cho Play Store / Manage Apps. |
| M1.6 | `<application android:isGame="false">` | Hoặc true nếu là game. |
| M1.7 | minSdk ≥ 21 (Android 5.0 Lollipop) | Phủ TV box VN từ 2015. |
| M1.8 | targetSdk = 34 hoặc cao hơn | Yêu cầu Google ATV Quality 2024. |
| M1.9 | APK signing v2/v3 | KHÔNG dùng v1-only — Android 11+ block. |
| M1.10 | Permission `INTERNET` declared | Bắt buộc để gọi API. |
| M1.11 | Permission `ACCESS_NETWORK_STATE` | Cho NetworkCallback (G1 overlay). |
| M1.12 | Permission `REQUEST_INSTALL_PACKAGES` | Chỉ khi có in-app update (Phương án A — không cần). |
| M1.13 | KHÔNG yêu cầu `CAMERA`, `MICROPHONE`, `LOCATION`, `READ_PHONE_STATE` | TV không có (hoặc rất hạn chế). |
| M1.14 | APK universal (không split per-ABI) | Dễ sideload — Phương án A đã chốt. |
| M1.15 | Support 64-bit ABI | Google Play yêu cầu từ 2019. |

## 2. UX 10-foot

| ID | Yêu cầu | Note |
|---|---|---|
| U2.1 | Mọi UI element clickable phải focusable bằng D-pad | Compose: `.focusable()` + focus indicator rõ ràng. |
| U2.2 | Focus indicator visible: scale ≥ 1.05 + border + shadow | Người dùng phải biết item nào đang focus từ ghế cách 3m. |
| U2.3 | Font size tối thiểu 12sp body, 14sp+ cho chữ thường đọc | TV viewing distance 2-3m. |
| U2.4 | Contrast ratio ≥ 4.5:1 cho text trên background | WCAG AA. |
| U2.5 | Safe area: padding 5% mỗi cạnh từ edge | Tránh overscan trên TV cũ. |
| U2.6 | Color palette: tránh pure white #FFFFFF (cháy mắt) | Dùng `#F5F5F5` hoặc tối hơn. |
| U2.7 | Background tối (dark mode mặc định) | TV thường được xem trong môi trường thiếu sáng. |
| U2.8 | Animation: motion ease 200-300ms | Không quá nhanh (giật) hoặc chậm (lag). |
| U2.9 | Loading skeleton thay spinner ở list | Skeleton cho biết shape + giảm cảm giác chờ. |
| U2.10 | Empty state với call-to-action rõ ràng | "Không có file nào — Upload từ điện thoại để bắt đầu". |
| U2.11 | Error state với retry button focusable | Default focus trên Retry để user 1-click. |
| U2.12 | Top app bar luôn focusable bằng D-pad up | Search/Settings không bị "lost" trong scroll. |
| U2.13 | Back press behavior chuẩn | Home → ExitDialog; Browse → up; Player → exit. |

## 3. D-pad navigation

| ID | Yêu cầu | Note |
|---|---|---|
| D3.1 | DpadUp/Down/Left/Right hoạt động trên mọi list, grid | Compose Foundation lazy + Tv-Material. |
| D3.2 | Default focus rõ ràng khi mở screen | `focusRequester.requestFocus()` trong LaunchedEffect. |
| D3.3 | Center button = OK / Enter | Trigger onClick. |
| D3.4 | Back button → navigateUp hoặc system back | Không exit app từ deep stack. |
| D3.5 | Long-press / chord shortcut (optional) | Cho power user. |
| D3.6 | Focus restoration khi quay lại screen | Compose `rememberSaveable` + focus state. |
| D3.7 | Spatial navigation: focus order ngang trước dọc | Predictable cho user. |
| D3.8 | Skip focus cho element disabled / loading | `.focusProperties { canFocus = false }`. |
| D3.9 | Audio controls (Play/Pause/Stop, FFW, REW) trên remote | Player phải bắt key event. |
| D3.10 | KEYCODE_MEDIA_PLAY_PAUSE → toggle | Thường gắn nút riêng trên remote. |
| D3.11 | KEYCODE_DPAD_LEFT/RIGHT trong Player → seek 10s | Nếu UI không hide controller. |

## 4. Performance

| ID | Yêu cầu | Note |
|---|---|---|
| P4.1 | Cold start ≤ 2s đến first content | Splash → Home đầu tiên. |
| P4.2 | Hot start ≤ 500ms | Từ background. |
| P4.3 | Frame drop ≤ 1% trên 60Hz device | Profile GPU rendering. |
| P4.4 | Memory peak ≤ 192MB cho TV box rẻ | Box VN 1-2GB RAM phổ biến. |
| P4.5 | APK size ≤ 30MB | Sideload qua USB / file manager. |
| P4.6 | Network: timeout ≤ 30s, retry exponential | NetworkLayer đã setup. |
| P4.7 | Image lazy + cache (Coil) | Đã dùng. |
| P4.8 | Database read on background thread | Room + Dispatchers.IO. |

## 5. Accessibility

| ID | Yêu cầu | Note |
|---|---|---|
| A5.1 | TalkBack support: contentDescription cho mọi icon-only button | Compose `Modifier.semantics`. |
| A5.2 | Screen reader đọc đúng thứ tự logic | Test với TalkBack. |
| A5.3 | Subtitle support trong Player | ExoPlayer SubtitleView. |
| A5.4 | Tăng font size theo system setting | Dùng `sp` không `dp` cho text. |
| A5.5 | High contrast mode (optional) | Cho user thị lực kém. |

## 6. Networking

| ID | Yêu cầu | Note |
|---|---|---|
| N6.1 | HTTPS only — không cleartext traffic | `android:usesCleartextTraffic="false"`. |
| N6.2 | Certificate pinning (optional, nâng cao) | Nếu paranoid. |
| N6.3 | NetworkSecurityConfig riêng cho TV | Có thể skip nếu không pin. |
| N6.4 | Detect offline trong < 5s | NetworkCallback đã setup. |
| N6.5 | Graceful degradation khi mất mạng | G1 overlay, retry button. |

## 7. State persistence

| ID | Yêu cầu | Note |
|---|---|---|
| S7.1 | Process death restoration | SavedStateHandle trong ViewModel. |
| S7.2 | Continue Watching persist | Room đã có. |
| S7.3 | Settings persist | DataStore Preferences. |
| S7.4 | Auth token persist (encrypted) | EncryptedSharedPreferences. |
| S7.5 | Crash → resume tại screen gần nhất | Splash → route correct. |

## 8. Security

| ID | Yêu cầu | Note |
|---|---|---|
| SE8.1 | Token + session_id KHÔNG log ra Logcat | redactHeader trong logging. |
| SE8.2 | Bearer Timfshare KHÔNG log | redactHeader. |
| SE8.3 | EncryptedSharedPreferences cho session | Đã. |
| SE8.4 | KHÔNG hardcode bearer trong source committed | Đọc từ env / keystore.properties. |
| SE8.5 | Verify APK signature trước khi cài update (Phương án A skip) | Out of scope V1. |

## 9. Distribution

| ID | Yêu cầu | Note |
|---|---|---|
| DI9.1 | APK universal (không split) | Đã. |
| DI9.2 | Signed v2/v3 | Build pipeline đã setup. |
| DI9.3 | versionCode tăng monotonically | gitVersion() trong app/build.gradle.kts. |
| DI9.4 | versionName Semver (vMAJOR.MINOR.PATCH) | Đã. |
| DI9.5 | Sideload instructions (manual install via TV File Manager hoặc USB) | Phương án A — đã document. |
| DI9.6 | Update notification mechanism (optional) | V1 skip — manual notify. |

## 10. Compatibility với các dòng TV

| ID | Yêu cầu | Note |
|---|---|---|
| C10.1 | Sony BRAVIA Android TV (2018+) | Android 7.0-10. |
| C10.2 | Sony BRAVIA Google TV (2021+) | Android 11+. |
| C10.3 | Xiaomi Mi TV / Mi Box S | Android 9-11, very common VN. |
| C10.4 | TCL Google TV (2022+) | Android 11+. |
| C10.5 | Sharp Aquos Android TV | Android 9-10. |
| C10.6 | LG WebOS — KHÔNG hỗ trợ | LG dùng webOS, không phải Android → không cài APK. |
| C10.7 | Samsung Tizen — KHÔNG hỗ trợ | Samsung dùng Tizen → không cài APK. |
| C10.8 | Vsmart, Casper, Asanzo (TV box VN) | Đa số AOSP Android 9-11. |
| C10.9 | Nvidia Shield TV | Android 9-11, premium. |
| C10.10 | Chromecast with Google TV | Android 10-12. |
| C10.11 | Amazon Fire TV — partial | Fire OS = Android fork; sideload phải bật ADB. |
| C10.12 | KaiOS TV — KHÔNG | Không phổ biến. |
| C10.13 | Resolution: 1280×720 (HD), 1920×1080 (FHD), 3840×2160 (4K) | Compose tự scale theo dpi. |
| C10.14 | DPI: hdpi (240), xhdpi (320), xxhdpi (480) — TV thường xhdpi | Asset đa density. |
| C10.15 | Screen size: 32"-85" — không cần đặc biệt | Compose responsive. |
| C10.16 | Orientation: landscape only | `screenOrientation="landscape"`. |
| C10.17 | KHÔNG support split-screen (TV không có) | Manifest exclude resizableActivity. |

## 11. Player-specific

| ID | Yêu cầu | Note |
|---|---|---|
| PL11.1 | Codec H.264 SD/HD | ExoPlayer mặc định. |
| PL11.2 | Codec H.265/HEVC | Hardware-accelerated trên TV box hiện đại. |
| PL11.3 | Container MP4, MKV, WebM | ExoPlayer support. |
| PL11.4 | Audio AAC, Opus | Mặc định. |
| PL11.5 | Subtitle SRT, ASS, WebVTT | Media3 subtitle. |
| PL11.6 | DRM Widevine (optional) | Skip cho V1. |
| PL11.7 | Resume từ saved position | Đã. |
| PL11.8 | Auto-advance Continue Watching | V1 skip. |
| PL11.9 | Audio passthrough Dolby/DTS (advanced) | Future. |
| PL11.10 | Variable playback speed (0.5x-2x) | V2. |

## 12. Lifecycle & Background

| ID | Yêu cầu | Note |
|---|---|---|
| L12.1 | Pause player khi onPause | Standard. |
| L12.2 | Release player resources khi onStop | Avoid memory leak. |
| L12.3 | Tránh crash khi orientation change | Compose state hoist + ViewModel. |
| L12.4 | Battery optimization not applicable | TV cắm điện. |
| L12.5 | Tránh wakelock không cần thiết | Player chỉ wake khi đang play. |

## 13. Telemetry / Analytics

| ID | Yêu cầu | Note |
|---|---|---|
| T13.1 | Anonymous client_id (UUID v4) | Không leak email user. |
| T13.2 | GA4 Measurement Protocol | Wave M3 đã có placeholder. |
| T13.3 | Opt-out trong Settings | V2. |
| T13.4 | Event budget: ≤ 1 event/sec | Tránh spam GA4. |

## 14. Internationalization

| ID | Yêu cầu | Note |
|---|---|---|
| I14.1 | values/strings.xml + values-vi/ + values-en/ | Đã có. |
| I14.2 | Pluralization correct | `<plurals>` cho count. |
| I14.3 | RTL support (optional, không cần cho VN) | V2. |
| I14.4 | Date/time format theo locale | TV system locale. |

## 15. Testing & QA

| ID | Yêu cầu | Note |
|---|---|---|
| Q15.1 | Unit test coverage ≥ 70% | Phase C đã setup. |
| Q15.2 | Instrumented test cho critical flow | Defer — cần emulator/device. |
| Q15.3 | Manual test catalog | 19_test-catalog.md. |
| Q15.4 | TV emulator test (Android Studio) | Pixel 8 → ATV emulator template. |
| Q15.5 | Real device test (≥ 2 dòng TV) | Pre-release blocker. |
