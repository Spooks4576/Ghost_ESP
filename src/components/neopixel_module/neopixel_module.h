#pragma once

#include "board_config.h" // Ensure this is compatible with your board configurations

#include <Adafruit_NeoPixel.h>

class NeopixelModule {
public:
    NeopixelModule(uint16_t numPixels, uint8_t pin);
    void init();
    void setColor(uint32_t color); // Set all pixels to a specific color
    void setPixelColor(uint16_t n, uint32_t color); // Set color of a single pixel
    void breatheLED(uint32_t color, int breatheTime, bool FadeOut);
    void rainbow(int strength, int stepDelay);
    void show();
    uint32_t Wheel(byte WheelPos);

public:
    Adafruit_NeoPixel strip;
    uint16_t numPixels;
    uint8_t pin;
};
