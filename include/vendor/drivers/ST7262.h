/*
 * SPDX-FileCopyrightText: 2023-2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LCD_ST7262_H
#define LCD_ST7262_H

#ifdef CONFIG_USE_7_INCHER

#include "esp_err.h"
#include "esp_lcd_types.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ST7262 LCD panel.
 *
 * This function initializes the ST7262 LCD panel connected via the RGB interface.
 *
 * @return
 *      - ESP_OK on success
 *      - Appropriate error code on failure
 */
esp_err_t lcd_st7262_init(void);

/**
 * @brief Deinitialize the ST7262 LCD panel.
 *
 * This function deinitializes the ST7262 LCD panel and releases any allocated resources.
 *
 * @return
 *      - ESP_OK on success
 *      - Appropriate error code on failure
 */
esp_err_t lcd_st7262_deinit(void);

/**
 * @brief Get the LCD panel handle.
 *
 * This function returns the handle to the LCD panel, which can be used for further operations.
 *
 * @return
 *      - esp_lcd_panel_handle_t on success
 *      - NULL on failure
 */
esp_lcd_panel_handle_t lcd_st7262_get_panel_handle(void);

/**
 * @brief Initialize LVGL display driver for the ST7262 LCD panel.
 *
 * This function sets up the LVGL display driver and registers the flush callback.
 *
 * @return
 *      - ESP_OK on success
 *      - Appropriate error code on failure
 */
esp_err_t lcd_st7262_lvgl_init(void);


#endif

#ifdef __cplusplus
}
#endif

#endif // LCD_ST7262_H