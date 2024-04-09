#include "hidusb.h"

#pragma once
#if CFG_TUD_HID
// | X | Y | Z | Rz | Rx | Ry (1 byte each) | hat/DPAD (1 byte) | Button Map (4 bytes) |
typedef struct {
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t Rz;
    int8_t Rx;
    int8_t Ry;
    uint8_t hat;
    uint32_t buttons;
}__attribute((packed)) hid_gamepad_t;
class HIDgamepad : public HIDusb
{
public:
    HIDgamepad(uint8_t id = 4);
    bool begin(char* str = nullptr);

    void buttons(uint32_t);
    void joystick1(int8_t, int8_t, int8_t);
    void joystick2(int8_t, int8_t, int8_t);
    void sendAll(uint32_t bt, int8_t x, int8_t y, int8_t z, int8_t rx, int8_t ry, int8_t rz, uint8_t hat);
    void hat(uint8_t);

private:
    void sendReport();
    hid_gamepad_t report;
};

#endif
