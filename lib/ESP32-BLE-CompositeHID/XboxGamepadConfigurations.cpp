#include "XboxGamepadConfiguration.h"
#include "XboxGamepadDevice.h"


// XboxGamepadDeviceConfiguration methods
XboxGamepadDeviceConfiguration::XboxGamepadDeviceConfiguration(uint8_t reportId) : 
    BaseCompositeDeviceConfiguration(reportId)
{
}

BLEHostConfiguration XboxOneSControllerDeviceConfiguration::getIdealHostConfiguration() const {
    // Fake a xbox controller
    BLEHostConfiguration config;

    // Vendor: Microsoft
    config.setVidSource(VENDOR_USB_SOURCE);
    config.setVid(XBOX_VENDOR_ID); 
    
    // Product: Xbox One Wireless Controller - Model 1708 pre 2021 firmware
    // Specifically picked since it provides rumble support on linux kernels earlier than 6.5
    config.setPid(XBOX_1708_PRODUCT_ID); 
    config.setGuidVersion(XBOX_1708_BCD_DEVICE_ID);
    config.setSerialNumber(XBOX_1708_SERIAL);

    return config;
}

uint8_t XboxOneSControllerDeviceConfiguration::getDeviceReportSize() const {
    // Return the size of the device report
    
    // Input
    // 2 * 16bit (2 bytes) for X and Y inclusive                = 4 bytes
    // 2 * 16bit (2 bytes) for Z and Rz inclusive               = 4 bytes
    // 1 * 10bit for brake + 6bit padding (2 bytes)             = 2 bytes
    // 1 * 10bit for accelerator + 6bit padding (2 bytes)       = 2 bytes
    // 1 * 4bit for hat switch + 4bit padding (1 byte)          = 1 byte
    // 15 * 1bit for buttons + 1bit padding (2 bytes)           = 2 bytes
    // 1 * 1bit for menu/select button + 7bit padding (1 byte)       = 1 byte

    // Additional input?
    // 1 * 1bit + 7bit padding (1 byte)                         = 1 byte

    // TODO: Split output size into seperate function
    // Output 
    // 1 * 4bit for DC Enable Actuators + 4bit padding (1 byte) = 1 byte
    // 4 * 8bit for Magnitude (4 bytes)                         = 4 bytes
    // 3 * 8bit for Duration, Start Delay, Loop Count (3 bytes) = 3 bytes

    return sizeof(XboxGamepadInputReportData); //16
}

size_t XboxOneSControllerDeviceConfiguration::makeDeviceReport(uint8_t* buffer, size_t bufferSize) const {
    size_t hidDescriptorSize = sizeof(XboxOneS_1708_HIDDescriptor);
    if(hidDescriptorSize < bufferSize){
        memcpy(buffer, XboxOneS_1708_HIDDescriptor, hidDescriptorSize);
    } else {
        return -1;
    }
    
    return hidDescriptorSize;
}


// -------------------------------------


BLEHostConfiguration XboxSeriesXControllerDeviceConfiguration::getIdealHostConfiguration() const {
    // Fake a xbox controller
    BLEHostConfiguration config;

    // Vendor: Microsoft
    config.setVidSource(VENDOR_USB_SOURCE);
    config.setVid(XBOX_VENDOR_ID); 
    
    // Product: Xbox Series X Wireless Controller - Model 1944 pre 2021 firmware
    // Only compatible on linux kernels >= 6.5
    config.setPid(XBOX_1914_PRODUCT_ID); 
    config.setGuidVersion(XBOX_1914_BCD_DEVICE_ID);
    config.setSerialNumber(XBOX_1914_SERIAL);

    return config;
}

uint8_t XboxSeriesXControllerDeviceConfiguration::getDeviceReportSize() const {
    // Return the size of the device report
    
    // Input
    // 2 * 16bit (2 bytes) for X and Y inclusive                = 4 bytes
    // 2 * 16bit (2 bytes) for Z and Rz inclusive               = 4 bytes
    // 1 * 10bit for brake + 6bit padding (2 bytes)             = 2 bytes
    // 1 * 10bit for accelerator + 6bit padding (2 bytes)       = 2 bytes
    // 1 * 4bit for hat switch + 4bit padding (1 byte)          = 1 byte
    // 15 * 1bit for buttons + 1bit padding (2 bytes)           = 2 bytes
    // 1 * 1bit for share button + 7bit padding (1 byte)       = 1 byte

    // Additional input?
    // 1 * 1bit + 7bit padding (1 byte)                         = 1 byte

    // TODO: Split output size into seperate function
    // Output 
    // 1 * 4bit for DC Enable Actuators + 4bit padding (1 byte) = 1 byte
    // 4 * 8bit for Magnitude (4 bytes)                         = 4 bytes
    // 3 * 8bit for Duration, Start Delay, Loop Count (3 bytes) = 3 bytes

    return sizeof(XboxGamepadInputReportData); //16;
}

size_t XboxSeriesXControllerDeviceConfiguration::makeDeviceReport(uint8_t* buffer, size_t bufferSize) const {
    size_t hidDescriptorSize = sizeof(XboxOneS_1914_HIDDescriptor);
    if(hidDescriptorSize < bufferSize){
        memcpy(buffer, XboxOneS_1914_HIDDescriptor, hidDescriptorSize);
    } else {
        return -1;
    }
    return hidDescriptorSize;
}