#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <stdint.h>

// ============================================================
// SX1262 pins
// ============================================================

// Arduino Giga R1 Wifi and Waveshare SX1262 Pinouts can be found here:
// Arduino Pinout: https://content.arduino.cc/assets/ABX00063-full-pinout.pdf
// SX1262 Pinout: https://www.waveshare.com/core1262-868m.htm 

// Arduino Pin D26 <- connected with -> SX1262 CS Pin
constexpr int PIN_CS = 26;

// Arduino Pin D32 <- connected with -> SX1262 BUSY Pin
constexpr int PIN_BUSY  = 32;

// Arduino Pin D38 <- connected with -> SX1262 DIO1 Pin
constexpr int PIN_DIO1  = 38;

// Arduino Pin D44 <- connected with -> SX1262 RESET Pin
constexpr int PIN_RESET = 44;

// ============================================================
// LoRa configuration
// ============================================================

constexpr float LORA_FREQUENCY = 868.0;
constexpr float LORA_BANDWIDTH = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;

#endif
