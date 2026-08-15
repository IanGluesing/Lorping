# TODO README

## Building

How to build, assuming `arduino-cli` is installed.

### Building Tx Example

```
# Compile only
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --output-dir ./build \
  tx_hopping/

# Compile and deploy
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --upload \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --port /dev/<CHANGE_ME_PATH_TO_GIGA_SERIAL> \
  --verbose \
  tx_hopping/
```
### Building Rx Example

```
# Compile only
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --output-dir ./build \
  rx_hopping/

# Compile and deploy
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --upload \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --port /dev/<CHANGE_ME_PATH_TO_GIGA_SERIAL> \
  --verbose \
  rx_hopping/
```