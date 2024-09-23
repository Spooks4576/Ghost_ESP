// wifi_manager.h

#ifndef DISP_MANAGER_H
#define DISP_MANAGER_H

#include <stdint.h>

void display_init();


void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data);

#endif