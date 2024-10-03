// wps_manager.h

#ifndef WPS_MANAGER_H
#define WPS_MANAGER_H

#include "esp_wifi.h"

void wps_manager_start();


void wps_manager_stop();


void wps_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


#endif