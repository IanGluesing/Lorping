#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <stdint.h>

// ============================================================
// General Module Configuration
// ============================================================

constexpr uint8_t SYNC_WORD = 0x12;

// ============================================================
// Raw LoRa Module Configuration
// ============================================================

constexpr float INITIAL_LORA_FREQUENCY = 915.0;
constexpr float INITIAL_LORA_BANDWIDTH = 250.0;
constexpr uint8_t LORA_SPREADING_FACTOR = 6;
constexpr uint8_t LORA_CODING_RATE = 4;

// ============================================================
// Waveshare Lora USB Module Configuration
// ============================================================

constexpr float WAVESHARE_INITIAL_LORA_FREQUENCY = 868.0;
constexpr float WAVESHARE_INITIAL_LORA_BANDWIDTH = 125.0;
constexpr uint8_t WAVESHARE_LORA_SPREADING_FACTOR = 7;
constexpr uint8_t WAVESHARE_LORA_CODING_RATE = 5;

// ============================================================
// Sx1262 FSK Mode Configuration
// ============================================================

constexpr float FSK_BIT_RATE_KBPS = 100.0;
constexpr float FSK_FREQ_DEVIATION_KHZ = 25.0;
constexpr float FSK_RX_BANDWIDTH_KHZ = 156.2;
constexpr uint8_t FSK_POWER_DBM = 10;
constexpr uint8_t FSK_NUM_PREAMBLE_BITS = 32;
constexpr float FSK_TXCO_VOLTAGE = 1.6;
constexpr bool FSK_DCDC_REGULATOR = false;

#endif
