/*
 * This code programs a number of pins on an ESP32 as buttons on a BLE gamepad
 *
 * It uses arrays to cut down on code
 *
 * Uses the Bounce2 library to debounce all buttons
 *
 * Uses the rose/fell states of the Bounce instance to track button states
 *
 * Before using, adjust the numOfButtons, buttonPins and physicalButtons to suit your senario
 *
 */

#define BOUNCE_WITH_PROMPT_DETECTION // Make button state changes available immediately

#include <Arduino.h>
#include <Bounce2.h>    // https://github.com/thomasfredericks/Bounce2
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID;
GamepadDevice* gamepad;

#define numOfButtons 10

Bounce debouncers[numOfButtons];

byte buttonPins[numOfButtons] = {0, 35, 17, 18, 19, 23, 25, 26, 27, 32};
byte physicalButtons[numOfButtons] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

void setup()
{
    Serial.begin(115200);

    for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++)
    {
        pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);

        debouncers[currentPinIndex] = Bounce();
        debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]); // After setting up the button, setup the Bounce instance :
        debouncers[currentPinIndex].interval(5);
    }

    GamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setButtonCount(numOfButtons);
    bleGamepadConfig.setAutoReport(false);

    gamepad = new GamepadDevice(bleGamepadConfig);

    compositeHID.addDevice(gamepad);
    compositeHID.begin();

    // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (compositeHID.isConnected())
    {
        bool sendReport = false;

        for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
        {
            debouncers[currentIndex].update();

            if (debouncers[currentIndex].fell())
            {
                gamepad->press(physicalButtons[currentIndex]);
                sendReport = true;
                Serial.print("Button ");
                Serial.print(physicalButtons[currentIndex]);
                Serial.println(" pushed.");
            }
            else if (debouncers[currentIndex].rose())
            {
                gamepad->release(physicalButtons[currentIndex]);
                sendReport = true;
                Serial.print("Button ");
                Serial.print(physicalButtons[currentIndex]);
                Serial.println(" released.");
            }
        }

        if (sendReport)
        {
            gamepad->sendGamepadReport();
        }

        // delay(20);	// (Un)comment to remove/add delay between loops
    }
}
