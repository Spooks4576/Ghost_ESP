#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include "esp_err.h"
#include <stddef.h>

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

#endif // DOWNLOAD_MANAGER_H 