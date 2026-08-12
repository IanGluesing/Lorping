# Install ArduinoCli

```
# Install arduino-cli
brew install arduino-cli

# Initialize arduino-cli
arduino-cli config init

# Update index
arduino-cli core update-index

# Verify and install required packages
arduino-cli core search mbed_giga
arduino-cli core install arduino:mbed_giga@4.5.0
arduino-cli lib install RadioLib@7.7.1
```