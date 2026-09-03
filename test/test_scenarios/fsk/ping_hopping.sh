#!/usr/bin/env bash
set -euo pipefail

## TODO: Add pulse in the middle of set of pings, and verify messages are received after the pulse
## ... Expected output may look something like this where internal clocks are only on same hop after pulse is received
# Request timeout for icmp_seq 0
# Request timeout for icmp_seq 1
# Request timeout for icmp_seq 2
# Request timeout for icmp_seq 3
# 64 bytes from 10.99.0.2: icmp_seq=4 ttl=64 time=141.399 ms
# 64 bytes from 10.99.0.2: icmp_seq=5 ttl=64 time=120.995 ms
# 64 bytes from 10.99.0.2: icmp_seq=6 ttl=64 time=150.460 ms
# 64 bytes from 10.99.0.2: icmp_seq=7 ttl=64 time=133.764 ms
# 64 bytes from 10.99.0.2: icmp_seq=8 ttl=64 time=116.378 ms
# 64 bytes from 10.99.0.2: icmp_seq=9 ttl=64 time=154.745 ms

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

echo "ATTEMPING TO SEND PPS PULSE"
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