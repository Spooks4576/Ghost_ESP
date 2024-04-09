/**
 * Simple HID keyboard
 * author: chegewara
 */

#include "hidkeyboard.h"
#if CFG_TUD_HID
HIDkeyboard dev;

class MyHIDCallbacks: public HIDCallbacks{
  void onData(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    Serial.printf("ID: %d, type: %d, size: %d\n", report_id, (int)report_type, bufsize);  
     for (size_t i = 0; i < bufsize; i++)
    {
        Serial.printf("%d\n", buffer[i]);
    }   
  }
};

void dataCB(uint8_t report_id, uint8_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    for (size_t i = 0; i < bufsize; i++)
    {
        Serial.printf("%d\n", buffer[i]);
        Serial.printf("%c\n", buffer[i]);
    }
}


void setup()
{
    Serial.begin(115200);
    dev.begin();
    dev.setCallbacks(new MyHIDCallbacks());
}

void loop()
{
    delay(1000);
    dev.sendKey(HID_KEY_A);
    delay(1000);
    Serial.println(dev.sendString(String("123456789\n"))?"OK":"FAIL");
    delay(1000);
    Serial.println(dev.sendString(String("abcdefghijklmnopqrst Uvwxyz\n"))?"OK":"FAIL");
}

#endif
