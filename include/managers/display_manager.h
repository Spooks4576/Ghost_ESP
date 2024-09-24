#ifndef DISP_MANAGER_H
#define DISP_MANAGER_H

#include <stdint.h>
#include "lvgl.h"

// Initializes the display using LVGL
void display_init(void);

// Draws a bitmap on the display
void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data);

// Draws text at a specific location
void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t width, uint16_t height, lv_color_t color, lv_align_t align);

// Sets the backlight brightness (0-100%)
void display_set_backlight(uint8_t brightness);

// Clears the display with a specified color
void display_clear(lv_color_t color);

// Handles LVGL task refreshing (must be called periodically)
void display_lv_task_handler(void);

#endif // DISP_MANAGER_H