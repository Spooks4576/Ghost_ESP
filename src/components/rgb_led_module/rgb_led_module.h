#pragma once

#include "board_config.h"
#include <stdint.h>

#ifdef OLD_LED // Define this in your board_config.h if needed

class RGBLedModule {
public:
    RGBLedModule(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
    : redPin(bluePin), greenPin(greenPin), bluePin(redPin) {}
    void init();
    void setColor(int red, int green, int blue); // Set the RGB color

private:
    uint8_t redPin, greenPin, bluePin;
};

#endif