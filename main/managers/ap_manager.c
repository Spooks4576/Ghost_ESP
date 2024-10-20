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
#include <core/serial_manager.h>
#include <mdns.h>
#include <cJSON.h>
#include <math.h>

#define MAX_LOG_BUFFER_SIZE 4096 // Adjust as needed
#define MIN_(a,b) ((a) < (b) ? (a) : (b))
static char log_buffer[MAX_LOG_BUFFER_SIZE];
static size_t log_buffer_index = 0;

static const char* TAG = "AP_MANAGER";
static httpd_handle_t server = NULL;
static esp_netif_t* netif = NULL;
static bool mdns_freed = false;

// Forward declarations
static esp_err_t http_get_handler(httpd_req_t* req);
static esp_err_t api_logs_handler(httpd_req_t* req);
static esp_err_t api_clear_logs_handler(httpd_req_t* req);
static esp_err_t api_settings_handler(httpd_req_t* req);
static esp_err_t api_command_handler(httpd_req_t *req);

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);


esp_err_t ap_manager_init(void) {
    esp_err_t ret;
    wifi_mode_t mode;


    ret = esp_wifi_get_mode(&mode);
    if (ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "Wi-Fi not initialized, initializing as Access Point...");

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
            return ret;
        }


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

    
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    const char* ssid = strlen(settings_get_ap_ssid(&G_Settings)) > 0 ? settings_get_ap_ssid(&G_Settings) : "GhostNet";
    
    const char* password = strlen(settings_get_ap_password(&G_Settings)) > 8 ? settings_get_ap_password(&G_Settings) : "GhostNet";

    
    wifi_config_t wifi_config = {
    .ap = {
        .channel = 6,
        .max_connection = 4,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .beacon_interval = 100,
    },
    };

    
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';

    
    wifi_config.ap.ssid_len = strlen(ssid);

    
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
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

    ESP_LOGI(TAG, "Wi-Fi Access Point started with SSID: %s", ssid);

    // Register event handlers for Wi-Fi events if not registered already
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Initialize mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    mdns_freed = false;

    
    FSettings* settings = &G_Settings;

    ret = mdns_hostname_set("ghostesp");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s", esp_err_to_name(ret));
        return ret;
    }

    
    ESP_LOGI(TAG, "mDNS hostname set to %s.local", "ghostesp");

    ret = mdns_service_add(NULL, "_http", "_http", 80, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mDNS service add failed: %s", esp_err_to_name(ret));
        return ret;
    }

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

    httpd_uri_t uri_post_logs = {
        .uri       = "/api/logs",
        .method    = HTTP_GET,
        .handler   = api_logs_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_POST,
        .handler   = api_settings_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_post_command = {
        .uri       = "/api/command",
        .method    = HTTP_POST,
        .handler   = api_command_handler,
        .user_ctx  = NULL
    };

    ret = httpd_register_uri_handler(server, &uri_post_logs);
        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }


    ret = httpd_register_uri_handler(server, &uri_post_settings);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }
    ret = httpd_register_uri_handler(server, &uri_get);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }

    ret = httpd_register_uri_handler(server, &uri_post_command);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }

    ESP_LOGI(TAG, "HTTP server started");

    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "ESP32 AP IP Address: " IPSTR, IP2STR(&ip_info.ip));
    } else {
        ESP_LOGE(TAG, "Failed to get IP address");
    }

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


void ap_manager_add_log(const char* log_message) {
    size_t message_length = strlen(log_message);
    if (log_buffer_index + message_length < MAX_LOG_BUFFER_SIZE) {
        strcpy(&log_buffer[log_buffer_index], log_message);
        log_buffer_index += message_length;
    } else {
        ESP_LOGW(TAG, "Log buffer full, clearing buffer and adding new log");

        memset(log_buffer, 0, MAX_LOG_BUFFER_SIZE);
        log_buffer_index = 0;


        strcpy(&log_buffer[log_buffer_index], log_message);
        log_buffer_index += message_length;
    }

    printf(log_message);
}

esp_err_t ap_manager_start_services() {
    esp_err_t ret;

    // Set Wi-Fi mode to AP
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start Wi-Fi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_hostname_set("ghostesp");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start HTTPD server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting HTTP server!");
        return ret;
    }

     httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_get_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_post_logs = {
        .uri       = "/api/logs",
        .method    = HTTP_GET,
        .handler   = api_logs_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_POST,
        .handler   = api_settings_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_post_command = {
        .uri       = "/api/command",
        .method    = HTTP_POST,
        .handler   = api_command_handler,
        .user_ctx  = NULL
    };

    ret = httpd_register_uri_handler(server, &uri_post_logs);
        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }


    ret = httpd_register_uri_handler(server, &uri_post_settings);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }
    ret = httpd_register_uri_handler(server, &uri_get);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }

    ret = httpd_register_uri_handler(server, &uri_post_command);

        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error registering URI /");
    }

    ESP_LOGI(TAG, "HTTP server started");

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "ESP32 AP IP Address: " IPSTR, IP2STR(&ip_info.ip));
    } else {
        ESP_LOGE(TAG, "Failed to get IP address");
    }

    return ESP_OK;
}

void ap_manager_stop_services() {
    wifi_mode_t wifi_mode;
    esp_err_t err = esp_wifi_get_mode(&wifi_mode);

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));

    if (err == ESP_OK) {
        if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_APSTA) {
            ESP_LOGI(TAG, "Stopping Wi-Fi...");
            ESP_ERROR_CHECK(esp_wifi_stop());
        }
    } else {
        ESP_LOGE(TAG, "Failed to get Wi-Fi mode, error: %d", err);
    }


    if (server) {
        httpd_stop(server);
        server = NULL;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    if (!mdns_freed)
    {
        mdns_free();
        mdns_freed = true;
    }
}


// Handler for GET requests (serves the HTML page)
static esp_err_t http_get_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Received HTTP GET request: %s", req->uri);
    httpd_resp_set_type(req, "text/html");
     return httpd_resp_send(req, (const char*)ghost_site_html, ghost_site_html_size);
}

static void handle_serial_command_task(void *pvParameters) {
    const char *command = (char*)pvParameters;

    // Execute the command
    if (handle_serial_command(command) == ESP_OK) {
        ESP_LOGI(TAG, "Command executed successfully: %s", command);
    } else {
        ESP_LOGE(TAG, "Command execution failed: %s", command);
    }
    

    vTaskDelete(NULL);
}

static esp_err_t api_command_handler(httpd_req_t *req)
{
    char content[100];
    int ret, command_len;

    
    command_len = MIN_(req->content_len, sizeof(content) - 1); 

    
    ret = httpd_req_recv(req, content, command_len);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);  
        }
        return ESP_FAIL;
    }

    
    content[command_len] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Invalid JSON", strlen("Invalid JSON"));
        return ESP_FAIL;
    }


    cJSON *command_json = cJSON_GetObjectItem(json, "command");
    if (command_json == NULL || !cJSON_IsString(command_json)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Missing or invalid 'command' field", strlen("Missing or invalid 'command' field"));
        cJSON_Delete(json);  // Cleanup JSON object
        return ESP_FAIL;
    }

    
    const char *command = command_json->valuestring;


    if (xTaskCreate(handle_serial_command_task, "serial_command_task", 4096, command, 5, NULL) != pdPASS) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Failed to create task", strlen("Failed to create task"));
        cJSON_Delete(json);
        return ESP_FAIL;
    }
   
    httpd_resp_send(req, "Command executed", strlen("Command executed"));

    cJSON_Delete(json);
    return ESP_OK;
}


static esp_err_t api_logs_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    if (log_buffer_index > 0) {
        char sse_event[2046];
        size_t log_offset = 0;

        while (log_offset < log_buffer_index) {
            size_t chunk_size = log_buffer_index - log_offset;
            if (chunk_size > 2046) {
                chunk_size = 2046;
            }

            snprintf(sse_event, sizeof(sse_event), "data: %.*s\n\n", (int)chunk_size, log_buffer + log_offset);
            httpd_resp_sendstr_chunk(req, sse_event);

            log_offset += chunk_size;
        }

        log_buffer_index = 0;
    } else {
        httpd_resp_sendstr_chunk(req, "data: [No new logs]\n\n");
    }

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
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

    // Core settings
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

    cJSON* rgb_mode = cJSON_GetObjectItem(root, "rgb_mode");
    if (rgb_mode) {
        settings_set_rgb_mode(settings, (RGBMode)rgb_mode->valueint);
    }

    cJSON* rgb_speed = cJSON_GetObjectItem(root, "rgb_speed");
    if (rgb_speed) {
        settings_set_rgb_speed(settings, rgb_speed->valueint);
    }

    cJSON* channel_delay = cJSON_GetObjectItem(root, "channel_delay");
    if (channel_delay) {
        settings_set_channel_delay(settings, (float)channel_delay->valuedouble);
    }

    // Evil Portal settings
    cJSON* portal_url = cJSON_GetObjectItem(root, "portal_url");
    if (portal_url) {
        settings_set_portal_url(settings, portal_url->valuestring);
    }

    cJSON* portal_ssid = cJSON_GetObjectItem(root, "portal_ssid");
    if (portal_ssid) {
        settings_set_portal_ssid(settings, portal_ssid->valuestring);
    }

    cJSON* portal_password = cJSON_GetObjectItem(root, "portal_password");
    if (portal_password) {
        settings_set_portal_password(settings, portal_password->valuestring);
    }

    cJSON* portal_ap_ssid = cJSON_GetObjectItem(root, "portal_ap_ssid");
    if (portal_ap_ssid) {
        settings_set_portal_ap_ssid(settings, portal_ap_ssid->valuestring);
    }

    cJSON* portal_domain = cJSON_GetObjectItem(root, "portal_domain");
    if (portal_domain) {
        settings_set_portal_domain(settings, portal_domain->valuestring);
    }

    cJSON* portal_offline_mode = cJSON_GetObjectItem(root, "portal_offline_mode");
    if (portal_offline_mode) {
        settings_set_portal_offline_mode(settings, portal_offline_mode->valueint != 0);
    }

    // Power Printer settings
    cJSON* printer_ip = cJSON_GetObjectItem(root, "printer_ip");
    if (printer_ip) {
        settings_set_printer_ip(settings, printer_ip->valuestring);
    }

    cJSON* printer_text = cJSON_GetObjectItem(root, "printer_text");
    if (printer_text) {
        settings_set_printer_text(settings, printer_text->valuestring);
    }

    cJSON* printer_font_size = cJSON_GetObjectItem(root, "printer_font_size");
    if (printer_font_size) {
        settings_set_printer_font_size(settings, printer_font_size->valueint);
    }

    cJSON* printer_alignment = cJSON_GetObjectItem(root, "printer_alignment");
    if (printer_alignment) {
        settings_set_printer_alignment(settings, (PrinterAlignment)printer_alignment->valueint);
    }

    cJSON* printer_connected = cJSON_GetObjectItem(root, "printer_connected");
    if (printer_connected) {
        settings_set_printer_connected(settings, printer_connected->valueint != 0);
    }

    // Pin Configuration settings
    cJSON* board_type = cJSON_GetObjectItem(root, "board_type");
    if (board_type) {
        settings_set_board_type(settings, (SupportedBoard)board_type->valueint);
    }

    cJSON* custom_pin_config = cJSON_GetObjectItem(root, "custom_pin_config");
    if (custom_pin_config && settings->board_type == CUSTOM) {
        PinConfig pin_config;

        cJSON* neopixel_pin = cJSON_GetObjectItem(custom_pin_config, "neopixel_pin");
        if (neopixel_pin) {
            pin_config.neopixel_pin = neopixel_pin->valueint;
        }

        cJSON* sd_card_spi_miso = cJSON_GetObjectItem(custom_pin_config, "sd_card_spi_miso");
        if (sd_card_spi_miso) {
            pin_config.sd_card_spi_miso = sd_card_spi_miso->valueint;
        }

        cJSON* sd_card_spi_mosi = cJSON_GetObjectItem(custom_pin_config, "sd_card_spi_mosi");
        if (sd_card_spi_mosi) {
            pin_config.sd_card_spi_mosi = sd_card_spi_mosi->valueint;
        }

        cJSON* sd_card_spi_clk = cJSON_GetObjectItem(custom_pin_config, "sd_card_spi_clk");
        if (sd_card_spi_clk) {
            pin_config.sd_card_spi_clk = sd_card_spi_clk->valueint;
        }

        cJSON* sd_card_spi_cs = cJSON_GetObjectItem(custom_pin_config, "sd_card_spi_cs");
        if (sd_card_spi_cs) {
            pin_config.sd_card_spi_cs = sd_card_spi_cs->valueint;
        }

        cJSON* sd_card_mmc_cmd = cJSON_GetObjectItem(custom_pin_config, "sd_card_mmc_cmd");
        if (sd_card_mmc_cmd) {
            pin_config.sd_card_mmc_cmd = sd_card_mmc_cmd->valueint;
        }

        cJSON* sd_card_mmc_clk = cJSON_GetObjectItem(custom_pin_config, "sd_card_mmc_clk");
        if (sd_card_mmc_clk) {
            pin_config.sd_card_mmc_clk = sd_card_mmc_clk->valueint;
        }

        cJSON* sd_card_mmc_d0 = cJSON_GetObjectItem(custom_pin_config, "sd_card_mmc_d0");
        if (sd_card_mmc_d0) {
            pin_config.sd_card_mmc_d0 = sd_card_mmc_d0->valueint;
        }

        cJSON* gps_tx_pin = cJSON_GetObjectItem(custom_pin_config, "gps_tx_pin");
        if (gps_tx_pin) {
            pin_config.gps_tx_pin = gps_tx_pin->valueint;
        }

        cJSON* gps_rx_pin = cJSON_GetObjectItem(custom_pin_config, "gps_rx_pin");
        if (gps_rx_pin) {
            pin_config.gps_rx_pin = gps_rx_pin->valueint;
        }

        settings_set_custom_pin_config(settings, pin_config);
    }

    settings_save(settings);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"settings_updated\"}");

    cJSON_Delete(root);

    return ESP_OK;
}

// Event handler for Wi-Fi events
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP started");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP stopped");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Station connected to AP");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "Station disconnected from AP");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected from Wi-Fi");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                break;
            case IP_EVENT_AP_STAIPASSIGNED:
                ESP_LOGI(TAG, "Assigned IP to STA");
                break;
            default:
                break;
        }
    }
}