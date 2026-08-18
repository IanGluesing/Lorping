#ifndef HOP_TABLE_H
#define HOP_TABLE_H

#include <cstddef>
#include <chrono>

using namespace std::chrono_literals;

// ============================================================
// Hop Table
// ============================================================

// User defined hop period Hz = 1 / Period => 10Hz Hop Rate
constexpr std::chrono::milliseconds HOP_PERIOD_MS = 100ms;

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
