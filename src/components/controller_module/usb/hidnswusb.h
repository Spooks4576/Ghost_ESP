#pragma once
#if CFG_TUD_HID

#include "hidusb.h"
#include "../descriptors/NSWDescriptors.h"

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
    bool Initialized;
    HIDNSWUSB(uint8_t id = 4);
    bool begin(char* str = nullptr);
    void SetInputState(SwitchButton state);


    void sendReport();
    hid_nsw_report_t report;
};

#endif
