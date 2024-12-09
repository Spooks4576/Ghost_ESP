/*
 * SPDX-FileCopyrightText: 2023-2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LCD_AXS15231B_H
#define LCD_AXS15231B_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the AXS15231B LCD panel.
 *
 * This function initializes the AXS15231B LCD panel
 *
 * @return
 *      - ESP_OK on success
 *      - Appropriate error code on failure
 */
esp_err_t lcd_axs15231b_init(void);

#endif

#ifdef __cplusplus
}
#endif