#include <Arduino.h>
#include <KeyboardDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID("ESP32 LED Keyboard", "Mystfit", 100);
KeyboardDevice* keyboard;

void OnLEDEvent(KeyboardOutputReport data){
    Serial.println(
    "LED Report: Number Lock: " + String(data.numLockActive) + 
    " Caps Lock: " + String(data.capsLockActive) + 
    " Scroll Lock: " + String(data.scrollLockActive) + 
    " Compose: " + String(data.composeActive) + 
    " Kana: " + String(data.kanaActive));
}

void setup()
{
    Serial.begin(115200);

    keyboard = new KeyboardDevice();
    FunctionSlot<KeyboardOutputReport> OnLEDEventSlot(OnLEDEvent);
    keyboard->onLED.attach(OnLEDEventSlot);

    compositeHID.addDevice(keyboard);
    compositeHID.begin();

    Serial.println("Waiting for connection");
    delay(3000);
}

void loop()
{
    keyboard->keyPress(KEY_NUMLOCK);
    delay(10);
    keyboard->keyRelease(KEY_NUMLOCK);
    delay(1000);

    keyboard->keyPress(KEY_CAPSLOCK);
    delay(10);
    keyboard->keyRelease(KEY_CAPSLOCK);
    delay(1000);

    keyboard->keyPress(KEY_SCROLLLOCK);
    delay(10);
    keyboard->keyRelease(KEY_SCROLLLOCK);
    delay(1000);

    keyboard->keyPress(KEY_COMPOSE);
    delay(10);
    keyboard->keyRelease(KEY_COMPOSE);
    delay(1000);
}
