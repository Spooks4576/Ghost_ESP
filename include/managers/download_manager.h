#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>
#include "cJSON.h"
#include "esp_http_client.h"

/**
 * WiFi connection status codes
 * Used to track the current state of WiFi connectivity
 */
typedef enum {
    DOWNLOAD_WIFI_STATUS_DISCONNECTED, // No WiFi connection available
    DOWNLOAD_WIFI_STATUS_CONNECTING,   // Currently attempting to connect
    DOWNLOAD_WIFI_STATUS_CONNECTED,    // Successfully connected to WiFi
    DOWNLOAD_WIFI_STATUS_ERROR        // Error occurred during connection
} download_wifi_status_t;

/**
 * Status codes to track download operations
 * Use download_manager_get_status() to check current status
 */
typedef enum {
    DOWNLOAD_STATUS_OK = 0,          // Download completed successfully
    DOWNLOAD_STATUS_IN_PROGRESS,     // Download is currently running
    DOWNLOAD_STATUS_CANCELLED,       // Download was cancelled by user
    DOWNLOAD_STATUS_NETWORK_ERROR,   // Network connectivity issues
    DOWNLOAD_STATUS_FILE_ERROR,      // Error writing to file or invalid file
    DOWNLOAD_STATUS_MEMORY_ERROR,    // Out of memory error
    DOWNLOAD_STATUS_INVALID_URL,     // URL is NULL or malformed
    DOWNLOAD_STATUS_HTTP_ERROR       // Server returned error status code
} download_status_t;

/**
 * HTTP request configuration structure
 * Used to configure detailed HTTP request parameters
 */
typedef struct {
    const char* url;          // Target URL for the request
    esp_http_client_method_t method;     // Using ESP-IDF's HTTP method enum
    const char* headers;      // Optional custom headers (NULL if none)
    const char* payload;      // Request body for POST/PUT (NULL if none)
    uint32_t timeout_ms;      // Request timeout (0 for default)
    bool verify_ssl;          // Whether to verify SSL certificates
} http_request_config_t;

/**
 * HTTP response structure
 * Contains the response data from HTTP requests
 */
typedef struct {
    char* body;              // Response body (must be freed by caller)
    int status_code;         // HTTP status code
    size_t content_length;   // Content length (if known)
    char* content_type;      // Content type header value
    cJSON* json;            // Add JSON parsed response
} http_response_t;

/**
 * Check if WiFi is connected and ready for downloads
 * 
 * Usage example:
 *   download_wifi_status_t status = download_manager_check_wifi();
 *   if (status != DOWNLOAD_WIFI_STATUS_CONNECTED) {
 *       // Handle disconnected state
 *   }
 * 
 * @return Current WiFi connection status
 */
download_wifi_status_t download_manager_check_wifi(void);

/**
 * Wait for WiFi connection with timeout
 * 
 * Usage example:
 *   esp_err_t ret = download_manager_await_wifi(5000);  // Wait up to 5 seconds
 *   if (ret != ESP_OK) {
 *       // Handle connection timeout
 *   }
 *
 * @param timeout_ms Maximum time to wait in milliseconds (0 for no timeout)
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT if timeout reached
 */
esp_err_t download_manager_await_wifi(uint32_t timeout_ms);

/**
 * Get the current status of the last/ongoing download operation
 * @return Current download status code
 */
download_status_t download_manager_get_status(void);

/**
 * Get the HTTP status code from the last download operation
 * @return HTTP status code (e.g., 200 for success, 404 for not found)
 */
int download_manager_get_http_status(void);

/**
 * Initialize the download manager. Must be called before using other functions.
 * @return ESP_OK on success
 */
esp_err_t download_manager_init(void);

/**
 * Download a file from a URL and save it to the local filesystem
 * 
 * Usage example:
 *   esp_err_t ret = download_manager_get_file("https://example.com/file.txt", "/spiffs/local.txt");
 *   if (ret != ESP_OK) {
 *       // Check download_manager_get_status() for detailed error
 *   }
 *
 * @param url Source URL to download from (must start with http:// or https://)
 * @param save_path Local filesystem path to save the downloaded file
 * @return ESP_OK on success, ESP_FAIL or other error code on failure
 */
esp_err_t download_manager_get_file(const char* url, const char* save_path);

/**
 * Download data from a URL and return it as a string in memory
 * 
 * Usage example:
 *   char* data = download_manager_fetch_string("https://example.com/api");
 *   if (data) {
 *       // Use the data
 *       free(data);  // Must free the returned string when done
 *   }
 *
 * @param url Source URL to download from
 * @return Allocated string containing the downloaded data (must be freed by caller), or NULL on error
 */
char* download_manager_fetch_string(const char* url);

/**
 * Get the progress of the current download operation
 * @return Progress percentage (0-100)
 */
int download_manager_get_progress(void);

/**
 * Cancel any ongoing download operation
 * The current operation will return with an error status
 */
void download_manager_cancel(void);

/**
 * Callback function type for download progress updates
 * @param progress Current progress percentage (0-100)
 * @param bytes_received Number of bytes received so far
 * @param total_bytes Total bytes to download (may be 0 if unknown)
 * @param user_data User-provided context data
 */
typedef void (*download_progress_cb_t)(int progress, size_t bytes_received, size_t total_bytes, void* user_data);

/**
 * Configure download timeout
 * @param timeout_ms Timeout in milliseconds (0 for default)
 * @return ESP_OK on success
 */
esp_err_t download_manager_set_timeout(uint32_t timeout_ms);

/**
 * Download a file with progress callback
 * @param url Source URL to download from
 * @param save_path Local filesystem path to save the downloaded file
 * @param progress_cb Optional callback function for progress updates (can be NULL)
 * @param user_data Optional user data passed to callback (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t download_manager_get_file_with_cb(
    const char* url,
    const char* save_path,
    download_progress_cb_t progress_cb,
    void* user_data
);

/**
 * Resume a partially downloaded file
 * @param url Source URL to download from
 * @param save_path Local filesystem path (must exist)
 * @param offset Byte offset to resume from
 * @param progress_cb Optional progress callback (can be NULL)
 * @param user_data Optional user data for callback (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t download_manager_resume_file(
    const char* url,
    const char* save_path,
    size_t offset,
    download_progress_cb_t progress_cb,
    void* user_data
);

/**
 * Simple GET request with default configuration
 * 
 * Usage example:
 *   http_response_t* response = download_manager_http_get("https://api.example.com/data");
 *   if (response) {
 *       printf("Response: %s\n", response->body);
 *       download_manager_free_response(response);
 *   }
 *
 * @param url Target URL
 * @return Response (must be freed by caller) or NULL on error
 */
http_response_t* download_manager_http_get(const char* url);

/**
 * Simple POST request with default configuration
 * 
 * Usage example:
 *   http_response_t* response = download_manager_http_post("https://api.example.com/data", "{\"key\":\"value\"}");
 *   if (response) {
 *       printf("Status: %d\n", response->status_code);
 *       download_manager_free_response(response);
 *   }
 *
 * @param url Target URL
 * @param payload POST data
 * @return Response (must be freed by caller) or NULL on error
 */
http_response_t* download_manager_http_post(const char* url, const char* payload);

/**
 * Perform HTTP request with custom configuration
 * 
 * Usage example:
 *   http_request_config_t config = {
 *       .url = "https://api.example.com",
 *       .method = HTTP_METHOD_POST,
 *       .headers = "Content-Type: application/json\nAuthorization: Bearer token",
 *       .payload = "{\"data\":\"test\"}",
 *       .timeout_ms = 5000,
 *       .verify_ssl = true
 *   };
 *   http_response_t* response = download_manager_http_request(&config);
 *
 * @param config Request configuration
 * @return Response (must be freed by caller) or NULL on error
 */
http_response_t* download_manager_http_request(const http_request_config_t* config);

/**
 * Free HTTP response structure
 * Must be called to clean up response data after use
 * 
 * @param response Response to free
 */
void download_manager_free_response(http_response_t* response);

/**
 * Parse response body as JSON
 * @param response HTTP response to parse
 * @return ESP_OK if parsed successfully, error code otherwise
 */
esp_err_t download_manager_parse_json(http_response_t* response);

#endif // DOWNLOAD_MANAGER_H 