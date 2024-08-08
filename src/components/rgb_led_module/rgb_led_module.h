#pragma once

#include "board_config.h"
#include <stdint.h>

class RGBLedModule {
public:
    RGBLedModule(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
    : redPin(bluePin), greenPin(greenPin), bluePin(redPin) {}
    void init();
    void setColor(int red, int green, int blue); // Set the RGB color
    void Rainbow(int strength, int stepDelay);
    void breatheLED(int ledPin, int breatheTime, bool FadeIn = false);
    void fadeOutAllPins(int fadeTime);
    void Song();
public:
    uint8_t redPin, greenPin, bluePin;
};