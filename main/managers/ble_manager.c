#include "esp_log.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "core/callbacks.h"
#include "esp_random.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "managers/ble_manager.h"
#include "managers/views/terminal_screen.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "vendor/pcap.h"
#include <esp_mac.h>
#include <managers/rgb_manager.h>
#include <managers/settings_manager.h>

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
static uint16_t conn_handle = 0xFFFF;

static const watch_model_t watch_models[] = {
{0x1A, "Fallback Watch"},
{0x01, "White Watch4 Classic 44m"},
{0x02, "Black Watch4 Classic 40m"},
{0x03, "White Watch4 Classic 40m"},
{0x04, "Black Watch4 44mm"},
{0x05, "Silver Watch4 44mm"},
{0x06, "Green Watch4 44mm"},
{0x07, "Black Watch4 40mm"},
{0x08, "White Watch4 40mm"},
{0x09, "Gold Watch4 40mm"},
{0x0A, "French Watch4"},
{0x0B, "French Watch4 Classic"},
{0x0C, "Fox Watch5 44mm"},
{0x11, "Black Watch5 44mm"},
{0x12, "Sapphire Watch5 44mm"},
{0x13, "Purpleish Watch5 40mm"},
{0x14, "Gold Watch5 40mm"},
{0x15, "Black Watch5 Pro 45mm"},
{0x16, "Gray Watch5 Pro 45mm"},
{0x17, "White Watch5 44mm"},
{0x18, "White & Black Watch5"},
{0xE4, "Black Watch5 Golf Edition"},
{0xE5, "White Watch5 Gold Edition"},
{0x1B, "Black Watch6 Pink 40mm"},
{0x1C, "Gold Watch6 Gold 40mm"},
{0x1D, "Silver Watch6 Cyan 44mm"},
{0x1E, "Black Watch6 Classic 43m"},
{0x20, "Green Watch6 Classic 43m"},
{0xEC, "Black Watch6 Golf Edition"},
{0xEF, "Black Watch6 TB Edition"}
};
  
static const earbuds_model_t earbuds_models[] = {
{0xEE7A0C, "Fallback Buds"},
{0x9D1700, "Fallback Dots"},
{0x39EA48, "Light Purple Buds2"},
{0xA7C62C, "Bluish Silver Buds2"},
{0x850116, "Black Buds Live"},
{0x3D8F41, "Gray & Black Buds2"},
{0x3B6D02, "Bluish Chrome Buds2"},
{0xAE063C, "Gray Beige Buds2"},
{0xB8B905, "Pure White Buds"},
{0xEAAA17, "Pure White Buds2"},
{0xD30704, "Black Buds"},
{0x9DB006, "French Flag Buds"},
{0x101F1A, "Dark Purple Buds Live"},
{0x859608, "Dark Blue Buds"},
{0x8E4503, "Pink Buds"},
{0x2C6740, "White & Black Buds2"},
{0x3F6718, "Bronze Buds Live"},
{0x42C519, "Red Buds Live"},
{0xAE073A, "Black & White Buds2"},
{0x011716, "Sleek Black Buds2"}
};

static const apple_action_t apple_actions[] = {
{0x13, "AppleTV AutoFill"},
{0x24, "Apple Vision Pro"},
{0x05, "Apple Watch"},
{0x27, "AppleTV Connecting..."},
{0x20, "Join This AppleTV?"},
{0x19, "AppleTV Audio Sync"},
{0x1E, "AppleTV Color Balance"},
{0x09, "Setup New iPhone"},
{0x2F, "Sign in to other device"},
{0x02, "Transfer Phone Number"},
{0x0B, "HomePod Setup"},
{0x01, "Setup New AppleTV"},
{0x06, "Pair AppleTV"},
{0x0D, "HomeKit AppleTV Setup"},
{0x2B, "AppleID for AppleTV?"},
};
  
#define WATCH_MODEL_COUNT (sizeof(watch_models) / sizeof(watch_models[0]))
#define EARBUDS_MODEL_COUNT (sizeof(earbuds_models) / sizeof(earbuds_models[0]))
#define APPLE_ACTION_COUNT (sizeof(apple_actions) / sizeof(apple_actions[0]))

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

static int8_t generate_random_rssi() { return (esp_random() % 121) - 100; }

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
        ESP_LOGE(TAG_BLE, "Error stopping advertisement");
    }

    rc = nimble_port_stop();
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error stopping NimBLE port");
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

    ble_unregister_handler(ble_skimmer_scan_callback);
    pcap_flush_buffer_to_file();
    pcap_file_close();

    int rc = ble_gap_disc_cancel();

    if (rc == 0) {
        printf("BLE skimmer detection stopped successfully.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE skimmer detection stopped successfully.\n");
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

        if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS16 || type == BLE_HS_ADV_TYPE_INCOMP_UUIDS16) &&
            uuids->uuid16_count < MAX_UUID16) {
            for (int i = 0; i < length - 1; i += 2) {
                uint16_t uuid16 = data[index + 2 + i] | (data[index + 3 + i] << 8);
                uuids->uuid16[uuids->uuid16_count++] = uuid16;
            }
        }

        else if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS32 || type == BLE_HS_ADV_TYPE_INCOMP_UUIDS32) &&
                 uuids->uuid32_count < MAX_UUID32) {
            for (int i = 0; i < length - 1; i += 4) {
                uint32_t uuid32 = data[index + 2 + i] | (data[index + 3 + i] << 8) |
                                  (data[index + 4 + i] << 16) | (data[index + 5 + i] << 24);
                uuids->uuid32[uuids->uuid32_count++] = uuid32;
            }
        }

        else if ((type == BLE_HS_ADV_TYPE_COMP_UUIDS128 ||
                  type == BLE_HS_ADV_TYPE_INCOMP_UUIDS128) &&
                 uuids->uuid128_count < MAX_UUID128) {
            snprintf(uuids->uuid128[uuids->uuid128_count],
                     sizeof(uuids->uuid128[uuids->uuid128_count]),
                     "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%"
                     "02x%02x",
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
    parse_device_name(event->disc.data, event->disc.length_data, advertisementName,
                      sizeof(advertisementName));

    ble_service_uuids_t uuids = {0};
    parse_service_uuids(event->disc.data, event->disc.length_data, &uuids);

    for (int i = 0; i < uuids.uuid16_count; i++) {
        if (uuids.uuid16[i] == 0x3082) {
            printf("Found White Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid16[i] == 0x3081) {
            printf("Found Black Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid16[i] == 0x3083) {
            printf("Found Transparent Flipper Device: \nMAC: %s, \nName: %s, \nRSSI: "
                   "%d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
    }
    for (int i = 0; i < uuids.uuid32_count; i++) {
        if (uuids.uuid32[i] == 0x3082) {
            printf("Found White Flipper Device (32-bit UUID): \nMAC: %s, \nName: %s, "
                   "\nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid32[i] == 0x3081) {
            printf("Found Black Flipper Device (32-bit UUID): \nMAC: %s, \nName: %s, "
                   "\nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (uuids.uuid32[i] == 0x3083) {
            printf("Found Transparent Flipper Device (32-bit UUID): \nMAC: %s, "
                   "\nName: %s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
    }
    for (int i = 0; i < uuids.uuid128_count; i++) {
        if (strstr(uuids.uuid128[i], "3082") != NULL) {
            printf("Found White Flipper Device (128-bit UUID): \nMAC: %s, \nName: "
                   "%s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found White Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (strstr(uuids.uuid128[i], "3081") != NULL) {
            printf("Found Black Flipper Device (128-bit UUID): \nMAC: %s, \nName: "
                   "%s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Black Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
                                   advertisementMac, advertisementName, advertisementRssi);
            pulse_once(&rgb_manager, 255, 165, 0);
        }
        if (strstr(uuids.uuid128[i], "3083") != NULL) {
            printf("Found Transparent Flipper Device (128-bit UUID): \nMAC: %s, "
                   "\nName: %s, \nRSSI: %d\n",
                   advertisementMac, advertisementName, advertisementRssi);
            TERMINAL_VIEW_ADD_TEXT("Found Transparent Flipper Device (128-bit UUID): "
                                   "\nMAC: %s, \nName: %s, \nRSSI: %d\n",
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

    const uint8_t* data = event->disc.data;
    uint16_t data_len = event->disc.length_data;

    if (data_len == 0) {
        return;
    }

    static char last_mac[18] = {0};
    static uint8_t last_data[256] = {0};
    static uint16_t last_data_len = 0;
    static uint32_t last_print_time = 0;
    
    uint32_t current_time = esp_timer_get_time() / 1000;

    bool is_duplicate = (strncmp(last_mac, advertisementMac, 18) == 0 && 
                        data_len == last_data_len && 
                        memcmp(last_data, data, data_len) == 0);
    
    if (is_duplicate && (current_time - last_print_time) < 1000) {
        return;
    }

    // Update tracking variables
    strncpy(last_mac, advertisementMac, 18);
    memcpy(last_data, data, data_len);
    last_data_len = data_len;
    last_print_time = current_time;

    if (event->type == BLE_GAP_EVENT_DISC) {
        // Print packet info
        printf("BLE Device Detected - MAC: %s, RSSI: %d dBm\n", advertisementMac, advertisementRssi);
        TERMINAL_VIEW_ADD_TEXT("BLE Device Detected - MAC: %s, RSSI: %d dBm\n", advertisementMac, advertisementRssi);

        if (data_len >= 2 && data[0] == 0x75 && data[1] == 0x00) {
            printf("Samsung device detected\n");
            TERMINAL_VIEW_ADD_TEXT("Samsung device detected\n");

            if (data_len >= 13 && data[2] == 0x01 && data[3] == 0x00) {
                uint8_t model_id = data[12];
                char* model_name = "Unknown Watch";
                for (size_t i = 0; i < sizeof(watch_models) / sizeof(watch_models[0]); i++) {
                    if (watch_models[i].id == model_id) {
                        model_name = watch_models[i].description;
                        break;
                    }
                }
                printf("Detected Watch Model: %s (ID: 0x%02X)\n", model_name, model_id);
                TERMINAL_VIEW_ADD_TEXT("Detected Watch Model: %s (ID: 0x%02X)\n", model_name, model_id);
            }
            else if (data_len >= 16 && data[2] == 0x42 && data[3] == 0x09) {
                uint32_t model_id = ((uint32_t)data[12] << 16) | 
                                ((uint32_t)data[13] << 8) | 
                                data[15];
                
                char* model_name = "Unknown Earbuds";
                for (size_t i = 0; i < sizeof(earbuds_models) / sizeof(earbuds_models[0]); i++) {
                    if (earbuds_models[i].id == model_id) {
                        model_name = earbuds_models[i].description;
                        break;
                    }
                }
                printf("Detected Earbuds Model: %s (ID: 0x%06lX)\n", model_name, (unsigned long)model_id);
                TERMINAL_VIEW_ADD_TEXT("Detected Earbuds Model: %s (ID: 0x%06lX)\n", model_name, (unsigned long)model_id);
            }
            else {
                printf("Unknown Samsung device type\n");
                TERMINAL_VIEW_ADD_TEXT("Unknown Samsung device type\n");
            }
        }
        else {
            printf("Non-Samsung device\n");
            TERMINAL_VIEW_ADD_TEXT("Non-Samsung device\n");
        }

        printf("Raw packet data (%d bytes):\n", data_len);
        TERMINAL_VIEW_ADD_TEXT("Raw packet data (%d bytes):\n", data_len);
        
        char hex_buffer[256] = {0};
        size_t offset = 0;
        for (uint16_t i = 0; i < data_len && offset < sizeof(hex_buffer) - 4; i++) {
            offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "%02X ", data[i]);
        }
        printf("%s\n", hex_buffer);
        TERMINAL_VIEW_ADD_TEXT("%s\n", hex_buffer);
        
        printf("\n");
        TERMINAL_VIEW_ADD_TEXT("\n");
    }
    else if (event->type == BLE_GAP_EVENT_CONNECT) {
        printf("Connection detected, handle: %d\n", event->connect.conn_handle);
        conn_handle = event->connect.conn_handle;
    }
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
            if ((payload[i] == 0x1E && payload[i + 1] == 0xFF && payload[i + 2] == 0x4C &&
                 payload[i + 3] == 0x00) ||
                (payload[i] == 0x4C && payload[i + 1] == 0x00 && payload[i + 2] == 0x12 &&
                 payload[i + 3] == 0x19)) {
                patternFound = true;
                break;
            }
        }

        if (patternFound) {
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
    const int max_retries = 50;

    while (!ble_hs_synced() && retry_count < max_retries) {
        vTaskDelay(pdMS_TO_TICKS(100));
        retry_count++;
    }

    if (retry_count >= max_retries) {
        ESP_LOGE(TAG_BLE, "Timeout waiting for BLE stack sync");
        return false;
    }

    uint8_t own_addr_type;
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Failed to set BLE address");
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

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event_general,
                          NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Error starting BLE scan");
        TERMINAL_VIEW_ADD_TEXT("Error starting BLE scan\n");
    } else {
        ESP_LOGI(TAG_BLE, "Scanning started...");
        TERMINAL_VIEW_ADD_TEXT("Scanning started...\n");
    }
}

esp_err_t ble_register_handler(ble_data_handler_t handler) {
    if (handler_count < MAX_HANDLERS) {
        ble_handler_t *new_handlers =
            realloc(handlers, (handler_count + 1) * sizeof(ble_handler_t));
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

        static StackType_t host_task_stack[4096];
        static StaticTask_t host_task_buf;

        xTaskCreateStatic(nimble_host_task, "nimble_host",
                          sizeof(host_task_stack) / sizeof(StackType_t), NULL, 5, host_task_stack,
                          &host_task_buf);

        ble_initialized = true;
        ESP_LOGI(TAG_BLE, "BLE initialized");
        TERMINAL_VIEW_ADD_TEXT("BLE initialized\n");
    }
#endif
}

void ble_start_find_flippers(void) {
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
    if (!ble_initialized) {
        return;
    }

    if (!ble_gap_disc_active()) {
        return;
    }

    if (last_company_id != NULL) {
        free(last_company_id);
        last_company_id = NULL;
    }

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
    pcap_flush_buffer_to_file();
    pcap_file_close();

    int rc = ble_gap_disc_cancel();

    switch (rc) {
    case 0:
        printf("BLE scan stopped successfully.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE scan stopped successfully.\n");
        break;
    case BLE_HS_EBUSY:
        printf("BLE scan is busy\n");
        TERMINAL_VIEW_ADD_TEXT("BLE scan is busy\n");
        break;
    case BLE_HS_ETIMEOUT:
        printf("BLE operation timed out.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE operation timed out.\n");
        break;
    case BLE_HS_ENOTCONN:
        printf("BLE not connected.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE not connected.\n");
        break;
    case BLE_HS_EINVAL:
        printf("BLE invalid parameter.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE invalid parameter.\n");
        break;
    default:
        printf("Error stopping BLE scan: %d\n", rc);
        TERMINAL_VIEW_ADD_TEXT("Error stopping BLE scan: %d\n", rc);
    }
}

void ble_start_blespam_detector(void) {
    ble_register_handler(detect_ble_spam_callback);
    ble_start_scanning();
}

void ble_start_raw_ble_packetscan(void) {
    ble_register_handler(ble_print_raw_packet_callback);
    ble_start_scanning();
}

void ble_start_airtag_scanner(void) {
    ble_register_handler(airtag_scanner_callback);
    ble_start_scanning();
}

static void ble_pcap_callback(struct ble_gap_event *event, size_t len) {
    if (!event || len == 0)
        return;

    uint8_t hci_buffer[258];
    size_t hci_len = 0;

    if (event->type == BLE_GAP_EVENT_DISC) {
        hci_buffer[0] = 0x04;
        hci_buffer[1] = 0x3E;
        uint8_t param_len = 10 + event->disc.length_data;
        hci_buffer[2] = param_len;
        hci_buffer[3] = 0x02;
        hci_buffer[4] = 0x01;
        hci_buffer[5] = 0x00;
        hci_buffer[6] = event->disc.addr.type;
        memcpy(&hci_buffer[7], event->disc.addr.val, 6);
        hci_buffer[13] = event->disc.length_data;
        if (event->disc.length_data > 0) {
            memcpy(&hci_buffer[14], event->disc.data, event->disc.length_data);
        }
        hci_buffer[14 + event->disc.length_data] = (uint8_t)event->disc.rssi;
        hci_len = 15 + event->disc.length_data;

        printf("BLE Packet Received:\nType: 0x04 (HCI Event)\nMeta: 0x3E "
               "(LE)\nLength: %d\n",
               hci_len);

        pcap_write_packet_to_buffer(hci_buffer, hci_len, PCAP_CAPTURE_BLUETOOTH);
    }
}

void ble_start_capture(void) {
    esp_err_t err = pcap_file_open("ble_capture", PCAP_CAPTURE_BLUETOOTH);
    if (err != ESP_OK) {
        ESP_LOGE("BLE_PCAP", "Failed to open PCAP file");
        return;
    }

    ble_register_handler(ble_pcap_callback);

    esp_timer_create_args_t timer_args = {.callback = (esp_timer_cb_t)pcap_flush_buffer_to_file,
                                          .name = "pcap_flush"};

    if (esp_timer_create(&timer_args, &flush_timer) == ESP_OK) {
        esp_timer_start_periodic(flush_timer, 1000000);
    }

    ble_start_scanning();
}

static void set_random_address(void) {
    uint8_t rand_addr[6];
    esp_fill_random(rand_addr, 6);
    rand_addr[0] |= 0xC0;

    int rc = ble_hs_id_set_rnd(rand_addr);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Failed to set random address: %d", rc);
        return;
    }

    ESP_LOGI(TAG_BLE, "Set random MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             rand_addr[0], rand_addr[1], rand_addr[2], rand_addr[3], rand_addr[4], rand_addr[5]);
}

static void build_watch_packet(uint8_t *packet, size_t *len) {
    int i = 0;
    uint8_t model = watch_models[esp_random() % WATCH_MODEL_COUNT].id;


    packet[i++] = 0x75; // Company ID (Samsung Electronics Co. Ltd.)
    packet[i++] = 0x00;
    packet[i++] = 0x01;
    packet[i++] = 0x00;
    packet[i++] = 0x02;
    packet[i++] = 0x00;
    packet[i++] = 0x01;
    packet[i++] = 0x01;
    packet[i++] = 0xFF;
    packet[i++] = 0x00;
    packet[i++] = 0x00;
    packet[i++] = 0x43;
    packet[i++] = model; // Random watch model

    *len = i; // Should be 13 bytes
    ESP_LOGI(TAG_BLE, "Watch packet built (%d bytes):", *len);
    for (size_t j = 0; j < *len; j++) {
        ESP_LOGI(TAG_BLE, "  [%d] = 0x%02X", j, packet[j]);
    }
}

static void build_earbuds_packet(uint8_t *adv_packet, size_t *adv_len, uint8_t *scan_packet, size_t *scan_len) {
    int i = 0;
    uint32_t model = earbuds_models[esp_random() % EARBUDS_MODEL_COUNT].id;

    adv_packet[i++] = 0x75; // Company ID (Samsung Electronics Co. Ltd.)
    adv_packet[i++] = 0x00;
    adv_packet[i++] = 0x42;
    adv_packet[i++] = 0x09;
    adv_packet[i++] = 0x81;
    adv_packet[i++] = 0x02;
    adv_packet[i++] = 0x14;
    adv_packet[i++] = 0x15;
    adv_packet[i++] = 0x03;
    adv_packet[i++] = 0x21;
    adv_packet[i++] = 0x01;
    adv_packet[i++] = 0x09;
    adv_packet[i++] = (model >> 16) & 0xFF; // Model bytes
    adv_packet[i++] = (model >> 8) & 0xFF;
    adv_packet[i++] = 0x01; // Static byte
    adv_packet[i++] = (model >> 0) & 0xFF;
    adv_packet[i++] = 0x06;
    adv_packet[i++] = 0x3C;
    adv_packet[i++] = 0x94;
    adv_packet[i++] = 0x8E;
    adv_packet[i++] = 0x00;
    adv_packet[i++] = 0x00;
    adv_packet[i++] = 0x00;
    adv_packet[i++] = 0x00;
    adv_packet[i++] = 0xC7;
    adv_packet[i++] = 0x00;

    *adv_len = i; // Should be 26 bytes

    i = 0;
    scan_packet[i++] = 0x75; // Company ID (Samsung Electronics Co. Ltd.)
    scan_packet[i++] = 0x00;
    scan_packet[i++] = 0x00;
    scan_packet[i++] = 0x63;
    scan_packet[i++] = 0x50;
    scan_packet[i++] = 0x8D;
    scan_packet[i++] = 0xB1;
    scan_packet[i++] = 0x17;
    scan_packet[i++] = 0x40;
    scan_packet[i++] = 0x46;
    scan_packet[i++] = 0x64;
    scan_packet[i++] = 0x64;
    scan_packet[i++] = 0x00;
    scan_packet[i++] = 0x01;
    scan_packet[i++] = 0x04;

    *scan_len = i; // Should be 15 bytes

    ESP_LOGI(TAG_BLE, "Earbuds adv packet built (%d bytes):", *adv_len);
    for (size_t j = 0; j < *adv_len; j++) {
        ESP_LOGI(TAG_BLE, "  [%d] = 0x%02X", j, adv_packet[j]);
    }
    ESP_LOGI(TAG_BLE, "Earbuds scan rsp packet built (%d bytes):", *scan_len);
    for (size_t j = 0; j < *scan_len; j++) {
        ESP_LOGI(TAG_BLE, "  [%d] = 0x%02X", j, scan_packet[j]);
    }
}

static void build_apple_packet(uint8_t *adv_packet, size_t *adv_len) {
    int i = 0;

    memset(adv_packet, 0, MAX_PACKET_SIZE);

    adv_packet[i++] = 0x4C;          // Company ID (Apple) - Little Endian
    adv_packet[i++] = 0x00;          // Company ID (Apple)
    adv_packet[i++] = 0x07;          // Continuity Type (Nearby Action)
    
    uint8_t action = apple_actions[esp_random() % APPLE_ACTION_COUNT].value;

    uint8_t flags = 0xC0;
    if (action == 0x20 && (esp_random() % 2)) flags--;
    if (action == 0x09 && (esp_random() % 2)) flags = 0x40;

    adv_packet[i++] = flags;         // Action Flags
    adv_packet[i++] = action;        // Action Type
    
    uint8_t tag[3];
    esp_fill_random(tag, 3);
    adv_packet[i++] = tag[0];
    adv_packet[i++] = tag[1];
    adv_packet[i++] = tag[2];

    *adv_len = i;

    // Verify packet length
    if (*adv_len != 8) {
        ESP_LOGE(TAG_BLE, "Invalid packet length: %d bytes (expected 8)", *adv_len);
    }

    // Logging
    ESP_LOGI(TAG_BLE, "Apple packet built (%d bytes):", *adv_len);
    for (size_t j = 0; j < *adv_len; j++) {
        ESP_LOGI(TAG_BLE, "  [%d] = 0x%02X", j, adv_packet[j]);
    }
}


static void start_advertising(ble_advertisement_type_t type) {
    struct ble_hs_adv_fields adv_fields;
    struct ble_hs_adv_fields rsp_fields;
    memset(&adv_fields, 0, sizeof(adv_fields));
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    uint8_t adv_packet[MAX_PACKET_SIZE];
    uint8_t scan_packet[MAX_PACKET_SIZE];
    size_t adv_len = 0;
    size_t scan_len = 0;

    switch (type) {
        case BLE_ADV_TYPE_WATCH:
            build_watch_packet(adv_packet, &adv_len);
            adv_fields.mfg_data = adv_packet;
            adv_fields.mfg_data_len = adv_len; // 13 bytes
            adv_fields.flags = BLE_HS_ADV_F_DISC_GEN; // Ensure flags for Watch
            ESP_LOGI(TAG_BLE, "Advertising as Watch: %s", watch_models[adv_packet[12]].description);
            break;

        case BLE_ADV_TYPE_EARBUDS:
            build_earbuds_packet(adv_packet, &adv_len, scan_packet, &scan_len);
            adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP; // 02 01 18
            adv_fields.mfg_data = adv_packet;
            adv_fields.mfg_data_len = adv_len; // 26 bytes
            rsp_fields.mfg_data = scan_packet;
            rsp_fields.mfg_data_len = scan_len; // 15 bytes
            ESP_LOGI(TAG_BLE, "Advertising as Earbuds: %s", earbuds_models[(adv_packet[12] << 16) | (adv_packet[13] << 8) | adv_packet[15]].description);
            break;
            case BLE_ADV_TYPE_APPLE: {
                build_apple_packet(adv_packet, &adv_len);
    
                uint8_t mfg_data[7];
                memcpy(mfg_data, adv_packet, 7);
    
                adv_fields.mfg_data = mfg_data;
                adv_fields.mfg_data_len = 7;
                adv_fields.flags = 0x18;
                ESP_LOGI(TAG_BLE, "Advertising as Apple Nearby Action: %s", apple_actions[mfg_data[4]].name);
                break;
            }

        case BLE_ADV_TYPE_DEFAULT:
        default:
            adv_packet[0] = 0xFF; // Dummy company ID
            adv_fields.mfg_data = adv_packet;
            adv_fields.mfg_data_len = 1;
            adv_fields.flags = BLE_HS_ADV_F_DISC_GEN;
            ESP_LOGI(TAG_BLE, "Advertising as Default");
            break;
    }

    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Failed to set advertising fields: %d", rc);
        return;
    }

    if (type == BLE_ADV_TYPE_EARBUDS) {
        rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
        if (rc != 0) {
            ESP_LOGE(TAG_BLE, "Failed to set scan response fields: %d", rc);
            return;
        }
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_NON,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN, // ~20ms
        .itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX,
    };
    rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Failed to start advertising: %d", rc);
    }
}

static void random_advertising_task(void *param) {
    ble_advertisement_type_t type = *(ble_advertisement_type_t *)param;

    if (!ble_initialized) {
        ble_init();
    }

    if (!wait_for_ble_ready()) {
        ESP_LOGE(TAG_BLE, "BLE stack not ready for advertising");
        TERMINAL_VIEW_ADD_TEXT("BLE stack not ready for advertising\n");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        ble_gap_adv_stop();
        set_random_address();
        start_advertising(type);
        vTaskDelay(pdMS_TO_TICKS(100)); // Cycle every 50ms
    }
}

void ble_start_random_advertising(ble_advertisement_type_t type) {
    static StackType_t adv_task_stack[4096];
    static StaticTask_t adv_task_buf;
    static ble_advertisement_type_t adv_type;

    adv_type = type;
    xTaskCreateStatic(random_advertising_task, "ble_adv_task",
                      sizeof(adv_task_stack) / sizeof(StackType_t), &adv_type, 5, adv_task_stack,
                      &adv_task_buf);
    ESP_LOGI(TAG_BLE, "Random advertising started with type: %d", type);
    TERMINAL_VIEW_ADD_TEXT("Random advertising started with type: %d\n", type);
}

void ble_start_skimmer_detection(void) {
    esp_err_t err = ble_register_handler(ble_skimmer_scan_callback);
    if (err != ESP_OK) {
        ESP_LOGE("BLE", "Failed to register skimmer detection callback");
        return;
    }

    ble_start_scanning();
}

#endif