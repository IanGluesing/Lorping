#!/usr/bin/env bash
set -euo pipefail

# Get dir of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Go to ping test dir
pushd $SCRIPT_DIR/host_computer/tun_ping_test

# Setup Tun test for ping
sudo ./env/bin/python3 tun_test.py &
TUN_PID=$!
echo "Got tun_test PID: $TUN_PID"
sleep 2

# Setup routes for tun test
sudo ifconfig utun6 10.99.0.1 10.99.0.2
sudo ifconfig utun7 10.99.0.2 10.99.0.1
sleep 2

# Run ping test command
PING_OUTPUT=$(ping -c 10 -W 500 10.99.0.2)

# Determine pass/fail
echo "$PING_OUTPUT"
if echo "$PING_OUTPUT" | grep -q "10 packets transmitted, 10 packets received, 0.0% packet loss"; then
    echo "PING SUCCESS"
else
    echo "PING FAILED"
fi

# Stop tun_test.py script
sudo kill "$TUN_PID"

# Return to test script dir
popd