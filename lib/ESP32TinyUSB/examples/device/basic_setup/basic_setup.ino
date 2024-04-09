/**
 * Simple example to show how to set product variables
 * author: chegewara
 */
#include "Arduino.h"
#include "cdcusb.h"

CDCusb USBSerial;

class MyUSBSallnbacks: public USBCallbacks {
    void onMount()
    {
        Serial.println("device mounted");
    }
    void onUnmount()
    {
        Serial.println("device unmounted");
    }
    void onSuspend()
    {
        Serial.println("device suspended");
    }
    void onResume(bool resume)
    {
        Serial.println("device resumed");
    }
};

void setup()
{
    Serial.begin(115200);
    USBSerial.manufacturer("espressif");
    USBSerial.serial("1234-567890");
    USBSerial.product("Test device");
    USBSerial.revision(100);
    USBSerial.deviceID(0xdead, 0xbeef);
    USBSerial.registerDeviceCallbacks(new MyUSBSallnbacks());


    if (!USBSerial.begin())
        Serial.println("Failed to start CDC USB device");

}

void loop()
{
    delay(1000);
}
