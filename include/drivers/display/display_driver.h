// display_driver.h

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*draw_bitmap)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data);
    esp_err_t (*invert_color)(bool invert);
    esp_err_t (*set_gap)(uint16_t x_gap, uint16_t y_gap);
    esp_err_t (*mirror)(bool mirror_x, bool mirror_y);
    esp_err_t (*swap_xy)(bool swap_axes);
    esp_err_t (*disp_on_off)(bool on_off);
} display_driver_t;

#endif // DISPLAY_DRIVER_H