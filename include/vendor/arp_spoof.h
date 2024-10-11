#ifndef ARP_SPOOF_H
#define ARP_SPOOF_H

#include <stdint.h>
#include "esp_wifi.h"

// Structure to hold ARP spoofing configuration for multiple targets
typedef struct {
    uint8_t target_ip[4];    // Target IP address
    uint8_t target_mac[6];   // Target MAC address
    uint8_t spoof_ip[4];     // IP address to spoof (e.g., router IP)
    uint8_t attacker_mac[6]; // Attacker (ESP32) MAC address
    int interval_ms;         // Interval in milliseconds between spoof packets
} arp_spoof_config_t;

// ARP packet structure
typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6]; // Sender hardware address (attacker MAC)
    uint8_t spa[4]; // Sender protocol address (spoofed IP)
    uint8_t tha[6]; // Target hardware address (victim MAC)
    uint8_t tpa[4]; // Target protocol address (victim IP)
} arp_packet_t;


void init_arp_spoof(arp_spoof_config_t *config, int num_targets);


void arp_spoof_task(void *pvParameter);

void packet_listener_task(void *pvParameters);


void add_target(const uint8_t *target_ip, const uint8_t *target_mac, const uint8_t *spoof_ip);


void stop_arp_spoof();

#endif // ARP_SPOOF_H