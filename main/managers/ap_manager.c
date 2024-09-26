#include "managers/ap_manager.h"
#include "managers/settings_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "managers/ghost_esp_site.h"
#include <esp_http_server.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <mdns.h>
#include <cJSON.h>

#define MAX_LOG_BUFFER_SIZE 4096 // Adjust as needed
static char log_buffer[MAX_LOG_BUFFER_SIZE];
static size_t log_buffer_index = 0;

static const char* TAG = "AP_MANAGER";
static httpd_handle_t server = NULL;
static esp_netif_t* netif = NULL;

// Forward declarations
static esp_err_t http_get_handler(httpd_req_t* req);
static esp_err_t api_logs_handler(httpd_req_t* req);
static esp_err_t api_clear_logs_handler(httpd_req_t* req);
static esp_err_t api_settings_handler(httpd_req_t* req);

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

// Initialize the network interfaces, mDNS, and HTTP server
esp_err_t ap_manager_init(void) {
    esp_err_t ret;
    wifi_mode_t mode;

    // Check if Wi-Fi is already initialized and get the current mode
    ret = esp_wifi_get_mode(&mode);
    if (ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "Wi-Fi not initialized, initializing as Access Point...");

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Create default Wi-Fi AP interface
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (!netif) {
            netif = esp_netif_create_default_wifi_ap();
            if (netif == NULL) {
                ESP_LOGE(TAG, "Failed to create default Wi-Fi AP");
                return ESP_FAIL;
            }
        }
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi already initialized, skipping Wi-Fi init.");
    } else {
        ESP_LOGE(TAG, "esp_wifi_get_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set Wi-Fi mode to AP
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure Wi-Fi AP with SSID and Password
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "GhostNet",
            .ssid_len = strlen("GhostNet"),
            .password = "GhostNet",
            .channel = 6,
            .max_connection = 4,        // Set max number of connections to AP
            .authmode = WIFI_AUTH_OPEN, // WPA/WPA2 security
            .beacon_interval = 100,
        },
    };

    // Check if no password is provided (open network)
    if (strlen("GhostNet") == 0) { // lol
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to get the AP network interface");
    } else {
        // Stop DHCP server before configuring
        esp_netif_dhcps_stop(ap_netif);

        // Configure IP address
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = ESP_IP4TOADDR(192, 168, 4, 1);   // IP address (192.168.4.1)
        ip_info.gw.addr = ESP_IP4TOADDR(192, 168, 4, 1);   // Gateway (usually same as IP)
        ip_info.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0); // Subnet mask
        esp_netif_set_ip_info(ap_netif, &ip_info);


        esp_netif_dhcps_start(ap_netif);
        ESP_LOGI(TAG, "DHCP server configured successfully.");
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Wi-Fi Access Point started with SSID: %s", "GhostNet");

    // Register event handlers for Wi-Fi events if not registered already
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &event_handler, NULL));

    // Initialize mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    
    FSettings* settings = &G_Settings;
    if (settings->mdnsHostname == NULL) {
        settings->mdnsHostname = malloc(64 * sizeof(char));
        if (settings->mdnsHostname == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for mDNS hostname");
            return ESP_ERR_NO_MEM; 
        }
    }

    strcpy(settings->mdnsHostname, "ghostesp");

    ret = mdns_hostname_set(settings->mdnsHostname);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "mDNS hostname set to %s.local", settings->mdnsHostname);

    // Start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768; // Control port (use default)

    ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting HTTP server!");
        return ret;
    }

    // Register URI handlers
    httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &uri_get);

    ESP_LOGI(TAG, "HTTP server started");

    esp_wifi_set_ps(WIFI_PS_NONE);

    return ESP_OK;
}

// Deinitialize and stop the servers
void ap_manager_deinit(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    if (netif) {
        esp_netif_destroy(netif);
        netif = NULL;
    }
    mdns_free();
    ESP_LOGI(TAG, "AP Manager deinitialized");
}

// Function to add log messages (to be called from other parts of your code)
void ap_manager_add_log(const char* log_message) {
    size_t message_length = strlen(log_message);
    if (log_buffer_index + message_length < MAX_LOG_BUFFER_SIZE) {
        strcpy(&log_buffer[log_buffer_index], log_message);
        log_buffer_index += message_length;
    } else {
        ESP_LOGW(TAG, "Log buffer full, cannot add new log");
    }
}

// Handler for GET requests (serves the HTML page)
static esp_err_t http_get_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
     return httpd_resp_send(req, (const char*)ghost_site_html, ghost_site_html_size);
}

// Handler for /api/logs (returns the log buffer)
static esp_err_t api_logs_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, log_buffer, log_buffer_index);
}

// Handler for /api/clear_logs (clears the log buffer)
static esp_err_t api_clear_logs_handler(httpd_req_t* req) {
    log_buffer_index = 0;
    memset(log_buffer, 0, sizeof(log_buffer));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"logs_cleared\"}");
    return ESP_OK;
}

// Handler for /api/settings (updates settings based on JSON payload)
static esp_err_t api_settings_handler(httpd_req_t* req) {
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    char* buf = malloc(total_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON payload");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            free(buf);
            ESP_LOGE(TAG, "Failed to receive JSON payload");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0'; // Null-terminate the received data

    // Parse JSON
    cJSON* root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    // Update settings
    FSettings* settings = &G_Settings;

    // WiFi/BLE Settings
    cJSON* random_mac = cJSON_GetObjectItem(root, "random_mac");
    if (random_mac) {
        settings_set_random_mac_enabled(settings, random_mac->valueint);
    }

    cJSON* broadcast_speed = cJSON_GetObjectItem(root, "broadcast_speed");
    if (broadcast_speed) {
        settings_set_broadcast_speed(settings, broadcast_speed->valueint);
    }

    cJSON* ap_ssid = cJSON_GetObjectItem(root, "ap_ssid");
    if (ap_ssid) {
        settings_set_ap_ssid(settings, ap_ssid->valuestring);
    }

    cJSON* ap_password = cJSON_GetObjectItem(root, "ap_password");
    if (ap_password) {
        settings_set_ap_password(settings, ap_password->valuestring);
    }

    // RGB Settings
    cJSON* breath_mode = cJSON_GetObjectItem(root, "breath_mode");
    if (breath_mode) {
        settings_set_breath_mode_enabled(settings, breath_mode->valueint);
    }

    cJSON* rainbow_mode = cJSON_GetObjectItem(root, "rainbow_mode");
    if (rainbow_mode) {
        settings_set_rainbow_mode_enabled(settings, rainbow_mode->valueint);
    }

    cJSON* rgb_speed = cJSON_GetObjectItem(root, "rgb_speed");
    if (rgb_speed) {
        settings_set_rgb_speed(settings, rgb_speed->valueint);
    }

    cJSON* static_color_r = cJSON_GetObjectItem(root, "static_color_r");
    cJSON* static_color_g = cJSON_GetObjectItem(root, "static_color_g");
    cJSON* static_color_b = cJSON_GetObjectItem(root, "static_color_b");
    if (static_color_r && static_color_g && static_color_b) {
        settings_set_static_color(settings, static_color_r->valueint, static_color_g->valueint, static_color_b->valueint);
    }

    // Debug/Logs Settings
    cJSON* logs_location = cJSON_GetObjectItem(root, "logs_location");
    if (logs_location) {
        settings_set_log_location(settings, logs_location->valuestring);
    }

    cJSON* save_to_sd = cJSON_GetObjectItem(root, "save_to_sd");
    if (save_to_sd) {
        settings_set_save_to_sd(settings, save_to_sd->valueint);
    }

    cJSON* enable_logging = cJSON_GetObjectItem(root, "enable_logging");
    if (enable_logging) {
        settings_set_logging_enabled(settings, enable_logging->valueint);
    }

    // Save updated settings
    settings_save(settings);

    // Send response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"settings_updated\"}");

    // Clean up
    cJSON_Delete(root);

    return ESP_OK;
}

// Event handler for Wi-Fi events
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI(TAG, "Disconnected from Wi-Fi, retrying...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        }
    }
}