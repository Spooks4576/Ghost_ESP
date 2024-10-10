#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <stdio.h>
#include <string.h>
#include "managers/dial_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"


static const char *TAG = "DIALManager";

typedef struct {
    char *buffer;
    int buffer_len;
    int buffer_size;
} response_buffer_t;


typedef struct {
    char* sid;         // Store SID
    char*  gsessionid;  // Store gsessionid
    char*  listId;      // Store listId
} BindSession_Params_t;

char* getC(const char* input) 
{
    const char *start = strstr(input, "c\",\"");
    
    if (start != NULL) {
        start += 4;
        
        const char *end = strchr(start, '"');
        
        if (end != NULL) {
            size_t len = end - start;
            char *result = (char*)malloc(len + 1);
            
            if (result != NULL) {
                strncpy(result, start, len);
                result[len] = '\0';
            }
            
            return result;
        }
    }
    
    return NULL;
}

char* getS(const char* input) 
{
    const char *start = strstr(input, "S\",\"");
    
    if (start != NULL) {
        start += 4;
        
        const char *end = strchr(start, '"');
        
        if (end != NULL) {
            size_t len = end - start;
            char *result = (char*)malloc(len + 1);
            
            if (result != NULL) {
                strncpy(result, start, len);
                result[len] = '\0';
            }
            
            return result;
        }
    }
    
    return NULL;
}


char* getlistId(const char* input) 
{
    const char *start = strstr(input, "\"playlistModified\",{\"listId\":\"");
    
    if (start != NULL) {
        start += 30;
        
        const char *end = strchr(start, '"');
        
        if (end != NULL) {
            size_t len = end - start;
            char *result = (char*)malloc(len + 1);
            
            if (result != NULL) {
                strncpy(result, start, len);
                result[len] = '\0';
            }
            
            return result;
        }
    }
    
    return NULL;
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    response_buffer_t *resp_buf = evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (resp_buf->buffer_len + evt->data_len >= resp_buf->buffer_size) {
                resp_buf->buffer_size += evt->data_len;
                char *new_buffer = realloc(resp_buf->buffer, resp_buf->buffer_size);
                if (new_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
                    return ESP_FAIL;
                }
                resp_buf->buffer = new_buffer;
            }
            memcpy(resp_buf->buffer + resp_buf->buffer_len, evt->data, evt->data_len);
            resp_buf->buffer_len += evt->data_len;
            break;
        default:
            break;
    }
    return ESP_OK;
}

#define SERVER_PORT 443
#define MAX_APP_URL_LENGTH 500

BindSession_Params_t parse_response(const char *response_body) {
    BindSession_Params_t result;

    result.gsessionid = getS(response_body);
    result.sid = getC(response_body);
    result.listId = getlistId(response_body);
    
    return result;
}


char *url_encode(const char *str) {
    const char *hex = "0123456789ABCDEF";
    char *encoded = malloc(strlen(str) * 3 + 1); // Worst case scenario
    char *pencoded = encoded;
    if (!encoded) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL encoding");
        return NULL;
    }

    while (*str) {
        if (('a' <= *str && *str <= 'z') ||
            ('A' <= *str && *str <= 'Z') ||
            ('0' <= *str && *str <= '9') ||
            (*str == '-' || *str == '_' || *str == '.' || *str == '~')) {
            *pencoded++ = *str;
        } else {
            *pencoded++ = '%';
            *pencoded++ = hex[(*str >> 4) & 0xF];
            *pencoded++ = hex[*str & 0xF];
        }
        str++;
    }
    *pencoded = '\0';
    return encoded;
}

char* generate_uuid() {
    static char uuid[37]; // 36 characters + null terminator
    snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%08x%04x",
             (unsigned int)esp_random(), // 8 hex digits
             (unsigned int)(esp_random() & 0xFFFF), // 4 hex digits
             (unsigned int)(esp_random() & 0xFFFF), // 4 hex digits
             (unsigned int)(esp_random() & 0xFFFF), // 4 hex digits
             (unsigned int)(esp_random()),          // First 8 hex digits of the last part
             (unsigned int)(esp_random() & 0xFFFF)); // Last 4 hex digits of the last part
    return uuid;
}

char *generate_zx() {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *zx = malloc(17);
    if (!zx) {
        ESP_LOGE(TAG, "Failed to allocate memory for zx");
        return NULL;
    }
    for (int i = 0; i < 16; ++i) {
        zx[i] = alphanum[esp_random() % (sizeof(alphanum) - 1)];
    }
    zx[16] = '\0';
    return zx;
}

char* extract_path_from_url(const char *url) {
    // Find the first slash after the host and port (i.e., after "://host:port/")
    const char *path_start = strchr(url + 7, '/');  // +7 to skip over "http://"
    if (path_start) {
        return strdup(path_start);  // Duplicate and return the path
    }
    return strdup("/");  // Default to "/" if no path is found
}

char* extract_application_url(const char *headers) {
    const char *app_url_header = strstr(headers, "Application-Url");
    if (!app_url_header) {
        return NULL;
    }

    const char *url_start = app_url_header + strlen("Application-Url");
    while (*url_start == ' ') url_start++;  // Skip leading spaces

    const char *url_end = strchr(url_start, '\r');
    if (!url_end) {
        return NULL;
    }

    size_t url_len = url_end - url_start;
    char *application_url = malloc(url_len + 1);
    if (application_url) {
        strncpy(application_url, url_start, url_len);
        application_url[url_len] = '\0';  // Null-terminate the URL
    }

    return application_url;
}

esp_err_t send_command(const char *command, const char *video_id, const Device *device) {
    // Check for valid arguments
    if (!device || !command || !video_id) {
        ESP_LOGE(TAG, "Invalid arguments: device, command, or video_id is NULL.");
        return ESP_ERR_INVALID_ARG;
    }

    // URL encode the required parameters
    char *encoded_loungeIdToken = url_encode(device->YoutubeToken);
    char *encoded_UUID = url_encode(device->UUID);
    char *encoded_zx = url_encode(generate_zx());
    char *encoded_SID = url_encode(device->SID);
    char *encoded_gsession = url_encode(device->gsession);

    
    if (!encoded_loungeIdToken || !encoded_UUID || !encoded_zx || !encoded_SID || !encoded_gsession) {
        ESP_LOGE(TAG, "URL encoding failed for one or more parameters.");
        goto cleanup;
    }

    
    ESP_LOGI(TAG, "Encoded Parameters:\n  loungeIdToken: %s\n  UUID: %s\n  zx: %s\n  SID: %s\n  gsession: %s",
             encoded_loungeIdToken, encoded_UUID, encoded_zx, encoded_SID, encoded_gsession);

    
    char url_params[1024];
    snprintf(url_params, sizeof(url_params),
             "device=REMOTE_CONTROL&loungeIdToken=%s&id=%s&VER=8&zx=%s&SID=%s&RID=%i&AID=5&gsessionid=%s",
             encoded_loungeIdToken, encoded_UUID, encoded_zx, encoded_SID, 1, encoded_gsession);

    
    char form_data[512];
    snprintf(form_data, sizeof(form_data),
             "count=1&ofs=0&req0__sc=%s&req0_videoId=%s&req0_listId=%s",
             command, video_id, device->listID);

    // Log the form data and URL
    ESP_LOGI(TAG, "Form Data: %s", form_data);

    // Initialize response buffer
    response_buffer_t resp_buf = {
        .buffer = malloc(1524),
        .buffer_len = 0,
        .buffer_size = 1524
    };
    if (!resp_buf.buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer.");
        goto cleanup;
    }

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = "https://www.youtube.com/api/lounge/bc/bind",
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .user_data = &resp_buf,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the full URL with parameters
    char full_url[1524];
    snprintf(full_url, sizeof(full_url), "%s?%s", config.url, url_params);
    esp_http_client_set_url(client, full_url);

    // Log the full URL
    ESP_LOGI(TAG, "Full URL: %s", full_url);

    // Set HTTP method, headers, and post data
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Origin", "https://www.youtube.com");
    esp_http_client_set_post_field(client, form_data, strlen(form_data));

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Get and log the HTTP status code
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status Code: %d", status_code);

    if (status_code != 200) {
        ESP_LOGE(TAG, "Unexpected HTTP response: %d", status_code);
    } else {
        ESP_LOGI(TAG, "Command sent successfully.");
    }

    // Ensure the response buffer is null-terminated
    if (resp_buf.buffer_len < resp_buf.buffer_size) {
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    } else {
        // Reallocate buffer if needed
        char *new_buffer = realloc(resp_buf.buffer, resp_buf.buffer_size + 1);
        if (!new_buffer) {
            ESP_LOGE(TAG, "Failed to reallocate memory for response buffer.");
            goto cleanup;
        }
        resp_buf.buffer = new_buffer;
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    }

    // Log the response from the server
    ESP_LOGI(TAG, "Response: %s", resp_buf.buffer);

cleanup:
    // Free allocated memory
    free(encoded_loungeIdToken);
    free(encoded_UUID);
    free(encoded_zx);
    free(encoded_SID);
    free(encoded_gsession);
    free(resp_buf.buffer);
    esp_http_client_cleanup(client);

    return ESP_OK;
}



esp_err_t bind_session_id(Device *device) {
    if (!device) {
        ESP_LOGE(TAG, "Invalid device pointer.");
        return ESP_ERR_INVALID_ARG;
    }


    strcpy(device->UUID, generate_uuid());

    // Generate ZX parameter
    char *zx = generate_zx();
    if (!zx) {
        ESP_LOGE(TAG, "Failed to generate zx");
        return ESP_FAIL;
    }

    // Update and increment RID
    static unsigned long rid = 0;
    rid++;

    // URL-encode parameter values
    char *encoded_loungeIdToken = url_encode(device->YoutubeToken);
    char *encoded_UUID = url_encode(device->UUID);
    char *encoded_zx = url_encode(zx);
    char *encoded_name = url_encode("Flipper_0");

    if (!encoded_loungeIdToken || !encoded_UUID || !encoded_zx || !encoded_name) {
        ESP_LOGE(TAG, "Failed to URL-encode parameters.");
        free(encoded_loungeIdToken);
        free(encoded_UUID);
        free(encoded_zx);
        free(encoded_name);
        free(zx);
        return ESP_FAIL;
    }

    // Prepare URL parameters with encoded values
    char url_params[1048];
    snprintf(url_params, sizeof(url_params),
             "device=REMOTE_CONTROL&mdx-version=3&ui=1&v=2&name=%s"
             "&app=youtube-desktop&loungeIdToken=%s&id=%s&VER=8&CVER=1&zx=%s&RID=%lu",
             encoded_name, encoded_loungeIdToken, encoded_UUID, encoded_zx, rid);

    // Log the full URL for debugging
    ESP_LOGI(TAG, "Constructed URL: %s?%s", "https://www.youtube.com/api/lounge/bc/bind", url_params);

    // Initialize response buffer
    response_buffer_t resp_buf = {
        .buffer = malloc(1524),
        .buffer_len = 0,
        .buffer_size = 1524
    };
    if (resp_buf.buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
        return ESP_FAIL;
    }

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = "https://www.youtube.com/api/lounge/bc/bind",
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .user_data = &resp_buf,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client.");
        free(resp_buf.buffer);
        return ESP_FAIL;
    }

    // Set HTTP method and headers
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Origin", "https://www.youtube.com");

    // Set the full URL with parameters
    char full_url[4096];
    snprintf(full_url, sizeof(full_url), "%s?%s", config.url, url_params);
    esp_http_client_set_url(client, full_url);

    
    const char *json_data = "{\"count\": 0}";
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    
    ESP_LOGI(TAG, "Request Body: %s", json_data);

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(resp_buf.buffer);
        return err;
    }

    // Null-terminate the response buffer
    if (resp_buf.buffer_len < resp_buf.buffer_size) {
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    } else {
        // Reallocate buffer to add null terminator
        char *new_buffer = realloc(resp_buf.buffer, resp_buf.buffer_size + 1);
        if (new_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for null terminator");
            esp_http_client_cleanup(client);
            free(resp_buf.buffer);
            return ESP_FAIL;
        }
        resp_buf.buffer = new_buffer;
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    }

    // Log the response
    printf("Response: %s", resp_buf.buffer);

    BindSession_Params_t result = parse_response(resp_buf.buffer);

    
    char *gsession_item = result.gsessionid;
    char *sid_item = result.sid;
    char *lid_item = result.listId;


    if (gsession_item && sid_item) {
        strcpy(device->gsession, gsession_item);
        strcpy(device->SID, sid_item);
        if (lid_item && lid_item) {
            strcpy(device->listID, lid_item);
        }
        ESP_LOGI(TAG, "Session bound successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to bind session ID.");
        esp_http_client_cleanup(client);
        free(resp_buf.buffer);
        return ESP_FAIL;
    }

    // Clean up
    free(encoded_loungeIdToken);
    free(encoded_UUID);
    free(encoded_zx);
    free(encoded_name);
    free(zx);
    esp_http_client_cleanup(client);
    free(resp_buf.buffer);
    return ESP_OK;
}

char* extract_token_from_json(const char *json_response) {
    cJSON *json = cJSON_Parse(json_response);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON.");
        return NULL;
    }

    
    cJSON *screens = cJSON_GetObjectItem(json, "screens");
    if (!screens || !cJSON_IsArray(screens)) {
        ESP_LOGE(TAG, "No screens array in response.");
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *first_screen = cJSON_GetArrayItem(screens, 0);
    if (!first_screen) {
        ESP_LOGE(TAG, "No screen element found.");
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *lounge_token = cJSON_GetObjectItem(first_screen, "loungeToken");
    if (!lounge_token || !cJSON_IsString(lounge_token)) {
        ESP_LOGE(TAG, "No loungeToken found.");
        cJSON_Delete(json);
        return NULL;
    }

    char *token = strdup(lounge_token->valuestring);
    cJSON_Delete(json);
    return token;
}

char* get_youtube_token(const char *screen_id) {
    response_buffer_t resp_buf = { .buffer = malloc(1024), .buffer_len = 0, .buffer_size = 1024 };
    if (resp_buf.buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
        return NULL;
    }

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = "https://www.youtube.com/api/lounge/pairing/get_lounge_token_batch",
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler =  _http_event_handler,
        .user_data = &resp_buf,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client.");
        free(resp_buf.buffer);
        return NULL;
    }

    // Prepare POST data
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "screen_ids=%s", screen_id);

    // Set the POST data
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(resp_buf.buffer);
        return NULL;
    }

    // Get the HTTP status code
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Response Code: %d", status_code);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to retrieve token. HTTP Response Code: %d", status_code);
        esp_http_client_cleanup(client);
        free(resp_buf.buffer);
        return NULL;
    }

    if (resp_buf.buffer_len < resp_buf.buffer_size) {
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    } else {
        // Reallocate buffer to add null terminator
        char *new_buffer = realloc(resp_buf.buffer, resp_buf.buffer_size + 1);
        if (new_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for null terminator");
            esp_http_client_cleanup(client);
            free(resp_buf.buffer);
            return NULL;
        }
        resp_buf.buffer = new_buffer;
        resp_buf.buffer[resp_buf.buffer_len] = '\0';
    }

    // Log the raw API response
    ESP_LOGI(TAG, "YouTube API raw response: %s", resp_buf.buffer);

    // Extract the lounge token from the JSON response
    char *lounge_token = extract_token_from_json(resp_buf.buffer);
    if (lounge_token) {
        ESP_LOGI(TAG, "Successfully retrieved loungeToken: %s", lounge_token);
    } else {
        ESP_LOGE(TAG, "Failed to parse JSON or retrieve loungeToken.");
    }

    // Free the allocated memory
    free(resp_buf.buffer);

    // Clean up the HTTP client
    esp_http_client_cleanup(client);

    // Return the lounge token (or NULL if extraction failed)
    return lounge_token;
}

char* extract_screen_id(const char* xml_data) {
    const char* start_tag = "<screenId>";
    const char* end_tag = "</screenId>";

    char* start = strstr(xml_data, start_tag);
    if (!start) {
        ESP_LOGE("DIALManager", "Start tag <screenId> not found.");
        return NULL;
    }
    start += strlen(start_tag);


    char* end = strstr(start, end_tag);
    if (!end) {
        ESP_LOGE("DIALManager", "End tag </screenId> not found.");
        return NULL;
    }

    size_t screen_id_len = end - start;
    if (screen_id_len == 0) {
        ESP_LOGE("DIALManager", "Extracted screenId is empty.");
        return NULL;
    }

    
    char* screen_id = malloc(screen_id_len + 1);
    if (screen_id) {
        strncpy(screen_id, start, screen_id_len);
        screen_id[screen_id_len] = '\0';
        ESP_LOGI("DIALManager", "Extracted screenId: %s", screen_id);
    }
    return screen_id;
}

// Initialize DIAL Manager
esp_err_t dial_manager_init(DIALManager *manager, DIALClient *client) {
    if (!manager || !client) {
        return ESP_ERR_INVALID_ARG;
    }
    manager->client = client;
    return ESP_OK;
}

bool fetch_screen_id_with_retries(const char *applicationUrl, Device *device, DIALManager *manager) {
    for (int i = 0; i < 5; i++) {
        if (check_app_status(manager, APP_YOUTUBE, applicationUrl, device) == ESP_OK && strlen(device->screenID) > 0) {
            ESP_LOGI(TAG, "Fetched Screen ID: %s", device->screenID);

            
            char *youtube_token = get_youtube_token(device->screenID);
            if (youtube_token) {
                
                strncpy(device->YoutubeToken, youtube_token, sizeof(device->YoutubeToken) - 1);
                device->YoutubeToken[sizeof(device->YoutubeToken) - 1] = '\0';
                free(youtube_token);
                ESP_LOGI(TAG, "Fetched YouTube Token: %s", device->YoutubeToken);
            } else {
                ESP_LOGE(TAG, "Failed to fetch YouTube token.");
                return false; 
            }

            
            if (bind_session_id(device) == ESP_OK) {
                ESP_LOGI(TAG, "Session successfully bound.");
                return true;
            } else {
                ESP_LOGE(TAG, "Failed to bind session ID.");
                return false; 
            }
        } else {
            ESP_LOGW(TAG, "Screen ID is empty. Retrying... (%d/%d)", i + 1, 5);
            vTaskDelay(500 / portTICK_PERIOD_MS); 
        }
    }

    ESP_LOGE(TAG, "Failed to fetch Screen ID after max retries.");
    return false;
}

char *g_app_url = NULL; 

esp_err_t _http_event_header_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "Header: %s: %s", evt->header_key, evt->header_value);
            if (strcasecmp(evt->header_key, "Application-Url") == 0) {
                if (evt->header_value != NULL) {
                    g_app_url = strdup(evt->header_value);
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

char* get_dial_application_url(const char *location_url) {
    char ip[64];
    uint16_t port = 0;

    // Extract IP and port from the location URL
    if (extract_ip_and_port(location_url, ip, &port) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to extract IP and port from URL");
        return NULL;
    }

    // Extract path from the location URL
    char *path = extract_path_from_url(location_url);
    if (!path) {
        ESP_LOGE(TAG, "Failed to extract path from URL");
        return NULL;
    }

    ESP_LOGI(TAG, "Connecting to IP: %s, Port: %u, Path: %s", ip, port, path);

    // Configure the HTTP client
    esp_http_client_config_t config = {
        .host = ip,
        .port = port,
        .path = path,
        .timeout_ms = 5000,
        .event_handler = _http_event_header_handler,  // Set the event handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(path);
        return NULL;
    }

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(path);
        return NULL;
    }

    // Check HTTP status code
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status Code: %d", status_code);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to fetch device description. HTTP Status Code: %d", status_code);
        esp_http_client_cleanup(client);
        free(path);
        return NULL;
    }

    // After the request, check if the Application-Url header was found
    if (g_app_url != NULL) {
        ESP_LOGI(TAG, "Application-Url: %s", g_app_url);
        char *app_url_copy = strdup(g_app_url);  // Copy the URL to return
        free(g_app_url);  // Free the global variable
        g_app_url = NULL; // Reset the global variable
        esp_http_client_cleanup(client);
        free(path);
        return app_url_copy;
    } else {
        ESP_LOGE(TAG, "Couldn't find 'Application-Url' in the headers.");
    }

    // Clean up resources
    esp_http_client_cleanup(client);
    free(path);
    return NULL;
}

// Helper to extract IP and port from URL
esp_err_t extract_ip_and_port(const char *url, char *ip_out, uint16_t *port_out) {
    // Assuming the URL is in the format http://<ip>:<port>/<path>
    const char *ip_start = strstr(url, "http://");
    if (!ip_start) {
        return ESP_ERR_INVALID_ARG;
    }
    ip_start += strlen("http://");

    const char *port_start = strchr(ip_start, ':');
    const char *path_start = strchr(ip_start, '/');
    if (!port_start || !path_start || port_start > path_start) {
        return ESP_ERR_INVALID_ARG;
    }

    // Extract the IP and port
    size_t ip_len = port_start - ip_start;
    strncpy(ip_out, ip_start, ip_len);
    ip_out[ip_len] = '\0';

    *port_out = atoi(port_start + 1);

    return ESP_OK;
}

// Helper to get the correct path for the app
const char* get_app_path(DIALAppType app) {
    switch (app) {
        case APP_YOUTUBE:
            return "/YouTube";
        case APP_NETFLIX:
            return "/Netflix";
        default:
            return "/";
    }
}

char* remove_ip_and_port(const char *url) {
    const char *path_start = strchr(url, '/');
    if (path_start) {
        if (strncmp(path_start, "//", 2) == 0) {
            path_start = strchr(path_start + 2, '/');
        }
    }

    
    if (path_start) {
        return strdup(path_start);
    }

    return NULL;
}


// Check the app status by communicating with the device
esp_err_t check_app_status(DIALManager *manager, DIALAppType app, const char *appUrl, Device *device) {
    if (!manager || !appUrl || !device) {
        return ESP_ERR_INVALID_ARG;
    }

    char ip[64];
    uint16_t port = 0;
    if (extract_ip_and_port(appUrl, ip, &port) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to extract IP and port from URL");
        return ESP_ERR_INVALID_ARG;
    }

    const char *app_path = get_app_path(app);
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", appUrl, app_path);

    ESP_LOGI(TAG, "Connecting to IP: %s, Port: %u, Path: %s", ip, port, app_path);

    char *path = remove_ip_and_port(full_path);

    esp_http_client_config_t config = {
        .host = ip,
        .port = port,
        .path = path,
        .timeout_ms = 5000
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);
    if (http_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_header(http_client, "Origin", "https://www.youtube.com");

    // Open the connection manually
    esp_err_t err = esp_http_client_open(http_client, 0);  // 0 means no request body
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(http_client);
        return err;
    }

    // Check if we got any headers back
    int status_code = esp_http_client_fetch_headers(http_client);
    if (status_code < 0) {
        ESP_LOGE(TAG, "Failed to fetch HTTP headers");
        esp_http_client_cleanup(http_client);
        return ESP_FAIL;
    }

    // Log HTTP status code
    status_code = esp_http_client_get_status_code(http_client);
    ESP_LOGI(TAG, "HTTP status code: %d", status_code);

    if (status_code == 200) {
        char response_body[512];
        int content_len = esp_http_client_read(http_client, response_body, sizeof(response_body) - 1);
        if (content_len >= 0) {
            response_body[content_len] = '\0';  // Null-terminate the response body

            ESP_LOGI(TAG, "Response Body:\n%s", response_body);

            // Check if app is running
            if (strstr(response_body, "<state>running</state>")) {
                ESP_LOGI("DIALManager", "%s app is running", (app == APP_YOUTUBE) ? "YouTube" : "Netflix");

                // Extract screenId from the response
                char *screen_id = extract_screen_id(response_body);
                if (screen_id) {
                    strncpy(device->screenID, screen_id, sizeof(device->screenID) - 1);  // Store in device
                    free(screen_id);  // Free allocated memory
                    esp_http_client_cleanup(http_client);
                    return ESP_OK;
                }
                esp_http_client_cleanup(http_client);
                return ESP_FAIL;
            } else {
                ESP_LOGW("DIALManager", "%s app is not running", (app == APP_YOUTUBE) ? "YouTube" : "Netflix");
                esp_http_client_cleanup(http_client);
                return ESP_ERR_NOT_FOUND;
            }
        } else {
            ESP_LOGE(TAG, "Failed to read HTTP response body");
            esp_http_client_cleanup(http_client);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE("DIALManager", "Unexpected HTTP status code: %d", status_code);
        esp_http_client_cleanup(http_client);
        return ESP_FAIL;
    }
}



bool launch_app(DIALManager *manager, DIALAppType app, const char *appUrl) {
    if (!manager || !appUrl) {
        return false;
    }

    char ip[64];
    uint16_t port = 0;
    if (extract_ip_and_port(appUrl, ip, &port) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to extract IP and port from URL");
        return false;
    }

    const char *app_path = get_app_path(app);
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", appUrl, app_path);

    ESP_LOGI(TAG, "Launching app: %s at IP: %s, Port: %u, Path: %s", 
             (app == APP_YOUTUBE) ? "YouTube" : "Netflix", ip, port, full_path);

    char *path = remove_ip_and_port(full_path);

    esp_http_client_config_t config = {
        .host = ip,
        .port = port,
        .path = path,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);

    esp_http_client_set_header(http_client, "Origin", "https://www.youtube.com");

    esp_err_t err = esp_http_client_perform(http_client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(http_client);
        if (status_code == 201 || status_code == 200) {
            ESP_LOGI(TAG, "Successfully launched the app: %s", (app == APP_YOUTUBE) ? "YouTube" : "Netflix");
            esp_http_client_cleanup(http_client);
            return true;
        } else {
            ESP_LOGE(TAG, "Failed to launch the app. HTTP Response Code: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "Failed to send the launch request. Error: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(http_client);
    return false;
}


void explore_network(DIALManager *manager) {
    const char *yt_url = "dQw4w9WgXcQ";
    
    for (int attempt = 0; attempt < 5; ++attempt) {
        Device *devices = (Device *)malloc(sizeof(Device) * 10);
        size_t device_count = 0;

        
        ESP_LOGI(TAG, "Discovering devices... (Attempt %d/%d)", attempt + 1, 10);
        if (dial_client_discover_devices(manager->client, devices, 10, &device_count) != ESP_OK || device_count == 0) {
            ESP_LOGW(TAG, "No devices discovered. Retrying...");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            continue;
        }

        
        for (size_t i = 0; i < device_count; ++i) {
            Device *device = &devices[i];
            ESP_LOGI(TAG, "Discovered Device: %s (Location: %s)", device->uniqueServiceName, device->location);


            char* appUrl = get_dial_application_url(device->location);

            if (appUrl == NULL)
            {
                continue;
            }

            
            if (check_app_status(manager, APP_YOUTUBE, appUrl, device) != ESP_OK) {
                ESP_LOGI(TAG, "YouTube app is not running. Launching the app...");
                if (!launch_app(manager, APP_YOUTUBE, appUrl)) {
                    ESP_LOGE(TAG, "Failed to launch YouTube app.");
                    continue;
                }

                
                unsigned long startTime = xTaskGetTickCount();
                bool isYouTubeRunning = false;
                while ((xTaskGetTickCount() - startTime) < (7000 / portTICK_PERIOD_MS)) {
                    if (check_app_status(manager, APP_YOUTUBE, appUrl, device) == ESP_OK) {
                        isYouTubeRunning = true;
                        break;
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                if (!isYouTubeRunning) {
                    ESP_LOGE(TAG, "YouTube app did not start within the timeout.");
                    continue;
                }
            }

           
            if (!fetch_screen_id_with_retries(appUrl, device, manager)) {
                ESP_LOGE(TAG, "Failed to fetch Screen ID.");
                continue;
            }

            
            if (send_command("setPlaylist", yt_url, device) == ESP_OK) {
                ESP_LOGI(TAG, "YouTube video command sent successfully.");
            } else {
                ESP_LOGE(TAG, "Failed to send YouTube command.");
            }
        }

        free(devices);
        break;
    }
}

#pragma GCC diagnostic pop
