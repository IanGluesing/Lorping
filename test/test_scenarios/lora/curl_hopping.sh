#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cleanup() {
    echo "Cleaning up..."

    if [ -n "$TUN_PID" ]; then
        sudo kill "$TUN_PID" 2>/dev/null || true
    fi

    if [ -n "$TEST_SERVER_PID" ]; then
        sudo kill "$TEST_SERVER_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

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

# Go to the simple python server directory
pushd simple_server

python3 test_server.py &
TEST_SERVER_PID=$!
echo "Got test_server PID: $TEST_SERVER_PID"
sleep 2

echo "ATTEMPING TO SEND PPS PULSE"
# Setup routing to Board controlling the giga GPIOs
sudo ifconfig en7 inet 10.10.10.1 netmask 255.255.255.0 up
# Ensure proper ssh key and ssh config has been setup with your jetson
ssh -t jetson "sudo busybox devmem 0x2434040 w 0x4; sudo busybox devmem 0x2430070 w 0x8; sudo ./Desktop/test_gpio/env/bin/python3 ./Desktop/test_gpio/pseudo_pps_pulse.py"

# Run ping test command
CURL_OUTPUT=$(curl --max-time 5 http://10.99.0.2:8000/)

# Determine pass/fail
echo "$CURL_OUTPUT"
if echo "$CURL_OUTPUT" | grep -q "Hello from the LoRa network!"; then
    echo "CURL SUCCESS"
else
    echo "CURL FAILED"
    exit 1
fi

# Return to test script dir
popd

# Return to test script dir
popd