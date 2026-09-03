#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Go to ping test dir
pushd $SCRIPT_DIR/../../host_computer/tun_ping_test

if sudo ./env/bin/python3 tone_test.py; then
    echo "AUDIO TEST SUCCESS"
else
    echo "AUDIO TEST FAILED"
    exit 1
fi

# Return to test script dir
popd