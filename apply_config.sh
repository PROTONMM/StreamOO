#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v idf.py >/dev/null 2>&1; then
    echo 'ERROR: use VS Code ESP-IDF terminal (idf.py missing)' >&2
    exit 1
fi
# This script should be copied into the StreamOO root, next to CMakeLists.txt.
if [[ ! -f CMakeLists.txt ]]; then
    echo 'ERROR: run script from StreamOO project root (CMakeLists.txt not found)' >&2
    exit 1
fi
stamp="$(date +%Y%m%d-%H%M%S)"
[[ ! -e sdkconfig ]] || cp -a sdkconfig "sdkconfig.backup.$stamp"
[[ ! -e sdkconfig.defaults ]] || cp -a sdkconfig.defaults "sdkconfig.defaults.backup.$stamp"
# Defaults are supplied by this package; keep the file provided in project root.
# Remove old sdkconfig, since it is for esp32 target and overrides defaults.
rm -f sdkconfig
idf.py set-target esp32s3
idf.py reconfigure
printf '\nCHECK CONFIGURATION:\n'
grep -E '^CONFIG_(IDF_TARGET=|ESPTOOLPY_FLASHSIZE=|SPIRAM=|SPIRAM_MODE_OCT=|SPIRAM_SPEED_80M=|SPIRAM_USE_MALLOC=|ESP_DEFAULT_CPU_FREQ_MHZ=|STREAMOO_)' sdkconfig || true
printf '\nRun: idf.py build\n'
