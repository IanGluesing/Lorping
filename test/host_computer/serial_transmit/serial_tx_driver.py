import serial
import threading
import sys

# Serial Params
TX_PORT = "/dev/cu.usbmodem2101"
BAUD_RATE = 115200

# Create Serial connection
serial_connection = serial.Serial(TX_PORT, BAUD_RATE, timeout=0.1)

# Allow seeing serial stream and writing at same time
def read_serial():
    while True:
        data = serial_connection.readline()
        if data:
            print(f"GIGA: {data.decode(errors='replace').rstrip()}")

# Start Serial read stream
serial_reader = threading.Thread(target=read_serial, daemon=True)
serial_reader.start()

# Take user input and write to serial stream
try:
    while True:
        message = input("> ")
        serial_connection.write((message).encode())
        serial_connection.flush()
except KeyboardInterrupt:
    print("\nExiting.")
finally:
    serial_connection.close()