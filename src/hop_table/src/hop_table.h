#ifndef HOP_TABLE_H
#define HOP_TABLE_H

#include <cstddef>

// ============================================================
// Hop Table
// ============================================================

// Shared Tx/Rx Hop Table
constexpr float HOP_TABLE[] = {
    868.0,
    868.2,
    868.4,
    868.6,
    868.8,
    869.0,
    869.2,
    869.4,
    869.6,
    869.8
};

// Hop Table Size
constexpr size_t HOP_TABLE_SIZE =
    sizeof(HOP_TABLE) / sizeof(HOP_TABLE[0]);

#endif
