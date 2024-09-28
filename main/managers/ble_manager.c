#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "host/ble_gap.h"
#include "managers/ble_manager.h"
#include "esp_random.h"
#include <esp_mac.h>
#include "nimble/nimble_port_freertos.h"
#include <managers/rgb_manager.h>
#include <managers/settings_manager.h>

#define MAX_DEVICES 30
#define MAX_HANDLERS 10
#define MAX_PACKET_SIZE 31

static const char *TAG_BLE = "BLE_MANAGER";
static int airTagCount = 0;

typedef struct {
    ble_data_handler_t handler;
} ble_handler_t;


static ble_handler_t *handlers = NULL;
static int handler_count = 0;
static int spam_counter = 0;
static uint16_t *last_company_id = NULL;
static TickType_t last_detection_time = 0;  


static void notify_handlers(struct ble_gap_event *event, int len) {
    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].handler) {
            handlers[i].handler(event, len);
        }
    }
}

void nimble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int8_t generate_random_rssi() {
    return (esp_random() % 121) - 100;
}

static void generate_random_name(char *name, size_t max_len) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t len = (esp_random() % (max_len - 1)) + 1;

    for (size_t i = 0; i < len; i++) {
        name[i] = charset[esp_random() % (sizeof(charset) - 1)];
    }
    name[len] = '\0';
}

static void generate_random_mac(uint8_t *mac_addr) {
    esp_fill_random(mac_addr, 6);

    mac_addr[0] |= 0xC0;

    mac_addr[0] &= 0xFE;
}

void stop_ble_stack() {
    int rc;

    
    rc = ble_gap_adv_stop();
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error stopping advertisement; rc=%d", rc);
    }

    
    rc = nimble_port_stop();
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error stopping NimBLE port; rc=%d", rc);
        return;
    }

    
    nimble_port_deinit();

    ESP_LOGI(TAG_BLE, "NimBLE stack and task deinitialized.");
}

static bool extract_company_id(const uint8_t *payload, size_t length, uint16_t *company_id) {
    size_t index = 0;

    while (index < length) {
        uint8_t field_length = payload[index];

        if (field_length == 0 || index + field_length >= length) {
            break;
        }

        uint8_t field_type = payload[index + 1];

        if (field_type == 0xFF && field_length >= 3) {
            *company_id = payload[index + 2] | (payload[index + 3] << 8);
            return true;
        }

        index += field_length + 1;
    }

    return false;
}

static void parse_device_name(const uint8_t *data, uint8_t data_len, char *name, size_t name_size) {
    int index = 0;

    while (index < data_len) {
        uint8_t length = data[index];
        if (length == 0) {
            break;
        }
        uint8_t type = data[index + 1];

        
        if (type == BLE_HS_ADV_TYPE_COMP_NAME) {
            int name_len = length - 1;
            if (name_len > name_size - 1) {
                name_len = name_size - 1;
            }
            strncpy(name, (const char *)&data[index + 2], name_len);
            name[name_len] = '\0'; 
            return;
        }

        index += length + 1; 
    }

    
    strncpy(name, "Unknown", name_size);
}

static void parse_service_uuids(const uint8_t *data, uint8_t data_len, ble_service_uuids_t *uuids) {
    int index = 0;

    while (index < data_len) {
        uint8_t length = data[index];
        if (length == 0) {
            break;
        }
        uint8_t type = data[index + 1];

        // Check for 16-bit UUIDs
        if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS16 || type == BLE_HS_ADV_TYPE_INCOMP_UUIDS16) && uuids->uuid16_count < MAX_UUID16) {
            for (int i = 0; i < length - 1; i += 2) {
                uint16_t uuid16 = data[index + 2 + i] | (data[index + 3 + i] << 8);
                uuids->uuid16[uuids->uuid16_count++] = uuid16;
            }
        }

        // Check for 32-bit UUIDs
        else if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS32 || type == BLE_HS_ADV_TYPE_INCOMP_UUIDS32) && uuids->uuid32_count < MAX_UUID32) {
            for (int i = 0; i < length - 1; i += 4) {
                uint32_t uuid32 = data[index + 2 + i] | (data[index + 3 + i] << 8) | (data[index + 4 + i] << 16) | (data[index + 5 + i] << 24);
                uuids->uuid32[uuids->uuid32_count++] = uuid32;
            }
        }

        // Check for 128-bit UUIDs
        else if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS128 || type == BLE_HS_ADV_TYPE_INCOMP_UUIDS128) && uuids->uuid128_count < MAX_UUID128) {
            snprintf(uuids->uuid128[uuids->uuid128_count], sizeof(uuids->uuid128[uuids->uuid128_count]),
                     "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     data[index + 17], data[index + 16], data[index + 15], data[index + 14],
                     data[index + 13], data[index + 12], data[index + 11], data[index + 10],
                     data[index + 9], data[index + 8], data[index + 7], data[index + 6],
                     data[index + 5], data[index + 4], data[index + 3], data[index + 2]);
            uuids->uuid128_count++;
        }

        index += length + 1;
    }
}


static int ble_gap_event_general(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            notify_handlers(event, event->disc.length_data);

            break;

        default:
            break;
    }

    return 0;
}


void ble_findtheflippers_callback(struct ble_gap_event *event, size_t len) {
    int advertisementRssi = event->disc.rssi;


    char advertisementMac[18];
    snprintf(advertisementMac, sizeof(advertisementMac), "%02x:%02x:%02x:%02x:%02x:%02x",
             event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2],
             event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);


    char advertisementName[32];
    parse_device_name(event->disc.data, event->disc.length_data, advertisementName, sizeof(advertisementName));


    ble_service_uuids_t uuids = {0};
    parse_service_uuids(event->disc.data, event->disc.length_data, &uuids);

    for (int i = 0; i < uuids.uuid16_count; i++) {
        if (uuids.uuid16[i] == 0x3082) {
            printf("Found White Flipper Device: MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (uuids.uuid16[i] == 0x3081) {
            printf("Found Black Flipper Device: MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (uuids.uuid16[i] == 0x3083) {
            printf("Found Transparent Flipper Device: MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
    }


    for (int i = 0; i < uuids.uuid32_count; i++) {
        if (uuids.uuid32[i] == 0x3082) {
            printf("Found White Flipper Device (32-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (uuids.uuid32[i] == 0x3081) {
            printf("Found Black Flipper Device (32-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (uuids.uuid32[i] == 0x3083) {
            printf("Found Transparent Flipper Device (32-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
    }

    
    for (int i = 0; i < uuids.uuid128_count; i++) {
        if (strstr(uuids.uuid128[i], "3082") != NULL) {
            printf("Found White Flipper Device (128-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (strstr(uuids.uuid128[i], "3081") != NULL) {
            printf("Found Black Flipper Device (128-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
        if (strstr(uuids.uuid128[i], "3083") != NULL) {
            printf("Found Transparent Flipper Device (128-bit UUID): MAC: %s, Name: %s, RSSI: %d\n", advertisementMac, advertisementName, advertisementRssi);
            rgb_manager_set_color(&rgb_manager, 0, 255, 165, 0, true);
        }
    }
}

void ble_print_raw_packet_callback(struct ble_gap_event *event, size_t len) {
    
    int advertisementRssi = event->disc.rssi;

    
    char advertisementMac[18];
    snprintf(advertisementMac, sizeof(advertisementMac), "%02x:%02x:%02x:%02x:%02x:%02x",
             event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2],
             event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);

    
    printf("Received BLE Advertisement from MAC: %s, RSSI: %d\n", advertisementMac, advertisementRssi);

    
    printf("Raw Advertisement Data (len=%zu): ", event->disc.length_data);
    for (size_t i = 0; i < event->disc.length_data; i++) {
        printf("%02x ", event->disc.data[i]);
    }
    printf("\n");
}

void detect_ble_spam_callback(struct ble_gap_event *event, size_t length) {
    if (length < 4) {
        return;
    }

    TickType_t current_time = xTaskGetTickCount();
    TickType_t time_elapsed = current_time - last_detection_time;


    uint16_t current_company_id;
    if (!extract_company_id(event->disc.data, length, &current_company_id)) {
        return; 
    }

    if (time_elapsed > pdMS_TO_TICKS(TIME_WINDOW_MS)) {
        spam_counter = 0;
    }

    if (last_company_id != NULL && *last_company_id == current_company_id) {
        spam_counter++;
        
        if (spam_counter > MAX_PAYLOADS) {
            ESP_LOGW(TAG_BLE, "BLE Spam detected! Company ID: 0x%04X", current_company_id);
            rgb_manager_set_color(&rgb_manager, 0, 255, 0, 0, true);
            spam_counter = 0;
        }
    } else {
        if (last_company_id == NULL) {
            last_company_id = (uint16_t *)malloc(sizeof(uint16_t));
        }

        if (last_company_id != NULL) {
            *last_company_id = current_company_id;
            spam_counter = 1;               
        }
    }

    last_detection_time = current_time;
}


void airtag_scanner_callback(struct ble_gap_event *event, size_t len) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        if (!event->disc.data || event->disc.length_data < 4) {
            return; 
        }

        const uint8_t *payload = event->disc.data;
        size_t payloadLength = event->disc.length_data;

        bool patternFound = false;
        for (size_t i = 0; i <= payloadLength - 4; i++) {
            if ((payload[i] == 0x1E && payload[i + 1] == 0xFF && payload[i + 2] == 0x4C && payload[i + 3] == 0x00) ||
                (payload[i] == 0x4C && payload[i + 1] == 0x00 && payload[i + 2] == 0x12 && payload[i + 3] == 0x19)) {
                patternFound = true;
                break;
            }
        }

        if (patternFound) {
            char macAddress[18];
            snprintf(macAddress, sizeof(macAddress), "%02x:%02x:%02x:%02x:%02x:%02x",
                     event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2],
                     event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);

            int rssi = event->disc.rssi;

            printf("AirTag found!\n");
            printf("Tag: %d\n", airTagCount);
            printf("MAC Address: %s\n", macAddress);
            printf("RSSI: %d dBm\n", rssi);

            printf("Payload Data: ");
            for (size_t i = 0; i < payloadLength; i++) {
                printf("%02X ", payload[i]);
            }
            printf("\n\n");
        }
    }
}

void ble_start_scanning(void) {
    struct ble_gap_disc_params disc_params = {0};
    disc_params.itvl = BLE_HCI_SCAN_ITVL_DEF;
    disc_params.window = BLE_HCI_SCAN_WINDOW_DEF;
    disc_params.filter_duplicates = 1;

    // Start a new BLE scan
    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event_general, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error starting BLE scan; rc=%d", rc);
    } else {
        ESP_LOGI(TAG_BLE, "Scanning started...");
    }
}


esp_err_t ble_register_handler(ble_data_handler_t handler) {
    if (handler_count < MAX_HANDLERS) {
        ble_handler_t *new_handlers = realloc(handlers, (handler_count + 1) * sizeof(ble_handler_t));
        if (!new_handlers) {
            ESP_LOGE(TAG_BLE, "Failed to allocate memory for handlers");
            return ESP_ERR_NO_MEM;
        }

        handlers = new_handlers;
        handlers[handler_count].handler = handler;
        handler_count++;
        return ESP_OK;
    }

    return ESP_ERR_NO_MEM;
}


esp_err_t ble_unregister_handler(ble_data_handler_t handler) {
    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].handler == handler) {
            for (int j = i; j < handler_count - 1; j++) {
                handlers[j] = handlers[j + 1];
            }

            handler_count--;
            ble_handler_t *new_handlers = realloc(handlers, handler_count * sizeof(ble_handler_t));
            if (new_handlers || handler_count == 0) {
                handlers = new_handlers;
            }
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

void ble_init(void) {
    nvs_flash_init();
    nimble_port_init();

    nimble_port_freertos_init(nimble_host_task);

    ESP_LOGI(TAG_BLE, "BLE initialized");
}

void ble_start_find_flippers(void)
{
    ble_register_handler(ble_findtheflippers_callback);
    ble_start_scanning();
}

void ble_spam_stop()
{
    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
    if (ble_spam_task_running) {
        ESP_LOGI(TAG_BLE, "Stopping beacon transmission...");
        if (ble_spam_task_handle != NULL) {
            vTaskDelete(ble_spam_task_handle);
            ble_spam_task_handle = NULL;
            ble_spam_task_running = false;
        }
    } else {
        ESP_LOGW(TAG_BLE, "No beacon transmission is running.");
    }
}

void ble_stop(void) {
    if (last_company_id != NULL) {
        free(last_company_id);
        last_company_id = NULL;
    }
    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
    ble_unregister_handler(ble_findtheflippers_callback);
    ble_unregister_handler(airtag_scanner_callback);
    ble_unregister_handler(ble_print_raw_packet_callback);
    ble_unregister_handler(detect_ble_spam_callback);
    int rc = ble_gap_disc_cancel();

    if (rc == 0) {
        ESP_LOGI(TAG_BLE, "BLE scanning stopped successfully.");
    } else if (rc == BLE_HS_EALREADY) {
        ESP_LOGW(TAG_BLE, "BLE scanning was not active.");
    } else {
        ESP_LOGE(TAG_BLE, "Failed to stop BLE scanning; rc=%d", rc);
    }
}

void ble_start_blespam_detector(void)
{
    ble_register_handler(detect_ble_spam_callback);
    ble_start_scanning();
}

void ble_start_raw_ble_packetscan(void)
{
    ble_register_handler(ble_print_raw_packet_callback);
    ble_start_scanning();
}

void ble_start_airtag_scanner(void)
{
    ble_register_handler(airtag_scanner_callback);
    ble_start_scanning();
}