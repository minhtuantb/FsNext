---
title: FsNext for Smart TV — Bộ tài liệu phân tích & khuyến nghị
version: 0.1 (draft)
date: 2026-05-04
owner: FsNext team
---

# FsNext for Smart TV — Hồ sơ chuyển đổi

Tài liệu này mô tả phương án chuyển dự án **FsNext (Fshare Tool v6.0 — Qt6/QML/C++ desktop)** sang nền tảng **Smart TV** với các yêu cầu trọng tâm:

1. Phân phối qua **file APK cài trực tiếp** (sideload), không phụ thuộc Google Play.
2. Cơ chế **tự nâng cấp (in-app update)** ngay trên TV.
3. **Player media nhúng** (xem phim trực tiếp trong app, không phải mở app ngoài như VLC/MPC ở bản desktop).

## Mục lục

| # | Tài liệu | Mục đích |
|---|----------|----------|
| 01 | [Phân tích hiện trạng](01_phan-tich-hien-trang.md) | Tổng kết codebase desktop, tính năng nào port, tính năng nào loại bỏ |
| 02 | [Khuyến nghị kiến trúc](02_khuyen-nghi-kien-truc.md) | Kiến trúc target, layer mapping desktop → TV |
| 03 | [Lựa chọn công nghệ](03_lua-chon-cong-nghe.md) | So sánh các stack (Qt-Android, native Android, Kotlin Compose-TV, Flutter-TV) — khuyến nghị stack |
| 04 | [APK & cơ chế nâng cấp](04_apk-va-update.md) | Build APK, ký, sideload, in-app update (tự tải + tự cài) |
| 05 | [Player media nhúng](05_player-media-nhung.md) | Khuyến nghị ExoPlayer/libVLC, audio passthrough, subtitle, codec |
| 06 | [UX 10-foot & D-pad](06_ux-10foot-dpad.md) | Leanback design, focus model, remote control, keyboard |
| 07 | [Lộ trình triển khai](07_lo-trinh-trien-khai.md) | 6 phase, mốc thời gian, resource ước lượng |
| 08 | [Rủi ro & giảm thiểu](08_rui-ro-va-giam-thieu.md) | Các rủi ro pháp lý, kỹ thuật, vận hành |
| 09 | [Design Specification](09_design-spec.md) | Toàn bộ yêu cầu chức năng + design tokens + catalog screens + user flows — **gửi đội Design** |
| 10 | [Design Handoff Requirements](10_design-handoff-requirements.md) | Yêu cầu export đầy đủ từ Design → Engineering: Figma, tokens JSON, assets, prototype, checklist 60 mục |
| 11 | [Audit Handoff v1.0](11_review-handoff-v1.md) | **✅ ACCEPT** — Audit gói "StreamIX TV" v1.0 đã pass; chỉ còn technical fix cho v1.1 |
| 12 | [Scope Decisions V1](12_scope-decisions-v1.md) | **✅ ALL APPROVED** — Brand = StreamIX TV; package = `vn.streamix.tv`; cắt 4 module; internal APK + Phương án A (manual notify/install) |
| 13 | [Implementation Spec V1](13_implementation-spec-v1.md) | **📐 BLUEPRINT v1.1** — Spec 18 screen × **Open API Gateway** (oapi.fshare.vn /v1) × tokens × behaviors × ViewModel skeleton. Đã migrate sang API gateway mới (JWT + X-App-Key, response envelope `{data, meta, request_id}`) |
| 14 | [Screen Catalog (flat list)](14_screen-catalog.md) | **📋 CATALOG** — Liệt kê ĐẦY ĐỦ: 9 root + 4 sub + 2 onboarding + 4 player overlays + 7 dialogs + 3 global overlays + 5 banner + 47 state variants + 11 snackbar instances = **~80 frames Figma** designer cần thiết kế |
| 15 | [Readiness Checklist](15_readiness-checklist.md) | **🟡 90% READY** — đánh giá readiness; 4 P0 blockers + 6 P1 coordination; action plan 2 tuần |
| 16 | [Implementation Summary](16_implementation-summary.md) | **✅ V1 codebase COMPLETE** — 91+ Kotlin files, 18 modules, all ~80 frames implemented; sẵn sàng build trong Android Studio |
| 17 | [API Migration Analysis](17_api-migration-analysis.md) | **🟢 RECOMMEND MIGRATE** — Phân tích so sánh Open API Gateway vs Legacy+Timfshare+Sheets; solve B1 blocker; effort 2 tuần |

## TL;DR khuyến nghị

- **Stack**: viết lại UI bằng **Kotlin + Jetpack Compose for TV** (AndroidX Leanback đang deprecated, Compose-TV là hướng chính thức của Google từ 2024). Reuse logic API/transfer engine bằng cách **port C++ TransferEngine sang Android NDK** (giữ libcurl + đa segment), hoặc viết lại bằng Kotlin coroutines + OkHttp.
- **APK**: ký bằng key riêng, host APK trên CDN của Fshare (`tv-update.fshare.vn/latest.json` + `latest.apk`), đặt `targetSdk=34`, `minSdk=21` (Android TV 5.0+ đủ phủ Sony/Sharp/TCL/Xiaomi 2018+).
- **Update**: dùng `PackageInstaller` API + `REQUEST_INSTALL_PACKAGES` permission cho self-update; KHÔNG ép user mở "Unknown sources" mỗi lần — chỉ cần grant 1 lần.
- **Player**: **ExoPlayer (Media3)** là lựa chọn mặc định. Dùng libVLC-Android làm fallback cho format lạ (mkv với codec hiếm, RMVB, ass subtitle phức tạp).
- **Bỏ khỏi TV build**: Upload, Chrome extension, RSS auto-download, subtitle search dạng nhập keyboard nặng. **Giữ + nâng cấp**: Login, browse files, download (tải về USB/storage), **stream xem trực tiếp** (mới).

## Cách dùng bộ tài liệu

Đọc theo thứ tự 01 → 08. Mỗi tài liệu độc lập đủ để review riêng. Quyết định kiến trúc nằm ở 02–03; quyết định sản phẩm (cắt/giữ tính năng) nằm ở 01 và 06.
