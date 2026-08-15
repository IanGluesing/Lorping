#ifndef DATALINK_RECEIVE_H
#define DATALINK_RECEIVE_H

#include <stdint.h>

#include "lwip/netif.h"
#include "lwip/pbuf.h"

// ============================================================
//
// RX Data buffer, handling the boundary between the SX1262
// and the lwip network interface handling
// 
// ============================================================

// RX Data Buffer
uint8_t rxBuffer[256];

// ============================================================
//
// Handle received data from the LORA module. Read data into rxBuffer off the lora module,
// copy into lwip buffer and forward up to the lwip network interface. 
//
// The full RX data path looks like this.
//
// )))))) -> Antenna -> Sx1262 Module -> Giga -> Giga Network Interface Handling
//
// This function handles specifically this boundary:
//
// [ Sx1262 Module -> Giga(rxBuffer) -> Giga Network Interface Handling ]
// 
// ============================================================

void processLoRaReceive(
    SX1262 *radio_module,
    netif *param_lora_netif_network_interface)
{
    // --------------------------------------------------------
    // Read data off SX1262 Module
    // --------------------------------------------------------

    // Non blocking data read
    int state = radio_module->readData(
        rxBuffer,
        sizeof(rxBuffer)
    );

    // Handle error
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa RX failed: ");
        Serial.println(state);

        return;
    }

    size_t rx_num_bytes_from_lora = radio_module->getPacketLength();

    // Debug
    Serial.print("LoRa RX packet, rx_num_bytes_from_lora=");
    Serial.println(rx_num_bytes_from_lora);
    Serial.print("RX buffer HEX: ");
    for (size_t i = 0; i < rx_num_bytes_from_lora; i++) {
        if (rxBuffer[i] < 0x10) {
            Serial.print('0');
        }

        Serial.print(rxBuffer[i], HEX);
        Serial.print(' ');
    }
    Serial.println();

    // --------------------------------------------------------
    // Allocate LWIP pbuf to send to LWIP
    // --------------------------------------------------------

    struct pbuf *p = pbuf_alloc(PBUF_RAW, rx_num_bytes_from_lora, PBUF_POOL);

    // Handle error
    if (p == nullptr) {
        Serial.println("pbuf_alloc failed");

        return;
    }

    // --------------------------------------------------------
    // Copy LoRa payload into LWIP pbuf.
    // --------------------------------------------------------

    uint16_t offset = 0;

    for (struct pbuf *q = p; q != nullptr; q = q->next) {
        memcpy(q->payload, &rxBuffer[offset], q->len);
        offset += q->len;
    }

    // --------------------------------------------------------
    // Give packet to LWIP and let it decide what to do
    // --------------------------------------------------------

    // Forward received data
    err_t result =
        param_lora_netif_network_interface->input(
            p,
            param_lora_netif_network_interface
        );

    // Handle error
    if (result != ERR_OK) {
        Serial.print("LWIP input failed: ");
        Serial.println(result);

        pbuf_free(p);
    } else {
        Serial.println("Packet delivered to LWIP");
    }

    // --------------------------------------------------------
    // Put radio back into receive mode.
    // --------------------------------------------------------

    radio_module->startReceive();
}

#endif
