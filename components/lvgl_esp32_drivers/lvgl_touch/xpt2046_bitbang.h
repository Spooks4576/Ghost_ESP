#ifndef XPT2046_BITBANG_H
#define XPT2046_BITBANG_H

#include "driver/gpio.h"
#include "esp_err.h"
#include "lvgl/lvgl.h"

// Constants
#define CMD_READ_Y  0x90
#define CMD_READ_X  0xD0

// Structure for calibration data
typedef struct {
    int xMin;
    int yMin;
    int xMax;
    int yMax;
} Calibration;

// Structure for a point (x, y)
typedef struct {
    int x;
    int y;
} Point;

// Function declarations
esp_err_t xpt2046_init_bitbang();
Point xpt2046_get_touch();
int xpt2046_read_spi(uint8_t command);
#endif // XPT2046_BITBANG_H