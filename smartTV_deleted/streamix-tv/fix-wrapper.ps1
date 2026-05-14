# StreamIX TV — Fix gradle-wrapper.jar bị thiếu
#
# Chạy: . .\fix-wrapper.ps1   (dot-source) hoặc:  .\fix-wrapper.ps1
#
# Script tự download gradle-wrapper.jar từ Gradle source GitHub
# matching version với gradle-wrapper.properties (8.10.2).

$ErrorActionPreference = "Stop"

$wrapperDir = ".\gradle\wrapper"
$wrapperJar = "$wrapperDir\gradle-wrapper.jar"
$gradleVersion = "8.10.2"

if (Test-Path $wrapperJar) {
    Write-Host "[OK] gradle-wrapper.jar da co tai $wrapperJar" -ForegroundColor Green
    return
}

if (-not (Test-Path $wrapperDir)) {
    New-Item -ItemType Directory -Path $wrapperDir -Force | Out-Null
}

# URL: Gradle source GitHub raw
$jarUrl = "https://raw.githubusercontent.com/gradle/gradle/v$gradleVersion/gradle/wrapper/gradle-wrapper.jar"

Write-Host "Downloading gradle-wrapper.jar v$gradleVersion ..."
Write-Host "  URL: $jarUrl"
Write-Host ""

try {
    # PowerShell 5.1 và 7+ đều support Invoke-WebRequest
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $jarUrl -OutFile $wrapperJar -UseBasicParsing
    $size = (Get-Item $wrapperJar).Length
    Write-Host "[OK] Downloaded $wrapperJar ($size bytes)" -ForegroundColor Green
    Write-Host ""
    Write-Host "Bay gio chay build:"
    Write-Host "  ./gradlew :app:assembleDebug"
} catch {
    Write-Host "[FAIL] Khong download duoc tu GitHub." -ForegroundColor Red
    Write-Host "Error: $_"
    Write-Host ""
    Write-Host "Cach khac — boostrap qua Android Studio:" -ForegroundColor Yellow
    Write-Host "  1. Mo Android Studio"
    Write-Host "  2. File -> Open -> chon folder streamix-tv"
    Write-Host "  3. Doi Gradle sync xong (IDE tu download wrapper jar)"
    Write-Host ""
    Write-Host "Hoac neu co gradle global cai san:"
    Write-Host "  gradle wrapper --gradle-version $gradleVersion"
}
