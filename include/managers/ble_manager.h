#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

typedef void (*ble_data_handler_t)(const uint8_t *data, size_t len);

esp_err_t ble_register_handler(ble_data_handler_t handler);
esp_err_t ble_unregister_handler(ble_data_handler_t handler);
void ble_init(void);

#endif // BLE_MANAGER_H