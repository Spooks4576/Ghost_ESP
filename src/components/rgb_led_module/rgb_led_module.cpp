#include "rgb_led_module.h"
#include <Arduino.h>

#ifdef OLD_LED

void RGBLedModule::init() {
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    digitalWrite(bluePin, HIGH);
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, LOW);
    delay(700);
    digitalWrite(bluePin, HIGH);
    digitalWrite(greenPin, LOW);
    digitalWrite(redPin, HIGH);
    delay(700);
    digitalWrite(bluePin, HIGH);
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, LOW);
    delay(700);
    digitalWrite(bluePin, HIGH);
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, HIGH);
}

void RGBLedModule::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);
}

#endif