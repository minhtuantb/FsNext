# StreamIX TV — PowerShell helper setup JAVA_HOME từ Android Studio JBR
#
# IMPORTANT — phải dot-source (chú ý dấu chấm + space đầu):
#     . .\setup-env.ps1
#
# Nếu run bằng:
#     .\setup-env.ps1
# thì env vars chỉ tồn tại trong script scope, KHÔNG set vào session của bạn.

$ErrorActionPreference = "Stop"

$candidates = @(
    "C:\Program Files\Android\Android Studio\jbr",
    "C:\Program Files\Android\Android Studio Hedgehog\jbr",
    "C:\Program Files\Android\Android Studio Iguana\jbr",
    "C:\Program Files\Android\Android Studio Jellyfish\jbr",
    "C:\Program Files\Android\Android Studio Koala\jbr",
    "C:\Program Files\Android\Android Studio Ladybug\jbr",
    "C:\Program Files\Eclipse Adoptium\jdk-17.0.13.11-hotspot",
    "C:\Program Files\Eclipse Adoptium\jdk-17.0.12.7-hotspot",
    "C:\Program Files\Eclipse Adoptium\jdk-17.0.11.9-hotspot",
    "C:\Program Files\Java\jdk-17",
    "$env:LOCALAPPDATA\Android\android-studio\jbr",
    "$env:USERPROFILE\.jdks\corretto-17",
    "$env:USERPROFILE\.jdks\temurin-17"
)

# Cũng quét dynamically Android Studio variants
$asPattern = Get-ChildItem -Path "C:\Program Files\Android" -Directory -ErrorAction SilentlyContinue `
    | Where-Object { $_.Name -like "Android Studio*" }
foreach ($as in $asPattern) {
    $candidates += "$($as.FullName)\jbr"
}

# Cũng quét dynamically Adoptium variants
$adoptiumPattern = Get-ChildItem -Path "C:\Program Files\Eclipse Adoptium" -Directory -ErrorAction SilentlyContinue `
    | Where-Object { $_.Name -like "jdk-17*" }
foreach ($jdk in $adoptiumPattern) {
    $candidates += $jdk.FullName
}

$found = $null
foreach ($p in $candidates) {
    if (Test-Path "$p\bin\java.exe") {
        $found = $p
        break
    }
}

if ($null -eq $found) {
    Write-Host "❌ Khong tim thay JDK 17 o cac duong dan quen thuoc." -ForegroundColor Red
    Write-Host ""
    Write-Host "Cai Android Studio (co san JBR 17): https://developer.android.com/studio"
    Write-Host "Hoac Temurin 17: https://adoptium.net/temurin/releases/?version=17" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Sau do set thu cong:"
    Write-Host '  $env:JAVA_HOME = "C:\path\to\jdk17"'
    Write-Host '  $env:Path = "$env:JAVA_HOME\bin;$env:Path"'
    return
}

# Set env cho session hiện tại
$env:JAVA_HOME = $found
$env:Path = "$env:JAVA_HOME\bin;$env:Path"

Write-Host "[OK] JAVA_HOME = $env:JAVA_HOME" -ForegroundColor Green
Write-Host ""

# Verify
& "$env:JAVA_HOME\bin\java.exe" -version
Write-Host ""

Write-Host "San sang chay build:"
Write-Host "  ./gradlew :app:assembleDebug"
Write-Host ""
Write-Host "TIP: de set permanent (1 lan thoi, khong phai chay script lai):"
$cmd = "[Environment]::SetEnvironmentVariable('JAVA_HOME', '$env:JAVA_HOME', 'User')"
Write-Host "  $cmd"
Write-Host ""
Write-Host "Sau do dong va mo lai PowerShell."
