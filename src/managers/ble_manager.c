#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "managers/ble_manager.h"

static const char *TAG_BLE = "BLE_MANAGER";

#define MAX_HANDLERS 10

typedef struct {
    ble_data_handler_t handler;
} ble_handler_t;

static ble_handler_t handlers[MAX_HANDLERS];
static int handler_count = 0;

static uint8_t service_uuid[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t char_uuid[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static int gatt_write(struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint8_t buf[20];
    int len = ctxt->om->om_len;
    int copy_len = 0;

    if (len > sizeof(buf)) {
        return -1;
    }


    ble_hs_mbuf_to_flat(ctxt->om, buf, len, copy_len);
    ESP_LOGI(TAG_BLE, "Received data: %s", buf);

    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].handler) {
            handlers[i].handler(buf, len);
        }
    }

    ble_gatt_access_rsp(ctxt, NULL, 0);
    return 0;
}

static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = service_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = char_uuid,
                .access_cb = gatt_write,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                0, // End of characteristics
            }
        },
    },
    {
        0, // End of services
    }
};

esp_err_t ble_register_handler(ble_data_handler_t handler) {
    if (handler_count < MAX_HANDLERS) {
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
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

void ble_init(void) {
    nvs_flash_init();
    nimble_port_init();
    ble_gatt_svc_def_register(gatt_services);
}
