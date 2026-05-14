---
title: 03 — So sánh & lựa chọn công nghệ
date: 2026-05-04
---

# 03. Lựa chọn công nghệ

## 3.1 Bài toán

- Phát hành APK chạy trên Android TV / Google TV / FireTV / Tizen / WebOS / một số TV Box Việt (FPT Play Box, MyTV Box).
- Tái sử dụng tối đa logic backend của bản desktop nếu có thể.
- Có embedded media player chất lượng cao.
- Build APK ký sẵn, phân phối ngoài Play Store (sideload), tự update.

## 3.2 Phạm vi target nền tảng

| Nền tảng | Kiến trúc CPU | OS | Phân phối | Có hỗ trợ? |
|----------|---------------|----|-----------|----|
| Android TV (Sony, Xiaomi, TCL, Sharp) | ARMv7/ARMv8 | Android 5.0+ | APK sideload OK | **CÓ** — chính |
| Google TV (Chromecast w/Google TV, mới hơn) | ARMv8 | Android 10+ | APK sideload OK | **CÓ** — chính |
| FireTV (Amazon) | ARMv8 | Fork Android (Fire OS) | APK sideload OK | **CÓ** — phụ, cần test |
| TV Box Việt (FPT Play Box, MyTV Box, Vinabox…) | ARMv8 | Android TV/AOSP | APK sideload OK | **CÓ** — chính (thị trường VN) |
| Smart TV Tizen (Samsung) | ARMv7 | Tizen | Tizen Studio + .tpk, KHÔNG APK | KHÔNG (V2+) |
| Smart TV WebOS (LG) | ARMv7 | WebOS | .ipk, KHÔNG APK | KHÔNG (V2+) |
| Smart TV Vidaa (Hisense) | ARMv7 | Vidaa OS | App store riêng | KHÔNG |

**Quyết định**: V1 chỉ hỗ trợ **Android-based TV/Box**. Yêu cầu user: APK + cài. Tizen/WebOS là roadmap V2 (cần codebase web — Vue/React + Tizen Studio), KHÔNG cùng codebase với V1.

## 3.3 Ma trận stack lựa chọn cho V1 Android-TV

| Tiêu chí | A. Native Kotlin + Compose-TV | B. Qt 6 for Android (port nguyên QML) | C. Flutter for TV | D. React Native TV |
|----------|-------------------------------|---------------------------------------|-------------------|-------------------|
| Tái sử dụng C++ logic | Có (qua NDK nếu muốn) | **Tối đa** (Qt cross-platform) | Có (qua FFI) | Có (qua C++ TurboModules) |
| Tái sử dụng QML UI | KHÔNG | **Có** (~70%) | KHÔNG | KHÔNG |
| TV focus model (D-pad) | **Hỗ trợ tốt nhất** (Compose-TV native) | Yếu, phải tự code KeyNavigation | Trung bình (gói flutter_tv chưa trưởng thành) | Có (react-native-tvos) |
| ExoPlayer integration | **Trực tiếp** | Phức tạp (QtAV/QMediaPlayer hạn chế codec) | Qua plugin (video_player) | Qua plugin |
| Build size APK | 15–25 MB | **60–120 MB** (Qt runtime nặng) | 25–40 MB | 25–40 MB |
| Animation/UI mượt 60fps | **Cao** | Trung bình (QML render path qua Vulkan tốt nhưng overhead) | Cao | Trung bình |
| Cộng đồng + tài liệu TV | **Lớn**, Google chính chủ | Rất nhỏ | Nhỏ | Trung bình |
| Học mới cho team Qt | Lớn (Kotlin + Compose) | **Không** | Lớn (Dart) | Lớn (JS/TS + RN) |
| Material 3 + TV guidelines | **Có** sẵn | Phải tự code lại | Material có, TV gloss thiếu | Thiếu |
| Time-to-MVP | Trung bình (~3 tháng) | **Nhanh nhất** (~6–8 tuần nếu QML port chạy) | Trung bình | Trung bình |
| Maintenance dài hạn | **Rất tốt** | Rủi ro (Qt cho TV không phải priority của Qt Group) | Trung bình | Rủi ro (Meta giảm đầu tư RN-tvos) |

### 3.3.1 Đánh giá chi tiết

**A. Native Kotlin + Jetpack Compose for TV** — khuyến nghị
- Ưu: hệ sinh thái TV mạnh nhất; ExoPlayer/Media3 native; focus engine của Compose-TV xử lý D-pad chuẩn TalkBack/Leanback; Material 3 design tokens; tài liệu Google chính thống; dễ tuyển dev Android có sẵn ở VN.
- Nhược: viết lại UI 100%; team Qt cần học Kotlin/Compose (~2–4 tuần ramp-up).
- Khuyến nghị: **chọn**.

**B. Qt 6 for Android (port nguyên QML)**
- Ưu: tái sử dụng nhiều nhất; team không phải học stack mới.
- Nhược nghiêm trọng:
  - **Focus / D-pad**: QML `KeyNavigation` không phải native TV; không có Leanback equivalent. Phải tự build focus manager → tốn nhiều effort, dễ buggy với bàn phím TV.
  - **APK size**: Qt cần share libs (~50–80 MB) cho 4 ABI → APK 100+ MB. TV có 8–32 GB eMMC, ~95% chỉ còn 4 GB trống — APK nặng dễ bị từ chối cài.
  - **Codec**: `QMediaPlayer` Android backend dùng `MediaCodec` nhưng kém ExoPlayer ở: subtitle, DRM, adaptive streaming, audio passthrough.
  - **Long-term**: Qt cho TV không có roadmap riêng; rủi ro vendor lock-in.
- Khuyến nghị: **không chọn cho V1**. Có thể giữ Qt cho desktop song song, nhưng TV nên độc lập.

**C. Flutter for TV**
- Ưu: hot reload, animation đẹp.
- Nhược: TV support chỉ là community plugin, không official từ Google. ExoPlayer integration qua plugin có giới hạn (ví dụ DRM, subtitle ASS yếu). Focus management phải custom rất nhiều.
- Khuyến nghị: **không chọn**.

**D. React Native (react-native-tvos)**
- Ưu: nếu team có sẵn JS/TS.
- Nhược: Meta không còn duy trì react-native-tvos chính thức (fork community); rủi ro khi nâng cấp RN core.
- Khuyến nghị: **không chọn**.

## 3.4 Quyết định stack V1

```
Ngôn ngữ          : Kotlin 2.0+
UI                : Jetpack Compose for TV (1.0+) + Material 3
Architecture      : MVVM + AAC + Hilt DI
Async             : Kotlin Coroutines + Flow
Network           : Retrofit 2 + OkHttp 4 (Range request cho download)
Persistence       : Room 2.6 + DataStore (Preferences)
Player            : Media3 ExoPlayer 1.4+ (primary) / libVLC-Android (fallback)
OAuth             : AppAuth-Android + Device Flow custom
Crash             : Firebase Crashlytics (hoặc Sentry self-host)
Background work   : WorkManager + Foreground Service
Build             : Gradle 8.7+, Android Gradle Plugin 8.5+, Kotlin DSL
Min SDK           : 21 (Android 5.0 — phủ ~99% TV box VN)
Target SDK        : 34 (Android 14)
```

## 3.5 Roadmap mở rộng nền tảng (V2+)

- **V2 — Tizen + WebOS**: viết lại UI bằng web app (Vue 3 hoặc lit). Reuse logic API qua TypeScript. Thực tế là một codebase **thứ hai**, không cùng app với V1.
- **V3 — Mobile companion app**: cùng codebase Kotlin với TV (KMP — Kotlin Multiplatform), share `core/domain` và `core/data`. Mobile app làm: QR login cho TV, cast video, push notification download xong.

## 3.6 Phụ thuộc cụ thể (build.gradle.kts)

```kotlin
// app/build.gradle.kts (excerpt)
dependencies {
    // Compose for TV
    implementation("androidx.tv:tv-foundation:1.0.0-rc01")
    implementation("androidx.tv:tv-material:1.0.0-rc01")
    implementation(platform("androidx.compose:compose-bom:2024.09.00"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.material3:material3")

    // Architecture
    implementation("androidx.hilt:hilt-navigation-compose:1.2.0")
    implementation("com.google.dagger:hilt-android:2.51")
    ksp("com.google.dagger:hilt-compiler:2.51")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.6")

    // Network
    implementation("com.squareup.retrofit2:retrofit:2.11.0")
    implementation("com.squareup.retrofit2:converter-moshi:2.11.0")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
    implementation("com.squareup.okhttp3:logging-interceptor:4.12.0")

    // Persistence
    implementation("androidx.room:room-runtime:2.6.1")
    implementation("androidx.room:room-ktx:2.6.1")
    ksp("androidx.room:room-compiler:2.6.1")
    implementation("androidx.datastore:datastore-preferences:1.1.1")

    // Player
    implementation("androidx.media3:media3-exoplayer:1.4.1")
    implementation("androidx.media3:media3-exoplayer-hls:1.4.1")
    implementation("androidx.media3:media3-exoplayer-dash:1.4.1")
    implementation("androidx.media3:media3-ui:1.4.1")
    implementation("androidx.media3:media3-session:1.4.1")
    // libVLC fallback (chỉ thêm khi cần)
    // implementation("org.videolan.android:libvlc-all:3.6.0")

    // OAuth
    implementation("net.openid:appauth:0.11.1")

    // Background
    implementation("androidx.work:work-runtime-ktx:2.9.1")

    // Crash
    implementation(platform("com.google.firebase:firebase-bom:33.3.0"))
    implementation("com.google.firebase:firebase-crashlytics")
}
```

## 3.7 Cảnh báo & gotchas

- **`androidx.tv:tv-material`** đang ở `1.0.0-rc01` (đầu 2024). Nếu rủi ro RC, có thể fall back về `androidx.leanback:leanback` cho V1 — nhưng Leanback đã được Google đánh dấu maintenance-only. Compose-TV là tương lai; bám lấy nó.
- **TV Box Việt (FPT Play Box, MyTV Box)** thường chạy Android TV bản cũ (5.1 / 7.1). Phải test thực thiết bị, không tin emulator.
- **ExoPlayer audio passthrough** chỉ hoạt động qua HDMI digital out; phải khai báo `AudioCapabilities` đúng để truyền AC3/EAC3/DTS qua AVR.

## 3.8 Kết luận chương 3

Stack V1: **Kotlin + Jetpack Compose for TV + Media3 ExoPlayer**. Không port Qt sang TV — cost focus model + APK size + maintenance không bõ. Logic Domain/Repository port 1-1 từ desktop sang Kotlin (~2 tuần). UI 10-foot viết mới.

Tiếp theo: APK & cơ chế update ([04](04_apk-va-update.md)).
