#include "KeyboardConfiguration.h"
#include "KeyboardDevice.h"
#include "KeyboardDescriptors.h"

KeyboardConfiguration::KeyboardConfiguration() :
    BaseCompositeDeviceConfiguration(KEYBOARD_REPORT_ID),
    _useMediaKeys(false)
{
}

KeyboardConfiguration::KeyboardConfiguration(uint8_t reportId) :
    BaseCompositeDeviceConfiguration(reportId),
    _useMediaKeys(false)
{
}

uint8_t KeyboardConfiguration::getDeviceReportSize() const
{
    return sizeof(_keyboardHIDReportDescriptor);
}

size_t KeyboardConfiguration::makeDeviceReport(uint8_t* buffer, size_t bufferSize) const
{
    size_t hidDescriptorSize = sizeof(_keyboardHIDReportDescriptor);
    if(hidDescriptorSize < bufferSize){
        memcpy(buffer, _keyboardHIDReportDescriptor, hidDescriptorSize);
    } else {
        return -1;
    }

    if(_useMediaKeys){
        size_t mediaKeysHidDescriptorSize = sizeof(_mediakeysHIDReportDescriptor);
        if(hidDescriptorSize + mediaKeysHidDescriptorSize < bufferSize){
            memcpy(buffer + hidDescriptorSize, _mediakeysHIDReportDescriptor, mediaKeysHidDescriptorSize);
            hidDescriptorSize += mediaKeysHidDescriptorSize;
        } else {
            return -1;
        }
    }
    
    return hidDescriptorSize;
}

bool KeyboardConfiguration::getUseMediaKeys() const
{
    return _useMediaKeys;
}

void KeyboardConfiguration::setUseMediaKeys(bool value)
{
    _useMediaKeys = value;
}