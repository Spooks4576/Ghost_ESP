/**
 * Simple HID gamepad, 32 buttons + 2x 3 axis + 8 position hat
 * author: chegewara
 */


#include "hidgamepad.h"
#if CFG_TUD_HID
HIDgamepad gamepad;

void setup()
{
    Serial.begin(115200);
    gamepad.begin();
}

void loop()
{
    delay(1000);
    if(!digitalRead(0)){
        // 32 buttons
        for (size_t i = 0; i < 32; i++)
        {
            // buttons send map of buttons represented by bits
            gamepad.buttons(1<<i);
            delay(100);
        }

        // hat 8 positions
        for (size_t i = 0; i < 9; i++)
        {
            gamepad.hat(i);
            delay(100);
        }

        gamepad.sendAll(0, 0, 0, 0, 0, 0, 0, 0);
        // x, y, z
        gamepad.joystick1(100, -100, 50);
        delay(1000);
        gamepad.joystick1(-100, 100, -50);
        delay(1000);
        // Rx, Ry, Rz
        gamepad.joystick2(100, -100, 50);
        delay(1000);
        gamepad.joystick2(-100, 100, -50);
        delay(1000);
        // Button Map |  X | Y | Z | Rx | Ry | Rz | hat
        gamepad.sendAll(0xffffffff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 5);
        delay(1000);
        gamepad.sendAll(0, 0, 0, 0, 0, 0, 0, 0);
    }
}

#endif
