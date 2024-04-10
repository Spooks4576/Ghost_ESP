#pragma once
#if CFG_TUD_HID

#include "hidusb.h"
#include "descriptors/XboxDescriptors.h"

// Button bitmasks
#define XBOX_BUTTON_A 0x01
#define XBOX_BUTTON_B 0x02
// UNUSED - 0x04
#define XBOX_BUTTON_X 0x08 
#define XBOX_BUTTON_Y 0x10
// UNUSED - 0x20
#define XBOX_BUTTON_LB 0x40
#define XBOX_BUTTON_RB 0x80
// UNUSED - 0x100
// UNUSED - 0x200
// UNUSED - 0x400
#define XBOX_BUTTON_SELECT 0x400
#define XBOX_BUTTON_START 0x800
#define XBOX_BUTTON_HOME 0x1000
#define XBOX_BUTTON_LS 0x2000
#define XBOX_BUTTON_RS 0x4000

// Select bitmask
// The share button lives in its own byte at the end of the input report
#define XBOX_BUTTON_SHARE 0x01

// Dpad values
#define XBOX_BUTTON_DPAD_NONE 0x00
#define XBOX_BUTTON_DPAD_NORTH 0x01
#define XBOX_BUTTON_DPAD_NORTHEAST 0x02
#define XBOX_BUTTON_DPAD_EAST 0x03
#define XBOX_BUTTON_DPAD_SOUTHEAST 0x04
#define XBOX_BUTTON_DPAD_SOUTH 0x05
#define XBOX_BUTTON_DPAD_SOUTHWEST 0x06
#define XBOX_BUTTON_DPAD_WEST 0x07
#define XBOX_BUTTON_DPAD_NORTHWEST 0x08

#define XBOX_TRIGGER_MIN 0
#define XBOX_TRIGGER_MAX 1023

// Thumbstick range
#define XBOX_STICK_MIN -32768
#define XBOX_STICK_MAX 32767

#pragma pack(push, 1)
struct XboxGamepadInputReportData {
    uint16_t x = 0;             // Left joystick X
    uint16_t y = 0;             // Left joystick Y
    uint16_t z = 0;             // Right jostick X
    uint16_t rz = 0;            // Right joystick Y
    uint16_t brake = 0;         // 10 bits for brake (left trigger) + 6 bit padding (2 bytes)
    uint16_t accelerator = 0;   // 10 bits for accelerator (right trigger) + 6bit padding
    uint8_t hat = 0x00;         // 4bits for hat switch (Dpad) + 4 bit padding (1 byte) 
    uint16_t buttons = 0x00;    // 15 * 1bit for buttons + 1 bit padding (2 bytes)
    uint8_t share = 0x00;      // 1 bits for share/menu button + 7 bit padding (1 byte)
};
#pragma pack(pop)

enum XinputButton {
    XboxButton_A = 0,
    XboxButton_B = 1,
    XboxButton_X = 2,
    XboxButton_Y = 3,
    XboxButton_LB = 4,
    XboxButton_RB = 5,
    XboxButton_Back = 6,
    XboxButton_Start = 7,
    XboxButton_LeftStick = 8,
    XboxButton_RightStick = 9,
    XboxButton_DPad_Up = 10,
    XboxButton_DPad_Down = 11,
    XboxButton_DPad_Left = 12,
    XboxButton_DPad_Right = 13,
    XboxButton_RT = 14,
    XboxButton_LT = 15,
    XboxButton_Home = 17,
    Xbox_NONE = 18,
};


typedef struct {
    char* name;
    XinputButton state;
} InputStateXinputMapping;


InputStateXinputMapping inputStateXinputMappings[] = {
    {"BUTTON_Y", XboxButton_Y},
    {"BUTTON_B", XboxButton_B},
    {"BUTTON_A", XboxButton_A},
    {"BUTTON_X", XboxButton_X},
    {"BUTTON_BACK", XboxButton_Back},
    {"BUTTON_START", XboxButton_Start},
    {"BUTTON_LBUMPER", XboxButton_LB},
    {"BUTTON_RBUMPER", XboxButton_RB},
    {"BUTTON_LTRIGGER", XboxButton_LT},
    {"BUTTON_RTRIGGER", XboxButton_RT},
    {"BUTTON_LTHUMBSTICK", XboxButton_LeftStick},
    {"BUTTON_RTHUMBSTICK", XboxButton_RightStick},
    {"DPAD_UP", XboxButton_DPad_Up},
    {"DPAD_DOWN", XboxButton_DPad_Down},
    {"DPAD_LEFT", XboxButton_DPad_Left},
    {"DPAD_RIGHT", XboxButton_DPad_Right},
    {"NONE", Xbox_NONE}
};

InputStateXinputMapping StringToXInputInputState(const char* str) {
    int i;
    for (i = 0; i < sizeof(inputStateXinputMappings) / sizeof(InputStateXinputMapping); i++) {
        if (strcmp(str, inputStateXinputMappings[i].name) == 0) {
            return {(char*)str, inputStateXinputMappings[i].state};
        }
    }
    return {"", Xbox_NONE};
}

class HIDXInputUSB : public HIDusb
{
public:
    bool Initialized;
    HIDXInputUSB(uint8_t id = 4);
    bool begin(char* str = nullptr);
    void SetInputState(XinputButton state);


    void sendReport();
    XboxGamepadInputReportData report;
};

#endif