#ifndef ESP32_KEYBOARD_DEVICE_H
#define ESP32_KEYBOARD_DEVICE_H

#include "NimBLECharacteristic.h"
#include <KeyboardHIDCodes.h>
#include <KeyboardConfiguration.h>
#include <BaseCompositeDevice.h>
#include <mutex>

struct KeyboardInputReport {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6]; // 8 bits per key * 101 keys = 6 bytes
};

struct KeyboardMediaInputReport {
    uint32_t keys;
};

struct KeyboardOutputReport {
    bool numLockActive;
    bool capsLockActive;
    bool scrollLockActive;
    bool composeActive;
    bool kanaActive;

    constexpr KeyboardOutputReport(uint8_t value = 0) noexcept : 
        numLockActive(value & KEY_LED_NUMLOCK),
        capsLockActive(value & KEY_LED_CAPSLOCK),
        scrollLockActive(value & KEY_LED_SCROLLLOCK),
        composeActive(value & KEY_LED_COMPOSE),
        kanaActive(value & KEY_LED_KANA)
    {}
};

// Forwards 
class KeyboardDevice;

class KeyboardCallbacks : public NimBLECharacteristicCallbacks {
public:
    KeyboardCallbacks(KeyboardDevice* device);

    void onWrite(NimBLECharacteristic* pCharacteristic) override;
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void onNotify(NimBLECharacteristic* pCharacteristic) override;
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) override;

private:
    KeyboardDevice* _device;
};


class KeyboardDevice : public BaseCompositeDevice {
private:
    KeyboardConfiguration _config;
    NimBLECharacteristic* _input;
    NimBLECharacteristic* _mediaInput;
    NimBLECharacteristic* _output;

    KeyboardInputReport _inputReport;
    KeyboardMediaInputReport _mediaKeyInputReport;
    KeyboardCallbacks* _callbacks;

public:
    KeyboardDevice();
    KeyboardDevice(const KeyboardConfiguration& config);
    ~KeyboardDevice();
    
    void init(NimBLEHIDDevice* hid) override;
    const BaseCompositeDeviceConfiguration* getDeviceConfig() const override;

    void resetKeys();

    void keyPress(uint8_t keyCode);
    void keyRelease(uint8_t keyCode);
    void modifierKeyPress(uint8_t modifier);
    void modifierKeyRelease(uint8_t modifier);
    void mediaKeyPress(uint32_t mediaKey);
    void mediaKeyRelease(uint32_t mediaKey);

    void sendKeyReport(bool defer = false);
    void sendMediaKeyReport(bool defer = false);

private:
    void sendKeyReportImpl();
    void sendMediaKeyReportImpl();

    // Threading
    std::mutex _mutex;
};

#endif