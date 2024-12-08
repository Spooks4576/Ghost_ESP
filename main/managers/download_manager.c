#include "managers/download_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    dynamic_buffer_t *buffer = (dynamic_buffer_t *)evt->user_data;
    
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (buffer != NULL) {
                if (buffer->size + evt->data_len > buffer->capacity) {
                    size_t new_capacity = buffer->capacity * 2 + evt->data_len;
                    char *new_buffer = realloc(buffer->buffer, new_capacity);
                    if (new_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory");
                        return ESP_FAIL;
                    }
                    buffer->buffer = new_buffer;
                    buffer->capacity = new_capacity;
                }
                memcpy(buffer->buffer + buffer->size, evt->data, evt->data_len);
                buffer->size += evt->data_len;
            }
            break;
        default:
            break;
    }
    return ESP_OK;
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

esp_err_t download_manager_get_file(const char* url, const char* save_path) {
    if (!url || !save_path) {
        current_status = DOWNLOAD_STATUS_INVALID_URL;
        return ESP_ERR_INVALID_ARG;
    }

    current_status = DOWNLOAD_STATUS_IN_PROGRESS;

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 20000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = MAX_DOWNLOAD_BUFFER,
        .buffer_size_tx = MAX_DOWNLOAD_BUFFER
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    FILE* file = fopen(save_path, "wb");
    if (!file) {
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        fclose(file);
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int total_read = 0;
    char *buffer = malloc(MAX_DOWNLOAD_BUFFER);
    if (!buffer) {
        current_status = DOWNLOAD_STATUS_MEMORY_ERROR;
        cleanup_resources(client, file, NULL);
        return ESP_ERR_NO_MEM;
    }

    // Get content type and size
    char *content_type = NULL;
    content_type = esp_http_client_get_header(client, "Content-Type");
    int64_t content_length = esp_http_client_get_content_length(client);

    // Optional: Validate content type if needed
    if (content_type) {
        ESP_LOGI(TAG, "Content-Type: %s", content_type);
    }

    // Validate content length
    if (content_length <= 0) {
        ESP_LOGW(TAG, "Unknown or invalid content length");
    } else if (content_length > MAX_ALLOWED_DOWNLOAD_SIZE) {
        ESP_LOGE(TAG, "Content length too large: %lld", content_length);
        current_status = DOWNLOAD_STATUS_FILE_ERROR;
        fclose(file);
        esp_http_client_cleanup(client);
        return ESP_ERR_INVALID_SIZE;
    }

    while (total_read < content_length && !cancel_download) {
        int read_len = esp_http_client_read(client, buffer, MAX_DOWNLOAD_BUFFER);
        if (read_len <= 0) {
            break;
        }
        fwrite(buffer, 1, read_len, file);
        total_read += read_len;
        download_progress = (total_read * 100) / content_length;
    }

    free(buffer);
    fclose(file);
    esp_http_client_cleanup(client);
    
    int http_status = esp_http_client_get_status_code(client);
    last_http_status = http_status;
    
    if (http_status != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status %d", http_status);
        current_status = DOWNLOAD_STATUS_HTTP_ERROR;
        return ESP_FAIL;
    }
    
    return cancel_download ? ESP_FAIL : ESP_OK;
}

char* download_manager_fetch_string(const char* url) {
    if (!url) {
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