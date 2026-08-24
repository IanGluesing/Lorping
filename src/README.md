# Library and Binary Source Code

The main binary is `hopping_modem`, containing functionality to transmit data passed over serial through either the LoRa or FSK modulation schemes, while any received data is passed back up to a host computer over serial as well. Low rate frequency hopping may also be enabled, with the assumption your transceiver pair has access to a shared time source.

## Building

Building can be done manually using `arduino-cli` to build a single binary, or automatically using `build.sh` to build everything.

### CLI

An example command to build the `hopping_modem` can be seen here.

```
# Compile only
arduino-cli compile \
  --fqbn arduino:mbed_giga:giga \
  --verbose \
  --libraries $(pwd)/hop_table \
  --libraries $(pwd)/lora_impl \
  --libraries $(pwd)/device_impl \
  --output-dir ./build_out \
  bin/hopping_modem/
```

### Build Script

You can build all available binaries using the provided `build.sh` script.

```
./build.sh
```

## Deploying

Deploying can be done automatically using `deploy.sh` to deploy a specifc binary to a specific board. More details can be seen by running `./deploy.sh -h`

```
# Deploying `hopping_modem` to board A, where A corresponds to a serial port set in `deploy.sh`
./deploy.sh A hopping_modem
```