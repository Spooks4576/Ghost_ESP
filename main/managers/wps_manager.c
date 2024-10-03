#include "managers/wps_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wps.h"
#include <string.h>

#define TAG "WPS_MANAGER"


void wps_manager_start(const char* pin) {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wps_config_t config = {
        .wps_type = WPS_TYPE_PIN,
        .pin = {0},
        .factory_info = {
            .manufacturer = "SAMMYSAM",
            .model_number = "siola",
            .model_name = "SS",
            .device_name = "SS",
        }
    };

    strncpy((char*)config.pin, pin, 8);


    ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));

    
    ESP_ERROR_CHECK(esp_wifi_wps_start(0));
    ESP_LOGI(TAG, "WPS PIN mode started");
}


void wps_manager_stop() {
    ESP_LOGI(TAG, "Stopping WPS...");
    ESP_ERROR_CHECK(esp_wifi_wps_disable());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_LOGI(TAG, "WPS stopped.");
}



void wps_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected to WiFi");
    }
    else if (event_base == WIFI_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_SUCCESS) {
        ESP_LOGI(TAG, "WPS Success! Connected to the network.");
        esp_wifi_wps_disable();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_FAILED) {
        ESP_LOGI(TAG, "WPS Failed");
        esp_wifi_wps_disable();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_PIN) {
        ESP_LOGI(TAG, "WPS PIN is: %s", (char*)event_data);
    }
}