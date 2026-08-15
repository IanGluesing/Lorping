#ifndef TCP_UTILS_H
#define TCP_UTILS_H

#include <Arduino.h>

#include "lwip/tcp.h"

// ============================================================
// Handle incoming TCP packet
// ============================================================

err_t tcpReceiveCallback(
    void *arg,
    tcp_pcb *pcb,
    pbuf *p,
    err_t err)
{
    if (p == nullptr) {
        // Connection closed
        tcp_close(pcb);
        return ERR_OK;
    }

    Serial.print("TCP RX: ");

    for (pbuf *q = p; q != nullptr; q = q->next) {
        Serial.write(
            (const char *)q->payload,
            q->len
        );
    }

    Serial.println();

    tcp_recved(pcb, p->tot_len);

    pbuf_free(p);

    return ERR_OK;
}

// ============================================================
// Handle when an incoming tcp connection is accepted
// ============================================================

err_t tcpAcceptCallback(
    void *arg,
    tcp_pcb *newPcb,
    err_t err
)
{
    if (err != ERR_OK || newPcb == nullptr) {
        Serial.print("TCP accept error: ");
        Serial.println(err);
        return ERR_VAL;
    }

    Serial.println("TCP connection accepted");

    tcp_recv(
        newPcb,
        tcpReceiveCallback
    );

    return ERR_OK;
}

#endif
