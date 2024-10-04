#include "core/callbacks.h"
#include "managers/wifi_manager.h"
#include <esp_log.h>
#include <string.h>
#include "vendor/pcap.h"

#define WPS_OUI 0x0050f204 
#define TAG "WIFI_MONITOR"
#define WPS_CONF_METHODS_PBC        0x0080
#define WPS_CONF_METHODS_PIN_DISPLAY 0x0004
#define WPS_CONF_METHODS_PIN_KEYPAD  0x0008
#define WIFI_PKT_DEAUTH 0x0C // Deauth subtype
#define WIFI_PKT_BEACON 0x08 // Beacon subtype
#define WIFI_PKT_PROBE_REQ 0x04  // Probe Request subtype
#define WIFI_PKT_PROBE_RESP 0x05 // Probe Response subtype

wps_network_t detected_wps_networks[MAX_WPS_NETWORKS];
int detected_network_count = 0;
esp_timer_handle_t stop_timer;

bool is_network_already_detected(const uint8_t *bssid) {
    for (int i = 0; i < detected_network_count; i++) {
        if (memcmp(detected_wps_networks[i].bssid, bssid, 6) == 0) {
            return true;
        }
    }
    return false;
}


void get_frame_type_and_subtype(const wifi_promiscuous_pkt_t *pkt, uint8_t *frame_type, uint8_t *frame_subtype) {
    if (pkt->rx_ctrl.sig_len < 24) {
        *frame_type = 0xFF;
        *frame_subtype = 0xFF;
        return;
    }

    
    const uint8_t* frame_ctrl = pkt->payload;


    *frame_type = (frame_ctrl[0] & 0x0C) >> 2;
    *frame_subtype = (frame_ctrl[0] & 0xF0) >> 4; 
}


bool is_beacon_packet(const wifi_promiscuous_pkt_t *pkt) {
    uint8_t frame_type, frame_subtype;
    get_frame_type_and_subtype(pkt, &frame_type, &frame_subtype);
    return (frame_type == WIFI_PKT_MGMT && frame_subtype == WIFI_PKT_BEACON);
}


bool is_deauth_packet(const wifi_promiscuous_pkt_t *pkt) {
    uint8_t frame_type, frame_subtype;
    get_frame_type_and_subtype(pkt, &frame_type, &frame_subtype);
    return (frame_type == WIFI_PKT_MGMT && frame_subtype == WIFI_PKT_DEAUTH);
}


bool is_probe_request(const wifi_promiscuous_pkt_t *pkt) {
    uint8_t frame_type, frame_subtype;
    get_frame_type_and_subtype(pkt, &frame_type, &frame_subtype);
    return (frame_type == WIFI_PKT_MGMT && frame_subtype == WIFI_PKT_PROBE_REQ);
}


bool is_probe_response(const wifi_promiscuous_pkt_t *pkt) {
    uint8_t frame_type, frame_subtype;
    get_frame_type_and_subtype(pkt, &frame_type, &frame_subtype);
    return (frame_type == WIFI_PKT_MGMT && frame_subtype == WIFI_PKT_PROBE_RESP);
}

void wifi_raw_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write Raw packet to PCAP buffer.");
    }
}

void wifi_probe_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    
    if (is_probe_request(pkt) || is_probe_response(pkt)) {
        ESP_LOGI(TAG, "Probe packet detected, length: %d", pkt->rx_ctrl.sig_len);
        
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write Probe packet to PCAP buffer.");
        }
    }
}


void wifi_beacon_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    
    if (is_beacon_packet(pkt)) {
        ESP_LOGI(TAG, "Beacon packet detected, length: %d", pkt->rx_ctrl.sig_len);

        
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write beacon packet to PCAP buffer.");
        }
    }
}


void wifi_deauth_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    
    if (is_deauth_packet(pkt)) {
        ESP_LOGI(TAG, "Deauth packet detected, length: %d", pkt->rx_ctrl.sig_len);
        
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write deauth packet to PCAP buffer.");
        }
    }
}

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
    bool wps_found = false;

    
    while (index + 1 < len) {
        uint8_t id = payload[index];
        uint8_t ie_len = payload[index + 1];


        if (index + 2 + ie_len > len) {
            ESP_LOGW(TAG, "Malformed IE, exceeding packet length");
            break;
        }


        if (id == 0 && ie_len <= 32) {
            memcpy(ssid, &payload[index + 2], ie_len);
            ssid[ie_len] = '\0';
        }

        
        if (id == 221 && ie_len >= 4) {
            uint32_t oui = (payload[index + 2] << 16) | (payload[index + 3] << 8) | payload[index + 4];
            uint8_t oui_type = payload[index + 5];

            if (oui == 0x0050f2 && oui_type == 0x04) {
                ESP_LOGI(TAG, "WPS IE detected for network: %s", ssid);
                wps_found = true;

                
                int attr_index = index + 6; 
                int wps_ie_end = index + 2 + ie_len;

                while (attr_index + 4 <= wps_ie_end) {
                    
                    uint16_t attr_id = (payload[attr_index] << 8) | payload[attr_index + 1];
                    uint16_t attr_len = (payload[attr_index + 2] << 8) | payload[attr_index + 3];

                    
                    ESP_LOGI(TAG, "Found WPS attribute - ID: 0x%04x, Length: %u", attr_id, attr_len);

                    
                    if (attr_len > (wps_ie_end - (attr_index + 4))) {
                        ESP_LOGW(TAG, "Invalid attribute length (attr_id: 0x%04x, attr_len: %u)", attr_id, attr_len);
                        break;
                    }


                    if (attr_id == 0x1008 && attr_len == 2) {
                        uint16_t config_methods = (payload[attr_index + 4] << 8) | payload[attr_index + 5];

                       
                        ESP_LOGI(TAG, "Configuration Methods found: 0x%04x", config_methods);

                        
                        if (config_methods & WPS_CONF_METHODS_PBC) {
                            ESP_LOGI(TAG, "WPS Push Button detected for network: %s", ssid);
                        } else if (config_methods & (WPS_CONF_METHODS_PIN_DISPLAY | WPS_CONF_METHODS_PIN_KEYPAD)) {
                            ESP_LOGI(TAG, "WPS PIN detected for network: %s", ssid);
                        } else {
                            ESP_LOGI(TAG, "WPS mode not detected (unknown config method) for network: %s", ssid);
                        }
                    }


                    attr_index += (4 + attr_len);
                }
            }
        }

        index += (2 + ie_len);
    }

    if (!wps_found) {
        ESP_LOGI(TAG, "No WPS IE found for network: %s", ssid);
    }
}