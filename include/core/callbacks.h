#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "esp_wifi_types.h"
#include <esp_timer.h>

void wifi_wps_detection_callback(void *buf, wifi_promiscuous_pkt_type_t type);

typedef struct {
    char ssid[33];
    uint8_t bssid[6]; 
    bool wps_enabled; 
} wps_network_t;

extern wps_network_t detected_wps_networks[MAX_WPS_NETWORKS];
extern int detected_network_count;
extern esp_timer_handle_t stop_timer;

#endif