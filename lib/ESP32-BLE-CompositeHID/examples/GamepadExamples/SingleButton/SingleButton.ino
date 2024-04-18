/*
 * A simple sketch that maps a single pin on the ESP32 to a single button on the controller
 */

#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

#define BUTTONPIN 35 // Pin button is attached to

BleCompositeHID compositeHID;
GamepadDevice* gamepad;

int previousButton1State = HIGH;

void setup()
{
    pinMode(BUTTONPIN, INPUT_PULLUP);
    gamepad = new GamepadDevice();
    compositeHID.addDevice(gamepad);
    compositeHID.begin();
}

void loop()
{
    if (compositeHID.isConnected())
    {

        int currentButton1State = digitalRead(BUTTONPIN);

        if (currentButton1State != previousButton1State)
        {
            if (currentButton1State == LOW)
            {
                gamepad->press(BUTTON_1);
            }
            else
            {
                gamepad->release(BUTTON_1);
            }
        }
        previousButton1State = currentButton1State;
    }
}