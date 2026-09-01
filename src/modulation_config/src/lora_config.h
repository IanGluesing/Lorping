#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <stdint.h>
#include <stddef.h>

// ============================================================
// General Module Configuration
// ============================================================

constexpr uint8_t SYNC_WORD = 0x12;

constexpr size_t MAX_PAYLOAD_SIZE = UINT8_MAX;

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

#endif
