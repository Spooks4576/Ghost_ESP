// m5gfx_wrapper.cpp
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#include "vendor/m5gfx_wrapper.h"

M5GFX display; // a Message From Spooky Fuck you M5Stack for not having normal driver support for your ST display 
               // IDK what gay ass shit your using in the library to initilize your display from the normal driver but this really pissed me off for a week 
               // Go fuck yourself you cunts

extern "C" void init_m5gfx_display() {
    auto panel = new lgfx::Panel_ST7789();
    panel->setRotation(1);
    display.setPanel(panel);
    display.init();                         // Initialize the display
    display.fillScreen(TFT_BLACK);          // Clear screen to black
}

extern "C" void m5gfx_write_pixels(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t *color_p) {
    display.startWrite();
    display.setAddrWindow(x1, y1, (x2 - x1 + 1), (y2 - y1 + 1));
    display.pushPixels(color_p, (x2 - x1 + 1) * (y2 - y1 + 1));
    display.endWrite();
}