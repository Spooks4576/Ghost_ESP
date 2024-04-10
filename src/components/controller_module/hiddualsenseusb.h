#pragma once
#if CFG_TUD_HID

#include "hidusb.h"
#include "descriptors/PS5Descriptors.h"

typedef struct {
    uint8_t axes0;                  // Left stick X axis
    uint8_t axes1;                  // Left stick Y axis
    uint8_t axes2;                  // Right stick X axis
    uint8_t axes3;                  // Right stick Y axis
    uint8_t axes4;                  // L2 Trigger
    uint8_t axes5;                  // R2 Trigger
    uint8_t seqNum;                 // Sequence Number (possibly for tracking reports)
    uint8_t buttons0;               // D-Pad and main buttons (Square, Cross, Circle, Triangle)
    uint8_t buttons1;               // Other buttons (L1, R1, L2, R2, Share, Options, L3, R3)
    uint8_t buttons2;               // System buttons (PS, Touchpad, Mute)
    uint8_t buttons3;               // Additional buttons if any (or can be used for extended features)
    uint8_t timestamp0;             // Timestamp or other time-related data, first byte
    uint8_t timestamp1;             // Timestamp, second byte
    uint8_t timestamp2;             // Timestamp, third byte
    uint8_t timestamp3;             // Timestamp, fourth byte
    uint8_t gyroX0;                 // Gyro X, first byte
    uint8_t gyroX1;                 // Gyro X, second byte
    uint8_t gyroY0;                 // Gyro Y, first byte
    uint8_t gyroY1;                 // Gyro Y, second byte
    uint8_t gyroZ0;                 // Gyro Z, first byte
    uint8_t gyroZ1;                 // Gyro Z, second byte
    uint8_t accelX0;                // Accelerometer X, first byte
    uint8_t accelX1;                // Accelerometer X, second byte
    uint8_t accelY0;                // Accelerometer Y, first byte
    uint8_t accelY1;                // Accelerometer Y, second byte
    uint8_t accelZ0;                // Accelerometer Z, first byte
    uint8_t accelZ1;                // Accelerometer Z, second byte
    uint8_t sensorTimestamp0;       // Sensor Timestamp, first byte
    uint8_t sensorTimestamp1;       // Sensor Timestamp, second byte
    uint8_t sensorTimestamp2;       // Sensor Timestamp, third byte
    uint8_t sensorTimestamp3;       // Sensor Timestamp, fourth byte
    uint8_t unknown31;
    uint8_t touch00;                // Touchpad data 0, first byte
    uint8_t touch01;                // Touchpad data 0, second byte
    uint8_t touch02;                // Touchpad data 0, third byte
    uint8_t touch03;                // Touchpad data 0, fourth byte
    uint8_t touch10;                // Touchpad data 1, first byte
    uint8_t touch11;                // Touchpad data 1, second byte
    uint8_t touch12;                // Touchpad data 1, third byte
    uint8_t touch13;                // Touchpad data 1, fourth byte
    uint8_t unknown40;
    uint8_t r2feedback;             // R2 feedback, possibly for adaptive triggers
    uint8_t l2feedback;             // L2 feedback, possibly for adaptive triggers
    uint8_t unknown43;
    uint8_t unknown44;
    uint8_t unknown45;
    uint8_t unknown46;
    uint8_t unknown47;
    uint8_t unknown48;
    uint8_t unknown49;
    uint8_t unknown50;
    uint8_t unknown51;
    uint8_t battery0;               // Battery level, first byte
    uint8_t battery1;               // Additional battery information, if applicable
    uint8_t unknown54;
    uint8_t unknown55;
    uint8_t unknown56;
    uint8_t unknown57;
    uint8_t unknown58;
    uint32_t checksum;
} __attribute__((packed)) ps5_hid_report;

typedef enum {
    PS5_BUTTON_TRIANGLE,
    PS5_BUTTON_CIRCLE,
    PS5_BUTTON_CROSS,
    PS5_BUTTON_SQUARE,
    PS5_BUTTON_SHARE,
    PS5_BUTTON_OPTIONS,
    PS5_BUTTON_PS,
    PS5_BUTTON_TOUCHPAD,
    PS5_BUTTON_L1,
    PS5_BUTTON_R1,
    PS5_BUTTON_L2,
    PS5_BUTTON_R2,
    PS5_BUTTON_L3,
    PS5_BUTTON_R3,
    PS5_DPAD_UP,
    PS5_DPAD_DOWN,
    PS5_DPAD_LEFT,
    PS5_DPAD_RIGHT,
    PS5_LSTICK_UP,
    PS5_LSTICK_DOWN,
    PS5_LSTICK_LEFT,
    PS5_LSTICK_RIGHT,
    PS5_RSTICK_UP,
    PS5_RSTICK_DOWN,
    PS5_RSTICK_LEFT,
    PS5_RSTICK_RIGHT,
    PS5_NONE
} PS5Button;


typedef struct {
    char* name;
    PS5Button state;
} InputStateDualSenseMapping;


InputStateDualSenseMapping inputStateDualSenseMappings[] = {
    {"BUTTON_Y", PS5_BUTTON_TRIANGLE},
    {"BUTTON_B", PS5_BUTTON_CIRCLE},
    {"BUTTON_A", PS5_BUTTON_CROSS},
    {"BUTTON_X", PS5_BUTTON_SQUARE},
    {"BUTTON_MINUS", PS5_BUTTON_SHARE},
    {"BUTTON_PLUS", PS5_BUTTON_OPTIONS},
    {"BUTTON_HOME", PS5_BUTTON_PS},
    {"BUTTON_SHARE", PS5_BUTTON_TOUCHPAD},
    {"BUTTON_LBUMPER", PS5_BUTTON_L1},
    {"BUTTON_RBUMPER", PS5_BUTTON_R1},
    {"BUTTON_LTRIGGER", PS5_BUTTON_L2},
    {"BUTTON_RTRIGGER", PS5_BUTTON_R2},
    {"BUTTON_LTHUMBSTICK", PS5_BUTTON_L3},
    {"BUTTON_RTHUMBSTICK", PS5_BUTTON_R3},
    {"DPAD_UP", PS5_DPAD_UP},
    {"DPAD_DOWN", PS5_DPAD_DOWN},
    {"DPAD_LEFT", PS5_DPAD_LEFT},
    {"DPAD_RIGHT", PS5_DPAD_RIGHT},
    {"LSTICK_UP", PS5_LSTICK_UP},
    {"LSTICK_DOWN", PS5_LSTICK_DOWN},
    {"LSTICK_LEFT", PS5_LSTICK_LEFT},
    {"LSTICK_RIGHT", PS5_LSTICK_RIGHT},
    {"RSTICK_UP", PS5_RSTICK_UP},
    {"RSTICK_DOWN", PS5_RSTICK_DOWN},
    {"RSTICK_LEFT", PS5_RSTICK_LEFT},
    {"RSTICK_RIGHT", PS5_RSTICK_RIGHT},
    {"NONE", PS5_NONE}
};

InputStateDualSenseMapping StringToDualSenseInputState(const char* str) {
    int i;
    for (i = 0; i < sizeof(inputStateDualSenseMappings) / sizeof(InputStateDualSenseMapping); i++) {
        if (strcmp(str, inputStateDualSenseMappings[i].name) == 0) {
            return {(char*)str, inputStateDualSenseMappings[i].state};
        }
    }
    return {"", PS5_NONE};
}

class HIDDualSenseUSB : public HIDusb
{
public:
    bool Initialized;
    HIDDualSenseUSB(uint8_t id = 4);
    bool begin(char* str = nullptr);
    void SetInputState(PS5Button state);
    void fillDualSenseChecksum();

    void sendReport();
    ps5_hid_report report;
};

#endif