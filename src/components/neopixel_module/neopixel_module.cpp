#include "neopixel_module.h"

#ifdef NEOPIXEL_PIN

NeopixelModule::NeopixelModule(uint16_t numPixels, uint8_t pin)
: strip(numPixels, pin, NEO_GRB + NEO_KHZ800), numPixels(numPixels), pin(pin) {}

void NeopixelModule::init() {
    strip.begin();
    setColor(strip.Color(1, 0, 0));
    delay(500);
    setColor(strip.Color(0, 1, 0));
    delay(500);
    setColor(strip.Color(1, 0, 0));
    delay(500);
    setColor(strip.Color(0, 0, 0));
    strip.show(); // Initialize all pixels to 'off'
}

void NeopixelModule::setColor(uint32_t color) {
    for(uint16_t i = 0; i < numPixels; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void NeopixelModule::setPixelColor(uint16_t n, uint32_t color) {
    strip.setPixelColor(n, color);
    strip.show();
}

void NeopixelModule::show() {
    strip.show();
}

#endif