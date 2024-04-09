#pragma once
#include "hidnswusb.h"

#include <Arduino.h>

struct usb_interface
{
public:
    bool UsbConnected;
    HIDNSWUSB NSWUsb;
};

