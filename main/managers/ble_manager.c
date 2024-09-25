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

static uint8_t get_random_action_type() {
    const uint8_t types[] = { 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24 }; // TODO ADD MORE TYPES
    return types[esp_random() % (sizeof(types) / sizeof(types[0]))];
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

void generate_apple_advertisement(uint8_t *packet, size_t *packet_len) {
    size_t i = 0;
    
    
    if (*packet_len < 17) {
        ESP_LOGE("BLE", "Buffer too small for Apple advertisement");
        return;
    }

    
    packet[i++] = 0x4C;    // Apple company ID (first byte)
    packet[i++] = 0x00;    // Apple company ID (second byte)
    packet[i++] = 0x0F;    // Type of Apple data
    packet[i++] = 0x05;    // Some Apple-specific field
    packet[i++] = 0xC1;    // Another Apple-specific field

    packet[i++] = get_random_action_type();  // Dynamic action type

    // Fill random data (3 bytes)
    esp_fill_random(&packet[i], 3);
    i += 3;

    packet[i++] = 0x00;
    packet[i++] = 0x00;

    packet[i++] = 0x10;    // Static value

    // Fill random data (3 bytes)
    esp_fill_random(&packet[i], 3);
    i += 3;

    *packet_len = i;       // Update packet length
}


void generate_google_advertisement(uint8_t *packet, size_t *packet_len) {
    size_t i = 0;

    
    if (*packet_len < 12) {
        ESP_LOGE("BLE", "Buffer too small for Google advertisement");
        return;
    }

    packet[i++] = 3;       // Length of the AD Type data
    packet[i++] = 0x03;    // Complete List of 16-bit Service Class UUIDs
    packet[i++] = 0x2C;    // Google UUID (part 1)
    packet[i++] = 0xFE;    // Google UUID (part 2)

    packet[i++] = 6;       // Length of the service data
    packet[i++] = 0x16;    // Service Data AD Type
    packet[i++] = 0x2C;    // Google UUID (part 1)
    packet[i++] = 0xFE;    // Google UUID (part 2)
    packet[i++] = 0x00;    // Custom data (static)
    packet[i++] = 0xB7;    // Custom data (static)
    packet[i++] = 0x27;    // Custom data (static)

    packet[i++] = 2;       // Length of TX power level data
    packet[i++] = 0x0A;    // TX power level AD Type
    packet[i++] = generate_random_rssi();  // Dynamic RSSI value

    *packet_len = i;       // Update packet length
}

void generate_samsung_advertisement(uint8_t *packet, size_t *packet_len, bool use_watch_model) {
    size_t i = 0;

   
    if (use_watch_model) {
        if (*packet_len < 15) {
            ESP_LOGE("BLE", "Buffer too small for Samsung watch advertisement");
            return;
        }


        packet[i++] = 0x75;  // Samsung company ID (first byte)
        packet[i++] = 0x00;  // Samsung company ID (second byte)

        // Custom data
        packet[i++] = 0x01;  // Custom data
        packet[i++] = 0x00;  // Custom data
        packet[i++] = 0x02;  // Custom data
        packet[i++] = 0x00;  // Custom data
        packet[i++] = 0x01;  // Custom data
        packet[i++] = 0x01;  // Custom data
        packet[i++] = 0xFF;  // Custom data
        packet[i++] = 0x00;  // Custom data
        packet[i++] = 0x00;  // Custom data
        packet[i++] = 0x43;  // Custom data

       
        uint8_t random_byte = esp_random() & 0xFF;
        packet[i++] = random_byte;  // Random byte

     
        *packet_len = i;

        ESP_LOGI("BLE", "Samsung advertisement packet length (watch model): %d bytes", *packet_len);
    } else {
        uint8_t advertisementPacket[] = {
            0x02, 0x01, 0x18, 0x1B, 0xFF, 0x75, 0x00, 0x42, 0x09, 0x81, 0x02, 0x14,
            0x15, 0x03, 0x21, 0x01, 0x09, 0xEF, 0x0C, 0x01, 0x47, 0x06, 0x3C, 0x94,
            0x8E, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x00
        };

        size_t default_packet_len = sizeof(advertisementPacket);

        if (*packet_len < default_packet_len) {
            ESP_LOGE("BLE", "Buffer too small for Samsung default advertisement");
            return;
        }

        memcpy(packet, advertisementPacket, default_packet_len);

        *packet_len = default_packet_len;

        ESP_LOGI("BLE", "Samsung advertisement packet length (default model): %d bytes", *packet_len);
    }
}

void generate_microsoft_advertisement(uint8_t *packet, size_t *packet_len) {
    char random_name[15];
    generate_random_name(random_name, sizeof(random_name));

    uint8_t name_len = strlen(random_name);
    size_t i = 0;

    // Ensure the packet has enough space
    if (*packet_len < (7 + name_len)) {
        ESP_LOGE("BLE", "Buffer too small for Microsoft advertisement");
        return;
    }


    packet[i++] = 0x06;             // Microsoft company ID (first byte)
    packet[i++] = 0x00;             // Microsoft company ID (second byte)
    packet[i++] = 0x03;             // Custom data
    packet[i++] = 0x00;             // Custom data
    packet[i++] = 0x80;             // Custom data

    memcpy(&packet[i], random_name, name_len);  // Dynamic random name
    i += name_len;

    *packet_len = i;
}

void generate_advertisement_packet(company_type_t company, uint8_t **packet, size_t *packet_len) {

    *packet_len = 31;
    *packet = (uint8_t *)malloc(*packet_len);

    if (*packet == NULL) {
        ESP_LOGE(TAG_BLE, "Failed to allocate memory for advertisement packet");
        *packet_len = 0;
        return;
    }


    switch (company) {
        case COMPANY_APPLE:
            generate_apple_advertisement(*packet, packet_len);
            break;
        case COMPANY_GOOGLE:
            generate_google_advertisement(*packet, packet_len);
            break;
        case COMPANY_SAMSUNG:
            generate_samsung_advertisement(*packet, packet_len, true);
            break;
        case COMPANY_MICROSOFT:
            generate_microsoft_advertisement(*packet, packet_len);
            break;
        default:
            ESP_LOGE(TAG_BLE, "Unknown company type");
            free(*packet);
            *packet = NULL;
            *packet_len = 0;
            return;
    }

    
    *packet = (uint8_t *)realloc(*packet, *packet_len);
    if (*packet == NULL) {
        ESP_LOGE(TAG_BLE, "Failed to reallocate memory for advertisement packet");
        *packet_len = 0;
    }
}

void send_ble_advertisement(company_type_t company) {
    uint8_t *adv_data = NULL;
    size_t adv_len = 0;

    uint8_t random_mac[6];
    generate_random_mac(random_mac);


    generate_advertisement_packet(company, &adv_data, &adv_len);

    if (adv_data != NULL) 
    {
        struct ble_gap_adv_params adv_params = {0};
        adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;  // Non-connectable advertisement
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;  // General discoverable mode

        struct ble_hs_adv_fields fields = {0};
        fields.mfg_data = adv_data;
        fields.mfg_data_len = adv_len;


        int rc = ble_gap_adv_set_fields(&fields);
        if (rc != 0) {
            ESP_LOGE(TAG_BLE, "Error setting advertisement fields; rc=%d", rc);
            return;
        }


        rc = ble_gap_adv_start(BLE_OWN_ADDR_RPA_PUBLIC_DEFAULT, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG_BLE, "Error starting advertisement; rc=%d", rc);
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        if (settings_get_random_ble_mac_enabled(&G_Settings))
        {
            stop_ble_stack();

            esp_base_mac_addr_set(random_mac);

            nimble_port_init();

            nimble_port_freertos_init(nimble_host_task);
        }
        free(adv_data);
    }
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

void detect_ble_spam_callback(struct ble_gap_event *event, size_t length) {
    // Ensure we have valid payload data and minimum length
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

void ble_spam_task(void *param)
{
    company_type_t company = *(company_type_t *)param;

    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 255, false);

    while (1)
    {     
        send_ble_advertisement(company);
    }
}

void ble_start_spam(company_type_t company) {
    if (!ble_spam_task_running) {
        ESP_LOGI(TAG_BLE, "Starting BLE transmission...");
        company_type_t *company_ptr = malloc(sizeof(company_type_t));
        *company_ptr = company;
        xTaskCreate(ble_spam_task, "ble_spam_task", 4096, (void *)company_ptr, 5, &ble_spam_task_handle);
        ble_spam_task_running = true;
    } else {
        ESP_LOGW(TAG_BLE, "BLE transmission already running.");
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

void ble_start_airtag_scanner(void)
{
    ble_register_handler(airtag_scanner_callback);
    ble_start_scanning();
}