#include "neopixel_module.h"

#ifdef NEOPIXEL_PIN

NeopixelModule::NeopixelModule(uint16_t numPixels, uint8_t pin)
: strip(numPixels, pin, NEO_GRB + NEO_KHZ800), numPixels(numPixels), pin(pin) {}

void NeopixelModule::init() {
    strip.begin();
    rainbow(255, 4);
    breatheLED(strip.Color(255, 255, 255), 200, true);
    strip.show();
}

void NeopixelModule::breatheLED(uint32_t color, int breatheTime, bool FadeOut) {
    int fadeAmount = 5;
    int wait = breatheTime / ((255 / fadeAmount) * 2);

    if (FadeOut) {
       
        for (int brightness = 255; brightness >= 0; brightness -= fadeAmount) {
            setColor(color);
            strip.setBrightness(brightness);
            strip.show(); 
            delay(wait);
        }
    } else {
       
        for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
            setColor(color);
            strip.setBrightness(brightness);
            strip.show();
            delay(wait);
        }
      
        for (int brightness = 255; brightness >= 0; brightness -= fadeAmount) {
            setColor(color);
            strip.setBrightness(brightness);
            strip.show();
            delay(wait);
        }
    }
}

void NeopixelModule::rainbow(int strength, int stepDelay) {
    
    strength = max(0, min(strength, 255));
   
    strip.setBrightness(strength);

   
    for (uint16_t i = 0; i < 256; i++) {
       
        uint32_t color = Wheel(i & 255);
        strip.setPixelColor(0, color); 
        strip.show(); 
        delay(stepDelay); 
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