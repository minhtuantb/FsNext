# run-suite.ps1 — build + chạy QtTest (CTest) + thu kết quả vào 1 thư mục run.
# Dùng: powershell -File qa\scripts\run-suite.ps1 -Filter "Vault|Crypto" -Module vault
# (Filter là regex ctest -R; bỏ trống = chạy tất cả.)

param(
  [string]$Filter = "",
  [string]$Module = "vault"
)
$ErrorActionPreference = "Continue"
$repo  = "D:\Work\FsNext"
$date  = Get-Date -Format "yyyy-MM-dd"
# r-số: tăng dần nếu trùng ngày
$base  = Join-Path $repo "qa\runs"
$n = 1; while (Test-Path (Join-Path $base "$date-$Module-r$n")) { $n++ }
$run   = Join-Path $base "$date-$Module-r$n"
$ev    = Join-Path $run "evidence"
New-Item -ItemType Directory -Force -Path $ev | Out-Null

# 1) Build (cần VCPKG_ROOT)
if (-not $env:VCPKG_ROOT) { $env:VCPKG_ROOT = "D:\OneDrive - FPT Corporation\Work\Fshare\tool\vcpkg" }
Write-Host "[run-suite] Building..."
cmd /c "$repo\scripts\build.bat" *> (Join-Path $ev "build.log")
$buildOk = ($LASTEXITCODE -eq 0)

# 2) CTest
Write-Host "[run-suite] Running ctest (filter='$Filter')..."
$ctestArgs = @("--test-dir", "$repo\build", "--output-on-failure")
if ($Filter) { $ctestArgs += @("-R", $Filter) }
& ctest @ctestArgs *> (Join-Path $ev "ctest.log")
$ctestOk = ($LASTEXITCODE -eq 0)

# 3) Crash check (10') từ Windows Event Log
$crash = Get-WinEvent -FilterHashtable @{LogName='Application'; Level=2; StartTime=(Get-Date).AddMinutes(-10)} -ErrorAction SilentlyContinue |
  Where-Object { $_.Message -match 'FsNext' } | ForEach-Object { ($_.Message -split "`n")[0..3] -join ' | ' }
$crash | Out-File (Join-Path $ev "crash-events.txt")

# 4) results.md skeleton
$failing = Select-String -Path (Join-Path $ev "ctest.log") -Pattern "Failed|\*\*\*" -ErrorAction SilentlyContinue | ForEach-Object { $_.Line }
@"
# Run: $date-$Module-r$n

- Build: $(if($buildOk){'OK'}else{'FAIL'})
- CTest: $(if($ctestOk){'ALL PASS'}else{'HAS FAILURES'}) (filter='$Filter')
- Crash events (10'): $(if($crash){'CÓ — xem evidence/crash-events.txt'}else{'không'})

## Cần điền: map case → kết quả → bug
| Case ID | Kết quả | Bug |
|---|---|---|
| … | PASS/FAIL | — |

## CTest failures (nếu có)
$($failing -join "`n")

> Mỗi FAIL → tạo qa/reports/bugs/VLT-BUG-####.md (status: NEW) + cập nhật BUG-INDEX.md
"@ | Out-File (Join-Path $run "results.md") -Encoding utf8

Write-Host "[run-suite] Done → $run"
Write-Host "  build=$buildOk ctest=$ctestOk crash=$([bool]$crash)"
