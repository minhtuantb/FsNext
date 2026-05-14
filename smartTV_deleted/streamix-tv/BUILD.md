# StreamIX TV — Build & Run Guide

## ⚠️ Lỗi `JAVA_HOME is not set` (Windows)

Trước khi chạy `./gradlew` ở terminal, phải có JDK 17 và set `JAVA_HOME`.

### Cách nhanh nhất — dùng helper script

```powershell
cd D:\Work\FsNext\smartTV\streamix-tv
. .\setup-env.ps1   # CHÚ Ý: dấu chấm + space đầu
./gradlew :app:assembleDebug
```

Script tự tìm JDK 17 ở Android Studio JBR hoặc Temurin/Corretto.

### Set permanent (đề xuất)

```powershell
# Cài Android Studio Hedgehog+ → tự có JBR 17
[Environment]::SetEnvironmentVariable("JAVA_HOME", "C:\Program Files\Android\Android Studio\jbr", "User")

# Đóng & mở lại PowerShell
java -version    # phải hiện 17.x
```

### Hoặc build trong Android Studio (không cần JAVA_HOME CLI)

File → Open → chọn `streamix-tv` folder → Run.

---

## Prerequisites

| Tool | Version | Note |
|------|---------|------|
| JDK | 17 | Bắt buộc cho AGP 8.5+ |
| Android SDK | API 34 + build-tools 34.0.0 | `compileSdk = 34` |
| Android Studio | Hedgehog (2023.1.1) hoặc mới hơn | Hỗ trợ Compose 1.6+ + AGP 8.5 |
| Gradle | 8.9 (qua wrapper) | Auto-bootstrap khi `./gradlew` lần đầu |

## First-time setup

```bash
# 1. Clone & cd
git clone <repo> streamix-tv && cd streamix-tv

# 2. Tạo local.properties (hoặc Android Studio tự tạo khi mở)
cat > local.properties << 'EOF'
sdk.dir=/path/to/Android/Sdk
EOF

# 3. (Lần đầu) Bootstrap gradle wrapper jar nếu chưa có
gradle wrapper --gradle-version 8.9
# HOẶC mở project trong Android Studio — IDE sẽ download wrapper tự động

# 4. Sync Gradle (chạy trong Android Studio hoặc CLI)
./gradlew tasks
```

## Build commands

```bash
# Debug APK (signing tự dùng debug key — chỉ để dev)
./gradlew :app:assembleDebug
# Output: app/build/outputs/apk/debug/app-debug.apk (~25 MB)

# Release APK (cần keystore + env vars)
export STREAMIX_APP_KEY=dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt
export KEYSTORE_PATH=$PWD/signing/release.jks
export KEYSTORE_PASSWORD=<masked>
export KEY_ALIAS=streamix-tv
export KEY_PASSWORD=<masked>
./gradlew :app:assembleRelease
# Output: app/build/outputs/apk/release/app-universal-release.apk (~22 MB)

# Verify signature v2 + v3
$ANDROID_HOME/build-tools/34.0.0/apksigner verify --print-certs \
  app/build/outputs/apk/release/app-universal-release.apk

# Run unit tests
./gradlew :core:common:test

# All checks (lint + test + build)
./gradlew check
```

## Install lên TV

```bash
# Mi Box S / TCL Android TV / Sharp Aquos
adb connect <ip-tv>:5555
adb install -r app/build/outputs/apk/debug/app-debug.apk

# FPT Play Box / MyTV Box (Android 7.x): uninstall trước nếu lần đầu
adb uninstall vn.streamix.tv.debug 2>/dev/null
adb install app/build/outputs/apk/debug/app-debug.apk

# Mở app
adb shell am start -n vn.streamix.tv.debug/vn.streamix.tv.MainActivity
```

## Module dependency graph

```
:app
├── :feature:splash → :core:* (data + ui + domain)
├── :feature:auth   → :core:*
├── :feature:home   → :core:*
├── :feature:browse → :core:*
├── :feature:player → :core:* (+ Media3 deps)
├── :feature:search → :core:*
├── :feature:settings → :core:*
└── :feature:onboarding → :core:*

:core:ui     → :core:domain (cho models trong components nếu cần)
:core:data   → :core:network + :core:domain
:core:network → :core:common + :core:domain
:core:domain → :core:common
:core:common → (no deps — pure Kotlin utility)

:build-logic:convention → AGP + Kotlin plugin (cho convention plugins)
```

## Compose-TV compatibility

| Android version | API | Status | Note |
|-----------------|-----|--------|------|
| Android 5.0 (Lollipop) | 21 | ✅ minSdk | TV box VN cũ |
| Android 5.1 | 22 | ✅ | |
| Android 6.0 | 23 | ✅ | FPT Play Box S |
| Android 7.x (Nougat) | 24-25 | ✅ | MyTV Box, Vinabox |
| Android 8.x (Oreo) | 26-27 | ✅ | |
| Android 9 (Pie) | 28 | ✅ | Mi Box S 2018 |
| Android 10 | 29 | ✅ | |
| Android 11 | 30 | ✅ | Sony Bravia 2020+ |
| Android 12 (TV) | 31 | ✅ | SplashScreen API mới |
| Android 13-14 (TV) | 33-34 | ✅ targetSdk | |

## TV Brand test matrix (Phase 5)

| Brand | Model | Android version | Status |
|-------|-------|-----------------|--------|
| Xiaomi Mi Box S | 1st gen | 9 | Primary |
| Xiaomi Mi TV Stick | 4K | 11 | Primary |
| FPT Play Box S | T550 | 7.1 | Primary (TV box VN) |
| TCL Android TV | C715 series | 11 | Primary |
| Sony Bravia | A8H | 9 | Secondary |
| Chromecast w/ Google TV | 4K | 12 | Tertiary |
| Sharp Aquos | LE3DXX | 9 | Secondary |
| Hisense Vidaa | — | — | Out of scope V1 (Vidaa OS, không Android) |
| Samsung Tizen | — | — | Out of scope V1 |
| LG WebOS | — | — | Out of scope V1 |

## CI / Release pipeline

Reference: `D:\Work\FsNext\smartTV\documents\04_apk-va-update.md` rev3 §4.7 cho full GitHub Actions YAML.

Stages:
1. Checkout với fetch-depth 0 (cho git tag versioning)
2. Setup JDK 17 (temurin)
3. Decode keystore từ secret base64
4. `./gradlew assembleRelease`
5. `apksigner verify`
6. SHA-256 hash
7. SCP upload lên `tv.streamix.vn:/var/www/streamix-tv/apk/`
8. SSH ssh vào server chạy `update-portal.sh` (update symlink + QR)
9. Slack webhook notify channel `#streamix-tv-releases`

## Troubleshooting

### "App not installed"
- Signature mismatch giữa version cũ + mới → gỡ app cũ trước khi cài bản mới
- Hoặc `versionCode` mới ≤ version đang cài → bump versionCode

### Compose preview không hiển thị
- Android Studio Hedgehog hoặc mới hơn (cần Compose 1.6+ runtime)
- Kotlin Compiler 2.0+ với Compose plugin riêng (đã setup ở `kotlin-compose` plugin trong libs.versions.toml)

### `Failed to resolve: com.android.tools.build:gradle`
- AGP 8.5.2 cần JDK 17. Check `java -version`
- Nếu đang dùng JDK 11, switch sang JDK 17 trong Android Studio Settings → Build, Execution, Deployment → Build Tools → Gradle → Gradle JDK

### TV không nhận remote D-pad
- Test với `adb shell input keyevent KEYCODE_DPAD_DOWN` để verify key event đến app
- Check Manifest có `<uses-feature android:name="android.software.leanback" android:required="true">`

### `Unknown command-line option '--jvm-vendor'`

Lỗi này xảy ra khi Kotlin compiler emit `-Xjvm-vendor` flag mà javac không recognize. Root causes thường gặp:

1. **Gradle quá cũ** — Gradle < 8.10 chưa fix toolchain vendor handling.
   ```bash
   ./gradlew --version  # check Gradle version
   ./gradlew wrapper --gradle-version 8.10.2  # update wrapper
   ```

2. **AGP + Kotlin version mismatch** — AGP < 8.6 không hiểu vendor handling của Kotlin 2.0+.
   - Fix: upgrade AGP 8.6.1 + Kotlin 2.0.21 (đã set trong `libs.versions.toml`).

3. **JDK version sai** — Phải chạy với JDK 17 (không phải JDK 8/11).
   ```bash
   java -version  # phải là 17.x
   ```
   Nếu dùng Android Studio: Settings → Build, Execution, Deployment → Build Tools → Gradle → Gradle JDK = 17.

4. **gradle.properties đã có**:
   ```properties
   kotlin.jvm.target.validation.mode=warning
   ```
   để tắt strict toolchain check.

5. **Clean build** sau khi update:
   ```bash
   ./gradlew clean
   ./gradlew --stop  # stop daemons
   ./gradlew :app:assembleDebug
   ```
