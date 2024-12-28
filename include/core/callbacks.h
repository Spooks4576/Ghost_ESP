#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "esp_wifi_types.h"
#include <esp_timer.h>
#include "vendor/GPS/MicroNMEA.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// nimble 
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "host/ble_gap.h"
#endif

#define MAX_PINEAP_NETWORKS 50
#define MAX_SSIDS_PER_BSSID 10
#define RECENT_SSID_COUNT 5

// PineAP detection structures
typedef struct {
    uint8_t bssid[6];
    uint8_t ssid_count;
    bool is_pineap;
    time_t first_seen;
    uint32_t ssid_hashes[MAX_SSIDS_PER_BSSID];
    // Circular buffer for recent SSIDs
    char recent_ssids[RECENT_SSID_COUNT][33];
    uint8_t recent_ssid_index;
    TaskHandle_t log_task_handle;
    int8_t last_channel;
    int8_t last_rssi;
} pineap_network_t;

// Structure for passing data to logging task
typedef struct {
    uint8_t bssid[6];
    char recent_ssids[RECENT_SSID_COUNT][33];
    int ssid_count;
    int8_t channel;
    int8_t rssi;
    struct pineap_network_t* network;  // Add network pointer
} pineap_log_data_t;

// PineAP detection control functions
void start_pineap_detection(void);
void stop_pineap_detection(void);

// Forward declarations of callback functions
void wifi_pineap_detector_callback(void *buf, wifi_promiscuous_pkt_type_t type);
void wifi_wps_detection_callback(void *buf, wifi_promiscuous_pkt_type_t type);
void wifi_beacon_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_deauth_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_pwn_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_probe_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_raw_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifi_eapol_scan_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void wardriving_scan_callback(void *buf, wifi_promiscuous_pkt_type_t type);
void ble_wardriving_callback(struct ble_gap_event *event, void *arg);
void ble_skimmer_scan_callback(struct ble_gap_event *event, void *arg);
void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_stations_sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type);


typedef enum {
    WPS_MODE_NONE = 0,   // No WPS support
    WPS_MODE_PBC,        // Push Button Configuration (PBC)
    WPS_MODE_PIN         // PIN method (Display or Keypad)
} wps_modes_t;

typedef struct {
    char ssid[33];        // SSID (max 32 characters + null terminator)
    uint8_t bssid[6];     // BSSID (MAC address)
    bool wps_enabled;     // True if WPS is enabled
    wps_modes_t wps_mode;  // WPS mode (PIN or PBC)
} wps_network_t;

extern gps_t *gps;
extern wps_network_t detected_wps_networks[MAX_WPS_NETWORKS];
extern int detected_network_count;
extern esp_timer_handle_t stop_timer;
extern int should_store_wps;
static uint8_t router_ip[4];

#endif
