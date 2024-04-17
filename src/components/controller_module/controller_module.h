#pragma once
#include "bt/nswcontroller.h"
#include "bt/pscontroller.h"

#include <Arduino.h>

typedef enum
{
    Nintendo_Switch,
    Xbox_One,
    Playstation
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
#ifdef HAS_BT
    NSWController NSW;
    PSController Playstation;
#endif

    void loop()
    {
        if (SelectedType == ControllerType::Nintendo_Switch)
        {
            #ifdef HAS_BT
            NSW.sendreport();
            #endif
        }
        else if (SelectedType == ControllerType::Xbox_One)
        {
            
        }
        else if (SelectedType == ControllerType::Playstation)
        {
            #ifdef HAS_BT
            Playstation.sendreport();
            #endif
        }
    }
};