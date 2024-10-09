#include "vendor/dial_client.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define DIAL_MULTICAST_IP "239.255.255.250"
#define DIAL_MULTICAST_PORT 1900
#define MAX_RETRIES 10
#define RETRY_DELAY_MS 2000
#define RESPONSE_BUFFER_SIZE 1024

static const char *TAG = "DIALClient";

// M-SEARCH request to search for devices
static const char msearch_request[] =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "ST: urn:dial-multiscreen-org:service:dial:1\r\n"
    "MX: 3\r\n\r\n";

// Initialize the DIAL Client
esp_err_t dial_client_init(DIALClient *client) {
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client->socket_fd < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return ESP_FAIL;
    }

    // Setup multicast address
    memset(&client->multicast_addr, 0, sizeof(client->multicast_addr));
    client->multicast_addr.sin_family = AF_INET;
    client->multicast_addr.sin_port = htons(DIAL_MULTICAST_PORT);
    inet_aton(DIAL_MULTICAST_IP, &client->multicast_addr.sin_addr);

    return ESP_OK;
}

// Discover devices on the network
esp_err_t dial_client_discover_devices(DIALClient *client, Device *devices, size_t max_devices, size_t *device_count) {
    int retries = 0;
    *device_count = 0;

    while (*device_count == 0 && retries < MAX_RETRIES) {
        if (retries > 0) {
            ESP_LOGI(TAG, "Retrying device discovery...");
            vTaskDelay(RETRY_DELAY_MS / portTICK_PERIOD_MS);
        }

        // Send M-SEARCH packets to the multicast group
        for (int i = 0; i < 5; i++) {
            if (sendto(client->socket_fd, msearch_request, sizeof(msearch_request), 0,
                       (struct sockaddr *)&client->multicast_addr, sizeof(client->multicast_addr)) < 0) {
                ESP_LOGE(TAG, "Failed to send M-SEARCH request");
                return ESP_FAIL;
            }
            vTaskDelay(500 / portTICK_PERIOD_MS); // delay 500ms between sends
        }

        // Set a 5 second timeout for receiving responses
        struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
        setsockopt(client->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Listen for responses
        char response_buffer[RESPONSE_BUFFER_SIZE];
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        while (recvfrom(client->socket_fd, response_buffer, RESPONSE_BUFFER_SIZE, 0,
                        (struct sockaddr *)&source_addr, &addr_len) > 0) {
            // Null-terminate the response
            response_buffer[RESPONSE_BUFFER_SIZE - 1] = '\0';

            // Parse SSDP response and add to device list if valid
            Device device;
            if (parse_ssdp_response(response_buffer, &device)) {
                if (*device_count < max_devices) {
                    devices[*device_count] = device;
                    (*device_count)++;
                } else {
                    ESP_LOGW(TAG, "Device list is full");
                    break;
                }
            }
        }
        retries++;
    }

    if (*device_count == 0) {
        ESP_LOGW(TAG, "No devices found after retries");
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

// Parse SSDP response to extract device details
bool parse_ssdp_response(const char *response, Device *device) {
    const char *usn_start = strstr(response, "USN: ");
    const char *location_start = strstr(response, "LOCATION: ");

    if (usn_start && location_start) {
        sscanf(usn_start, "USN: %127s", device->usn);
        sscanf(location_start, "LOCATION: %255s", device->location);

        // Extract unique service name
        snprintf(device->uniqueServiceName, sizeof(device->uniqueServiceName), "%s", device->usn);

        ESP_LOGI(TAG, "Discovered Device: USN=%s, Location=%s", device->usn, device->location);
        return true;
    }

    return false;
}

// Deinitialize the DIAL Client
void dial_client_deinit(DIALClient *client) {
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
}