#pragma once
#include "hidnswusb.h"
#include "hidxinput.h"

#include <Arduino.h>

typedef enum
{
    Nintendo_Switch,
    Xbox_One
} USBType;

struct usb_interface
{
public:
    bool UsbConnected;
    USBType SelectedType;
#if CFG_TUD_HID
    HIDNSWUSB NSWUsb;
    HIDXInputUSB XInputUsb;
#endif
};

