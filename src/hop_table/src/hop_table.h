#ifndef HOP_TABLE_H
#define HOP_TABLE_H

#include <cstddef>
#include <chrono>

using namespace std::chrono_literals;

// ============================================================
// Hop Table
// ============================================================

// Current hop count
volatile uint32_t hopCount = 0;

// Shared Tx/Rx Hop Table
constexpr float HOP_TABLE[] = {
    915.0,
    915.2,
    915.4,
    915.6,
    915.8,
    916.0,
    916.2,
    916.4,
    916.6,
    916.8
};

// Hop Table Size
constexpr size_t HOP_TABLE_SIZE =
    sizeof(HOP_TABLE) / sizeof(HOP_TABLE[0]);

// ============================================================
// Transmit Timing Values
// ============================================================

// User defined hop period Hz = 1 / Period
constexpr std::chrono::milliseconds HOP_PERIOD_MS = 200ms;

// RadioLib overhead for standby(), transmit(), startReceive() calls
//
// Total Transmit Time = standby(), transmit(), startReceive() and transmitting one character.
// Time on Air for 1 char = getTimeOnAir(1)
//
// RadioLib overhead = Total Transmit Time - Time on Air for 1 char
//
uint16_t DEFAULT_RADIOLIB_TRANSMIT_CYCLE_TIME_MICROS = 12300;

// Change in RadioLib overhead time for each additional character to transmit
//
// Determine RadioLib overhead for 1-100 characters, and find the slope of these values.
//
uint16_t DEFAULT_RADIOLIB_PER_CHARACTER_TIME_MICROS = 8;

// Time, in micros, of the last hop that took place
unsigned long last_hop_time_micros = 0;

// Grace period before and after the hop occurs, that a transmit should not be attempted, to
// allow and assume the receiver has hopped to the next frequency
//
// Transmission should only be attempted within this window: 
//
//      [last_hop_time_micros + HOP_GRACE_PERIOD_MICROS, last_hop_time_micros + HOP_PERIOD_MS - HOP_GRACE_PERIOD_MICROS]
//
constexpr uint16_t HOP_GRACE_PERIOD_MICROS = 20000; // 20ms

// ============================================================
// HW Timer interrupt callback
// ============================================================

// Flag denoting a hop needs to occur
volatile bool hopPending = false;

// Hardware timer callback signalling a hop needs to occur
void hopTickOccurred()
{
    hopPending = true;
}

#endif
