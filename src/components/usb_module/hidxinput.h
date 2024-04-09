#pragma once
#if CFG_TUD_HID

#include "hidusb.h"

typedef struct
{                                                    // No REPORT ID byte                                                    // Collection: CA:GamePad CP:
  uint16_t GD_GamePadX;                              // Usage 0x00010030: X, Value = 0 to 65535
  uint16_t GD_GamePadY;                              // Usage 0x00010031: Y, Value = 0 to 65535
  uint16_t GD_GamePadRx;                             // Usage 0x00010033: Rx, Value = 0 to 65535
  uint16_t GD_GamePadRy;                             // Usage 0x00010034: Ry, Value = 0 to 65535
                                                     // Collection: CA:GamePad
  uint16_t GD_GamePadZ : 10;                         // Usage 0x00010032: Z, Value = 0 to 1023
  uint8_t  : 6;                                      // Pad
  uint16_t GD_GamePadRz : 10;                        // Usage 0x00010035: Rz, Value = 0 to 1023
  uint8_t  : 6;                                      // Pad
  uint8_t  BTN_GamePadButton1 : 1;                   // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 0
  uint8_t  BTN_GamePadButton2 : 1;                   // Usage 0x00090002: Button 2 Secondary, Value = 0 to 0
  uint8_t  BTN_GamePadButton3 : 1;                   // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 0
  uint8_t  BTN_GamePadButton4 : 1;                   // Usage 0x00090004: Button 4, Value = 0 to 0
  uint8_t  BTN_GamePadButton5 : 1;                   // Usage 0x00090005: Button 5, Value = 0 to 0
  uint8_t  BTN_GamePadButton6 : 1;                   // Usage 0x00090006: Button 6, Value = 0 to 0
  uint8_t  BTN_GamePadButton7 : 1;                   // Usage 0x00090007: Button 7, Value = 0 to 0
  uint8_t  BTN_GamePadButton8 : 1;                   // Usage 0x00090008: Button 8, Value = 0 to 0
  uint8_t  BTN_GamePadButton9 : 1;                   // Usage 0x00090009: Button 9, Value = 0 to 0
  uint8_t  BTN_GamePadButton10 : 1;                  // Usage 0x0009000A: Button 10, Value = 0 to 0
  uint8_t  : 6;                                      // Pad
  uint8_t  GD_GamePadHatSwitch : 4;                  // Usage 0x00010039: Hat switch, Value = 1 to 8, Physical = (Value - 1) x 45 in degrees
  uint8_t  : 4;                                      // Pad
                                                     // Collection: CA:GamePad CP:SystemControl
  uint8_t  GD_GamePadSystemControlSystemMainMenu : 1; // Usage 0x00010085: System Main Menu, Value = 0 to 1
  uint8_t  : 7;                                      // Pad
                                                     // Collection: CA:GamePad
  uint8_t  GEN_GamePadBatteryStrength;               // Usage 0x00060020: Battery Strength, Value = 0 to 255
} __attribute((packed)) XinputReport_t;


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



#define XINPUT_HID_REPORT_DESCRIPTOR \
  0x05, 0x01,  /* Usage Page (Generic Desktop) */ \
  0x09, 0x05,  /* Usage (Game Pad) */ \
  0xA1, 0x01,  /* Collection (Application) */ \
  0x09, 0x30,  /* Usage (X) */ \
  0x09, 0x31,  /* Usage (Y) */ \
  0x15, 0x00,  /* Logical Minimum (0) */ \
  0x27, 0xFF, 0xFF, 0x00, 0x00,  /* Logical Maximum (65535) */ \
  0x95, 0x02,  /* Report Count (2) */ \
  0x75, 0x10,  /* Report Size (16) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x09, 0x33,  /* Usage (Rx) */ \
  0x09, 0x34,  /* Usage (Ry) */ \
  0x95, 0x02,  /* Report Count (2) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x05, 0x01,  /* Usage Page (Generic Desktop) */ \
  0x09, 0x32,  /* Usage (Z) */ \
  0x26, 0xFF, 0x03,  /* Logical Maximum (1023) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x75, 0x0A,  /* Report Size (10) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x09, 0x35,  /* Usage (Rz) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x05, 0x09,  /* Usage Page (Button) */ \
  0x19, 0x01,  /* Usage Minimum (Button 1) */ \
  0x29, 0x0A,  /* Usage Maximum (Button 10) */ \
  0x95, 0x0A,  /* Report Count (10) */ \
  0x75, 0x01,  /* Report Size (1) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x09, 0x39,  /* Usage (Hat switch) */ \
  0x15, 0x01,  /* Logical Minimum (1) */ \
  0x25, 0x08,  /* Logical Maximum (8) */ \
  0x35, 0x00,  /* Physical Minimum (0) */ \
  0x46, 0x3B, 0x01,  /* Physical Maximum (315) */ \
  0x66, 0x14, 0x00,  /* Unit (Rotation in degrees) */ \
  0x75, 0x04,  /* Report Size (4) */ \
  0x81, 0x42,  /* Input (Data, Variable, Absolute, Null state) */ \
  0x09, 0x70,  /* Usage (Magnitude) */ \
  0x25, 0x64,  /* Logical Maximum (100) */ \
  0x75, 0x08,  /* Report Size (8) */ \
  0x95, 0x04,  /* Report Count (4) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x09, 0x50,  /* Usage (Duration) */ \
  0x66, 0x01, 0x10,  /* Unit (Time in seconds) */ \
  0x55, 0x0E,  /* Unit Exponent (-2) */ \
  0x26, 0xFF, 0x00,  /* Logical Maximum (255) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x91, 0x02,  /* Output (Data, Variable, Absolute) */ \
  0x09, 0xA7,  /* Usage (Start Delay) */ \
  0x91, 0x02,  /* Output (Data, Variable, Absolute) */ \
  0x09, 0x7C,  /* Usage (Loop Count) */ \
  0x91, 0x02,  /* Output (Data, Variable, Absolute) */ \
  0xC0,        /* End Collection */ \
  0x05, 0x01,  /* Usage Page (Generic Desktop) */ \
  0x09, 0x80,  /* Usage (System Control) */ \
  0xA1, 0x00,  /* Collection (Physical) */ \
  0x09, 0x85,  /* Usage (System Main Menu) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x75, 0x01,  /* Report Size (1) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0x75, 0x07,  /* Report Size (7) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x81, 0x03,  /* Input (Constant) */ \
  0xC0,        /* End Collection */ \
  0x05, 0x06,  /* Usage Page (Generic Device Controls) */ \
  0x09, 0x20,  /* Usage (Battery Strength) */ \
  0x26, 0xFF, 0x00,  /* Logical Maximum (255) */ \
  0x75, 0x08,  /* Report Size (8) */ \
  0x95, 0x01,  /* Report Count (1) */ \
  0x81, 0x02,  /* Input (Data, Variable, Absolute) */ \
  0xC0         /* End Collection */


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
    XinputReport_t report;
};

#endif