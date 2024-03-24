#include "neopixel_module.h"

#ifdef NEOPIXEL_PIN

NeopixelModule::NeopixelModule(uint16_t numPixels, uint8_t pin)
: strip(numPixels, pin, NEO_GRB + NEO_KHZ800), numPixels(numPixels), pin(pin) {}

void NeopixelModule::init() {
    strip.begin();
    rainbow(255, 4);
    //breatheLED(strip.Color(255, 255, 255), 400, true); // White color for breathing
    strip.show(); // Initialize all pixels to 'off'
}

void NeopixelModule::breatheLED(uint32_t color, int breatheTime, bool FadeOut) {
    int fadeAmount = 5; // Adjust for different speeds
    int wait = breatheTime / ((255 / fadeAmount) * 2); // Total time for one breathe cycle

    if (FadeOut) {
        // Fade in (from dark to bright), since the original logic seems inverted
         for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
            setColor(strip.Color((color >> 16) * brightness / 255, (color >> 8 & 0xFF) * brightness / 255, (color & 0xFF) * brightness / 255));
            strip.setBrightness(brightness);
            strip.show(); // Update the strip with the new color
            delay(wait);
        }
    } else {
        // Fade in (from dark to bright)
        for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
            setColor(strip.Color((color >> 16) * brightness / 255, (color >> 8 & 0xFF) * brightness / 255, (color & 0xFF) * brightness / 255));
            strip.setBrightness(brightness);
            strip.show(); // Update the strip with the new color
            delay(wait);
        }
        // Fade out (from bright to dark)
        for (int brightness = 255; brightness >= 0; brightness -= fadeAmount) {
            setColor(strip.Color((color >> 16) * brightness / 255, (color >> 8 & 0xFF) * brightness / 255, (color & 0xFF) * brightness / 255));
            strip.setBrightness(brightness);
            strip.show(); // Update the strip with the new color
            delay(wait);
        }
    }
}

void NeopixelModule::rainbow(int strength, int stepDelay) {
    // Ensure the 'strength' parameter is within the expected range
    strength = max(0, min(strength, 255));
    // Set the brightness of the strip
    strip.setBrightness(strength);

    // Generate rainbow colors for a single pixel over time
    for (uint16_t i = 0; i < 256; i++) { // Loop through all 256 color positions
        // Use the Wheel function to generate colors across the spectrum
        uint32_t color = Wheel(i & 255);
        strip.setPixelColor(0, color); // Update only the first LED (or change 0 to another index if needed)
        strip.show(); // Update the strip to show the new color
        delay(stepDelay); // Wait for 'stepDelay' milliseconds before the next color change
    }
}

uint32_t NeopixelModule::Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else if(WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}

void NeopixelModule::setColor(uint32_t color) {
    strip.setPixelColor(0, color);
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