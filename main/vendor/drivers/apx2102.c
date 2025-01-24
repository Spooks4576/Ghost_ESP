#include "driver/i2c.h"
#include "vendor/drivers/axp2101.h"
#include <stdio.h>

#define BIT_MASK(bit) (1 << (bit))
#define IS_BIT_SET(value, bit) (((value) & (1 << (bit))) != 0)

bool i2c_initialized = false;

#define AXP2101_REG_POWER_LEVEL 0xA4
#define AXP202_MODE_CHGSTATUS 0x01

bool axp202_is_battery_connected(void) {
  if (!i2c_initialized) {
    printf("ERROR [%s]: I2C is not initialized\n", __func__);
    return false;
  }

  uint8_t reg = 0;
  uint8_t reg_addr = AXP202_MODE_CHGSTATUS;

  esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR,
                                             &reg_addr, 1, pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to write register address 0x%02X: %s\n",
           __func__, reg_addr, esp_err_to_name(err));
    return false;
  }

  err = i2c_master_read_from_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR, &reg, 1,
                                    pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to read register 0x%02X: %s\n", __func__,
           reg_addr, esp_err_to_name(err));
    return false;
  }

  bool battery_connected = IS_BIT_SET(reg, 5);
  printf("INFO [%s]: Battery connection status: %s\n", __func__,
         battery_connected ? "Connected" : "Not Connected");
  return battery_connected;
}

bool axp202_is_charging(void) {
  if (!i2c_initialized) {
    printf("ERROR [%s]: I2C is not initialized\n", __func__);
    return false;
  }

  uint8_t reg = 0;
  uint8_t reg_addr = AXP202_MODE_CHGSTATUS;

  esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR,
                                             &reg_addr, 1, pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to write register address 0x%02X: %s\n",
           __func__, reg_addr, esp_err_to_name(err));
    return false;
  }

  err = i2c_master_read_from_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR, &reg, 1,
                                    pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to read register 0x%02X: %s\n", __func__,
           reg_addr, esp_err_to_name(err));
    return false;
  }

  bool charging = IS_BIT_SET(reg, 6);
  printf("INFO [%s]: Charging status: %s\n", __func__,
         charging ? "Charging" : "Not Charging");
  return !charging; // for some reason this bool needs to be flipped
}

esp_err_t axp2101_init(void) {
  if (i2c_initialized) {
    printf("WARNING [%s]: AXP2101 is already initialized\n", __func__);
    return ESP_OK;
  }

  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
      .clk_flags = 0,
  };

  esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to configure I2C parameters: %s\n", __func__,
           esp_err_to_name(err));
    return err;
  }

  err = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to install I2C driver: %s\n", __func__,
           esp_err_to_name(err));
    return err;
  }

  i2c_initialized = true;
  printf("INFO [%s]: AXP2101 initialized successfully\n", __func__);
  return ESP_OK;
}

esp_err_t axp2101_deinit(void) {
  if (!i2c_initialized) {
    printf("WARNING [%s]: AXP2101 is not initialized\n", __func__);
    return ESP_OK;
  }

  esp_err_t err = i2c_driver_delete(I2C_MASTER_NUM);
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to delete I2C driver: %s\n", __func__,
           esp_err_to_name(err));
    return err;
  }

  i2c_initialized = false;
  printf("INFO [%s]: AXP2101 deinitialized successfully\n", __func__);
  return ESP_OK;
}

esp_err_t axp2101_get_power_level(uint8_t *power_level) {
  if (!i2c_initialized) {
    printf("ERROR [%s]: AXP2101 is not initialized\n", __func__);
    return ESP_ERR_INVALID_STATE;
  }

  if (power_level == NULL) {
    printf("ERROR [%s]: Invalid parameter: power_level is NULL\n", __func__);
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t reg_addr = AXP2101_REG_POWER_LEVEL;
  uint8_t data = 0;

  esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR,
                                             &reg_addr, 1, pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to write to AXP2101 register 0x%02X: %s\n",
           __func__, reg_addr, esp_err_to_name(err));
    return err;
  }

  err = i2c_master_read_from_device(I2C_MASTER_NUM, AXP2101_I2C_ADDR, &data, 1,
                                    pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    printf("ERROR [%s]: Failed to read from AXP2101 register 0x%02X: %s\n",
           __func__, reg_addr, esp_err_to_name(err));
    return err;
  }

  if (!(data & BIT_MASK(7))) {
    *power_level = data & (~BIT_MASK(7));
    printf("INFO [%s]: Battery percentage: %d%%\n", __func__, *power_level);
    return ESP_OK;
  } else {
    printf("WARNING [%s]: Battery percentage value is invalid (data: 0x%02X)\n",
           __func__, data);
    return ESP_ERR_INVALID_RESPONSE;
  }
}