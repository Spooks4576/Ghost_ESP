/*
 * This example shows how to set the battery level to be reported to the host OS
 * 
 * It reduces the power by 1% every 30 seconds
 * 
 */

#include <Arduino.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID;

int batteryLevel = 100;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    compositeHID.begin();
}

void loop()
{
    if (compositeHID.isConnected())
    {
        if(batteryLevel > 0)
        {
            batteryLevel -= 1;
        }
        
        Serial.print("Battery Level Set To: ");
        Serial.println(batteryLevel);
        compositeHID.setBatteryLevel(batteryLevel);
        delay(30000);
    }
}
