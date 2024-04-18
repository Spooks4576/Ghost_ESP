#ifndef ESP32_BLE_MOUSE_CONFIG_H
#define ESP32_BLE_MOUSE_CONFIG_H

#include <BaseCompositeDevice.h>
#include <MouseConfiguration.h>

#define MOUSE_REPORT_ID 0x20
#define MOUSE_DEVICE_NAME "Mouse"

#define MOUSE_LOGICAL_LEFT_BUTTON 0x01
#define MOUSE_LOGICAL_RIGHT_BUTTON 0x02
#define MOUSE_LOGICAL_BUTTON_3 0x03
#define MOUSE_LOGICAL_BUTTON_4 0x04
#define MOUSE_LOGICAL_BUTTON_5 0x05

#define MOUSE_DEFAULT_AXIS_COUNT 2
#define MOUSE_POSSIBLE_AXIS_COUNT 8

// Keyboard

class MouseConfiguration : public BaseCompositeDeviceConfiguration
{
private:
    uint16_t _buttonCount;
    bool _whichAxes[MOUSE_POSSIBLE_AXIS_COUNT];
    uint16_t _mouseButtonCount;

public:
    MouseConfiguration();

    const char* getDeviceName() const override;
    uint8_t getDeviceReportSize() const override;
    size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override;
    uint8_t getMouseButtonNumBytes() const;

    uint16_t getMouseButtonCount() const;
    uint16_t getMouseAxisCount() const;

    void setMouseButtonCount(uint16_t value);

private:
    uint8_t getMouseButtonPaddingBits() const;
};

#endif
