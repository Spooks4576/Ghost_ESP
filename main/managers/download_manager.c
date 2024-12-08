#include "managers/download_manager.h"
#include "managers/ap_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "esp_httpd.h"

static const char *TAG = "DOWNLOAD_MANAGER";
static int download_progress = 0;
static bool cancel_download = false;

#define MAX_DOWNLOAD_BUFFER 1024
#define MAX_REDIRECT_COUNT 10
#define MAX_RETRIES 3
#define RETRY_DELAY_MS 1000

typedef struct {
    char *buffer;
    size_t size;
    size_t capacity;
} dynamic_buffer_t;

static int last_http_status = 0;
static download_status_t current_status = DOWNLOAD_STATUS_OK;

static uint32_t g_timeout_ms = 30000; // Default 30 second timeout
static download_progress_cb_t g_progress_cb = NULL;
static void* g_user_data = NULL;

// Default values
#define DEFAULT_TIMEOUT_MS 30000
#define DEFAULT_BUFFER_SIZE 1024
#define MAX_HEADER_SIZE 1024

static download_wifi_status_t wifi_status = DOWNLOAD_WIFI_STATUS_DISCONNECTED;

static esp_err_t check_wifi_connection(void) {
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (ret == ESP_OK) {
        wifi_status = DOWNLOAD_WIFI_STATUS_CONNECTED;
        return ESP_OK;
    }
    
    wifi_status = DOWNLOAD_WIFI_STATUS_DISCONNECTED;
    ESP_LOGE(TAG, "WiFi not connected: %s", esp_err_to_name(ret));
    return ESP_ERR_WIFI_NOT_CONNECT;
}

download_wifi_status_t download_manager_check_wifi(void) {
    if (check_wifi_connection() == ESP_OK) {
        return DOWNLOAD_WIFI_STATUS_CONNECTED;
    }
    return wifi_status;
}

esp_err_t download_manager_await_wifi(uint32_t timeout_ms) {
    uint32_t start_time = esp_timer_get_time() / 1000;
    
    while (1) {
        if (check_wifi_connection() == ESP_OK) {
            return ESP_OK;
        }
        
        // Check timeout
        if (timeout_ms > 0) {
            uint32_t current_time = esp_timer_get_time() / 1000;
            if ((current_time - start_time) >= timeout_ms) {
                return ESP_ERR_TIMEOUT;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms before next check
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    http_response_t* response = (http_response_t*)evt->user_data;
    
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!response->body) {
                response->body = malloc(evt->data_len + 1);
                if (!response->body) {
                    ESP_LOGE(TAG, "Failed to allocate memory for response");
                    return ESP_FAIL;
                }
                memcpy(response->body, evt->data, evt->data_len);
                response->body[evt->data_len] = '\0';
            } else {
                size_t current_len = strlen(response->body);
                char* new_body = realloc(response->body, current_len + evt->data_len + 1);
                if (!new_body) {
                    ESP_LOGE(TAG, "Failed to reallocate memory for response");
                    return ESP_FAIL;
                }
                response->body = new_body;
                memcpy(response->body + current_len, evt->data, evt->data_len);
                response->body[current_len + evt->data_len] = '\0';
            }
            break;

        case HTTP_EVENT_ON_HEADER:
            if (evt->header_key && evt->header_value) {
                if (strcmp(evt->header_key, "Content-Type") == 0) {
                    response->content_type = strdup(evt->header_value);
                }
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

static http_response_t* create_response(void) {
    http_response_t* response = calloc(1, sizeof(http_response_t));
    if (!response) {
        ESP_LOGE(TAG, "Failed to allocate response structure");
        return NULL;
    }
    return response;
}

void download_manager_free_response(http_response_t* response) {
    if (response) {
        if (response->body) free(response->body);
        if (response->content_type) free(response->content_type);
        free(response);
    }
}

http_response_t* download_manager_http_request(const http_request_config_t* config) {
    if (!config || !config->url) {
        ESP_LOGE(TAG, "Invalid request configuration");
        return NULL;
    }

    // Check WiFi connection first
    if (check_wifi_connection() != ESP_OK) {
        ESP_LOGE(TAG, "No WiFi connection available");
        current_status = DOWNLOAD_STATUS_NETWORK_ERROR;
        return NULL;
    }

    http_response_t* response = create_response();
    if (!response) return NULL;

    esp_http_client_config_t esp_config = {
        .url = config->url,
        .timeout_ms = config->timeout_ms ? config->timeout_ms : DEFAULT_TIMEOUT_MS,
        .buffer_size = DEFAULT_BUFFER_SIZE,
        .event_handler = http_event_handler,
        .user_data = response,
        .crt_bundle_attach = config->verify_ssl ? esp_crt_bundle_attach : NULL
    };

    esp_http_client_handle_t client = esp_http_client_init(&esp_config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        download_manager_free_response(response);
        return NULL;
    }

    // Set method
    esp_http_client_set_method(client, config->method);

    // Set custom headers if provided
    if (config->headers) {
        char* headers_copy = strdup(config->headers);
        char* header = strtok(headers_copy, "\n");
        while (header) {
            char* separator = strchr(header, ':');
            if (separator) {
                *separator = '\0';
                char* value = separator + 1;
                while (*value == ' ') value++; // Skip leading spaces
                esp_http_client_set_header(client, header, value);
            }
            header = strtok(NULL, "\n");
        }
        free(headers_copy);
    }

    // Set payload for POST/PUT requests
    if (config->payload && (config->method == HTTP_METHOD_POST || config->method == HTTP_METHOD_PUT)) {
        esp_http_client_set_post_field(client, config->payload, strlen(config->payload));
    }

    // Perform request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        download_manager_free_response(response);
        esp_http_client_cleanup(client);
        return NULL;
    }

    // Get status code
    response->status_code = esp_http_client_get_status_code(client);
    response->content_length = esp_http_client_get_content_length(client);

    esp_http_client_cleanup(client);
    return response;
}

http_response_t* download_manager_http_get(const char* url) {
    http_request_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .headers = NULL,
        .payload = NULL,
        .timeout_ms = DEFAULT_TIMEOUT_MS,
        .verify_ssl = true
    };
    return download_manager_http_request(&config);
}

http_response_t* download_manager_http_post(const char* url, const char* payload) {
    http_request_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .headers = "Content-Type: application/json\n",
        .payload = payload,
        .timeout_ms = DEFAULT_TIMEOUT_MS,
        .verify_ssl = true
    };
    return download_manager_http_request(&config);
}

static void cleanup_resources(esp_http_client_handle_t client, FILE* file, void* buffer) {
    if (buffer) {
        free(buffer);
    }
    if (file) {
        fclose(file);
    }
    if (client) {
        esp_http_client_cleanup(client);
    }
}

esp_err_t download_manager_init(void) {
    download_progress = 0;
    cancel_download = false;
    return ESP_OK;
}

esp_err_t download_manager_set_timeout(uint32_t timeout_ms) {
    if (timeout_ms == 0) {
        g_timeout_ms = 30000; // Reset to default
    } else {
        g_timeout_ms = timeout_ms;
    }
    return ESP_OK;
}

// Helper function to invoke progress callback
static void update_progress(size_t received, size_t total) {
    int progress = (total > 0) ? (received * 100) / total : 0;
    download_progress = progress; // Update global progress

    if (g_progress_cb) {
        g_progress_cb(progress, received, total, g_user_data);
    }
}

esp_err_t download_manager_get_file_with_cb(
    const char* url,
    const char* save_path,
    download_progress_cb_t progress_cb,
    void* user_data
) {
    // Store callback info
    g_progress_cb = progress_cb;
    g_user_data = user_data;

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = g_timeout_ms,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = MAX_DOWNLOAD_BUFFER,
        .buffer_size_tx = MAX_DOWNLOAD_BUFFER
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    // Open file for writing
    FILE* file = fopen(save_path, "wb");
    if (!file) {
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        cleanup_resources(client, file, NULL);
        return err;
    }

    // Get content length
    int64_t content_length = esp_http_client_get_content_length(client);
    size_t total_read = 0;

    // Allocate read buffer
    char *buffer = malloc(MAX_DOWNLOAD_BUFFER);
    if (!buffer) {
        cleanup_resources(client, file, NULL);
        return ESP_ERR_NO_MEM;
    }

    // Download loop
    while (!cancel_download) {
        int read_len = esp_http_client_read(client, buffer, MAX_DOWNLOAD_BUFFER);
        if (read_len <= 0) {
            break;
        }

        if (fwrite(buffer, 1, read_len, file) != read_len) {
            free(buffer);
            cleanup_resources(client, file, NULL);
            return ESP_FAIL;
        }

        total_read += read_len;
        update_progress(total_read, content_length);
    }

    // Cleanup
    free(buffer);
    fclose(file);
    
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    // Reset callback info
    g_progress_cb = NULL;
    g_user_data = NULL;

    return (status_code == 200 && !cancel_download) ? ESP_OK : ESP_FAIL;
}

esp_err_t download_manager_resume_file(
    const char* url,
    const char* save_path,
    size_t offset,
    download_progress_cb_t progress_cb,
    void* user_data
) {
    // Verify file exists and get size
    struct stat st;
    if (stat(save_path, &st) != 0 || st.st_size < offset) {
        return ESP_ERR_INVALID_ARG;
    }

    // Store callback info
    g_progress_cb = progress_cb;
    g_user_data = user_data;

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = g_timeout_ms,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = MAX_DOWNLOAD_BUFFER,
        .buffer_size_tx = MAX_DOWNLOAD_BUFFER
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    // Set Range header
    char range_header[64];
    snprintf(range_header, sizeof(range_header), "bytes=%zu-", offset);
    esp_http_client_set_header(client, "Range", range_header);

    // Open file for appending
    FILE* file = fopen(save_path, "ab");
    if (!file) {
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        cleanup_resources(client, file, NULL);
        return err;
    }

    // Verify server accepts range request
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 206) { // 206 Partial Content
        cleanup_resources(client, file, NULL);
        return ESP_FAIL;
    }

    int64_t content_length = esp_http_client_get_content_length(client);
    size_t total_read = offset;

    char *buffer = malloc(MAX_DOWNLOAD_BUFFER);
    if (!buffer) {
        cleanup_resources(client, file, NULL);
        return ESP_ERR_NO_MEM;
    }

    // Download remaining data
    while (!cancel_download) {
        int read_len = esp_http_client_read(client, buffer, MAX_DOWNLOAD_BUFFER);
        if (read_len <= 0) {
            break;
        }

        if (fwrite(buffer, 1, read_len, file) != read_len) {
            free(buffer);
            cleanup_resources(client, file, NULL);
            return ESP_FAIL;
        }

        total_read += read_len;
        update_progress(total_read, offset + content_length);
    }

    // Cleanup
    free(buffer);
    fclose(file);
    esp_http_client_cleanup(client);

    // Reset callback info
    g_progress_cb = NULL;
    g_user_data = NULL;

    return (!cancel_download) ? ESP_OK : ESP_FAIL;
}

char* download_manager_fetch_string(const char* url) {
    if (!url) {
        ESP_LOGE(TAG, "Invalid URL");
        current_status = DOWNLOAD_STATUS_INVALID_URL;
        return NULL;
    }

    // Check WiFi connection first
    if (check_wifi_connection() != ESP_OK) {
        ESP_LOGE(TAG, "No WiFi connection available");
        current_status = DOWNLOAD_STATUS_NETWORK_ERROR;
        return NULL;
    }

    dynamic_buffer_t buffer = {
        .buffer = malloc(MAX_DOWNLOAD_BUFFER),
        .size = 0,
        .capacity = MAX_DOWNLOAD_BUFFER
    };

    if (!buffer.buffer) {
        return NULL;
    }

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
        .user_data = &buffer
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(buffer.buffer);
        return NULL;
    }

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        free(buffer.buffer);
        return NULL;
    }

    // Ensure null termination
    if (buffer.size + 1 > buffer.capacity) {
        char *new_buffer = realloc(buffer.buffer, buffer.size + 1);
        if (!new_buffer) {
            free(buffer.buffer);
            return NULL;
        }
        buffer.buffer = new_buffer;
        buffer.capacity = buffer.size + 1;
    }
    buffer.buffer[buffer.size] = '\0';

    return buffer.buffer;
} 