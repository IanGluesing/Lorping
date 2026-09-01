#ifndef FSK_CONFIG_H
#define FSK_CONFIG_H

#include <stdint.h>
#include <stddef.h>

// ============================================================
// Sx1262 FSK Mode Configuration
// ============================================================

constexpr float INITIAL_FREQUENCY = 915.0;
constexpr float FSK_BIT_RATE_KBPS = 100.0;
constexpr float FSK_FREQ_DEVIATION_KHZ = 25.0;
constexpr float FSK_RX_BANDWIDTH_KHZ = 156.2;
constexpr uint8_t FSK_POWER_DBM = 10;
constexpr uint8_t FSK_NUM_PREAMBLE_BITS = 32;
constexpr float FSK_TXCO_VOLTAGE = 1.6;
constexpr bool FSK_DCDC_REGULATOR = false;

// ============================================================
// FSK Mode Configuration
// ============================================================

constexpr size_t MAX_PAYLOAD_SIZE = UINT8_MAX;

#endif
