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
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LOG_BUFFER_SIZE 4096 // Adjust as needed
#define MAX_FILE_SIZE (5 * 1024 * 1024) // 5 MB
#define BUFFER_SIZE (1024) // 1 KB buffer size for reading chunks
#define MIN_(a,b) ((a) < (b) ? (a) : (b))
static char log_buffer[MAX_LOG_BUFFER_SIZE];
static size_t log_buffer_index = 0;

static const char* TAG = "AP_MANAGER";
static httpd_handle_t server = NULL;
static esp_netif_t* netif = NULL;
static bool mdns_freed = false;

// Forward declarations
static esp_err_t http_get_handler(httpd_req_t* req);
static esp_err_t api_clear_logs_handler(httpd_req_t* req);
static esp_err_t api_settings_handler(httpd_req_t* req);
static esp_err_t api_command_handler(httpd_req_t *req);
static esp_err_t api_settings_get_handler(httpd_req_t* req);

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);



static esp_err_t scan_directory(const char *base_path, cJSON *json_array) {
    DIR *dir = opendir(base_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", base_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Dynamically allocate memory for full_path
        size_t full_path_len = strlen(base_path) + strlen(entry->d_name) + 2; // +2 for '/' and '\0'
        char *full_path = malloc(full_path_len);
        if (!full_path) {
            ESP_LOGE(TAG, "Failed to allocate memory for full path.");
            closedir(dir);
            return ESP_ERR_NO_MEM;
        }

        snprintf(full_path, full_path_len, "%s/%s", base_path, entry->d_name);

        struct stat entry_stat;
        if (stat(full_path, &entry_stat) != 0) {
            ESP_LOGE(TAG, "Failed to stat file: %s", full_path);
            free(full_path);
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            // Add folder
            cJSON *folder = cJSON_CreateObject();
            cJSON_AddStringToObject(folder, "name", entry->d_name);
            cJSON_AddStringToObject(folder, "type", "folder");

            // Recursively scan children
            cJSON *children = cJSON_CreateArray();
            if (scan_directory(full_path, children) == ESP_OK) {
                cJSON_AddItemToObject(folder, "children", children);
            } else {
                cJSON_Delete(children);
            }

            cJSON_AddItemToArray(json_array, folder);
        } else if (S_ISREG(entry_stat.st_mode)) {
            // Add file
            cJSON *file = cJSON_CreateObject();
            cJSON_AddStringToObject(file, "name", entry->d_name);
            cJSON_AddStringToObject(file, "type", "file");
            cJSON_AddStringToObject(file, "path", full_path);
            cJSON_AddItemToArray(json_array, file);
        }

        // Free dynamically allocated memory
        free(full_path);
    }

    closedir(dir);
    return ESP_OK;
}


static esp_err_t api_sd_card_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received request for SD card structure.");


    const char *base_path = "/mnt";


    struct stat st;
    if (stat(base_path, &st) != 0) {
        ESP_LOGE(TAG, "SD card not mounted or inaccessible.");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"SD card not supported or not mounted.\"}");
        return ESP_FAIL;
    }

    
    cJSON *response_json = cJSON_CreateArray();
    if (scan_directory(base_path, response_json) != ESP_OK) {
        cJSON_Delete(response_json);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to scan SD card.\"}");
        return ESP_FAIL;
    }

    
    char *response_string = cJSON_Print(response_json);
    if (!response_string) {
        ESP_LOGE(TAG, "Failed to serialize JSON.");
        cJSON_Delete(response_json);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to serialize SD card data.\"}");
        return ESP_FAIL;
    }

    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response_string);

    
    cJSON_Delete(response_json);
    free(response_string);

    return ESP_OK;
}

static esp_err_t api_sd_card_post_handler(httpd_req_t *req) {
    char buf[512];
    int received = httpd_req_recv(req, buf, sizeof(buf));
    if (received <= 0) {
        ESP_LOGE(TAG, "Failed to receive request payload.");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\": \"Invalid request payload.\"}");
        return ESP_FAIL;
    }

    // Parse JSON payload
    buf[received] = '\0';  // Null-terminate the received string
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON payload.");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\": \"Invalid JSON payload.\"}");
        return ESP_FAIL;
    }

    cJSON *path_item = cJSON_GetObjectItem(json, "path");
    if (!cJSON_IsString(path_item) || !path_item->valuestring) {
        ESP_LOGE(TAG, "Missing or invalid 'path' in request payload.");
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\": \"'path' is required and must be a string.\"}");
        return ESP_FAIL;
    }

    const char *file_path = path_item->valuestring;

    // Open the file
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_sendstr(req, "{\"error\": \"File not found.\"}");
        return ESP_FAIL;
    }

    // Get file size
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        ESP_LOGE(TAG, "Failed to get file stats: %s", file_path);
        fclose(file);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to get file size.\"}");
        return ESP_FAIL;
    }

    size_t file_size = file_stat.st_size;

    // Allocate memory for the buffer
    char *file_buf = malloc(file_size);
    if (!file_buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for file buffer.");
        fclose(file);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to allocate memory for file.\"}");
        return ESP_FAIL;
    }

    // Read the file into the buffer
    size_t bytes_read = fread(file_buf, 1, file_size, file);
    fclose(file);

    if (bytes_read != file_size) {
        ESP_LOGE(TAG, "Failed to read entire file: %s", file_path);
        free(file_buf);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to read file.\"}");
        return ESP_FAIL;
    }

    // Set response headers
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment");

    // Send the file content
    if (httpd_resp_send(req, file_buf, file_size) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send file.");
        free(file_buf);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File sent successfully: %s", file_path);

    // Free allocated memory
    free(file_buf);
    cJSON_Delete(json);

    return ESP_OK;
}

#define MAX_PATH_LENGTH 512

esp_err_t get_query_param(httpd_req_t *req, const char *key, char *value, size_t max_len) {
    size_t query_len = httpd_req_get_url_query_len(req) + 1;

    if (query_len > 1) { // >1 because query string starts with '?'
        char *query = malloc(query_len);
        if (!query) {
            ESP_LOGE(TAG, "Failed to allocate memory for query string.");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_url_query_str(req, query, query_len) == ESP_OK) {
            char encoded_value[max_len];
            if (httpd_query_key_value(query, key, encoded_value, sizeof(encoded_value)) == ESP_OK) {
                url_decode(value, encoded_value);
                free(query);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Key '%s' not found in query string.", key);
            }
        } else {
            ESP_LOGE(TAG, "Failed to get query string.");
        }

        free(query);
    } else {
        ESP_LOGE(TAG, "No query string found in the URL.");
    }

    return ESP_ERR_NOT_FOUND;
}


esp_err_t api_sd_card_delete_file_handler(httpd_req_t *req) {
    char filepath[256 + 1];

    size_t query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len > 1) {
        char query[query_len];
        httpd_req_get_url_query_str(req, query, query_len);

        
        char path[256];
        if (httpd_query_key_value(query, "path", path, sizeof(path)) == ESP_OK) {
            snprintf(filepath, sizeof(filepath), "%s", path);
            ESP_LOGI(TAG, "Deleting file: %s", filepath);

            
            struct _reent r;
            memset(&r, 0, sizeof(struct _reent));
            int res = _unlink_r(&r, filepath);
            if (res == 0) {
                ESP_LOGI(TAG, "File deleted successfully");
                httpd_resp_set_status(req, "200 OK");
                httpd_resp_send(req, "File deleted successfully", HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to delete file: %s, errno: %d", filepath, errno);
                httpd_resp_set_status(req, "500 Internal Server Error");
                httpd_resp_send(req, "Failed to delete the file", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
        }
    }

    ESP_LOGE(TAG, "Invalid query parameters");
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_send(req, "Missing or invalid 'path' parameter", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
}

// Handler for uploading files to SD card
static esp_err_t api_sd_card_upload_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received file upload request.");

    // 1. Retrieve 'path' query parameter
    char path_param[MAX_PATH_LENGTH] = {0};
    if (get_query_param(req, "path", path_param, sizeof(path_param)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get 'path' from query parameters.");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Missing or invalid 'path' query parameter.\"}");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Upload path: %s", path_param);

    // 2. Retrieve Content-Type header and boundary
    char content_type[128] = {0};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Content-Type header.");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Missing Content-Type header.\"}");
        return ESP_FAIL;
    }

    const char *boundary_prefix = "boundary=";
    char *boundary_start = strstr(content_type, boundary_prefix);
    if (!boundary_start) {
        ESP_LOGE(TAG, "Failed to parse boundary.");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Boundary missing.\"}");
        return ESP_FAIL;
    }
    boundary_start += strlen(boundary_prefix);

    // Allocate memory for the boundary
    size_t boundary_len = strlen(boundary_start) + 3; // +3 for "--" and null terminator
    char *boundary = malloc(boundary_len);
    if (!boundary) {
        ESP_LOGE(TAG, "Failed to allocate memory for boundary.");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Memory allocation failed.\"}");
        return ESP_FAIL;
    }
    snprintf(boundary, boundary_len, "--%s", boundary_start);
    ESP_LOGD(TAG, "Parsed boundary: %s", boundary);

    // Allocate memory for the buffer
    char *buf = malloc(BUFFER_SIZE + 1); // +1 for null-terminator
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for request buffer.");
        free(boundary);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Memory allocation failed.\"}");
        return ESP_FAIL;
    }

    FILE *file = NULL;
    char *file_path = malloc(MAX_PATH_LENGTH + 128); // Allocate heap memory for file_path
    if (!file_path) {
        ESP_LOGE(TAG, "Failed to allocate memory for file path.");
        free(buf);
        free(boundary);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Memory allocation failed.\"}");
        return ESP_FAIL;
    }

    size_t total_received = 0;
    int received;

    // 4. Process the multipart form-data
    while ((received = httpd_req_recv(req, buf, BUFFER_SIZE)) > 0) {
        buf[received] = '\0'; // Null-terminate for string operations

        char *boundary_ptr = strstr(buf, boundary);
        if (boundary_ptr) {
            char *headers_end = strstr(boundary_ptr, "\r\n\r\n");
            if (!headers_end) {
                ESP_LOGE(TAG, "Malformed part headers.");
                free(buf);
                free(boundary);
                free(file_path);
                if (file) fclose(file);
                httpd_resp_set_status(req, "400 Bad Request");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_sendstr(req, "{\"error\": \"Malformed part headers.\"}");
                return ESP_FAIL;
            }
            headers_end += 4;

            if (strstr(boundary_ptr, "Content-Disposition: form-data; name=\"file\"")) {
                char *filename_start = strstr(boundary_ptr, "filename=\"");
                char original_filename[128] = {0};
                if (filename_start) {
                    filename_start += strlen("filename=\"");
                    char *filename_end = strstr(filename_start, "\"");
                    if (filename_end && (filename_end - filename_start) < sizeof(original_filename)) {
                        strncpy(original_filename, filename_start, filename_end - filename_start);
                        original_filename[filename_end - filename_start] = '\0';
                        ESP_LOGI(TAG, "Original filename: %s", original_filename);
                    }
                }

                if (strlen(original_filename) > 0) {
                    snprintf(file_path, MAX_PATH_LENGTH + 128, "%s/%s", path_param, original_filename);
                } else {
                    snprintf(file_path, MAX_PATH_LENGTH + 128, "%s/received_file", path_param);
                }

                file = fopen(file_path, "wb");
                if (!file) {
                    ESP_LOGE(TAG, "Failed to open file for writing: %s", file_path);
                    free(buf);
                    free(boundary);
                    free(file_path);
                    httpd_resp_set_status(req, "500 Internal Server Error");
                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_sendstr(req, "{\"error\": \"Failed to open file.\"}");
                    return ESP_FAIL;
                }
                ESP_LOGI(TAG, "Opened file for writing: %s", file_path);

                size_t data_len = received - (headers_end - buf);
                if (data_len > 0 && fwrite(headers_end, 1, data_len, file) != data_len) {
                    ESP_LOGE(TAG, "Failed to write file data.");
                    fclose(file);
                    free(buf);
                    free(boundary);
                    free(file_path);
                    httpd_resp_set_status(req, "500 Internal Server Error");
                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_sendstr(req, "{\"error\": \"Failed to write file data.\"}");
                    return ESP_FAIL;
                }
                total_received += data_len;
            }
        } else if (file) {
            if (fwrite(buf, 1, received, file) != received) {
                ESP_LOGE(TAG, "Failed to write file data.");
                fclose(file);
                free(buf);
                free(boundary);
                free(file_path);
                httpd_resp_set_status(req, "500 Internal Server Error");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_sendstr(req, "{\"error\": \"Failed to write file data.\"}");
                return ESP_FAIL;
            }
            total_received += received;
        }
    }

    if (received < 0) {
        ESP_LOGE(TAG, "Error receiving file data.");
        free(buf);
        free(boundary);
        free(file_path);
        if (file) fclose(file);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Failed to receive file data.\"}");
        return ESP_FAIL;
    }

    free(buf);
    free(boundary);
    free(file_path);
    if (file) fclose(file);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"message\": \"File uploaded successfully.\"}");
    ESP_LOGI(TAG, "File uploaded successfully: %zu bytes received.", total_received);

    return ESP_OK;
}

esp_err_t ap_manager_init(void) {
    esp_err_t ret;
    wifi_mode_t mode;


    ret = esp_wifi_get_mode(&mode);
    if (ret == ESP_ERR_WIFI_NOT_INIT) {
        printf("Wi-Fi not initialized, initializing as Access Point...\n");

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            printf("esp_wifi_init failed: %s\n", esp_err_to_name(ret));
            return ret;
        }


        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (!netif) {
            netif = esp_netif_create_default_wifi_ap();
            if (netif == NULL) {
                printf("Failed to create default Wi-Fi AP\n");
                return ESP_FAIL;
            }
        }
    } else if (ret == ESP_OK) {
        printf("Wi-Fi already initialized, skipping Wi-Fi init.\n");
    } else {
        printf("esp_wifi_get_mode failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        printf("esp_wifi_set_mode failed: %s\n", esp_err_to_name(ret));
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
        printf("esp_wifi_set_config failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif == NULL) {
        printf("Failed to get the AP network interface\n");
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
        printf("DHCP server configured successfully.\n");
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        printf("esp_wifi_start failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    printf("Wi-Fi Access Point started with SSID: %s\n", ssid);

    // Register event handlers for Wi-Fi events if not registered already
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Initialize mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        return ret;
    }

    mdns_freed = false;

    ret = mdns_hostname_set("ghostesp");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_instance_name_set("GhostESP Web Interface");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_instance_name_set failed: %s\n", esp_err_to_name(ret));
    }

    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), "192.168.4.1");
    
    mdns_txt_item_t serviceTxtData[] = {
        {"ip", ip_str}
    };
    
    ret = mdns_service_add("GhostESP", "_http", "_tcp", 80, serviceTxtData, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_service_add failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_service_txt_set("_http", "_tcp", serviceTxtData, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_service_txt_set failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    char ip_txt[20];
    snprintf(ip_txt, sizeof(ip_txt), "192.168.4.1");
    mdns_txt_item_t ip_data[] = {
        {"ipv4", ip_txt}
    };
    ret = mdns_service_txt_set("_http", "_tcp", ip_data, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mdns_service_txt_set failed: %s\n", esp_err_to_name(ret));
    }
    
    FSettings* settings = &G_Settings;

    ret = mdns_hostname_set("ghostesp");
    if (ret != ESP_OK) {
        printf("mdns_hostname_set failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    
    printf("mDNS hostname set to ghostesp.local\n");

    ret = mdns_service_add(NULL, "_http", "_http", 80, NULL, 0);
    if (ret != ESP_OK) {
        printf("mDNS service add failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // Start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768; // Control port (use default)
    config.max_uri_handlers = 30;


    ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        printf("Error starting HTTP server!\n");
        return ret;
    }

     // Register URI handlers
    httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_get_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_POST,
        .handler   = api_settings_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_get_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_GET,
        .handler   = api_settings_get_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_sd_card_get = {
        .uri       = "/api/sdcard",
        .method    = HTTP_GET,
        .handler   = api_sd_card_get_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_sd_card_post = {
        .uri       = "/api/sdcard/download",
        .method    = HTTP_POST,
        .handler   = api_sd_card_post_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_sd_card_post_upload = {
        .uri       = "/api/sdcard/upload",
        .method    = HTTP_POST,
        .handler   = api_sd_card_upload_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_command = {
        .uri       = "/api/command",
        .method    = HTTP_POST,
        .handler   = api_command_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_delete_command = {
        .uri       = "/api/sdcard",
        .method    = HTTP_DELETE,
        .handler   = api_sd_card_delete_file_handler,
        .user_ctx  = NULL
    };

    ret = httpd_register_uri_handler(server, &uri_delete_command);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_post_upload);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_post);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_get);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_get_settings);
    if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }


    ret = httpd_register_uri_handler(server, &uri_post_settings);

        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }
    ret = httpd_register_uri_handler(server, &uri_get);

        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_post_command);

        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    printf("HTTP server started\n");

    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        printf("ESP32 AP IP Address: \n" IPSTR, IP2STR(&ip_info.ip));
    } else {
        printf("Failed to get IP address\n");
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
    printf("AP Manager deinitialized\n");
}


void ap_manager_add_log(const char* log_message) {
    size_t message_length = strlen(log_message);
    if (log_buffer_index + message_length < MAX_LOG_BUFFER_SIZE) {
        strcpy(&log_buffer[log_buffer_index], log_message);
        log_buffer_index += message_length;
    } else {
        printf("Log buffer full, clearing buffer and adding new log\n");

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
        printf("esp_wifi_set_mode failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // Start Wi-Fi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        printf("esp_wifi_start failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // Start mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        printf("mdns_init failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_hostname_set("ghostesp");
    if (ret != ESP_OK) {
        printf("mdns_hostname_set failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // Start HTTPD server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 30;

    ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        printf("Error starting HTTP server!\n");
        return ret;
    }

     httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_get_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_POST,
        .handler   = api_settings_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_get_settings = {
        .uri       = "/api/settings",
        .method    = HTTP_GET,
        .handler   = api_settings_get_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_sd_card_get = {
        .uri       = "/api/sdcard",
        .method    = HTTP_GET,
        .handler   = api_sd_card_get_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_sd_card_post = {
        .uri       = "/api/sdcard/download",
        .method    = HTTP_POST,
        .handler   = api_sd_card_post_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_sd_card_post_upload = {
        .uri       = "/api/sdcard/upload",
        .method    = HTTP_POST,
        .handler   = api_sd_card_upload_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t uri_delete_command = {
        .uri       = "/api/sdcard",
        .method    = HTTP_DELETE,
        .handler   = api_sd_card_delete_file_handler,
        .user_ctx  = NULL
    };


    httpd_uri_t uri_post_command = {
        .uri       = "/api/command",
        .method    = HTTP_POST,
        .handler   = api_command_handler,
        .user_ctx  = NULL
    };

    ret = httpd_register_uri_handler(server, &uri_delete_command);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_post_upload);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_post);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_sd_card_get);
        if (ret != ESP_OK) {
        printf("Error registering URI\n");
    }

    ret = httpd_register_uri_handler(server, &uri_get_settings);
        if (ret != ESP_OK) {
        printf("Error registering URI \n");
    }

    ret = httpd_register_uri_handler(server, &uri_post_settings);

        if (ret != ESP_OK) {
        printf("Error registering URI \n");
    }
    ret = httpd_register_uri_handler(server, &uri_get);

        if (ret != ESP_OK) {
        printf("Error registering URI \n");
    }

    ret = httpd_register_uri_handler(server, &uri_post_command);

        if (ret != ESP_OK) {
         printf("Error registering URI \n");
    }

    printf("HTTP server started\n");

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        printf("ESP32 AP IP Address: \n" IPSTR, IP2STR(&ip_info.ip));
    } else {
        printf("Failed to get IP address\n");
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
            printf("Stopping Wi-Fi...\n");
            ESP_ERROR_CHECK(esp_wifi_stop());
        }
    } else {
        printf("Failed to get Wi-Fi mode, error: %d\n", err);
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
    printf("Received HTTP GET request: %s\n", req->uri);
    httpd_resp_set_type(req, "text/html");
     return httpd_resp_send(req, (const char*)ghost_site_html, ghost_site_html_size);
}

static esp_err_t api_command_handler(httpd_req_t *req)
{
    char content[500];
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


    simulateCommand(command);
   
    httpd_resp_send(req, "Command executed", strlen("Command executed"));

    cJSON_Delete(json);
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
        printf("Failed to allocate memory for JSON payload\n");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            free(buf);
            printf("Failed to receive JSON payload\n");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0'; // Null-terminate the received data

    // Parse JSON
    cJSON* root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        printf("Failed to parse JSON\n");
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

    cJSON* rgb_mode = cJSON_GetObjectItem(root, "rainbow_mode");
    if (cJSON_IsBool(rgb_mode)) {
        bool rgb_mode_value = cJSON_IsTrue(rgb_mode);
        printf("Debug: Passed rgb_mode_value = %d to settings_set_rgb_mode()\n", rgb_mode_value);
        settings_set_rgb_mode(settings, (RGBMode)rgb_mode_value);
    } else {
        printf("Error: 'rgb_mode' is not a boolean.\n");
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
        printf("PRINTER FONT SIZE %i", printer_font_size->valueint);
        settings_set_printer_font_size(settings, printer_font_size->valueint);
    }

    cJSON* printer_alignment = cJSON_GetObjectItem(root, "printer_alignment");
    if (printer_alignment) {
        printf("printer_alignment %i", printer_alignment->valueint);
        settings_set_printer_alignment(settings, (PrinterAlignment)printer_alignment->valueint);
    }

    cJSON* flappy_ghost_name = cJSON_GetObjectItem(root, "flappy_ghost_name");
    if (flappy_ghost_name) {
        settings_set_flappy_ghost_name(settings, flappy_ghost_name->valuestring);
    }

    cJSON* time_zone_str_name = cJSON_GetObjectItem(root, "timezone_str");
    if (time_zone_str_name) {
        settings_set_timezone_str(settings, time_zone_str_name->valuestring);
    }

    cJSON* hex_accent_color_str = cJSON_GetObjectItem(root, "hex_accent_color");
    if (hex_accent_color_str) {
        settings_set_accent_color_str(settings, hex_accent_color_str->valuestring);
    }

    cJSON* rts_enabled_bool = cJSON_GetObjectItem(root, "rts_enabled");
    if (rts_enabled_bool) {
        settings_set_rts_enabled(settings, rts_enabled_bool->valueint != 0);
    }

    cJSON* gps_rx_pin = cJSON_GetObjectItem(root, "gps_rx_pin");
    if (gps_rx_pin) {
        settings_set_gps_rx_pin(settings, gps_rx_pin->valueint);
    }

    // Handle display timeout
    cJSON* display_timeout = cJSON_GetObjectItem(root, "display_timeout");
    if (display_timeout) {
        settings_set_display_timeout(settings, display_timeout->valueint);
        ESP_LOGI(TAG, "Setting display timeout to: %d ms", display_timeout->valueint);
    }
    printf("About to Save Settings\n");

    settings_save(settings);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"settings_updated\"}");

    cJSON_Delete(root);

    return ESP_OK;
}


static esp_err_t api_settings_get_handler(httpd_req_t* req) {
    FSettings* settings = &G_Settings;

    cJSON* root = cJSON_CreateObject();
    if (!root) {
        printf("Failed to create JSON object\n");
        return ESP_FAIL;
    }

    
    cJSON_AddNumberToObject(root, "broadcast_speed", settings_get_broadcast_speed(settings));
    cJSON_AddStringToObject(root, "ap_ssid", settings_get_ap_ssid(settings));
    cJSON_AddStringToObject(root, "ap_password", settings_get_ap_password(settings));
    cJSON_AddNumberToObject(root, "rgb_mode", settings_get_rgb_mode(settings));
    cJSON_AddNumberToObject(root, "rgb_speed", settings_get_rgb_speed(settings));
    cJSON_AddNumberToObject(root, "channel_delay", settings_get_channel_delay(settings));

    
    cJSON_AddStringToObject(root, "portal_url", settings_get_portal_url(settings));
    cJSON_AddStringToObject(root, "portal_ssid", settings_get_portal_ssid(settings));
    cJSON_AddStringToObject(root, "portal_password", settings_get_portal_password(settings));
    cJSON_AddStringToObject(root, "portal_ap_ssid", settings_get_portal_ap_ssid(settings));
    cJSON_AddStringToObject(root, "portal_domain", settings_get_portal_domain(settings));
    cJSON_AddBoolToObject(root, "portal_offline_mode", settings_get_portal_offline_mode(settings));

    
    cJSON_AddStringToObject(root, "printer_ip", settings_get_printer_ip(settings));
    cJSON_AddStringToObject(root, "printer_text", settings_get_printer_text(settings));
    cJSON_AddNumberToObject(root, "printer_font_size", settings_get_printer_font_size(settings));
    cJSON_AddNumberToObject(root, "printer_alignment", settings_get_printer_alignment(settings));
    cJSON_AddStringToObject(root, "hex_accent_color", settings_get_accent_color_str(settings));
    cJSON_AddStringToObject(root, "timezone_str", settings_get_timezone_str(settings));
    cJSON_AddNumberToObject(root, "gps_rx_pin", settings_get_gps_rx_pin(settings));
    cJSON_AddNumberToObject(root, "display_timeout", settings_get_display_timeout(settings));
    cJSON_AddNumberToObject(root, "rts_enabled_bool", settings_get_rts_enabled(settings));
    
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        if (ip_info.ip.addr != 0) {
            char ip_str[16];
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
            cJSON_AddStringToObject(root, "station_ip", ip_str);
        }
    }

    
    const char* json_response = cJSON_Print(root);
    if (!json_response) {
        cJSON_Delete(root);
        printf("Failed to print JSON object\n");
        return ESP_FAIL;
    }

    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_response);

    
    cJSON_Delete(root);
    free((void*)json_response);

    return ESP_OK;
}


// Event handler for Wi-Fi events
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                printf("AP started\n");
                break;
            case WIFI_EVENT_AP_STOP:
                printf("AP stopped\n");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                printf("Station connected to AP\n");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                printf("Station disconnected from AP\n");
                break;
            case WIFI_EVENT_STA_START:
                printf("STA started\n");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                printf("Disconnected from Wi-Fi\n");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                break;
            case IP_EVENT_AP_STAIPASSIGNED:
                printf("Assigned IP to STA\n");
                break;
            default:
                break;
        }
    }
}