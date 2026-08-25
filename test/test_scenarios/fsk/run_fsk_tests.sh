#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run FSK Non Hopping Tests

pushd $SCRIPT_DIR/../../../src/

## Build for FSK and Non Hopping
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --libraries $(pwd)/device_impl \
  --build-property compiler.cpp.extra_flags="-DMODE_FSK=1 -DFREQUENCY_HOPPING_ENABLED=0" \
  --build-path ./build_out \
  --output-dir ./binaries_out \
  bin/hopping_modem/

## Deploy Non Hopping FSK Build
./deploy.sh A hopping_modem
./deploy.sh B hopping_modem

popd

## Run Non Hopping FSK tests
$SCRIPT_DIR/ping_non_hopping.sh
$SCRIPT_DIR/curl_non_hopping.sh

# Run FSK Hopping Tests