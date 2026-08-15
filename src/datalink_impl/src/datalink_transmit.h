#ifndef DATALINK_TRANSMIT_H
#define DATALINK_TRANSMIT_H

#include <stdint.h>

// ============================================================
//
// TX Data buffer, handling the boundary between the SX1262
// and the lwip network interface handling
// 
// ============================================================

// TX Data Buffer
uint8_t txBuffer[256];

#endif
