#pragma once

#include "btcontroller_i.h"
#include "../descriptors/PS5Descriptors.h"

typedef struct
{
    uint8_t x;           // Left stick X axis: left (0x00), right (0xFF), neutral (~0x80)
    uint8_t y;           // Left stick Y axis: up (0x00), down (0xFF), neutral (~0x80)
    uint8_t z;           // Right stick X axis: left (0x00), right (0xFF), neutral (~0x80)
    uint8_t rz;          // Right stick Y axis: up (0x00), down (0xFF), neutral (~0x80)
    uint8_t buttons1;    // Includes hat switch (4 bits) + button square, cross, circle, triangle (1 bit each)
    uint8_t buttons2;    // Includes L1, R1, L2, R2, Create, Options, L3, R3 (1 bit each)
    uint8_t buttons3;    // Includes PS, Touchpad (1 bit each), reserved (6 bits)
    uint8_t rx;          // L2 axis: neutral (0x00), pressed (0xFF)
    uint8_t ry;          // R2 axis: neutral (0x00), pressed (0xFF)
} __attribute__((packed)) ps_hid_bt_report;


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
} __attribute__((packed)) ps4_hid_usb_report;

typedef enum {
    PS_BUTTON_TRIANGLE,
    PS_BUTTON_CIRCLE,
    PS_BUTTON_CROSS,
    PS_BUTTON_SQUARE,
    PS_BUTTON_SHARE,
    PS_BUTTON_OPTIONS,
    PS_BUTTON_PS,
    PS_BUTTON_TOUCHPAD,
    PS_BUTTON_L1,
    PS_BUTTON_R1,
    PS_BUTTON_L2,
    PS_BUTTON_R2,
    PS_BUTTON_L3,
    PS_BUTTON_R3,
    PS_DPAD_UP,
    PS_DPAD_DOWN,
    PS_DPAD_LEFT,
    PS_DPAD_RIGHT,
    PS_LSTICK_UP,
    PS_LSTICK_DOWN,
    PS_LSTICK_LEFT,
    PS_LSTICK_RIGHT,
    PS_RSTICK_UP,
    PS_RSTICK_DOWN,
    PS_RSTICK_LEFT,
    PS_RSTICK_RIGHT,
    PS_NONE
} PSButton;


typedef struct {
    char* name;
    PSButton state;
} InputStatePSMapping;


InputStatePSMapping inputStatePSMappings[] = {
    {"BUTTON_Y", PS_BUTTON_TRIANGLE},
    {"BUTTON_B", PS_BUTTON_CIRCLE},
    {"BUTTON_A", PS_BUTTON_CROSS},
    {"BUTTON_X", PS_BUTTON_SQUARE},
    {"BUTTON_MINUS", PS_BUTTON_SHARE},
    {"BUTTON_PLUS", PS_BUTTON_OPTIONS},
    {"BUTTON_HOME", PS_BUTTON_PS},
    {"BUTTON_SHARE", PS_BUTTON_TOUCHPAD},
    {"BUTTON_LBUMPER", PS_BUTTON_L1},
    {"BUTTON_RBUMPER", PS_BUTTON_R1},
    {"BUTTON_LTRIGGER", PS_BUTTON_L2},
    {"BUTTON_RTRIGGER", PS_BUTTON_R2},
    {"BUTTON_LTHUMBSTICK", PS_BUTTON_L3},
    {"BUTTON_RTHUMBSTICK", PS_BUTTON_R3},
    {"DPAD_UP", PS_DPAD_UP},
    {"DPAD_DOWN", PS_DPAD_DOWN},
    {"DPAD_LEFT", PS_DPAD_LEFT},
    {"DPAD_RIGHT", PS_DPAD_RIGHT},
    {"LSTICK_UP", PS_LSTICK_UP},
    {"LSTICK_DOWN", PS_LSTICK_DOWN},
    {"LSTICK_LEFT", PS_LSTICK_LEFT},
    {"LSTICK_RIGHT", PS_LSTICK_RIGHT},
    {"RSTICK_UP", PS_RSTICK_UP},
    {"RSTICK_DOWN", PS_RSTICK_DOWN},
    {"RSTICK_LEFT", PS_RSTICK_LEFT},
    {"RSTICK_RIGHT", PS_RSTICK_RIGHT},
    {"NONE", PS_NONE}
};

InputStatePSMapping StringToPSInputState(const char* str) {
    int i;
    for (i = 0; i < sizeof(inputStatePSMappings) / sizeof(InputStatePSMapping); i++) {
        if (strcmp(str, inputStatePSMappings[i].name) == 0) {
            return {(char*)str, inputStatePSMappings[i].state};
        }
    }
    return {"", PS_NONE};
}

#ifdef HAS_BT

class PSController : public BTHIDDevice, public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks
{
public:
    virtual void connect() override;
    virtual void sendreport() override;
    virtual void onConnect(NimBLEServer* pServer) override;
    virtual void onDisconnect(NimBLEServer* pServer) override;
    virtual void onWrite(NimBLECharacteristic* me) override;
    void SetInputState(PSButton state);

    ps_hid_bt_report* report;
};

#endif

