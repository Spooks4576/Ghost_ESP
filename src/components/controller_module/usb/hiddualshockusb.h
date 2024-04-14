#pragma once
#if CFG_TUD_HID

#include "hidusb.h"
#include "../descriptors/PS4Descriptors.h"


typedef struct {
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t buttons1; // This indeed should cover D-PAD and the four face buttons (triangle, circle, cross, square)
    uint8_t buttons2; // Correct for R3, L3, option, share, R2, L2, R1, L1 buttons
    uint8_t buttons3; // Correct for counter, T-PAD click, PS button
    uint8_t l2Trigger;
    uint8_t r2Trigger;
    uint16_t timestamp;
    uint8_t batteryLevel;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    uint8_t unknown1[5];
    uint8_t headsetInfo;
    uint8_t unknown2[3]; // Correctly includes the space for future products and control commands
    uint8_t tPadEvent;
    uint8_t tPadLastUpdate;
    uint8_t finger1Status;
    uint16_t finger1X:12; // Bit-fields for X and Y coordinates can't be split across byte boundaries directly in a standard way
    uint16_t finger1Y:12; // This approach may not be portable or work as intended
    uint8_t finger2Status;
    uint16_t finger2X:12; // Same issue as with finger1X
    uint16_t finger2Y:12; // Same issue as with finger1Y
    uint8_t previousFinger1[4]; // Needs clarification on tracking numbers and positions being able to fit in this format
    uint8_t previousFinger2[4]; // Similar concern as previousFinger1
    uint8_t todo[12]; // Placeholder for unassigned bytes
} __attribute__((packed)) ps4_hid_report;

typedef enum {
    PS4_BUTTON_TRIANGLE,
    PS4_BUTTON_CIRCLE,
    PS4_BUTTON_CROSS,
    PS4_BUTTON_SQUARE,
    PS4_BUTTON_SHARE,
    PS4_BUTTON_OPTIONS,
    PS4_BUTTON_PS,
    PS4_BUTTON_TOUCHPAD,
    PS4_BUTTON_L1,
    PS4_BUTTON_R1,
    PS4_BUTTON_L2,
    PS4_BUTTON_R2,
    PS4_BUTTON_L3,
    PS4_BUTTON_R3,
    PS4_DPAD_UP,
    PS4_DPAD_DOWN,
    PS4_DPAD_LEFT,
    PS4_DPAD_RIGHT,
    PS4_LSTICK_UP,
    PS4_LSTICK_DOWN,
    PS4_LSTICK_LEFT,
    PS4_LSTICK_RIGHT,
    PS4_RSTICK_UP,
    PS4_RSTICK_DOWN,
    PS4_RSTICK_LEFT,
    PS4_RSTICK_RIGHT,
    PS4_NONE
} PS4Button;


typedef struct {
    char* name;
    PS4Button state;
} InputStateDualShockMapping;


InputStateDualShockMapping inputStateDualShockMappings[] = {
    {"BUTTON_Y", PS4_BUTTON_TRIANGLE},
    {"BUTTON_B", PS4_BUTTON_CIRCLE},
    {"BUTTON_A", PS4_BUTTON_CROSS},
    {"BUTTON_X", PS4_BUTTON_SQUARE},
    {"BUTTON_MINUS", PS4_BUTTON_SHARE},
    {"BUTTON_PLUS", PS4_BUTTON_OPTIONS},
    {"BUTTON_HOME", PS4_BUTTON_PS},
    {"BUTTON_SHARE", PS4_BUTTON_TOUCHPAD},
    {"BUTTON_LBUMPER", PS4_BUTTON_L1},
    {"BUTTON_RBUMPER", PS4_BUTTON_R1},
    {"BUTTON_LTRIGGER", PS4_BUTTON_L2},
    {"BUTTON_RTRIGGER", PS4_BUTTON_R2},
    {"BUTTON_LTHUMBSTICK", PS4_BUTTON_L3},
    {"BUTTON_RTHUMBSTICK", PS4_BUTTON_R3},
    {"DPAD_UP", PS4_DPAD_UP},
    {"DPAD_DOWN", PS4_DPAD_DOWN},
    {"DPAD_LEFT", PS4_DPAD_LEFT},
    {"DPAD_RIGHT", PS4_DPAD_RIGHT},
    {"LSTICK_UP", PS4_LSTICK_UP},
    {"LSTICK_DOWN", PS4_LSTICK_DOWN},
    {"LSTICK_LEFT", PS4_LSTICK_LEFT},
    {"LSTICK_RIGHT", PS4_LSTICK_RIGHT},
    {"RSTICK_UP", PS4_RSTICK_UP},
    {"RSTICK_DOWN", PS4_RSTICK_DOWN},
    {"RSTICK_LEFT", PS4_RSTICK_LEFT},
    {"RSTICK_RIGHT", PS4_RSTICK_RIGHT},
    {"NONE", PS4_NONE}
};

InputStateDualShockMapping StringToDualShockInputState(const char* str) {
    int i;
    for (i = 0; i < sizeof(inputStateDualShockMappings) / sizeof(InputStateDualShockMapping); i++) {
        if (strcmp(str, inputStateDualShockMappings[i].name) == 0) {
            return {(char*)str, inputStateDualShockMappings[i].state};
        }
    }
    return {"", PS4_NONE};
}

class HIDDualShockUSB : public HIDusb
{
public:
    bool Initialized;
    HIDDualShockUSB(uint8_t id = 4);
    bool begin(char* str = nullptr);
    void SetInputState(PS4Button state);


    void sendReport();
    ps4_hid_report report;
};

#endif