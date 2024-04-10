#ifndef ESP32_BLE_MULTI_HID_H
#define ESP32_BLE_MULTI_HID_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BleConnectionStatus.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"

#include "BLEHostConfiguration.h"
#include "BaseCompositeDevice.h"

#include <vector>
#include "SafeQueue.hpp"

class BleCompositeHID
{
public:
    BleCompositeHID(std::string deviceName = "ESP32 BLE Composite HID", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
    ~BleCompositeHID();
    void begin();
    void begin(const BLEHostConfiguration& config);
    void end();

    void setBatteryLevel(uint8_t level);
    void addDevice(BaseCompositeDevice* device);
    bool isConnected();

    void queueDeviceDeferredReport(std::function<void()> && reportFunc);
    void sendDeferredReports();

    uint8_t batteryLevel;
    std::string deviceManufacturer;
    std::string deviceName;

protected:
    virtual void onStarted(NimBLEServer *pServer){};

private:
    static void taskServer(void *pvParameter);
    static void timedSendDeferredReports(void *pvParameter);

    BLEHostConfiguration _configuration;
    BleConnectionStatus* _connectionStatus;
    NimBLEHIDDevice* _hid;

    std::vector<BaseCompositeDevice*> _devices;
    SafeQueue<std::function<void()>> _deferredReports;
    TaskHandle_t _autoSendTaskHandle;
};

#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_MULTI_HID_H
