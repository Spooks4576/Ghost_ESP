#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define MAX_UUID16 10
#define MAX_UUID32 5
#define MAX_UUID128 3
#define MAX_PAYLOADS 10         // Maximum number of similar payloads to consider as spam
#define PAYLOAD_COMPARE_LEN 20  // Length of the payload to compare for similarity
#define TIME_WINDOW_MS 3000

typedef void (*ble_data_handler_t)(struct ble_gap_event *event, size_t len);

typedef struct {
    uint16_t uuid16[MAX_UUID16];
    int uuid16_count;

    uint32_t uuid32[MAX_UUID32];
    int uuid32_count;

    char uuid128[MAX_UUID128][37];
    int uuid128_count;
} ble_service_uuids_t;

static void* ble_spam_task_handle;
static int ble_spam_task_running = 0;


typedef enum {
    COMPANY_APPLE,
    COMPANY_GOOGLE,
    COMPANY_SAMSUNG,
    COMPANY_MICROSOFT
} company_type_t;

esp_err_t ble_register_handler(ble_data_handler_t handler);
esp_err_t ble_unregister_handler(ble_data_handler_t handler);
void ble_init(void);
void ble_start_find_flippers(void);
void ble_stop_scanning(void);
void ble_start_airtag_scanner(void);
void ble_start_blespam_detector(void);
void ble_start_spam(company_type_t company);

#endif // BLE_MANAGER_H