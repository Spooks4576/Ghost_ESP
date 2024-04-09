
#include "hidusb.h"
#include "hidkeylayout.h"
#pragma once
#define LEFT_BTN    1
#define RIGHT_BTN   2
#define MIDDLE_BTN  3
#define BACK_BTN    4
#define FORWARD_BTN 5
#if CFG_TUD_HID

class HIDcomposite : public HIDusb
{
public:
    HIDcomposite(uint8_t id = 2);
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

    bool sendKey(uint8_t _keycode, uint8_t modifier = 0);
    bool sendChar(uint8_t _keycode);
    bool sendPress(uint8_t _keycode, uint8_t modifier = 0);
    bool sendRelease();
    bool sendString(const char* text);
    bool sendString(String text);

private:
    uint8_t report_mouse;
    uint8_t report_keyboard;
    uint8_t button;
};

#endif
