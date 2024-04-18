#include <Arduino.h>
#include <KeyboardDevice.h>
#include <BleCompositeHID.h>

#define KEY_RANGE_MIN KEY_A
#define KEY_RANGE_MAX KEY_RIGHTMETA

BleCompositeHID compositeHID("ESP32 Keyboard", "Mystfit", 100);
KeyboardDevice* keyboard;

void setup()
{
    Serial.begin(115200);

    keyboard = new KeyboardDevice();
    compositeHID.addDevice(keyboard);
    compositeHID.begin();

    Serial.println("Waiting for connection");
    delay(3000);
}

void loop()
{
    if (compositeHID.isConnected())
    {
        for(uint8_t keyCode = KEY_RANGE_MIN; keyCode <= KEY_RANGE_MAX; keyCode++)
        {
            Serial.println("Pressing key " + String(keyCode));
            keyboard->keyPress(keyCode);
            delay(10);
            keyboard->keyRelease(keyCode);
            delay(1000);
        }
    }
}
