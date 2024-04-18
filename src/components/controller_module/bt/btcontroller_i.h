#pragma once

#include <Arduino.h>
#include "board_config.h"
#ifdef HAS_BT
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#include "NimBLECharacteristic.h"
#include "NimBLEHIDDevice.h"

class BTHIDDevice {
public:
    NimBLEHIDDevice* hid;
    NimBLEAdvertising*  advertising;
    NimBLECharacteristic* inputcontroller;
    NimBLECharacteristic* outputcontroller;
    NimBLECharacteristic* inputreport;
    uint16_t vid       = 0x05ac;
    uint16_t pid       = 0x820a;
    const char* DeviceName;
    bool Initilized;
    bool Connected;

    virtual void connect() = 0;
    virtual void disconnect()
    {
        NimBLEDevice::deinit();
        Initilized = false;
    }
    virtual void sendreport() = 0;
};

#endif