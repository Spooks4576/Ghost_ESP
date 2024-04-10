/*
 * Flight controller test
 */

#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

#define numOfButtons 32
#define numOfHatSwitches 0
#define enableX true
#define enableY true
#define enableZ false
#define enableRZ false
#define enableRX false
#define enableRY false
#define enableSlider1 false
#define enableSlider2 false
#define enableRudder true
#define enableThrottle true
#define enableAccelerator false
#define enableBrake false
#define enableSteering false

//BleGamepad bleGamepad("BLE Flight Controller", "lemmingDev", 100);
BleCompositeHID compositeHID("BLE Flight Controller", "lemmingDev", 100);
GamepadDevice* gamepad;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Setup controller with 10 buttons, accelerator, brake and steering
    GamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    bleGamepadConfig.setButtonCount(numOfButtons);
    bleGamepadConfig.setWhichAxes(enableX, enableY, enableZ, enableRX, enableRY, enableRZ, enableSlider1, enableSlider2);      // Can also be done per-axis individually. All are true by default
    bleGamepadConfig.setWhichSimulationControls(enableRudder, enableThrottle, enableAccelerator, enableBrake, enableSteering); // Can also be done per-control individually. All are false by default
    bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);                                                                      // 1 by default
    // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
    bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    
    gamepad = new GamepadDevice(bleGamepadConfig);
    compositeHID.addDevice(gamepad);

	compositeHID.begin();
    // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again

    // Set throttle to min
    gamepad->setThrottle(-32767);

    // Set x and y axes and rudder to center
    gamepad->setX(0);
    gamepad->setY(0);
    gamepad->setRudder(0);
}

void loop()
{
    if (compositeHID.isConnected())
    {
        Serial.println("Press all buttons one by one");
        for (int i = 1; i <= numOfButtons; i += 1)
        {
            gamepad->press(i);
            gamepad->sendGamepadReport();
            delay(100);
            gamepad->release(i);
            gamepad->sendGamepadReport();
            delay(25);
        }

        Serial.println("Press start and select");
        gamepad->pressStart();
        delay(100);
        gamepad->pressSelect();
        delay(100);
        gamepad->releaseStart();
        delay(100);
        gamepad->releaseSelect();

        Serial.println("Move x axis from center to max");
        for (int i = 0; i > -32767; i -= 256)
        {
            gamepad->setX(i);
            gamepad->sendGamepadReport();
            delay(10);
        }

        Serial.println("Move x axis from min to max");
        for (int i = -32767; i < 32767; i += 256)
        {
            gamepad->setX(i);
            gamepad->sendGamepadReport();
            delay(10);
        }

        Serial.println("Move x axis from max to center");
        for (int i = 32767; i > 0; i -= 256)
        {
            gamepad->setX(i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setX(0);
        gamepad->sendGamepadReport();

        Serial.println("Move y axis from center to max");
        for (int i = 0; i > -32767; i -= 256)
        {
            gamepad->setY(i);
            gamepad->sendGamepadReport();
            delay(10);
        }

        Serial.println("Move y axis from min to max");
        for (int i = -32767; i < 32767; i += 256)
        {
            gamepad->setY(i);
            gamepad->sendGamepadReport();
            delay(10);
        }

        Serial.println("Move y axis from max to center");
        for (int i = 32767; i > 0; i -= 256)
        {
            gamepad->setY(i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setY(0);
        gamepad->sendGamepadReport();

        Serial.println("Move rudder from min to max");
        // for(int i = 32767 ; i > -32767 ; i -= 256)    //Use this for loop setup instead if rudder is reversed
        for (int i = -32767; i < 32767; i += 256)
        {
            gamepad->setRudder(i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setRudder(0);
        gamepad->sendGamepadReport();

        Serial.println("Move throttle from min to max");
        for (int i = -32767; i < 32767; i += 256)
        {
            gamepad->setThrottle(i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setThrottle(-32767);
        gamepad->sendGamepadReport();
    }
}