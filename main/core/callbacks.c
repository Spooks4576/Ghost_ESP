#include "core/callbacks.h"
#include "managers/wifi_manager.h"
#include <esp_log.h>
#include <string.h>

#define WPS_OUI 0x0050f204 
#define TAG "WIFI_MONITOR"

wps_network_t detected_wps_networks[MAX_WPS_NETWORKS];
int detected_network_count = 0;
esp_timer_handle_t stop_timer;

void wifi_wps_detection_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) {
        return;
    }

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    const uint8_t *payload = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;

    uint8_t frame_type = hdr->frame_ctrl & 0xFC;

    if (frame_type != 0x80 && frame_type != 0x50) {
        return;
    }

    
    int index = 36;
    char ssid[33] = {0};
    while (index < len) {
        uint8_t id = payload[index];
        uint8_t ie_len = payload[index + 1];

        
        if (id == 0 && ie_len <= 32) {
            memcpy(ssid, &payload[index + 2], ie_len);
            ssid[ie_len] = '\0';
        }

        if (id == 221 && ie_len >= 4) {
            uint32_t oui = (payload[index + 2] << 16) | (payload[index + 3] << 8) | payload[index + 4];
            uint8_t oui_type = payload[index + 5];
           
            if (oui == 0x0050f2 && oui_type == 0x04) {
                if (detected_network_count < MAX_WPS_NETWORKS) {
                    wps_network_t *network = &detected_wps_networks[detected_network_count];
                    memcpy(network->bssid, hdr->addr2, 6);


                    strncpy(network->ssid, ssid, sizeof(network->ssid) - 1);
                    network->ssid[sizeof(network->ssid) - 1] = '\0';  // Ensure null-termination

                    network->wps_enabled = true;
                    detected_network_count++;


                    ESP_LOGI(TAG, "WPS enabled on network: SSID: %s, BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
                        network->ssid,
                        network->bssid[0], network->bssid[1], network->bssid[2],
                        network->bssid[3], network->bssid[4], network->bssid[5]);

                    
                    if (detected_network_count >= MAX_WPS_NETWORKS) {
                        ESP_LOGI(TAG, "Maximum WPS networks detected, stopping monitor mode...");
                        wifi_manager_stop_monitor_mode();
                    }
                }
            } else {
            }
        }

        index += (2 + ie_len);
    }
}