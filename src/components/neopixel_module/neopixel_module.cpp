#include "neopixel_module.h"

#ifdef NEOPIXEL_PIN

NeopixelModule::NeopixelModule(uint16_t numPixels, uint8_t pin)
: strip(numPixels, pin, NEO_GRB + NEO_KHZ800), numPixels(numPixels), pin(pin) {}

void NeopixelModule::init() {
    strip.begin();
    rainbow(1, 7);
    breatheLED(strip.Color(255, 255, 255), 700, true); // White color for breathing
    strip.show(); // Initialize all pixels to 'off'
}

void NeopixelModule::breatheLED(uint32_t color, int breatheTime, bool FadeOut) {
    int fadeAmount = 5; // Adjust for different speeds
    int wait = breatheTime / (255 / fadeAmount * 2); // Total time for one breathe cycle
    
    if (FadeOut) {
        // Fade out (from bright to dark)
        for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
            setColor(strip.Color(brightness, brightness, brightness)); // Update the entire strip
            delay(wait);
        }
        return;
    }

    // Fade in (from dark to bright)
    for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
        setColor(strip.Color((color >> 16) * brightness / 255, (color >> 8 & 0xFF) * brightness / 255, (color & 0xFF) * brightness / 255));
        delay(wait);
    }
    // Fade out (from bright to dark)
    for (int brightness = 255; brightness >= 0; brightness -= fadeAmount) {
        setColor(strip.Color((color >> 16) * brightness / 255, (color >> 8 & 0xFF) * brightness / 255, (color & 0xFF) * brightness / 255));
        delay(wait);
    }
}

void NeopixelModule::rainbow(int strength, int stepDelay) {
    // Ensure the 'strength' is used properly for the Neopixel brightness
    for (uint16_t i = 0; i < numPixels; i++) {
        // Generate rainbow colors across pixels
        strip.setPixelColor(i, strip.Color((i * 256 / numPixels), 255, 255 - (i * 256 / numPixels)));
    }
    strip.show();
    delay(stepDelay);
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