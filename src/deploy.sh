#!/bin/bash

set -e

FQBN="arduino:mbed_giga:giga"

SERIAL_A="/dev/cu.usbmodem2101"
SERIAL_B="/dev/cu.usbmodem101"

BOARD="$1"
NAME="$2"

if [ -z "$BOARD" ] || [ -z "$NAME" ]; then
    echo "Usage: $0 <A|B> <name>"
    echo "Example: $0 A hopping_modem"
    exit 1
fi

case "$BOARD" in
    A|a)
        PORT="$SERIAL_A"
        ;;
    B|b)
        PORT="$SERIAL_B"
        ;;
    *)
        echo "Invalid board: $BOARD"
        echo "Use A or B"
        exit 1
        ;;
esac

SKETCH="bin/$NAME/$NAME.ino"

if [ ! -f "$SKETCH" ]; then
    echo "Sketch not found:"
    echo "  $SKETCH"
    exit 1
fi

echo "Deploying $NAME to GIGA $BOARD..."
echo "Port: $PORT"
echo "Sketch: $SKETCH"
echo

arduino-cli upload \
    --fqbn "$FQBN" \
    -p "$PORT" \
    "$SKETCH"