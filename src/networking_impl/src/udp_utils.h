#ifndef UDP_UTILS_H
#define UDP_UTILS_H

#include <Arduino.h>
#include <stdint.h>

#include "lwip/udp.h"

// ============================================================
// Handle incoming UDP packet
// ============================================================

void udpReceiveCallback(
    void *arg,
    struct udp_pcb *pcb,
    struct pbuf *p,
    const ip_addr_t *addr,
    u16_t port)
{
    (void)arg;
    (void)pcb;

    if (p == nullptr) {
        return;
    }

    Serial.println();
    Serial.println("===== UDP RX =====");

    Serial.print("From: ");
    Serial.print(ip4addr_ntoa(ip_2_ip4(addr)));
    Serial.print(":");
    Serial.println(port);

    Serial.print("Length: ");
    Serial.println(p->tot_len);

    Serial.print("Data: ");

    for (struct pbuf *q = p; q != nullptr; q = q->next) {
        uint8_t *data = (uint8_t *)q->payload;

        for (uint16_t i = 0; i < q->len; i++) {
            Serial.write(data[i]);
        }
    }

    Serial.println();

    Serial.println("==================");

    pbuf_free(p);
}

// ============================================================
// Setup UDP
// ============================================================

bool setupUDP(udp_pcb *&udpPcb)
{
    udpPcb = udp_new();

    if (udpPcb == nullptr) {
        Serial.println("ERROR: udp_new() failed");
        return false;
    }

    err_t result = udp_bind(
        udpPcb,
        IP_ADDR_ANY,
        UDP_PORT
    );

    if (result != ERR_OK) {

        Serial.print("udp_bind failed: ");
        Serial.println(result);

        udp_remove(udpPcb);
        udpPcb = nullptr;

        return false;
    }

    udp_recv(
        udpPcb,
        udpReceiveCallback,
        nullptr
    );

    return true;
}

// ============================================================
// Send UDP
// ============================================================

void sendUDP(udp_pcb *&udpPcb, uint8_t REMOTE_IP_LAST)
{
    Serial.println();
    Serial.println("Sending UDP packet...");

    if (udpPcb == nullptr) {
        Serial.println("UDP PCB is null");
        return;
    }

    // --------------------------------------------------------
    // Destination
    // --------------------------------------------------------

    ip4_addr_t destination4;

    IP4_ADDR(
        &destination4,
        10,
        0,
        0,
        REMOTE_IP_LAST
    );

    ip_addr_t destination;

    ip_addr_copy_from_ip4(
        destination,
        destination4
    );

    // --------------------------------------------------------
    // Payload
    // --------------------------------------------------------

    const char message[] =
        "Hello from GIGA over LoRa!";

    constexpr uint16_t length =
        sizeof(message) - 1;

    // --------------------------------------------------------
    // Allocate UDP payload
    // --------------------------------------------------------

    struct pbuf *p =
        pbuf_alloc(
            PBUF_TRANSPORT,
            length,
            PBUF_RAM
        );

    if (p == nullptr) {

        Serial.println(
            "UDP pbuf_alloc failed"
        );

        return;
    }

    memcpy(
        p->payload,
        message,
        length
    );

    // --------------------------------------------------------
    // Send
    // --------------------------------------------------------

    err_t result =
        udp_sendto(
            udpPcb,
            p,
            &destination,
            UDP_PORT
        );

    Serial.print(
        "UDP send result: "
    );

    Serial.println(result);

    pbuf_free(p);
}

#endif
