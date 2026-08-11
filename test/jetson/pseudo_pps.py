import Jetson.GPIO as GPIO
import time

# Set GPIO mode
GPIO.setmode(GPIO.BOARD)

# Jetson GPIO Pins connected to Arduino Giga R1 Wifi Pins
JETSON_PPS1 = 31
JETSON_PPS2 = 33

# Initialize Pins
GPIO.setup(JETSON_PPS1, GPIO.OUT, initial=GPIO.LOW)
GPIO.setup(JETSON_PPS2, GPIO.OUT, initial=GPIO.LOW)

try:
    # Generate 1Hz PPS for 5 Seconds, update as needed
    for i in range(5):
        # PPS Rising Edge
        GPIO.output(JETSON_PPS1, GPIO.HIGH)
        GPIO.output(JETSON_PPS2, GPIO.HIGH)
        time.sleep(0.01)
        
        # PPS Falling Edge
        GPIO.output(JETSON_PPS1, GPIO.LOW)
        GPIO.output(JETSON_PPS2, GPIO.LOW)
        time.sleep(0.99)
finally:
    # Revert to Low on completion
    GPIO.output(JETSON_PPS1, GPIO.LOW)
    GPIO.output(JETSON_PPS2, GPIO.LOW)
    GPIO.cleanup()