
#include "hidusb.h"
#include "hidkeylayout.h"

#pragma once
#if CFG_TUD_HID

class HIDkeyboard : public HIDusb
{
public:
    HIDkeyboard(uint8_t id = 3);
    bool begin(char* str = nullptr);

    bool sendKey(uint8_t _keycode, uint8_t modifier = 0);
    bool sendChar(uint8_t _keycode);
    bool sendPress(uint8_t _keycode, uint8_t modifier = 0);
    bool sendRelease();
    bool sendString(const char* text);
    bool sendString(String text);
};

#endif
