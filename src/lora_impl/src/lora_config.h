#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <stdint.h>

// ============================================================
// LoRa configuration
// ============================================================

constexpr float INITIAL_LORA_FREQUENCY = 868.0;
constexpr float INITIAL_LORA_BANDWIDTH = 125.0;
constexpr uint8_t LORA_SPREADING_FACTOR = 7;
constexpr uint8_t LORA_CODING_RATE = 5;

#endif
