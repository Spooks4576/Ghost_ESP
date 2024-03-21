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

void RGBLedModule::breatheLED(int ledPin, int breatheTime)
{
    int fadeAmount = 5; // Amount of brightness change per step, adjust for different breathing speeds
    int wait = breatheTime / (255 / fadeAmount * 2); // Calculate wait time to fit the breathe cycle into the given total time

    // Fade in
    for (int brightness = 0; brightness <= 255; brightness += fadeAmount) {
        analogWrite(ledPin, brightness);
        delay(wait);
    }
    // Fade out
    for (int brightness = 255; brightness >= 0; brightness -= fadeAmount) {
        analogWrite(ledPin, brightness);
        delay(wait);
    }
}

void RGBLedModule::Song()
{
    
}

void RGBLedModule::Rainbow(int strength, int stepDelay) 
{

    float brightnessFactor = constrain(brightnessFactor, 0.0, 1.0);

    // Ensure strength is between 0 and 255
    int maxStrength = static_cast<int>(255 * brightnessFactor);

    // Color transitions
    for (int g = 0; g <= maxStrength; g += 5) {
        analogWrite(redPin, maxStrength);
        analogWrite(greenPin, g);
        analogWrite(bluePin, 0);
        delay(stepDelay);
    }

    for (int r = maxStrength; r >= 0; r -= 5) {
        analogWrite(redPin, r);
        analogWrite(greenPin, maxStrength);
        analogWrite(bluePin, 0);
        delay(stepDelay);
    }

    for (int b = 0; b <= maxStrength; b += 5) {
        analogWrite(redPin, 0);
        analogWrite(greenPin, maxStrength);
        analogWrite(bluePin, b);
        delay(stepDelay);
    }

    for (int g = maxStrength; g >= 0; g -= 5) {
        analogWrite(redPin, 0);
        analogWrite(greenPin, g);
        analogWrite(bluePin, maxStrength);
        delay(stepDelay);
    }

    for (int r = 0; r <= maxStrength; r += 5) {
        analogWrite(redPin, r);
        analogWrite(greenPin, 0);
        analogWrite(bluePin, maxStrength);
        delay(stepDelay);
    }

    for (int b = maxStrength; b >= 0; b -= 5) {
        analogWrite(redPin, maxStrength);
        analogWrite(greenPin, 0);
        analogWrite(bluePin, b);
        delay(stepDelay);
    }
}

void RGBLedModule::setColor(int red, int green, int blue) {

    int pwmRed = red;
    int pwmGreen = green;
    int pwmBlue = blue;

    digitalWrite(redPin, pwmRed);
    digitalWrite(greenPin, pwmGreen);
    digitalWrite(bluePin, pwmBlue);
}

#endif