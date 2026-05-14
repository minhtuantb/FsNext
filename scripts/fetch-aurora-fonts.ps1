# SPDX-License-Identifier: Proprietary
# fetch-aurora-fonts.ps1 — download fonts required by the FsAurora design
#
# Runs from project root. Drops TTFs into resources/fonts/. CMake globs that
# directory into the qrc, so after running this script a fresh build bundles
# the fonts automatically.
#
# Usage:  pwsh scripts/fetch-aurora-fonts.ps1
# Re-run is safe — existing files are kept unless -Force is passed.

param([switch]$Force)

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'  # hide noisy progress bars

# Resolve to <repo>/resources/fonts regardless of where script was launched
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir
$fontsDir = Join-Path $projectRoot 'resources/fonts'

if (-not (Test-Path $fontsDir)) {
    New-Item -ItemType Directory -Path $fontsDir | Out-Null
}

# Fonts published on github.com/google/fonts under SIL OFL. Variable fonts
# where available (Geist, GeistMono) — one file covers the whole wght axis.
# Be Vietnam Pro has no variable, so we pull 4 static weights.
$fonts = @(
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/geist/Geist%5Bwght%5D.ttf';
       Out = 'Geist-Variable.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/geistmono/GeistMono%5Bwght%5D.ttf';
       Out = 'GeistMono-Variable.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/instrumentserif/InstrumentSerif-Regular.ttf';
       Out = 'InstrumentSerif-Regular.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/instrumentserif/InstrumentSerif-Italic.ttf';
       Out = 'InstrumentSerif-Italic.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Regular.ttf';
       Out = 'BeVietnamPro-Regular.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Medium.ttf';
       Out = 'BeVietnamPro-Medium.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-SemiBold.ttf';
       Out = 'BeVietnamPro-SemiBold.ttf' },
    @{ Url = 'https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Bold.ttf';
       Out = 'BeVietnamPro-Bold.ttf' }
)

$ok = 0; $skip = 0; $fail = 0

foreach ($f in $fonts) {
    $dest = Join-Path $fontsDir $f.Out
    if ((Test-Path $dest) -and -not $Force) {
        Write-Host ("[skip] {0}" -f $f.Out) -ForegroundColor DarkGray
        $skip++
        continue
    }
    try {
        Write-Host ("[get ] {0}" -f $f.Out) -ForegroundColor Cyan
        Invoke-WebRequest -Uri $f.Url -OutFile $dest -UseBasicParsing
        if ((Get-Item $dest).Length -lt 1024) {
            throw 'downloaded file is suspiciously small (<1 KB)'
        }
        $ok++
    } catch {
        Write-Host ("[fail] {0}  -  {1}" -f $f.Out, $_.Exception.Message) -ForegroundColor Red
        if (Test-Path $dest) { Remove-Item $dest -Force }
        $fail++
    }
}

Write-Host ""
Write-Host ("Done. downloaded={0} skipped={1} failed={2}" -f $ok, $skip, $fail) `
    -ForegroundColor ($(if ($fail -gt 0) { 'Yellow' } else { 'Green' }))
Write-Host ("Fonts in: {0}" -f $fontsDir)

if ($fail -gt 0) { exit 1 }
