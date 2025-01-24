/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CH422G_H
#define CH422G_H

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CH422G device structure
 */
typedef struct {
  i2c_port_t i2c_num;   /*!< I2C port number */
  uint32_t i2c_address; /*!< I2C device address */
  struct {
    uint8_t wr_set; /*!< WR_SET register value */
    uint8_t wr_oc;  /*!< WR_OC register value */
    uint8_t wr_io;  /*!< WR_IO register value */
  } regs;           /*!< Internal registers */
} esp_io_expander_ch422g_t;

/**
 * @brief Initialize a new CH422G device
 *
 * @param i2c_num I2C port number
 * @param i2c_address I2C address of the CH422G device
 * @param out_dev Pointer to store the allocated device handle
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_new_device(i2c_port_t i2c_num, uint32_t i2c_address,
                            esp_io_expander_ch422g_t **out_dev);

/**
 * @brief Delete a CH422G device
 *
 * @param dev Pointer to the CH422G device to delete
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
void cleanup_resources(esp_io_expander_ch422g_t *ch422g_dev,
                       i2c_port_t i2c_num);

/**
 * @brief Read the input register of the CH422G device
 *
 * @param dev Pointer to the CH422G device
 * @param value Pointer to store the read value
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_read_input_reg(esp_io_expander_ch422g_t *dev, uint32_t *value);

/**
 * @brief Write the output register of the CH422G device
 *
 * @param dev Pointer to the CH422G device
 * @param value Value to write to the output register
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_write_output_reg(esp_io_expander_ch422g_t *dev,
                                  uint32_t value);

/**
 * @brief Read the output register of the CH422G device
 *
 * @param dev Pointer to the CH422G device
 * @param value Pointer to store the read value
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_read_output_reg(esp_io_expander_ch422g_t *dev,
                                 uint32_t *value);

/**
 * @brief Write the direction register of the CH422G device
 *
 * @param dev Pointer to the CH422G device
 * @param value Direction value to write (0 for input, non-zero for output)
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_write_direction_reg(esp_io_expander_ch422g_t *dev,
                                     uint32_t value);

/**
 * @brief Read the direction register of the CH422G device
 *
 * @param dev Pointer to the CH422G device
 * @param value Pointer to store the read value
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_read_direction_reg(esp_io_expander_ch422g_t *dev,
                                    uint32_t *value);

/**
 * @brief Reset the CH422G device to default configuration
 *
 * @param dev Pointer to the CH422G device
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t ch422g_reset(esp_io_expander_ch422g_t *dev);

#ifdef __cplusplus
}
#endif

#endif // CH422G_H