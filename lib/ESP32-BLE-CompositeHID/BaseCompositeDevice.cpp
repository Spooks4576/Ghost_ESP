#include "BaseCompositeDevice.h"
#include "BleCompositeHID.h"

BaseCompositeDeviceConfiguration::BaseCompositeDeviceConfiguration(uint8_t reportId) : 
    _autoReport(true),
    _reportId(reportId),
    _autoDefer(false)
{
}

const char* BaseCompositeDeviceConfiguration::getDeviceName() const {
    return "BaseCompositeDevice";
}

BLEHostConfiguration BaseCompositeDeviceConfiguration::getIdealHostConfiguration() const {
    return BLEHostConfiguration();
}

bool BaseCompositeDeviceConfiguration::getAutoReport() const { return _autoReport; }
uint8_t BaseCompositeDeviceConfiguration::getReportId() const { return _reportId; }

void BaseCompositeDeviceConfiguration::setAutoReport(bool value) { _autoReport = value; }
void BaseCompositeDeviceConfiguration::setHidReportId(uint8_t value) { _reportId = value; }

void BaseCompositeDeviceConfiguration::setAutoDefer(bool value) { _autoDefer = value; }
bool BaseCompositeDeviceConfiguration::getAutoDefer() const { return _autoDefer; }

// ---------------

void BaseCompositeDevice::queueDeferredReport(std::function<void()> && reportFunc) {
    if(auto parent = getParent()){
        parent->queueDeviceDeferredReport(std::forward<std::function<void()>>(reportFunc));
    }
}

BleCompositeHID* BaseCompositeDevice::getParent() { return _parent; }

void BaseCompositeDevice::setCharacteristics(NimBLECharacteristic* input, NimBLECharacteristic* output) {
    _input = input;
    _output = output;
}

NimBLECharacteristic* BaseCompositeDevice::getInput() { return _input; }
NimBLECharacteristic* BaseCompositeDevice::getOutput() { return _output; }
