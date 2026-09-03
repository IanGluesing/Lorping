#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run LoRa Non Hopping Tests

pushd $SCRIPT_DIR/../../../src/

## Build for LoRa and Non Hopping
echo "Building for: LORA, NON-HOPPING"
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/modulation_config \
  --libraries $(pwd)/device_impl \
  --build-property compiler.cpp.extra_flags="-DSX1262_MODE_FSK=0 -DFREQUENCY_HOPPING_ENABLED=0" \
  --build-path ./build_out \
  bin/hopping_modem/

## Deploy Non Hopping LoRa Build
./deploy.sh A hopping_modem
./deploy.sh B hopping_modem

popd

## Run Non Hopping LoRa tests
$SCRIPT_DIR/ping_non_hopping.sh
$SCRIPT_DIR/curl_non_hopping.sh
$SCRIPT_DIR/audio_non_hopping.sh

# Run LoRa Hopping Tests

# pushd $SCRIPT_DIR/../../../src/

# ## Build for LoRa and Non Hopping
# echo "Building for: LORA, HOPPING"
# arduino-cli compile \
#   --fqbn arduino:mbed_giga:giga \
#   --libraries $(pwd)/hop_table \
#   --libraries $(pwd)/modulation_config \
#   --libraries $(pwd)/device_impl \
#   --build-property compiler.cpp.extra_flags="-DSX1262_MODE_FSK=0 -DFREQUENCY_HOPPING_ENABLED=1" \
#   --build-path ./build_out \
#   bin/hopping_modem/

# ## Deploy Non Hopping LoRa Build
# ./deploy.sh A hopping_modem
# ./deploy.sh B hopping_modem

# popd

# ## Run Non Hopping LoRa tests
# $SCRIPT_DIR/ping_hopping.sh