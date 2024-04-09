/**
 * Simple HID mouse
 * author: chegewara
 */

#include "hidmouse.h"
#if CFG_TUD_HID
HIDmouse mouse;

void setup()
{
    Serial.begin(115200);
    mouse.begin();
}

void loop()
{
    delay(1000);
    mouse.doublePressLeft();
    delay(1000);
    mouse.pressRight();
    delay(1000);
    mouse.move(-15, -15);
    delay(1000);
    mouse.pressLeft();
    delay(1000);
    mouse.scrollUp(1);
    delay(1000);
    mouse.scrollDown(1);
    delay(1000);
}

#endif
