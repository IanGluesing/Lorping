#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Go to ping test dir
pushd $SCRIPT_DIR/../../host_computer/tun_ping_test

# Setup Tun test for ping
sudo ./env/bin/python3 tun_test.py &
TUN_PID=$!
echo "Got tun_test PID: $TUN_PID"
sleep 2

# Setup routes for tun test
sudo ifconfig utun6 10.99.0.1 10.99.0.2
sudo ifconfig utun7 10.99.0.2 10.99.0.1
sleep 2

# Setup routing to Board controlling the giga GPIOs
sudo ifconfig en7 inet 10.10.10.1 netmask 255.255.255.0 up
# Ensure proper ssh key and ssh config has been setup with your jetson
ssh -t jetson "sudo busybox devmem 0x2434040 w 0x4; sudo busybox devmem 0x2430070 w 0x8; sudo ./Desktop/test_gpio/env/bin/python3 ./Desktop/test_gpio/pseudo_pps_pulse.py"

# Run ping test command
PING_OUTPUT=$(ping -c 10 -W 500 10.99.0.2)

# Stop tun_test.py script
sudo kill "$TUN_PID"

# Determine pass/fail
echo "$PING_OUTPUT"
if echo "$PING_OUTPUT" | grep -q "10 packets transmitted, 10 packets received, 0.0% packet loss"; then
    echo "PING SUCCESS"
else
    echo "FAILED NON HOPPING LORA PING TEST"
    exit 1
fi

# Return to test script dir
popd