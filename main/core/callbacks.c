#include "core/callbacks.h"
#include "managers/wifi_manager.h"
#include <esp_log.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "vendor/pcap.h"
#include "vendor/GPS/gps_logger.h"
#include "managers/gps_manager.h"
#include "managers/rgb_manager.h"
#include "esp_wifi.h"
#include "managers/views/terminal_screen.h"

#define WPS_OUI 0x0050f204 
#define TAG "WIFI_MONITOR"
#define WPS_CONF_METHODS_PBC        0x0080
#define WPS_CONF_METHODS_PIN_DISPLAY 0x0004
#define WPS_CONF_METHODS_PIN_KEYPAD  0x0008
#define WIFI_PKT_DEAUTH 0x0C // Deauth subtype
#define WIFI_PKT_BEACON 0x08 // Beacon subtype
#define WIFI_PKT_PROBE_REQ 0x04  // Probe Request subtype
#define WIFI_PKT_PROBE_RESP 0x05 // Probe Response subtype
#define WIFI_PKT_EAPOL 0x80
#define ESP_WIFI_VENDOR_METADATA_LEN 8  // Channel(1) + RSSI(1) + Rate(1) + Timestamp(4) + Noise(1)

#define MIN_SSIDS_FOR_DETECTION 2  // Minimum SSIDs needed to flag as PineAP

// Structure to track blacklisted BSSIDs
typedef struct {
    uint8_t bssid[6];
    time_t detection_time;
    time_t last_update_time;  // Track last update
} blacklisted_ap_t;

static blacklisted_ap_t blacklist[MAX_PINEAP_NETWORKS];
static int blacklist_count = 0;

// Check if a BSSID is blacklisted
static bool is_blacklisted(const uint8_t* bssid) {
    for(int i = 0; i < blacklist_count; i++) {
        if(memcmp(blacklist[i].bssid, bssid, 6) == 0) {
            return true;
        }
    }
    return false;
}

// Update blacklist check to allow updates after a timeout
static bool should_update_blacklisted(const uint8_t* bssid) {
    for(int i = 0; i < blacklist_count; i++) {
        if(memcmp(blacklist[i].bssid, bssid, 6) == 0) {
            time_t current_time = time(NULL);
            // Allow updates every 30 seconds
            if(current_time - blacklist[i].last_update_time >= 30) {
                blacklist[i].last_update_time = current_time;
                return true;
            }
            return false;
        }
    }
    return false;
}

// Update the add_to_blacklist function
static void add_to_blacklist(const uint8_t* bssid) {
    time_t current_time = time(NULL);
    
    // First check if BSSID exists
    for(int i = 0; i < blacklist_count; i++) {
        if(memcmp(blacklist[i].bssid, bssid, 6) == 0) {
            blacklist[i].last_update_time = current_time;
            return;
        }
    }
    
    // If not found and we have space, add new entry
    if(blacklist_count < MAX_PINEAP_NETWORKS) {
        memcpy(blacklist[blacklist_count].bssid, bssid, 6);
        blacklist[blacklist_count].detection_time = current_time;
        blacklist[blacklist_count].last_update_time = current_time;
        blacklist_count++;
    }
}

// Forward declarations
static uint32_t hash_ssid(const char* ssid);
static bool ssid_hash_exists(pineap_network_t* network, uint32_t hash);
static void trim_trailing(char *str);
static bool compare_bssid(const uint8_t *bssid1, const uint8_t *bssid2);
static bool is_beacon_packet(const wifi_promiscuous_pkt_t *pkt);

wps_network_t detected_wps_networks[MAX_WPS_NETWORKS];
int detected_network_count = 0;
esp_timer_handle_t stop_timer;
int should_store_wps = 1;
gps_t *gps = NULL;
extern RGBManager_t rgb_manager;

#define MAX_PINEAP_NETWORKS 50
#define MAX_SSIDS_PER_BSSID 10
#define MAX_WIFI_CHANNEL 13
#define CHANNEL_HOP_INTERVAL_MS 200
#define RECENT_SSID_COUNT 5  // Keep track of last 5 SSIDs
#define LOG_DELAY_MS 5000  // 5 second delay between logs

static pineap_network_t pineap_networks[MAX_PINEAP_NETWORKS];
static int pineap_network_count = 0;
static bool pineap_detection_active = false;
static uint8_t current_channel = 1;
static esp_timer_handle_t channel_hop_timer = NULL;

static void channel_hop_timer_callback(void* arg) {
    if (!pineap_detection_active) return;
    
    current_channel = (current_channel % MAX_WIFI_CHANNEL) + 1;
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
}

static esp_err_t start_channel_hopping(void) {
    esp_timer_create_args_t timer_args = {
        .callback = channel_hop_timer_callback,
        .name = "channel_hop"
    };
    
    if (channel_hop_timer == NULL) {
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &channel_hop_timer));
    }
    
    // Start timer for channel hopping
    return esp_timer_start_periodic(channel_hop_timer, CHANNEL_HOP_INTERVAL_MS * 1000);
}

static void stop_channel_hopping(void) {
    if (channel_hop_timer) {
        esp_timer_stop(channel_hop_timer);
        esp_timer_delete(channel_hop_timer);
        channel_hop_timer = NULL;
    }
}

// Helper function to find or create network entry
static pineap_network_t* find_or_create_network(const uint8_t* bssid) {
    // First try to find existing network
    for (int i = 0; i < pineap_network_count; i++) {
        if (compare_bssid(pineap_networks[i].bssid, bssid)) {
            return &pineap_networks[i];
        }
    }
    
    // If not found and we have space, create new entry
    if (pineap_network_count < MAX_PINEAP_NETWORKS) {
        pineap_network_t* network = &pineap_networks[pineap_network_count++];
        memcpy(network->bssid, bssid, 6);
        network->ssid_count = 0;
        network->is_pineap = false;
        network->first_seen = time(NULL);
        return network;
    }
    
    return NULL;
}

static uint32_t hash_ssid(const char* ssid) {
    uint32_t hash = 5381;
    int c;
    while ((c = *ssid++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}

static bool ssid_hash_exists(pineap_network_t* network, uint32_t hash) {
    for (int i = 0; i < network->ssid_count; i++) {
        if (network->ssid_hashes[i] == hash) {
            return true;
        }
    }
    return false;
}

void start_pineap_detection(void) {
    pineap_detection_active = true;
    pineap_network_count = 0;
    memset(pineap_networks, 0, sizeof(pineap_networks));
    current_channel = 1;
    start_channel_hopping();
}

void stop_pineap_detection(void) {
    pineap_detection_active = false;
    stop_channel_hopping();
}

static void log_pineap_detection(void* arg) {
    pineap_log_data_t* log_data = (pineap_log_data_t*)arg;
    pineap_network_t* network = log_data->network;

    // Add delay before logging to allow collection of multiple SSIDs
    vTaskDelay(pdMS_TO_TICKS(5000));  // 5 second delay

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
            log_data->bssid[0], log_data->bssid[1], log_data->bssid[2],
            log_data->bssid[3], log_data->bssid[4], log_data->bssid[5]);

    // Build SSIDs string, filtering out empty SSIDs
    char ssids_str[256] = {0};
    int valid_ssid_count = 0;
    
    // Use the most up-to-date SSIDs from the network structure
    for (int i = 0; i < network->ssid_count && i < RECENT_SSID_COUNT; i++) {
        if (strlen(network->recent_ssids[i]) > 0) {
            if (valid_ssid_count > 0) strcat(ssids_str, ", ");
            strcat(ssids_str, network->recent_ssids[i]);
            valid_ssid_count++;
        }
    }

    // Only log if we have valid SSIDs
    if (valid_ssid_count >= MIN_SSIDS_FOR_DETECTION) {
        // Pulse RGB purple (red + blue) to indicate Pineapple detection
        pulse_once(&rgb_manager, 255, 0, 255);

        printf("\nPineapple detected!\n");
        printf("BSSID: %s\n", mac_str);
        printf("Channel: %d\n", network->last_channel);
        printf("RSSI: %d\n", network->last_rssi);
        printf("SSIDs (%d): %s\n", valid_ssid_count, ssids_str);

        TERMINAL_VIEW_ADD_TEXT("\nPineapple detected!\n");
        TERMINAL_VIEW_ADD_TEXT("BSSID: %s\n", mac_str);
        TERMINAL_VIEW_ADD_TEXT("Channel: %d\n", network->last_channel);
        TERMINAL_VIEW_ADD_TEXT("RSSI: %d\n", network->last_rssi);
        TERMINAL_VIEW_ADD_TEXT("SSIDs (%d): %s\n", valid_ssid_count, ssids_str);
    }

    // Clean up
    free(log_data);
    network->log_task_handle = NULL;  // Clear handle before deletion
    vTaskDelete(NULL);
}

static void start_log_task(pineap_network_t* network, const char* new_ssid, int8_t channel, int8_t rssi) {
    // Check if a task is already running
    if (network->log_task_handle != NULL) {
        TaskHandle_t existing_handle = network->log_task_handle;
        network->log_task_handle = NULL;  // Clear it first to avoid race conditions
        vTaskDelete(existing_handle);  // Clean up existing task
    }

    pineap_log_data_t* log_data = malloc(sizeof(pineap_log_data_t));
    if (!log_data) return;

    // Copy network data
    memcpy(log_data->bssid, network->bssid, 6);
    memcpy(log_data->recent_ssids, network->recent_ssids, sizeof(network->recent_ssids));
    log_data->ssid_count = network->ssid_count;
    log_data->channel = channel;
    log_data->rssi = rssi;
    log_data->network = network;  // Store network pointer for cleanup

    // Create logging task
    BaseType_t result = xTaskCreate(log_pineap_detection, "pineap_log", 4096, log_data, 1, &network->log_task_handle);
    if (result != pdPASS) {
        // Task creation failed
        free(log_data);
        network->log_task_handle = NULL;
    }
}

// Helper function to check if SSID is valid and unique
static bool is_valid_unique_ssid(const char* new_ssid, pineap_network_t* network) {
    // Check if SSID is empty or just whitespace
    if (strlen(new_ssid) == 0) return false;
    
    bool all_whitespace = true;
    for (const char* p = new_ssid; *p; p++) {
        if (!isspace((unsigned char)*p)) {
            all_whitespace = false;
            break;
        }
    }
    if (all_whitespace) return false;

    // Check if this SSID is already in our recent list
    for (int i = 0; i < network->ssid_count && i < RECENT_SSID_COUNT; i++) {
        if (strcasecmp(network->recent_ssids[i], new_ssid) == 0) {
            return false;  // SSID already exists (case insensitive)
        }
    }

    return true;
}

void wifi_pineap_detector_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!pineap_detection_active || type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    // Only process beacon frames
    if (!is_beacon_packet(ppkt)) return;

    // Find or create network
    pineap_network_t* network = find_or_create_network(hdr->addr3);
    if (!network) return;

    // Update channel and RSSI
    network->last_channel = ppkt->rx_ctrl.channel;
    network->last_rssi = ppkt->rx_ctrl.rssi;

    // Extract SSID from beacon
    const uint8_t *payload = ppkt->payload;
    int len = ppkt->rx_ctrl.sig_len;
    
    // Skip fixed parameters (24 bytes header + 12 bytes fixed params)
    int index = 36;
    if (index + 2 > len) return;

    // Look specifically for SSID element (ID = 0)
    if (payload[index] != 0) return;
    
    uint8_t ie_len = payload[index + 1];
    if (ie_len > 32 || index + 2 + ie_len > len) return;

    // Get SSID
    char ssid[33] = {0};
    memcpy(ssid, &payload[index + 2], ie_len);
    ssid[ie_len] = '\0';
    trim_trailing(ssid);
    
    // Only proceed if this is a valid and unique SSID
    if (!is_valid_unique_ssid(ssid, network)) return;
    
    uint32_t ssid_hash = hash_ssid(ssid);

    // If this is a new SSID hash for this BSSID, add it
    if (!ssid_hash_exists(network, ssid_hash) && network->ssid_count < MAX_SSIDS_PER_BSSID) {
        network->ssid_hashes[network->ssid_count++] = ssid_hash;
        
        // Add to recent SSIDs circular buffer
        strncpy(network->recent_ssids[network->recent_ssid_index], ssid, 32);
        network->recent_ssid_index = (network->recent_ssid_index + 1) % RECENT_SSID_COUNT;

        // If we detect multiple SSIDs from same BSSID, mark as potential Pineap
        if (network->ssid_count >= MIN_SSIDS_FOR_DETECTION && 
            (!is_blacklisted(hdr->addr3) || should_update_blacklisted(hdr->addr3))) {
            
            network->is_pineap = true;
            add_to_blacklist(hdr->addr3);

            // Create new logging task if previous one has completed
            if (network->log_task_handle == NULL) {
                pineap_log_data_t* log_data = malloc(sizeof(pineap_log_data_t));
                if (!log_data) return;

                memcpy(log_data->bssid, network->bssid, 6);
                log_data->network = network;  // Pass network pointer for up-to-date info

                BaseType_t result = xTaskCreate(log_pineap_detection, "pineap_log", 4096, log_data, 1, &network->log_task_handle);
                if (result != pdPASS) {
                    free(log_data);
                    network->log_task_handle = NULL;
                }
            }
            
            // Write to PCAP if capture is active
            if (pcap_file != NULL) {
                pcap_write_packet_to_buffer(ppkt->payload, ppkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
            }
        }
    }
}

static void trim_trailing(char *str) {
    int i = strlen(str) - 1;
    while (i >= 0 && (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
        str[i] = '\0';
        i--;
    }
}

void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        break;
    default:
        break;
    }
}


bool compare_bssid(const uint8_t *bssid1, const uint8_t *bssid2) {
    for (int i = 0; i < 6; i++) {
        if (bssid1[i] != bssid2[i]) {
            return false;
        }
    }
    return true;
}

bool is_network_duplicate(const char *ssid, const uint8_t *bssid) {
    for (int i = 0; i < detected_network_count; i++) {
        if (strcmp(detected_wps_networks[i].ssid, ssid) == 0 && compare_bssid(detected_wps_networks[i].bssid, bssid)) {
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

bool is_eapol_response(const wifi_promiscuous_pkt_t *pkt) {
    const uint8_t *frame = pkt->payload;

    
    if ((frame[30] == 0x88 && frame[31] == 0x8E) || 
        (frame[32] == 0x88 && frame[33] == 0x8E)) {
        return true;
    }

    return false;
}

bool is_pwn_response(const wifi_promiscuous_pkt_t *pkt) {
    const uint8_t *frame = pkt->payload;
    
    if (frame[0] == 0x80)
    {
        return true;
    }

    return false;
}


void wifi_raw_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("RAW_SCAN", "Failed to write packet to buffer");
        }
    }
}

void wardriving_scan_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
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
    uint8_t bssid[6];
    memcpy(bssid, hdr->addr3, 6);

    int rssi = pkt->rx_ctrl.rssi;
    int channel = pkt->rx_ctrl.channel;

    bool network_found = false;
    char encryption_type[8] = "OPEN";

    
    while (index + 1 < len) {
        uint8_t id = payload[index];
        uint8_t ie_len = payload[index + 1];

        if (index + 2 + ie_len > len) {
            break;
        }

        
        if (id == 0 && ie_len <= 32) {
            memcpy(ssid, &payload[index + 2], ie_len);
            ssid[ie_len] = '\0';
            trim_trailing(ssid);
        }

        
        if (id == 48) {
            strncpy(encryption_type, "WPA2", sizeof(encryption_type));
        } else if (id == 221) {
            uint32_t oui = (payload[index + 2] << 16) | (payload[index + 3] << 8) | payload[index + 4];
            uint8_t oui_type = payload[index + 5];
            if (oui == 0x0050f2 && oui_type == 0x01) {
                strncpy(encryption_type, "WPA", sizeof(encryption_type));
            } else if (oui == 0x0050f2 && oui_type == 0x02) {
                strncpy(encryption_type, "WEP", sizeof(encryption_type));
            }
        }

        index += (2 + ie_len);
    }

    double latitude = 0;
    double longitude = 0;

    if (gps != NULL)
    {
        latitude = gps->latitude;
        longitude = gps->longitude;
    }
    

    
    wardriving_data_t wardriving_data;
    strncpy(wardriving_data.ssid, ssid, sizeof(wardriving_data.ssid) - 1);
    wardriving_data.ssid[sizeof(wardriving_data.ssid) - 1] = '\0';  // Null-terminate
    snprintf(wardriving_data.bssid, sizeof(wardriving_data.bssid), "%02x:%02x:%02x:%02x:%02x:%02x",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    wardriving_data.rssi = rssi;
    wardriving_data.channel = channel;
    wardriving_data.latitude = latitude;
    wardriving_data.longitude = longitude;
    strncpy(wardriving_data.encryption_type, encryption_type, sizeof(wardriving_data.encryption_type) - 1);
    wardriving_data.encryption_type[sizeof(wardriving_data.encryption_type) - 1] = '\0';

    esp_err_t err = gps_manager_log_wardriving_data(&wardriving_data);
}

void wifi_probe_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("PROBE_SCAN", "Failed to write packet to buffer");
        }
    }
}

void wifi_beacon_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("BEACON_SCAN", "Failed to write packet to buffer");
        }
    }
}

void wifi_deauth_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("DEAUTH_SCAN", "Failed to write packet to buffer");
        }
    }
}

void wifi_pwn_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("PWN_SCAN", "Failed to write packet to buffer");
        }
    }
}

void wifi_eapol_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len > 0) {
        esp_err_t ret = pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
        if (ret != ESP_OK) {
            ESP_LOGE("EAPOL_SCAN", "Failed to write packet to buffer");
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
    uint8_t bssid[6];
    memcpy(bssid, hdr->addr3, 6);

    
    while (index + 1 < len) {
        uint8_t id = payload[index];
        uint8_t ie_len = payload[index + 1];

       
        if (index + 2 + ie_len > len) {
            break;
        }


        if (id == 0 && ie_len <= 32) {
            memcpy(ssid, &payload[index + 2], ie_len);
            ssid[ie_len] = '\0';
            trim_trailing(ssid);
        }


        if (is_network_duplicate(ssid, bssid)) {
            return;
        }


        if (id == 221 && ie_len >= 4) {
            uint32_t oui = (payload[index + 2] << 16) | (payload[index + 3] << 8) | payload[index + 4];
            uint8_t oui_type = payload[index + 5];

           
            if (oui == 0x0050f2 && oui_type == 0x04) {
                wps_found = true;

                
                int attr_index = index + 6;
                int wps_ie_end = index + 2 + ie_len;

                while (attr_index + 4 <= wps_ie_end) {
                    uint16_t attr_id = (payload[attr_index] << 8) | payload[attr_index + 1];
                    uint16_t attr_len = (payload[attr_index + 2] << 8) | payload[attr_index + 3];

                    
                    if (attr_len > (wps_ie_end - (attr_index + 4))) {
                        break;
                    }

                    
                    if (attr_id == 0x1008 && attr_len == 2) {
                        uint16_t config_methods = (payload[attr_index + 4] << 8) | payload[attr_index + 5];

                       
                        printf("Configuration Methods found: 0x%04x", config_methods);

                        
                        if (config_methods & WPS_CONF_METHODS_PBC) {
                            printf("WPS Push Button detected for network: %s", ssid);
                        } else if (config_methods & (WPS_CONF_METHODS_PIN_DISPLAY | WPS_CONF_METHODS_PIN_KEYPAD)) {
                            printf("WPS PIN detected for network: %s", ssid);
                        } else {
                            printf("WPS mode not detected (unknown config method) for network: %s", ssid);
                        }


                        if (should_store_wps == 1)
                        {
                            wps_network_t new_network;
                            strncpy(new_network.ssid, ssid, sizeof(new_network.ssid) - 1);
                            new_network.ssid[sizeof(new_network.ssid) - 1] = '\0';  // Ensure null termination
                            memcpy(new_network.bssid, bssid, sizeof(new_network.bssid));
                            new_network.wps_enabled = true;
                            new_network.wps_mode = config_methods & (WPS_CONF_METHODS_PIN_DISPLAY | WPS_CONF_METHODS_PIN_KEYPAD) ? WPS_MODE_PIN : WPS_MODE_PBC;

                            detected_wps_networks[detected_network_count++] = new_network;
                        }
                        else 
                        {
                            pcap_write_packet_to_buffer(pkt->payload, pkt->rx_ctrl.sig_len, PCAP_CAPTURE_WIFI);
                        }
                        
                        if (detected_network_count >= MAX_WPS_NETWORKS) {
                            printf("Maximum number of WPS networks detected. Stopping monitor mode.");
                            wifi_manager_stop_monitor_mode();
                        }     

                    }


                    attr_index += (4 + attr_len);
                }
            }
        }


        index += (2 + ie_len);
    }
}

#ifndef CONFIG_IDF_TARGET_ESP32S2
// Forward declare the struct and callback before use
struct ble_hs_adv_field;
static int ble_hs_adv_parse_fields_cb(const struct ble_hs_adv_field *field, void *arg);

void ble_wardriving_callback(struct ble_gap_event *event, void *arg) {
    if (!event || event->type != BLE_GAP_EVENT_DISC) {
        return;
    }

    wardriving_data_t wardriving_data = {0};
    wardriving_data.ble_data.is_ble_device = true;

    // Get BLE MAC and RSSI
    snprintf(wardriving_data.ble_data.ble_mac, sizeof(wardriving_data.ble_data.ble_mac),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             event->disc.addr.val[0], event->disc.addr.val[1], 
             event->disc.addr.val[2], event->disc.addr.val[3], 
             event->disc.addr.val[4], event->disc.addr.val[5]);
    
    wardriving_data.ble_data.ble_rssi = event->disc.rssi;
    
    // Parse BLE name if available
    if (event->disc.length_data > 0) {
        ble_hs_adv_parse(event->disc.data, 
                        event->disc.length_data,
                        ble_hs_adv_parse_fields_cb,
                        &wardriving_data);
    }

    // Get GPS data from the global handle
    gps_t* gps = &((esp_gps_t*)nmea_hdl)->parent;
    if (gps != NULL && gps->valid) {
        wardriving_data.gps_quality.satellites_used = gps->sats_in_use;
        wardriving_data.gps_quality.hdop = gps->dop_h;
        wardriving_data.gps_quality.speed = gps->speed;
        wardriving_data.gps_quality.course = gps->cog;
        wardriving_data.gps_quality.fix_quality = gps->fix;
        wardriving_data.gps_quality.has_valid_fix = (gps->fix >= GPS_FIX_GPS);
    }

    // Use GPS manager to log data
    esp_err_t err = gps_manager_log_wardriving_data(&wardriving_data);
    if (err != ESP_OK) {
        ESP_LOGD("BLE_WD", "Skipped logging entry - GPS data not ready");
    }
}

// Move the callback implementation inside the ESP32S2 guard
static int ble_hs_adv_parse_fields_cb(const struct ble_hs_adv_field *field, void *arg) {
    wardriving_data_t *data = (wardriving_data_t *)arg;
    
    if (field->type == BLE_HS_ADV_TYPE_COMP_NAME) {
        size_t name_len = MIN(field->length, sizeof(data->ble_data.ble_name) - 1);
        memcpy(data->ble_data.ble_name, field->value, name_len);
        data->ble_data.ble_name[name_len] = '\0';
    }
    
    return 0;
}
#endif


// wrap for esp32s2
#ifndef CONFIG_IDF_TARGET_ESP32S2
static const char *SKIMMER_TAG = "SKIMMER_DETECT";

// suspicious device names commonly used in skimmers
static const char *suspicious_names[] = {
    "HC-03", "HC-05", "HC-06", "HC-08", 
    "BT-HC05", "JDY-31", "AT-09", "HM-10",
    "CC41-A", "MLT-BT05", "SPP-CA", "FFD0"
};
static const int suspicious_names_count = sizeof(suspicious_names) / sizeof(suspicious_names[0]);
void ble_skimmer_scan_callback(struct ble_gap_event *event, void *arg) {
    if (!event || event->type != BLE_GAP_EVENT_DISC) {
        return;
    }

    struct ble_hs_adv_fields fields;
    int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, 
                                    event->disc.length_data);
    
    if (rc != 0) {
        ESP_LOGD(SKIMMER_TAG, "Failed to parse advertisement data");
        return;
    }

    // Check device name
    if (fields.name != NULL && fields.name_len > 0) {
        char device_name[32] = {0};
        size_t name_len = MIN(fields.name_len, sizeof(device_name) - 1);
        memcpy(device_name, fields.name, name_len);

        // Check against suspicious names
        for (int i = 0; i < suspicious_names_count; i++) {
            if (strcasecmp(device_name, suspicious_names[i]) == 0) {
                char mac_addr[18];
                snprintf(mac_addr, sizeof(mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
                        event->disc.addr.val[0], event->disc.addr.val[1],
                        event->disc.addr.val[2], event->disc.addr.val[3],
                        event->disc.addr.val[4], event->disc.addr.val[5]);

                ESP_LOGW(SKIMMER_TAG, "POTENTIAL SKIMMER DETECTED!");
                ESP_LOGW(SKIMMER_TAG, "Device Name: %s", device_name);
                ESP_LOGW(SKIMMER_TAG, "MAC Address: %s", mac_addr);
                ESP_LOGW(SKIMMER_TAG, "RSSI: %d dBm", event->disc.rssi);

                // pulse rgb red once when skimmer is detected
                pulse_once(&rgb_manager, 255, 0, 0);

                // Create enhanced PCAP packet with metadata
                if (pcap_file != NULL) {
                    // Format: [Timestamp][MAC][RSSI][Name][Raw Data]
                    uint8_t enhanced_packet[256] = {0};
                    size_t packet_len = 0;
                    
                    // Add MAC address
                    memcpy(enhanced_packet + packet_len, event->disc.addr.val, 6);
                    packet_len += 6;
                    
                    // Add RSSI
                    enhanced_packet[packet_len++] = (uint8_t)event->disc.rssi;
                    
                    // Add device name length and name
                    enhanced_packet[packet_len++] = (uint8_t)name_len;
                    memcpy(enhanced_packet + packet_len, device_name, name_len);
                    packet_len += name_len;
                    
                    // Add reason for flagging
                    const char *reason = suspicious_names[i];
                    uint8_t reason_len = strlen(reason);
                    enhanced_packet[packet_len++] = reason_len;
                    memcpy(enhanced_packet + packet_len, reason, reason_len);
                    packet_len += reason_len;
                    
                    // Add raw advertisement data
                    memcpy(enhanced_packet + packet_len, 
                           event->disc.data, 
                           event->disc.length_data);
                    packet_len += event->disc.length_data;

                    // Write to PCAP with proper BLE packet format
                    pcap_write_packet_to_buffer(enhanced_packet, 
                                              packet_len,
                                              PCAP_CAPTURE_BLUETOOTH);
                    
                    // Force flush to ensure suspicious device is captured
                    pcap_flush_buffer_to_file();
                }
                break;
            }
        }
    }
}
#endif

