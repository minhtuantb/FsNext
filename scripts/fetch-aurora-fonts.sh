#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# fetch-aurora-fonts.sh — download fonts required by the FsAurora design
#
# Drops TTFs into <repo>/resources/fonts/. CMake globs that directory into
# the qrc, so after running this a fresh build bundles the fonts.
#
# Usage:  bash scripts/fetch-aurora-fonts.sh [--force]

set -euo pipefail

FORCE=0
if [[ "${1:-}" == "--force" ]]; then FORCE=1; fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
FONTS_DIR="${PROJECT_ROOT}/resources/fonts"
mkdir -p "${FONTS_DIR}"

# url|output pairs (SIL OFL, source: github.com/google/fonts)
FONTS=(
  "https://github.com/google/fonts/raw/main/ofl/geist/Geist%5Bwght%5D.ttf|Geist-Variable.ttf"
  "https://github.com/google/fonts/raw/main/ofl/geistmono/GeistMono%5Bwght%5D.ttf|GeistMono-Variable.ttf"
  "https://github.com/google/fonts/raw/main/ofl/instrumentserif/InstrumentSerif-Regular.ttf|InstrumentSerif-Regular.ttf"
  "https://github.com/google/fonts/raw/main/ofl/instrumentserif/InstrumentSerif-Italic.ttf|InstrumentSerif-Italic.ttf"
  "https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Regular.ttf|BeVietnamPro-Regular.ttf"
  "https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Medium.ttf|BeVietnamPro-Medium.ttf"
  "https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-SemiBold.ttf|BeVietnamPro-SemiBold.ttf"
  "https://github.com/google/fonts/raw/main/ofl/bevietnampro/BeVietnamPro-Bold.ttf|BeVietnamPro-Bold.ttf"
)

ok=0; skip=0; fail=0
for entry in "${FONTS[@]}"; do
  url="${entry%%|*}"
  out="${entry##*|}"
  dest="${FONTS_DIR}/${out}"

  if [[ -f "${dest}" && "${FORCE}" -eq 0 ]]; then
    echo "[skip] ${out}"; skip=$((skip+1)); continue
  fi

  echo "[get ] ${out}"
  if curl -fsSL "${url}" -o "${dest}"; then
    # size sanity check — a 404 page is usually < 1 KB
    size=$(wc -c < "${dest}")
    if [[ "${size}" -lt 1024 ]]; then
      echo "[fail] ${out}  -  file suspiciously small (${size} bytes)" >&2
      rm -f "${dest}"; fail=$((fail+1))
    else
      ok=$((ok+1))
    fi
  else
    echo "[fail] ${out}" >&2
    rm -f "${dest}"; fail=$((fail+1))
  fi
done

echo
echo "Done. downloaded=${ok} skipped=${skip} failed=${fail}"
echo "Fonts in: ${FONTS_DIR}"
exit "${fail}"
