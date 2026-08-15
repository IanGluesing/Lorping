#ifndef PING_H
#define PING_H

void sendPing(
    struct netif *param_lora_netif_network_interface, 
    uint8_t param_LOCAL_IP_LAST, 
    uint8_t param_REMOTE_IP_LAST)
{
    Serial.println();
    Serial.println("Sending ICMP echo request...");

    // --------------------------------------------------------
    // Build IP/ICMP packet manually.
    //
    // IPv4 header = 20 bytes
    // ICMP header = 8 bytes
    //
    // Total = 28 bytes
    //
    // We currently use a 16-byte ICMP section so the packet
    // remains 36 bytes, matching our existing test.
    // --------------------------------------------------------

    constexpr uint16_t ICMP_SIZE = 16;
    constexpr uint16_t IP_SIZE = sizeof(struct ip_hdr);

    constexpr uint16_t TOTAL_SIZE = IP_SIZE + ICMP_SIZE;

    // --------------------------------------------------------
    // Allocate packet buffer.
    // --------------------------------------------------------

    struct pbuf *p =
        pbuf_alloc(
            PBUF_RAW,
            TOTAL_SIZE,
            PBUF_RAM
        );

    if (p == nullptr) {
        Serial.println("Unable to allocate ping packet");

        return;
    }

    // Clear entire packet.

    memset(p->payload, 0, TOTAL_SIZE);

    // --------------------------------------------------------
    // IP addresses
    // --------------------------------------------------------

    ip4_addr_t src_ip;
    ip4_addr_t dst_ip;

    IP4_ADDR(&src_ip, 10, 0, 0, param_LOCAL_IP_LAST);
    IP4_ADDR(&dst_ip, 10, 0, 0, param_REMOTE_IP_LAST);

    // --------------------------------------------------------
    // IP header
    // --------------------------------------------------------

    struct ip_hdr *iph = (struct ip_hdr *)p->payload;

    IPH_VHL_SET(iph, 4, 5);
    IPH_TOS_SET(iph, 0);
    IPH_LEN_SET(iph, lwip_htons(TOTAL_SIZE));
    IPH_ID_SET(iph, 0);
    IPH_OFFSET_SET(iph, 0);
    IPH_TTL_SET(iph, 64);
    IPH_PROTO_SET(iph, IP_PROTO_ICMP);

    // --------------------------------------------------------
    // Set source and destination IP addresses.
    // --------------------------------------------------------

    ip4_addr_copy(
        *ip_2_ip4(&iph->src),
        src_ip
    );

    ip4_addr_copy(
        *ip_2_ip4(&iph->dest),
        dst_ip
    );

    // --------------------------------------------------------
    // Calculate IP header checksum.
    // --------------------------------------------------------

    IPH_CHKSUM_SET(
        iph,
        0
    );

    IPH_CHKSUM_SET(
        iph,
        inet_chksum(
            iph,
            IP_SIZE
        )
    );

    // --------------------------------------------------------
    // ICMP header
    // --------------------------------------------------------

    struct icmp_echo_hdr *icmp =
        (struct icmp_echo_hdr *)
        (
            (uint8_t *)p->payload +
            IP_SIZE
        );

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->id = PP_HTONS(0x1234);
    icmp->seqno = PP_HTONS(1);

    // --------------------------------------------------------
    // ICMP checksum.
    // --------------------------------------------------------

    icmp->chksum = 0;
    icmp->chksum = inet_chksum(icmp, ICMP_SIZE);

    // --------------------------------------------------------
    // Print packet for debugging.
    // --------------------------------------------------------

    Serial.print("TX packet HEX: ");
    uint8_t *data = (uint8_t *)p->payload;

    for (uint16_t i = 0; i < TOTAL_SIZE; i++) {
        if (data[i] < 0x10) {
            Serial.print('0');
        }

        Serial.print(data[i], HEX);
        Serial.print(' ');
    }

    Serial.println();

    // --------------------------------------------------------
    // Send through our LoRa network interface.
    // --------------------------------------------------------

    err_t result =
        param_lora_netif_network_interface->linkoutput(
            param_lora_netif_network_interface,
            p
        );

    Serial.print("Ping TX result: ");
    Serial.println(result);

    // --------------------------------------------------------
    // Free packet.
    // --------------------------------------------------------

    pbuf_free(p);
}

#endif