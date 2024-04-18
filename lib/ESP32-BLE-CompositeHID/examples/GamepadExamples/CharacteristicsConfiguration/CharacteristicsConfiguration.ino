/*
 * Sets BLE characteristic options
 * Also shows how to set a custom MAC address
 * Use BLE Scanner etc on Android to see them
 */

#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID("Custom Contoller Name", "lemmingDev", 100);   // Create a BleCompositeHID object to manage all of the devices

GamepadDevice* bleGamepad; // Set custom device name, manufacturer and initial battery level
BLEHostConfiguration bleHostConfig;   // Create a BleGamepadConfiguration object to store all of the options

// Use the procedure below to set a custom Bluetooth MAC address
// Compiler adds 0x02 to the last value of board's base MAC address to get the BT MAC address, so take 0x02 away from the value you actually want when setting
// I've noticed the first number is a little picky and if set incorrectly don't work and will default to the board's embedded address
// 0xAA definately works, so use that, or experiment
uint8_t newMACAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF - 0x02};
    
void setup()
{
    esp_base_mac_addr_set(&newMACAddress[0]); // Set new MAC address

    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Set up the gamepad specific configuration options
    GamepadConfiguration gamepadConfig;
    gamepadConfig.setAutoReport(false);
    gamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    
    // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
    //bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
	gamepadConfig.setAxesMin(0x0000); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal    
    gamepadConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal     
    
    // Set up gamepad device late so we can pass our new config
    bleGamepad = new GamepadDevice(gamepadConfig);
    
    // Add gamepad to composite HID device
    compositeHID.addDevice(bleGamepad); 

    // Set up the host configuration options for the top level composite HID device
    bleHostConfig.setVid(0xe502);
    bleHostConfig.setPid(0xabcd);
    bleHostConfig.setModelNumber("1.0");
    bleHostConfig.setSoftwareRevision("Software Rev 1");
    bleHostConfig.setSerialNumber("9876543210");
    bleHostConfig.setFirmwareRevision("2.0");
    bleHostConfig.setHardwareRevision("1.7");

    // Start the composite HID device
    compositeHID.begin(bleHostConfig); 
}

void loop()
{
    if (compositeHID.isConnected())
    {

    }
}