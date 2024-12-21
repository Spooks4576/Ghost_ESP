#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_gap.h"
#include "managers/ble_manager.h"
#include "esp_random.h"
#include <esp_mac.h>
#include <managers/rgb_manager.h>
#include <managers/settings_manager.h>
#include "managers/views/terminal_screen.h"
#include "vendor/pcap.h"
#include "core/callbacks.h"


#define MAX_DEVICES 30
#define MAX_HANDLERS 10
#define MAX_PACKET_SIZE 31

static const char *TAG_BLE = "BLE_MANAGER";
static int airTagCount = 0;
static bool ble_initialized = false;
static esp_timer_handle_t flush_timer = NULL;

typedef struct {
    ble_data_handler_t handler;
} ble_handler_t;


static ble_handler_t *handlers = NULL;
static int handler_count = 0;
static int spam_counter = 0;
static uint16_t *last_company_id = NULL;
static TickType_t last_detection_time = 0;  
static void ble_pcap_callback(struct ble_gap_event *event, size_t len);

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

void ble_stop_skimmer_detection(void) {
    ESP_LOGI("BLE", "Stopping skimmer detection scan...");
    TERMINAL_VIEW_ADD_TEXT("Stopping skimmer detection scan...\n");
    
    // Unregister the skimmer detection callback
    ble_unregister_handler(ble_skimmer_scan_callback);
    pcap_flush_buffer_to_file(); // Final flush
    pcap_file_close(); // Close the file after final flush
    
    int rc = ble_gap_disc_cancel();

    if (rc == 0) {
        printf("BLE skimmer detection stopped successfully.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE skimmer detection stopped successfully.\n");
    } else if (rc == BLE_HS_EALREADY) {
        printf("BLE scanning was not active.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE scanning was not active.\n");
    } else {
        printf("Failed to stop BLE scanning; rc=%d\n", rc);
        TERMINAL_VIEW_ADD_TEXT("Failed to stop BLE scanning; rc=%d\n", rc);
    }
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
            printf("Found White Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid16[i] == 0x3081) {
            printf("Found Black Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid16[i] == 0x3083) {
            printf("Found Transparent Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
    }
    for (int i = 0; i < uuids.uuid32_count; i++) {
        if (uuids.uuid32[i] == 0x3082) {
            printf("Found White Flipper Device (32-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid32[i] == 0x3081) {
            printf("Found Black Flipper Device (32-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid32[i] == 0x3083) {
            printf("Found Transparent Flipper Device (32-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
    }
    for (int i = 0; i < uuids.uuid128_count; i++) {
        if (strstr(uuids.uuid128[i], "3082") != NULL) {
            printf("Found White Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (strstr(uuids.uuid128[i], "3081") != NULL) {
            printf("Found Black Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (strstr(uuids.uuid128[i], "3083") != NULL) {
            printf("Found Transparent Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): \nMAC: %s, \nName: %s, \nRSSI: %d\n", 
                                advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
    }
}

void ble_print_raw_packet_callback(struct ble_gap_event *event, size_t len) {
    int advertisementRssi = event->disc.rssi;

    char advertisementMac[18];
    snprintf(advertisementMac, sizeof(advertisementMac), "%02x:%02x:%02x:%02x:%02x:%02x",
             event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2],
             event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);

    // stop logging raw advertisement data
    //
    // printf("Received BLE Advertisement from MAC: %s, RSSI: %d\n", advertisementMac, advertisementRssi);
    // TERMINAL_VIEW_ADD_TEXT("Received BLE Advertisement from MAC: %s, RSSI: %d\n", advertisementMac, advertisementRssi);
    
    // printf("Raw Advertisement Data (len=%zu): ", event->disc.length_data);
    // TERMINAL_VIEW_ADD_TEXT("Raw Advertisement Data (len=%zu): ", event->disc.length_data);
    // for (size_t i = 0; i < event->disc.length_data; i++) {
    //     printf("%02x ", event->disc.data[i]);
    // }
    // printf("\n");
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
            TERMINAL_VIEW_ADD_TEXT("BLE Spam detected! Company ID: 0x%04X\n", current_company_id);
            // pulse rgb purple once when spam is detected
            pulse_once(&rgb_manager, 128, 0, 128);
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
            // pulse rgb blue once when air tag is found
            pulse_once(&rgb_manager, 0, 0, 255);
        
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

            TERMINAL_VIEW_ADD_TEXT("AirTag found!\n");
            TERMINAL_VIEW_ADD_TEXT("Tag: %d\n", airTagCount);
            TERMINAL_VIEW_ADD_TEXT("MAC Address: %s\n", macAddress);
            TERMINAL_VIEW_ADD_TEXT("RSSI: %d dBm\n", rssi);
            TERMINAL_VIEW_ADD_TEXT("\n\n");
        }
    }
}

static bool wait_for_ble_ready(void) {
    int rc;
    int retry_count = 0;
    const int max_retries = 50; // 5 seconds total timeout

    while (!ble_hs_synced() && retry_count < max_retries) {
        vTaskDelay(pdMS_TO_TICKS(100));  // Wait for 100ms
        retry_count++;
    }

    if (retry_count >= max_retries) {
        ESP_LOGE(TAG_BLE, "Timeout waiting for BLE stack sync");
        return false;
    }
    
    uint8_t own_addr_type;
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Failed to set BLE address; rc=%d", rc);
        return false;
    }
    
    return true;
}

void ble_start_scanning(void) {
    if (!ble_initialized) {
        ble_init();
    }
    
    if (!wait_for_ble_ready()) {
        ESP_LOGE(TAG_BLE, "BLE stack not ready");
        TERMINAL_VIEW_ADD_TEXT("BLE stack not ready\n");
        return;
    }
    
    struct ble_gap_disc_params disc_params = {0};
    disc_params.itvl = BLE_HCI_SCAN_ITVL_DEF;
    disc_params.window = BLE_HCI_SCAN_WINDOW_DEF;
    disc_params.filter_duplicates = 1;

    // Start a new BLE scan
    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event_general, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error starting BLE scan; rc=%d", rc);
        TERMINAL_VIEW_ADD_TEXT("Error starting BLE scan; rc=%d\n", rc);
    } else {
        ESP_LOGI(TAG_BLE, "Scanning started...");
        TERMINAL_VIEW_ADD_TEXT("Scanning started...\n");
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
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if (!ble_initialized) {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        if (handlers == NULL) {
            handlers = malloc(sizeof(ble_handler_t) * MAX_HANDLERS);
            if (handlers == NULL) {
                ESP_LOGE(TAG_BLE, "Failed to allocate handlers array");
                return;
            }
            memset(handlers, 0, sizeof(ble_handler_t) * MAX_HANDLERS);
            handler_count = 0;
        }

        ret = nimble_port_init();
        if (ret != 0) {
            ESP_LOGE(TAG_BLE, "Failed to init nimble port: %d", ret);
            free(handlers);
            handlers = NULL;
            return;
        }

        // Configure and start the NimBLE host task
        static StackType_t host_task_stack[4096];
        static StaticTask_t host_task_buf;
        
        xTaskCreateStatic(nimble_host_task, "nimble_host", sizeof(host_task_stack) / sizeof(StackType_t),
                         NULL, 5, host_task_stack, &host_task_buf);

        ble_initialized = true;
        ESP_LOGI(TAG_BLE, "BLE initialized");
        TERMINAL_VIEW_ADD_TEXT("BLE initialized\n");
    }
#endif
}

void ble_start_find_flippers(void)
{
    ble_register_handler(ble_findtheflippers_callback);
    ble_start_scanning();
}

void ble_deinit(void) {
    if (ble_initialized) {
        if (handlers != NULL) {
            free(handlers);
            handlers = NULL;
            handler_count = 0;
        }
        
        nimble_port_stop();
        nimble_port_deinit();
        ble_initialized = false;
        ESP_LOGI(TAG_BLE, "BLE deinitialized successfully.");
        TERMINAL_VIEW_ADD_TEXT("BLE deinitialized successfully.\n");
    }
}

void ble_stop(void) {
    if (last_company_id != NULL) {
        free(last_company_id);
        last_company_id = NULL;
    }

    // Stop and delete the flush timer if it exists
    if (flush_timer != NULL) {
        esp_timer_stop(flush_timer);
        esp_timer_delete(flush_timer);
        flush_timer = NULL;
    }
    
    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
    ble_unregister_handler(ble_findtheflippers_callback);
    ble_unregister_handler(airtag_scanner_callback);
    ble_unregister_handler(ble_print_raw_packet_callback);
    ble_unregister_handler(detect_ble_spam_callback);
    pcap_flush_buffer_to_file(); // Final flush
    pcap_file_close(); // Close the file after final flush
    
    int rc = ble_gap_disc_cancel();

    if (rc == 0) {
        printf("BLE scanning stopped successfully.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE scanning stopped successfully.\n");
    } else if (rc == BLE_HS_EALREADY) {
        printf("BLE scanning was not active.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE scanning was not active.\n");
    } else {
        printf("Failed to stop BLE scanning; rc=%d\n", rc);
        TERMINAL_VIEW_ADD_TEXT("Failed to stop BLE scanning; rc=%d\n", rc);
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

static void ble_pcap_callback(struct ble_gap_event *event, size_t len) {
    if (!event || len == 0) return;
    
    uint8_t hci_buffer[258];  // Max HCI packet size
    size_t hci_len = 0;
    
    if (event->type == BLE_GAP_EVENT_DISC) {
        // [1] HCI packet type (0x04 for HCI Event)
        hci_buffer[0] = 0x04;
        
        // [2] HCI Event Code (0x3E for LE Meta Event)
        hci_buffer[1] = 0x3E;
        
        // [3] Calculate total parameter length
        uint8_t param_len = 10 + event->disc.length_data;  // 1 (subevent) + 1 (num reports) + 1 (event type) + 1 (addr type) + 6 (addr)
        hci_buffer[2] = param_len;
        
        // [4] LE Meta Subevent (0x02 for LE Advertising Report)
        hci_buffer[3] = 0x02;
        
        // [5] Number of reports
        hci_buffer[4] = 0x01;
        
        // [6] Event type (ADV_IND = 0x00)
        hci_buffer[5] = 0x00;
        
        // [7] Address type
        hci_buffer[6] = event->disc.addr.type;
        
        // [8] Address (6 bytes)
        memcpy(&hci_buffer[7], event->disc.addr.val, 6);
        
        // [9] Data length
        hci_buffer[13] = event->disc.length_data;
        
        // [10] Data
        if (event->disc.length_data > 0) {
            memcpy(&hci_buffer[14], event->disc.data, event->disc.length_data);
        }
        
        // [11] RSSI
        hci_buffer[14 + event->disc.length_data] = (uint8_t)event->disc.rssi;
        
        hci_len = 15 + event->disc.length_data;  // Total length
        
        // Debug logging
        ESP_LOGI("BLE_PCAP", "HCI Event: type=0x04, meta=0x3E, len=%d", hci_len);
        printf("Packet: ");
        for (int i = 0; i < hci_len; i++) {
            printf("%02x ", hci_buffer[i]);
        }
        printf("\n");
        
        pcap_write_packet_to_buffer(hci_buffer, hci_len, PCAP_CAPTURE_BLUETOOTH);
    }
}

void ble_start_capture(void) {
    // Open PCAP file first
    esp_err_t err = pcap_file_open("ble_capture", PCAP_CAPTURE_BLUETOOTH);
    if (err != ESP_OK) {
        ESP_LOGE("BLE_PCAP", "Failed to open PCAP file: %d", err);
        return;
    }
    
    // Register BLE handler only after file is open
    ble_register_handler(ble_pcap_callback);
    
    // Create a timer to flush the buffer periodically
    esp_timer_create_args_t timer_args = {
        .callback = (esp_timer_cb_t)pcap_flush_buffer_to_file,
        .name = "pcap_flush"
    };
    
    if (esp_timer_create(&timer_args, &flush_timer) == ESP_OK) {
        esp_timer_start_periodic(flush_timer, 1000000); // Flush every second
    }
    
    ble_start_scanning();
}

void ble_start_skimmer_detection(void) {
    ESP_LOGI("BLE", "Starting skimmer detection scan...");
    TERMINAL_VIEW_ADD_TEXT("Starting skimmer detection scan...\n");
    
    // Register the skimmer detection callback
    esp_err_t err = ble_register_handler(ble_skimmer_scan_callback);
    if (err != ESP_OK) {
        ESP_LOGE("BLE", "Failed to register skimmer detection callback");
        return;
    }
    
    // Start BLE scanning
    ble_start_scanning();
}

#endif