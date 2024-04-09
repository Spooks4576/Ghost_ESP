/**
 * Simple webUSB device to echo with tinyusb webecho terminal
 * VID = 0xcafe to match filter on website
 * author: chegewara
 */
#include "webusb.h"
#if CFG_TUD_VENDOR
WebUSB USBSerial;

class MyWebUSBCallbacks : public WebUSBCallbacks{
    void onConnect(bool state) {
      Serial.printf("webusb is %s\n", state ? "connected":"disconnected");
    }
};

void setup() {
    Serial.begin(115200);
    USBSerial.deviceID(0xcafe, 0x0002);

    USBSerial.setCallbacks(new MyWebUSBCallbacks());

    if(!USBSerial.begin())
        Serial.println("Failed to start webUSB stack");

}

void echo_all(char c)
{
    Serial.write(c);
    USBSerial.write(c);
}

void loop() {
    while (USBSerial.available())
    {
        echo_all(USBSerial.read());
    }

    while (Serial.available())
    {
        echo_all(Serial.read());
    }
}

#endif
