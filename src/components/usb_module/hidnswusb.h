#include "hidusb.h"

#pragma once
#if CFG_TUD_HID

typedef struct {
    uint8_t buttonCombos;      // Combined states of buttons, bumpers, and triggers
                               // X = 08, Y = 01, A = 04, B = 02,
                               // Right Bumper = 20, Left Bumper = 10,
                               // Left Trigger = 40, Right Trigger = 80
    uint8_t additionalButtons; // Combined states of other buttons
                               // - = 01, + = 02, Home = 10, Share = 20,
                               // Left Thumbstick = 04, Right Thumbstick = 08
    uint8_t dpad;              // D-pad state: 00 (Up), 02 (Right), 04 (Down), 06 (Left), 0F (Neutral)
    uint8_t leftStickX;       // Left Stick horizontal movement: 00 (left) to FF (right), 80 (neutral)
    uint8_t leftStickY;       // Left Stick vertical movement: 00 (up) to FF (down), 80 (neutral)
    uint8_t rightStickX;      // Right Stick horizontal movement: 00 (left) to FF (right), 80 (neutral)
    uint8_t rightStickY;      // Right Stick vertical movement: 00 (up) to FF (down), 80 (neutral)
    uint8_t reserved;
} __attribute((packed)) hid_nsw_report_t;


typedef enum {
    BUTTON_Y,
    BUTTON_B,
    BUTTON_A,
    BUTTON_X,
    BUTTON_MINUS,
    BUTTON_PLUS,
    BUTTON_HOME,
    BUTTON_SHARE,
    BUTTON_LBUMPER,
    BUTTON_RBUMPER,
    BUTTON_LTRIGGER,
    BUTTON_RTRIGGER,
    BUTTON_LTHUMBSTICK,
    BUTTON_RTHUMBSTICK,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
    LSTICK_UP,
    LSTICK_DOWN,
    LSTICK_LEFT,
    LSTICK_RIGHT,
    RSTICK_UP,
    RSTICK_DOWN,
    RSTICK_LEFT,
    RSTICK_RIGHT,
    NONE
} SwitchButton;

#define NSW_HID_REPORT_DESCRIPTOR \
  0x05, 0x01,                    /* Usage Page (Generic Desktop) */ \
  0x09, 0x05,                    /* Usage (Game Pad) */ \
  0xA1, 0x01,                    /* Collection (Application) */ \
  0x15, 0x00,                    /* Logical Minimum (0) */ \
  0x25, 0x01,                    /* Logical Maximum (1) */ \
  0x35, 0x00,                    /* Physical Minimum (0) */ \
  0x45, 0x01,                    /* Physical Maximum (1) */ \
  0x75, 0x01,                    /* Report Size (1) */ \
  0x95, 0x0E,                    /* Report Count (14) */ \
  0x05, 0x09,                    /* Usage Page (Button) */ \
  0x19, 0x01,                    /* Usage Minimum (1) */ \
  0x29, 0x0E,                    /* Usage Maximum (14) */ \
  0x81, 0x02,                    /* Input (Data, Var, Abs) */ \
  0x95, 0x02,                    /* Report Count (2) */ \
  0x81, 0x01,                    /* Input (Const, Var, Abs) */ \
  0x05, 0x01,                    /* Usage Page (Generic Desktop) */ \
  0x25, 0x07,                    /* Logical Maximum (7) */ \
  0x46, 0x3B, 0x01,              /* Physical Maximum (315) */ \
  0x75, 0x04,                    /* Report Size (4) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x65, 0x14,                    /* Unit (Eng Rot:Angular Pos) */ \
  0x09, 0x39,                    /* Usage (Hat switch) */ \
  0x81, 0x42,                    /* Input (Data,Var,Abs,Null) */ \
  0x65, 0x00,                    /* Unit (None) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x81, 0x01,                    /* Input (Const,Var,Abs) */ \
  0x26, 0xFF, 0x00,              /* Logical Maximum (255) */ \
  0x46, 0xFF, 0x00,              /* Physical Maximum (255) */ \
  0x09, 0x30,                    /* Usage (X) */ \
  0x09, 0x31,                    /* Usage (Y) */ \
  0x09, 0x32,                    /* Usage (Z) */ \
  0x09, 0x35,                    /* Usage (Rz) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x04,                    /* Report Count (4) */ \
  0x81, 0x02,                    /* Input (Data, Var, Abs) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x81, 0x01,                    /* Input (Const, Var, Abs) */ \
  0x05, 0x0C,                    /* Usage Page (Consumer) */ \
  0x09, 0x00,                    /* Usage (Undefined) */ \
  0x15, 0x80,                    /* Logical Minimum (-128) */ \
  0x25, 0x7F,                    /* Logical Maximum (127) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x40,                    /* Report Count (64) */ \
  0xB1, 0x02,                    /* Feature (Data, Var, Abs) */ \
  0xC0                           /* End Collection */


typedef struct {
    char* name;
    SwitchButton state;
} InputStateNSWMapping;


InputStateNSWMapping inputStateNSWMappings[] = {
    {"BUTTON_Y", BUTTON_Y},
    {"BUTTON_B", BUTTON_B},
    {"BUTTON_A", BUTTON_A},
    {"BUTTON_X", BUTTON_X},
    {"BUTTON_MINUS", BUTTON_MINUS},
    {"BUTTON_PLUS", BUTTON_PLUS},
    {"BUTTON_HOME", BUTTON_HOME},
    {"BUTTON_SHARE", BUTTON_SHARE},
    {"BUTTON_LBUMPER", BUTTON_LBUMPER},
    {"BUTTON_RBUMPER", BUTTON_RBUMPER},
    {"BUTTON_LTRIGGER", BUTTON_LTRIGGER},
    {"BUTTON_RTRIGGER", BUTTON_RTRIGGER},
    {"BUTTON_LTHUMBSTICK", BUTTON_LTHUMBSTICK},
    {"BUTTON_RTHUMBSTICK", BUTTON_RTHUMBSTICK},
    {"DPAD_UP", DPAD_UP},
    {"DPAD_DOWN", DPAD_DOWN},
    {"DPAD_LEFT", DPAD_LEFT},
    {"DPAD_RIGHT", DPAD_RIGHT},
    {"LSTICK_UP", LSTICK_UP},
    {"LSTICK_DOWN", LSTICK_DOWN},
    {"LSTICK_LEFT", LSTICK_LEFT},
    {"LSTICK_RIGHT", LSTICK_RIGHT},
    {"RSTICK_UP", RSTICK_UP},
    {"RSTICK_DOWN", RSTICK_DOWN},
    {"RSTICK_LEFT", RSTICK_LEFT},
    {"RSTICK_RIGHT", RSTICK_RIGHT},
    {"NONE", NONE}
};

InputStateNSWMapping StringToNSWInputState(const char* str) {
    int i;
    for (i = 0; i < sizeof(inputStateNSWMappings) / sizeof(InputStateNSWMapping); i++) {
        if (strcmp(str, inputStateNSWMappings[i].name) == 0) {
            return {(char*)str, inputStateNSWMappings[i].state};
        }
    }
    return {"", NONE};
}

class HIDNSWUSB : public HIDusb
{
public:
    bool Initlized;
    HIDNSWUSB(uint8_t id = 4);
    bool begin(char* str = nullptr);
    void SetInputState(SwitchButton state);


    void sendReport();
    hid_nsw_report_t report;
};

#endif
