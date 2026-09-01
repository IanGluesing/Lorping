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

# Go to the simple python server directory
pushd simple_server

# Run the test server, assume 2 second sleep for background process to start
python3 test_server.py &
TEST_SERVER_PID=$!
echo "Got test_server PID: $TEST_SERVER_PID"
sleep 2

# Run ping test command
PING_OUTPUT=$(curl --max-time 5 http://10.99.0.2:8000/)

# Kill helper scripts
sudo kill "$TUN_PID"
sudo kill "$TEST_SERVER_PID"

# Determine pass/fail
echo "$PING_OUTPUT"
if echo "$PING_OUTPUT" | grep -q "Hello from the LoRa network!"; then
    echo "CURL SUCCESS"
else
    echo "CURL FAILED"
    exit 1
fi

# Return to test script dir
popd

# Return to test script dir
popd