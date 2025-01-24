#include <esp_log.h>
#include <driver/i2c.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include <lvgl.h>
#else
#include <lvgl/lvgl.h>
#endif
#include "cst820.h"

#define TAG "CST820"
#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MASTER_FREQ_HZ 400000

esp_err_t cst820_i2c_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR_CST820 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR_CST820 << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t cst820_i2c_write(uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR_CST820 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

void cst820_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CYD28_TouchC_SDA,
        .scl_io_num = CYD28_TouchC_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(0, conf.mode, 0, 0, 0));

    gpio_set_direction(CYD28_TouchC_INT, GPIO_MODE_OUTPUT);
    gpio_set_level(CYD28_TouchC_INT, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(CYD28_TouchC_INT, 0);
    vTaskDelay(pdMS_TO_TICKS(1));

    gpio_set_direction(CYD28_TouchC_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(CYD28_TouchC_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CYD28_TouchC_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(300));

    ESP_ERROR_CHECK(cst820_i2c_write(0xFE, 0xFF));
}

static void convert_raw_xy(int16_t raw_x, int16_t raw_y, int16_t *x, int16_t *y) {
    *x = raw_x;
    *y = raw_y;

    *x = (*x * LV_HOR_RES) / 240;
    *y = (*y * LV_VER_RES) / 320;

    ESP_LOGI(TAG, "Raw: x=%d, y=%d, Converted: x=%d, y=%d", raw_x, raw_y, *x, *y);
}

bool cst820_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    uint8_t touch_points = 0;

    if (cst820_i2c_read(0x02, &touch_points, 1) != ESP_OK) {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    if (!touch_points) {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    uint8_t touch_data[4];
    if (cst820_i2c_read(0x03, touch_data, 4) != ESP_OK) {
        return false;
    }

    int16_t raw_x = ((touch_data[0] & 0x0f) << 8) | touch_data[1];
    int16_t raw_y = ((touch_data[2] & 0x0f) << 8) | touch_data[3];
    
    int16_t x, y;
    convert_raw_xy(raw_x, raw_y, &x, &y);

    last_x = x;
    last_y = y;
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PR;

    ESP_LOGV(TAG, "Touch: x=%d, y=%d", x, y);
    return false;
}