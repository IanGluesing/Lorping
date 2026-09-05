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
constexpr bool LORA_CRC_ENABLED = true;
constexpr uint8_t LORA_PREAMBLE_LENGTH = 8;
constexpr bool LORA_IMPLICIT = false;
constexpr bool LORA_LDRO = false;

// ------------------------------------------------------------
// Invert RadioLib's SX126x::calculateTimeOnAir()
// ------------------------------------------------------------

// Determine the max payload size for remaining time in current hop window
size_t maxPayloadForTime(uint32_t timeBudgetUs) {
    const uint8_t sf = LORA_SPREADING_FACTOR;
    const uint32_t bw = INITIAL_LORA_BANDWIDTH;
    const uint8_t cr = LORA_CODING_RATE;

    // --------------------------------------------------------
    // RadioLib:
    //
    // symbolLength_us =
    //     ((1000 * 10) << SF) / (BW * 10)
    // --------------------------------------------------------

    uint32_t symbolLengthUs = (10000UL << sf) / (bw * 10);

    // --------------------------------------------------------
    // RadioLib's SF-dependent constants
    // --------------------------------------------------------

    uint8_t sfCoeff1_x4 = 17;
    uint8_t sfCoeff2 = 8;

    if (sf == 5 || sf == 6) {
        sfCoeff1_x4 = 25;
        sfCoeff2 = 0;
    }

    // --------------------------------------------------------
    // RadioLib:
    //
    // sfDivisor = 4 * SF
    //
    // or
    //
    // sfDivisor = 4 * (SF - 2)
    // --------------------------------------------------------

    uint8_t sfDivisor = 4 * sf;

    if (LORA_LDRO) {
        sfDivisor = 4 * (sf - 2);
    }

    // --------------------------------------------------------
    // RadioLib:
    //
    // N_symbol_header = implicit ? 0 : 20
    // --------------------------------------------------------

    const int8_t bitsPerCrc = 16;

    const int8_t N_symbol_header = LORA_IMPLICIT ? 0 : 20;

    // --------------------------------------------------------
    // RadioLib final calculation:
    //
    // return((symbolLength_us * nSymbol_x4) / 4);
    //
    // We need the largest nSymbol_x4 such that:
    //
    //     (symbolLength_us * nSymbol_x4) / 4
    //         <= timeBudgetUs
    //
    // --------------------------------------------------------

    uint64_t maxNSymbolX4 = ((uint64_t)timeBudgetUs * 4) / symbolLengthUs;

    // --------------------------------------------------------
    // Fixed symbols:
    //
    // nSymbol_x4 =
    //     (preamble + 8) * 4
    //     + sfCoeff1_x4
    //     + nPreCodedSymbols * CR * 4
    // --------------------------------------------------------

    uint32_t fixedSymbolX4 = ((uint32_t)LORA_PREAMBLE_LENGTH + 8) * 4 + sfCoeff1_x4;

    if (maxNSymbolX4 < fixedSymbolX4) {
        return 0;
    }

    // --------------------------------------------------------
    // How many coded symbols can remain?
    // --------------------------------------------------------

    uint64_t remainingX4 = maxNSymbolX4 - fixedSymbolX4;
    uint64_t maxPreCodedSymbols = remainingX4 / ((uint64_t)cr * 4);

    // --------------------------------------------------------
    // RadioLib:
    //
    // nPreCodedSymbols =
    //     ceil(bitCount / sfDivisor)
    //
    // If:
    //
    //     ceil(bitCount / sfDivisor) <= N
    //
    // then:
    //
    //     bitCount <= N * sfDivisor
    // --------------------------------------------------------

    uint64_t maxBitCount = maxPreCodedSymbols * sfDivisor;

    // --------------------------------------------------------
    // RadioLib:
    //
    // bitCount =
    //     8 * len
    //     + CRC * 16
    //     - 4 * SF
    //     + sfCoeff2
    //     + header
    //
    // --------------------------------------------------------

    int32_t fixedBitCount = (LORA_CRC_ENABLED ? bitsPerCrc : 0) - 4 * sf + sfCoeff2 + N_symbol_header;

    // Therefore:
    //
    // 8 * len + fixedBitCount <= maxBitCount
    //
    // 8 * len <= maxBitCount - fixedBitCount
    // --------------------------------------------------------

    int64_t maxPayloadNumerator = (int64_t)maxBitCount - fixedBitCount;

    if (maxPayloadNumerator < 0) {
        return 0;
    }

    size_t maxPayload = maxPayloadNumerator / 8;

    // SX1262 maximum payload
    if (maxPayload > 255) {
        maxPayload = 255;
    }

    return maxPayload;
}

// ============================================================
// Waveshare Lora USB Module Configuration
// ============================================================

constexpr float WAVESHARE_INITIAL_LORA_FREQUENCY = 868.0;
constexpr float WAVESHARE_INITIAL_LORA_BANDWIDTH = 125.0;
constexpr uint8_t WAVESHARE_LORA_SPREADING_FACTOR = 7;
constexpr uint8_t WAVESHARE_LORA_CODING_RATE = 5;

#endif
