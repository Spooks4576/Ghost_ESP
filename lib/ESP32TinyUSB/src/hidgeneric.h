
#include "hidusb.h"

#pragma once
#if CFG_TUD_HID

class HIDgeneric : public HIDusb
{
public:
    HIDgeneric(uint8_t id = 1);
    bool begin(char* str = nullptr);
};

#endif
