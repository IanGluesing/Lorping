#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

// Arduino Giga R1 Wifi and Waveshare SX1262 Pinouts can be found here:
// Arduino Pinout: https://content.arduino.cc/assets/ABX00063-full-pinout.pdf
// SX1262 Pinout: https://www.waveshare.com/core1262-868m.htm 

// ============================================================
// Configuration
// ============================================================

// Baud Rate used for Serial connection
constexpr uint32_t SERIAL_BAUD_RATE = 115200; 

// Arduino Pin handling PPS input from GPS Source or Pseudo PPS Provider: D25
constexpr int PPS_PIN = 25;

// ============================================================
// SX1262 pins
// ============================================================

// Arduino Pin D26 <- connected with -> SX1262 CS Pin
constexpr int PIN_CS = 26;

// Arduino Pin D32 <- connected with -> SX1262 BUSY Pin
constexpr int PIN_BUSY  = 32;

// Arduino Pin D38 <- connected with -> SX1262 DIO1 Pin
constexpr int PIN_DIO1  = 38;

// Arduino Pin D44 <- connected with -> SX1262 RESET Pin
constexpr int PIN_RESET = 44;

// ============================================================
// GPS PPS Configuration
// ============================================================

// Flag denoting a PPS Signal was received
volatile bool ppsReceived = false;

// Callback triggered on rising edge of PPS signal
void ppsISR()
{
    ppsReceived = true;
}

#endif
