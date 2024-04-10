#ifndef COMPOSITE_CONFIG_H
#define COMPOSITE_CONFIG_H

#include <Arduino.h>
//#include <HIDKeyboardTypes.h>
#include <NimBLECharacteristic.h>
#include <NimBLEHIDDevice.h>
#include "BLEHostConfiguration.h"

// Forwards
class BleCompositeHID;

class BaseCompositeDeviceConfiguration 
{
public:
    BaseCompositeDeviceConfiguration(uint8_t reportId);

    bool getAutoReport() const;
    void setAutoReport(bool value);

    uint8_t getReportId() const;
    void setHidReportId(uint8_t value);
    
    void setAutoDefer(bool value);
    bool getAutoDefer() const;

    virtual const char* getDeviceName() const;
    virtual BLEHostConfiguration getIdealHostConfiguration() const;
    virtual uint8_t getDeviceReportSize() const = 0;
    virtual size_t makeDeviceReport(uint8_t* buffer, size_t bufferSize) const = 0;

private:
    bool _autoReport;
    bool _autoDefer;
    uint8_t _reportId;
};


class BaseCompositeDevice 
{
    friend class BleCompositeHID;
public:
    virtual void init(NimBLEHIDDevice* hid) = 0;
    virtual const BaseCompositeDeviceConfiguration* getDeviceConfig() const = 0;
    
    BleCompositeHID* getParent();

protected:
    void queueDeferredReport(std::function<void()> && reportFunc);
    void setCharacteristics(NimBLECharacteristic* input, NimBLECharacteristic* output);
    NimBLECharacteristic* getInput();
    NimBLECharacteristic* getOutput();

private:
    BleCompositeHID* _parent;
    NimBLECharacteristic* _input;
    NimBLECharacteristic* _output;
};

#endif