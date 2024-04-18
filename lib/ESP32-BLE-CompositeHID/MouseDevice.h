
#ifndef ESP32_MOUSE_DEVICE_H
#define ESP32_MOUSE_DEVICE_H

#include "NimBLECharacteristic.h"
#include <MouseConfiguration.h>
#include <BaseCompositeDevice.h>
#include <mutex>

class MouseDevice : public BaseCompositeDevice {
private:
    MouseConfiguration _config;
    NimBLECharacteristic* _input;
    NimBLECharacteristic* _output;

    uint8_t _mouseButtons[16]; // 8 bits x 16 --> 128 bits
    signed char _mouseX;
    signed char _mouseY;
    signed char _mouseWheel;
    signed char _mouseHWheel;

public:
    MouseDevice();
    MouseDevice(const MouseConfiguration& config);
    
    void init(NimBLEHIDDevice* hid) override;
    const BaseCompositeDeviceConfiguration* getDeviceConfig() const override;

    void resetButtons();
    void mouseClick(uint8_t button = MOUSE_LOGICAL_LEFT_BUTTON);
    void mousePress(uint8_t button = MOUSE_LOGICAL_LEFT_BUTTON);
    void mouseRelease(uint8_t button = MOUSE_LOGICAL_LEFT_BUTTON);
    void mouseMove(signed char x, signed char y, signed char scrollX = 0, signed char scrollY = 0);
    
    void sendMouseReport(bool defer = false);

private:
    void sendMouseReportImpl();

    // Threading
    std::mutex _mutex;
};

#endif