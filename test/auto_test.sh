#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Handle LoRa Tests
pushd $SCRIPT_DIR/test_scenarios/lora

./run_lora_tests.sh

# Return to test script dir
popd

# Handle FSK Tests
pushd $SCRIPT_DIR/test_scenarios/fsk

./run_fsk_tests.sh

# Return to test script dir
popd