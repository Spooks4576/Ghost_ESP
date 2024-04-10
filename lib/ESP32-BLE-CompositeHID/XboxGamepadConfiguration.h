#ifndef XBOX_GAMEPAD_CONFIGURATION_H
#define XBOX_GAMEPAD_CONFIGURATION_H

#include "XboxDescriptors.h"
#include "BaseCompositeDevice.h"

class XboxGamepadDeviceConfiguration : public BaseCompositeDeviceConfiguration {
public:
    XboxGamepadDeviceConfiguration(uint8_t reportId = XBOX_INPUT_REPORT_ID);
    virtual uint8_t getDeviceReportSize() const override { return 0; }
    virtual size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override { 
        return -1;
    }
};


class XboxOneSControllerDeviceConfiguration : public XboxGamepadDeviceConfiguration {
public:
    virtual const char* getDeviceName() const { return "XboxOneS"; }
    virtual BLEHostConfiguration getIdealHostConfiguration() const override;
    virtual uint8_t getDeviceReportSize() const override;
    virtual size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override;
};


class XboxSeriesXControllerDeviceConfiguration : public XboxGamepadDeviceConfiguration {
public:
    virtual const char* getDeviceName() const { return "XboxSeriesX"; }
    virtual BLEHostConfiguration getIdealHostConfiguration() const override;
    virtual uint8_t getDeviceReportSize() const override;
    virtual size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const override;
};

#endif // XBOX_GAMEPAD_CONFIGURATION_H
