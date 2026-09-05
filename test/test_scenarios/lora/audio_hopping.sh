#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Go to ping test dir
pushd $SCRIPT_DIR/../../host_computer/tun_ping_test

echo "RUNNING AUDIO HOPPING TEST"

echo "ATTEMPING TO SEND PPS PULSE"
# Setup routing to Board controlling the giga GPIOs
sudo ifconfig en7 inet 10.10.10.1 netmask 255.255.255.0 up
# Ensure proper ssh key and ssh config has been setup with your jetson
ssh -t jetson "sudo busybox devmem 0x2434040 w 0x4; sudo busybox devmem 0x2430070 w 0x8; sudo ./Desktop/test_gpio/env/bin/python3 ./Desktop/test_gpio/pseudo_pps_pulse.py"

if sudo ./env/bin/python3 tone_test.py; then
    echo "AUDIO TEST SUCCESS"
else
    echo "AUDIO TEST FAILED"
    exit 1
fi

# Return to test script dir
popd