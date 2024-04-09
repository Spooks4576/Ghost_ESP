/**
 * This is (al)most complex example
 * Device is using all available on S2 endpoints, with 2 LUNs mounted on FAT 2 partitions
 * DFU for update from website for example, CDC and WebUSB serial and of course Serial from CP210x
 * 
 * author: chegewara
 */
#include "Arduino.h"
#include "webusb.h"
#include "cdcusb.h"
#include "mscusb.h"
#include "dfuusb.h"
#include "flashdisk.h"
#include "hidkeyboard.h"

WebUSB WebUSBSerial;
CDCusb CDCUSBSerial;
HIDkeyboard keyboard;
FlashUSB fat1;
FlashUSB fat2;
DFUusb dfu;

char *l1 = "ffat";
char *l2 = "ffat1";

class MyWebUSBCallbacks : public WebUSBCallbacks{
    void onConnect(bool state) {
      Serial.printf("webusb is %s\n", state ? "connected":"disconnected");
    }
};

class MyCDCCallbacks : public CDCCallbacks {
    void onCodingChange(cdc_line_coding_t const* p_line_coding)
    {
        int bitrate = CDCUSBSerial.getBitrate();
        Serial.printf("new bitrate: %d\n", bitrate);
    }

    bool onConnect(bool dtr, bool rts)
    {
        Serial.printf("connection state changed, dtr: %d, rts: %d\n", dtr, rts);
        return true;  // allow to persist reset, when Arduino IDE is trying to enter bootloader mode
    }

    void onData()
    {
        int len = CDCUSBSerial.available();
        Serial.printf("\nnew data, len %d\n", len);
        uint8_t buf[len] = {};
        CDCUSBSerial.read(buf, len);
        Serial.write(buf, len);
    }
};

class Device: public USBCallbacks {
    void onMount() { Serial.println("Mount"); }
    void onUnmount() { Serial.println("Unmount"); }
    void onSuspend(bool remote_wakeup_en) { Serial.println("Suspend"); }
    void onResume() { Serial.println("Resume"); }
};

class MyHIDCallbacks : public HIDCallbacks
{
    void onData(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
    {
        Serial.printf("ID: %d, type: %d, size: %d\n", report_id, (int)report_type, bufsize);
        for (size_t i = 0; i < bufsize; i++)
        {
            Serial.printf("%d\n", buffer[i]);
        }
    }
};

void setup()
{
    Serial.begin(115200);

    keyboard.setBaseEP(3);
    keyboard.begin();
    keyboard.setCallbacks(new MyHIDCallbacks());

    if (fat1.init("/fat1", "ffat"))
    {
        if (fat1.begin())
        {
            Serial.println("MSC lun 1 begin");
        }
        else
            log_e("LUN 1 failed");
    }
    if (fat2.init("/fat2", "ffat1"))
    {
        if (fat2.begin())
        {
            Serial.println("MSC lun 2 begin");
        }
        else
            log_e("LUN 2 failed");
    }

    WebUSBSerial.setBaseEP(5);

    if (!WebUSBSerial.begin())
        Serial.println("Failed to start webUSB stack");
    if (!CDCUSBSerial.begin())
        Serial.println("Failed to start CDC USB stack");

    WebUSBSerial.setCallbacks(new MyWebUSBCallbacks());
    CDCUSBSerial.setCallbacks(new MyCDCCallbacks());

    dfu.begin();
    
    EspTinyUSB::registerDeviceCallbacks(new Device());


    xTaskCreate(keyboardTask, "kTask", 3*1024, NULL, 5, NULL);
}

void keyboardTask(void* p)
{
    while(1)
    {
        delay(5000);
        Serial.println(keyboard.sendString(String("123456789\n")) ? "OK" : "FAIL");
        delay(5000);
        Serial.println(keyboard.sendString(String("abcdefghijklmnopqrst Uvwxyz\n")) ? "OK" : "FAIL");
    }
}


void echo_all(char c)
{
    WebUSBSerial.write(c);
    CDCUSBSerial.write(c);
    Serial.write(c);
}

void loop()
{
    while (WebUSBSerial.available())
    {
        echo_all(WebUSBSerial.read());
    }
    while (CDCUSBSerial.available())
    {
        echo_all(CDCUSBSerial.read());
    }
    while (Serial.available())
    {
        echo_all(Serial.read());
    }
}