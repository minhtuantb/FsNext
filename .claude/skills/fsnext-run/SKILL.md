---
name: fsnext-run
description: Build and run the FsNext desktop app (Qt6/QML/C++ Fshare client on Windows). Use when asked to build, run, launch, start, smoke-test, or screenshot the FsNext app, or to verify a change works in the real app. Covers CMake/Ninja build via scripts/build.bat or presets, killing stale single-instance processes, launching output/FsNext.exe, and qmllint for QML.
---

# Build & Run FsNext

FsNext là Fshare desktop client: C++17 + Qt 6 QML, build bằng CMake + Ninja + MSVC, deps qua vcpkg.
Windows-first. Exe đích: `output/FsNext.exe`.

## Yêu cầu môi trường
- Qt **6.8.3 msvc2022_64** (mặc định `C:\Qt\6.8.3\msvc2022_64`; override bằng env `QT_MSVC_ROOT`).
- Visual Studio 2022 (BuildTools/Community/Pro/Enterprise) — `scripts/build.bat` tự dò `vcvars64.bat` (vswhere → các path phổ biến → env `VSVARS`).
- CMake ≥ 3.24 + Ninja; vcpkg (`VCPKG_ROOT`).

## Build (ưu tiên cách 1)

**Cách 1 — script lo hết env (khuyến nghị):** chạy từ gốc repo.
```
cmd.exe /c "scripts\build.bat"
```
> ⚠ Gọi qua PowerShell, KHÔNG qua Bash tool: `& cmd.exe /c "scripts\build.bat"` (Bash `cmd.exe /c` bị nuốt,
> chỉ in banner rồi exit 0 mà không build). Build.bat in `[PASS] Build successful` khi xong; artifact ở `output/FsNext.exe`.
Script: kiểm tra Qt → load MSVC env → `cmake --preset msvc2022` → `cmake --build build --config Release -j 8`.
Log: `build-log.txt` (gitignored). Thành công in `[PASS] Build successful!`, exit 0.

**Cách 2 — thủ công** (khi MSVC env đã load trong shell hiện tại):
```
cmake --preset msvc2022
cmake --build --preset release          # hoặc: cmake --build build --config Release -j 8
```
Preset khác: `msvc2022-debug` (→ build-debug/), `msvc2022-production` (→ build-production/).

## Run / smoke test

App là single-instance → luôn kill bản cũ trước, nếu không lệnh thứ 2 chỉ forward rồi thoát.
```
taskkill /F /IM FsNext.exe 2>NUL
output\FsNext.exe
```
Từ PowerShell, bắt log khởi động:
```powershell
Set-Location "D:\Work\FsNext\output"; & .\FsNext.exe 2>&1 | Select-Object -First 50; "ExitCode: $LASTEXITCODE"
```
Kiểm tra đang chạy: `tasklist /FI "IMAGENAME eq FsNext.exe"`.

## Screenshot verify
Sau khi launch, dùng công cụ chụp màn hình của harness để chụp cửa sổ FsNext (xác nhận UI render, không
crash). Đặt tên file `verify-*.png` (đã gitignore). Kill app sau khi chụp xong.

## Lint QML (không cần build)
qmllint từ bản Qt mingw, phải truyền `-I qml` (và `-I <Qt>/qml` nếu thiếu module Qt):
```
"C:\Qt\6.8.3\mingw_64\bin\qmllint.exe" -I qml qml/Fshare/Pages/<Page>.qml
```

## Tests
```
ctest --test-dir build --output-on-failure
```
(8 test: file_cache_db, file_cache_service, budget_manager, speed_meter, fshare_url, filename_sanitizer,
filename_resolver, resolve_proxy_url.)

## Gỡ rối nhanh
- **Qt not found** → set `QT_MSVC_ROOT`.
- **vcvars64 not found** → set env `VSVARS` trỏ tới `vcvars64.bat`.
- **vcpkg deps lỗi** → set `VCPKG_ROOT`, đảm bảo toolchain file đúng trong preset.
- **App mở rồi tắt ngay / không thấy cửa sổ** → có instance cũ đang chạy: `taskkill /F /IM FsNext.exe`.
- **Đổi/ thêm file C++** → cập nhật danh sách source trong `CMakeLists.txt` rồi configure lại.
