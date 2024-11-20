#include <esp_log.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#else
#include <lvgl/lvgl.h>
#endif
#include "cst820.h"

#include "lvgl_i2c/i2c_manager.h"

#define TAG "CST820"

esp_err_t cst820_i2c_read(uint8_t slave_addr, uint16_t register_addr, uint8_t *data_buf, uint8_t len) {
    return lvgl_i2c_read(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, data_buf, len);
}

esp_err_t cst820_i2c_write8(uint8_t slave_addr, uint16_t register_addr, uint8_t data) {
    uint8_t buffer = data;
    return lvgl_i2c_write(CONFIG_LV_I2C_TOUCH_PORT, slave_addr, register_addr | I2C_REG_16, &buffer, 1);
}

void cst820_init(uint8_t dev_addr)
{
    cst820_i2c_write8(CST820_I2C_SLAVE_ADDR, 0xFE, 0XFF);
}

esp_err_t cst820_read_continuous(uint16_t register_addr, uint8_t *data_buf, uint32_t length)
{
    esp_err_t err = cst820_i2c_read(CST820_I2C_SLAVE_ADDR, register_addr, data_buf, length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read continuous data from address: 0x%04X, error: %d", register_addr, err);
        return err;
    }
    return ESP_OK;
}

bool cst820_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;

    uint8_t FingerIndex;
    cst820_i2c_read(CST820_I2C_SLAVE_ADDR, 0x02, &FingerIndex, 1);

    if (!FingerIndex)
    {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    uint8_t touchdata[4];
    cst820_read_continuous(0x03, touchdata, 4);

    int16_t raw_x = ((touchdata[0] & 0x0f) << 8) | touchdata[1];
    int16_t raw_y = ((touchdata[2] & 0x0f) << 8) | touchdata[3];

#if CONFIG_LV_CT820_INVERT_X
    raw_x = raw_x * -1;
#endif
#if CONFIG_LV_CT820_INVERT_Y
    raw_y = raw_y * -1;
#endif
#if CONFIG_LV_CT820_SWAPXY
    int16_t swap_buf = raw_x;
    raw_x = raw_y;
    raw_y = swap_buf;
#endif

    
    const int16_t raw_max_x = 4095;
    const int16_t raw_max_y = 4095;

    
    lv_coord_t screen_width = LV_HOR_RES;
    lv_coord_t screen_height = LV_VER_RES;

    
    last_x = (raw_x * screen_width) / raw_max_x;
    last_y = (raw_y * screen_height) / raw_max_y;

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;

    ESP_LOGW(TAG, "Raw X=%d, Y=%d", raw_x, raw_y);
    ESP_LOGW(TAG, "Mapped X=%u, Y=%u", data->point.x, data->point.y);

    return false;
}