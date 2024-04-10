#pragma once
#include "hidnswusb.h"
#include "hidxinput.h"

#include <Arduino.h>

typedef enum
{
    Nintendo_Switch,
    Xbox_One
} ControllerType;

typedef enum 
{
    BLE,
    USB
} ConnectionType;

struct controller_interface
{
public:
    bool UsbConnected;
    ControllerType SelectedType;
    ConnectionType SelectedConnection;
#if CFG_TUD_HID
    HIDNSWUSB NSWUsb;
    HIDXInputUSB XInputUsb;
#endif
};

