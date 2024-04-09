/**
 * Simple HID keyboard
 * author: chegewara
 */


#include "hidkeyboard.h"
#include "Wire.h"
#if CFG_TUD_HID
#define KEYBOARD_I2C_ADDR     0X5f

HIDkeyboard dev;

void setup()
{
    Serial.begin(115200);
    Wire.begin(7,8);
    dev.begin();
}

void loop()
{
    delay(10);
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);  // request 1 byte from keyboard
    while (Wire.available()) { 
      uint8_t key_val = Wire.read();                  // receive a byte as character
      if(key_val != 0) {
        dev.sendChar(key_val); // send ASCII char
      }
    }
}

#endif
