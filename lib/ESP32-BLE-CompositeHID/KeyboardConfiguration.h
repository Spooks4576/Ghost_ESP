#ifndef KEYBOARD_CONFIGURATION_H
#define KEYBOARD_CONFIGURATION_H

#include <BaseCompositeDevice.h>

class KeyboardConfiguration : public BaseCompositeDeviceConfiguration
{   
public:
    KeyboardConfiguration();
    KeyboardConfiguration(uint8_t reportId);
    uint8_t getDeviceReportSize() const override;
    size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override;

    bool getUseMediaKeys() const;
    void setUseMediaKeys(bool value);
private:
    bool _useMediaKeys;
};

#endif