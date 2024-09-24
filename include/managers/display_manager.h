#ifndef DISP_MANAGER_H
#define DISP_MANAGER_H

#include <stdint.h>
#include "lvgl/lvgl.h"

void display_init();
void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data);
void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t width, uint16_t height, lv_color_t color, lv_align_t align);
void display_draw_image(uint16_t x, uint16_t y, const void *image_src);
void display_lv_task_handler(void);
#endif