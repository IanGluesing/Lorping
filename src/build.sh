#!/usr/bin/env bash

set -euo pipefail

arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --libraries $(pwd)/device_impl \
  --libraries $(pwd)/datalink_impl \
  --build-property compiler.cpp.extra_flags="-DMBED_CONF_LWIP_IPV4_ENABLED=1 -DMBED_CONF_LWIP_IPV6_ENABLED=0 -DMBED_CONF_LWIP_IP_VER_PREF=4 -DMBED_CONF_NSAPI_DEFAULT_STACK=LWIP -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/lwip/src/include -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/include/lwipstack -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/lwip-sys" \
  --output-dir ./build_out \
  tx_hopping/

arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --libraries $(pwd)/device_impl \
  --libraries $(pwd)/datalink_impl \
  --build-property compiler.cpp.extra_flags="-DMBED_CONF_LWIP_IPV4_ENABLED=1 -DMBED_CONF_LWIP_IPV6_ENABLED=0 -DMBED_CONF_LWIP_IP_VER_PREF=4 -DMBED_CONF_NSAPI_DEFAULT_STACK=LWIP -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/lwip/src/include -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/include/lwipstack -I$HOME/Library/Arduino15/packages/arduino/hardware/mbed_giga/4.5.0/cores/arduino/mbed/connectivity/lwipstack/lwip-sys" \
  --output-dir ./build_out \
  rx_hopping/