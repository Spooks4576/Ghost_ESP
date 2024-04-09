
#include "hidusb.h"

#pragma once
#define LEFT_BTN    1
#define RIGHT_BTN   2
#define MIDDLE_BTN  3
#define BACK_BTN    4
#define FORWARD_BTN 5
#if CFG_TUD_HID

class HIDmouse : public HIDusb
{
public:
    HIDmouse(uint8_t id = 2);
    bool begin(char* str = nullptr);

    void buttons(uint8_t buttons);
    void pressLeft();
    void pressMiddle();
    void pressRight();
    void doublePressLeft();
    void backwardBtn();
    void forwardBtn();
    void scrollUp(uint8_t);
    void scrollDown(uint8_t);

    void move(int8_t x, int8_t y);
    void wheel(int8_t x, int8_t y);

    uint8_t button;
};

#endif
