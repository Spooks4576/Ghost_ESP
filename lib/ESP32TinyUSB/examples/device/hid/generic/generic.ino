/**
 * Simple vendor In/Out HID device
 * author: chegewara
 */

#include "hidgeneric.h"
#if CFG_TUD_HID
HIDgeneric dev;

class MyHIDCallbacks: public HIDCallbacks{
  void onData(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    Serial.printf("ID: %d, type: %d, size: %d\n", report_id, (int)report_type, bufsize);  
     for (size_t i = 0; i < bufsize; i++)
    {
        Serial.printf("%c", buffer[i]);
    }
Serial.println();
  }
};

void setup()
{
    Serial.begin(115200);
    dev.begin();
    dev.setCallbacks(new MyHIDCallbacks());
}

void loop()
{
    delay(1000);
    dev.write("test", 4);
}

#endif
