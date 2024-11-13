/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "driver/i2c.h"
#include "esp_bit_defs.h"
#include "esp_check.h"
#include "esp_log.h"

#include "vendor/drivers/CH422G.h"

/* Timeout of each I2C communication */
#define I2C_TIMEOUT_MS          (10)

#define IO_COUNT                (12)

/* Register address */
#define CH422G_REG_WR_SET       (0x48 >> 1)
#define CH422G_REG_WR_OC        (0x46 >> 1)
#define CH422G_REG_WR_IO        (0x70 >> 1)
#define CH422G_REG_RD_IO        (0x4D >> 1)

/* Default register value when reset */
#define REG_WR_SET_DEFAULT_VAL  (0x01UL)
#define REG_WR_OC_DEFAULT_VAL   (0x0FUL)
#define REG_WR_IO_DEFAULT_VAL   (0xFFUL)
#define REG_OUT_DEFAULT_VAL     ((REG_WR_OC_DEFAULT_VAL << 8) | REG_WR_IO_DEFAULT_VAL)
#define REG_DIR_DEFAULT_VAL     (0xFFFUL)

#define REG_WR_SET_BIT_IO_OE    (1 << 0)
#define REG_WR_SET_BIT_OD_EN    (1 << 2)

/* Default direction values */
#define DIR_OUT_VALUE           (0xFFF)
#define DIR_IN_VALUE            (0xF00)

static const char *TAG = "ch422g";

esp_err_t ch422g_new_device(i2c_port_t i2c_num, uint32_t i2c_address, esp_io_expander_ch422g_t **out_dev)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .master.clk_speed = 400000,
    };
    esp_err_t err = i2c_param_config(i2c_num, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
    }



    ESP_RETURN_ON_FALSE(i2c_num < I2C_NUM_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid I2C port");
    ESP_RETURN_ON_FALSE(out_dev, ESP_ERR_INVALID_ARG, TAG, "Invalid output device pointer");

    esp_io_expander_ch422g_t *dev = calloc(1, sizeof(esp_io_expander_ch422g_t));
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory");

    dev->i2c_num = i2c_num;
    dev->i2c_address = i2c_address;
    dev->regs.wr_set = REG_WR_SET_DEFAULT_VAL;
    dev->regs.wr_oc = REG_WR_OC_DEFAULT_VAL;
    dev->regs.wr_io = REG_WR_IO_DEFAULT_VAL;

    ESP_ERROR_CHECK(ch422g_reset(dev));
    *out_dev = dev;

    return ESP_OK;
}


void cleanup_resources(esp_io_expander_ch422g_t *ch422g_dev, i2c_port_t i2c_num)
{
    if (ch422g_dev) {
        free(ch422g_dev);
        ESP_LOGI(TAG, "CH422G device deleted");
    }

    
    esp_err_t err = i2c_driver_delete(i2c_num);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C driver uninstalled");
    } else {
        ESP_LOGE(TAG, "Failed to uninstall I2C driver: %s", esp_err_to_name(err));
    }
}

esp_err_t ch422g_read_input_reg(esp_io_expander_ch422g_t *ch422g, uint32_t *value)
{
    uint8_t temp = 0;

    esp_err_t err = i2c_master_read_from_device(
        ch422g->i2c_num, ch422g->i2c_address | CH422G_REG_RD_IO, &temp, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "Failed to read input register");

    *value = temp;
    return ESP_OK;
}

esp_err_t ch422g_write_output_reg(esp_io_expander_ch422g_t *ch422g, uint32_t value)
{
    uint8_t wr_oc_data = (value & 0xF00) >> 8;
    uint8_t wr_io_data = value & 0xFF;

    if (wr_oc_data) {
        ESP_RETURN_ON_ERROR(
            i2c_master_write_to_device(ch422g->i2c_num, CH422G_REG_WR_OC, &wr_oc_data, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS)),
            TAG, "Failed to write WR_OC register");
        ch422g->regs.wr_oc = wr_oc_data;
    }

    if (wr_io_data) {
        ESP_RETURN_ON_ERROR(
            i2c_master_write_to_device(ch422g->i2c_num, CH422G_REG_WR_IO, &wr_io_data, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS)),
            TAG, "Failed to write WR_IO register");
        ch422g->regs.wr_io = wr_io_data;
    }

    return ESP_OK;
}

esp_err_t ch422g_read_output_reg(esp_io_expander_ch422g_t *ch422g, uint32_t *value)
{
    *value = ch422g->regs.wr_io | ((uint32_t)ch422g->regs.wr_oc << 8);
    return ESP_OK;
}

esp_err_t ch422g_write_direction_reg(esp_io_expander_ch422g_t *ch422g, uint32_t value)
{
    uint8_t data = ch422g->regs.wr_set;

    value &= 0xFF;
    if (value != 0) {
        data |= REG_WR_SET_BIT_IO_OE;
    } else {
        data &= ~REG_WR_SET_BIT_IO_OE;
    }

    ESP_RETURN_ON_ERROR(
        i2c_master_write_to_device(ch422g->i2c_num, CH422G_REG_WR_SET, &data, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS)),
        TAG, "Failed to write direction register");

    ch422g->regs.wr_set = data;
    return ESP_OK;
}

esp_err_t ch422g_read_direction_reg(esp_io_expander_ch422g_t *ch422g, uint32_t *value)
{
    *value = (ch422g->regs.wr_set & REG_WR_SET_BIT_IO_OE) ? DIR_OUT_VALUE : DIR_IN_VALUE;
    return ESP_OK;
}

esp_err_t ch422g_reset(esp_io_expander_ch422g_t *ch422g)
{
    ESP_RETURN_ON_ERROR(ch422g_write_direction_reg(ch422g, REG_DIR_DEFAULT_VAL), TAG, "Failed to reset direction");
    ESP_RETURN_ON_ERROR(ch422g_write_output_reg(ch422g, REG_OUT_DEFAULT_VAL), TAG, "Failed to reset output");

    return ESP_OK;
}