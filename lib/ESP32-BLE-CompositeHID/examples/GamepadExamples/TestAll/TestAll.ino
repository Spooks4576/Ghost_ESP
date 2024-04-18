/*
 * Test all gamepad buttons, axes and dpad
 */

#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

#define numOfButtons 64
#define numOfHatSwitches 4

BleCompositeHID compositeHID;
GamepadDevice* gamepad;
GamepadConfiguration bleGamepadConfig;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    
    BLEHostConfiguration hostConfig;
    hostConfig.setVid(0xe502);
    hostConfig.setPid(0xabcd);

    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    bleGamepadConfig.setButtonCount(numOfButtons);
    bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);
    // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
    //bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
	bleGamepadConfig.setAxesMin(0x0000); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    
    gamepad = new GamepadDevice(bleGamepadConfig); // Simulation controls, special buttons and hats 2/3/4 are disabled by default
    compositeHID.addDevice(gamepad);
    compositeHID.begin();
    // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (compositeHID.isConnected())
    {
        Serial.println("\nn--- Axes Decimal ---");
        Serial.print("Axes Min: ");
        Serial.println(bleGamepadConfig.getAxesMin());
        Serial.print("Axes Max: ");
        Serial.println(bleGamepadConfig.getAxesMax());
        Serial.println("\nn--- Axes Hex ---");
        Serial.print("Axes Min: ");
        Serial.println(bleGamepadConfig.getAxesMin(), HEX);
        Serial.print("Axes Max: ");
        Serial.println(bleGamepadConfig.getAxesMax(), HEX);        
        
        Serial.println("\n\nPress all buttons one by one");
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
        gamepad->sendGamepadReport();
        delay(100);
        gamepad->pressSelect();
        gamepad->sendGamepadReport();
        delay(100);
        gamepad->releaseStart();
        gamepad->sendGamepadReport();
        delay(100);
        gamepad->releaseSelect();
        gamepad->sendGamepadReport();

        Serial.println("Move all axis simultaneously from min to max");
        for (int i = bleGamepadConfig.getAxesMin(); i < bleGamepadConfig.getAxesMax(); i += (bleGamepadConfig.getAxesMax() / 256) + 1)
        {
            gamepad->setAxes(i, i, i, i, i, i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setAxes(); // Reset all axes to zero
        gamepad->sendGamepadReport();

        Serial.println("Move all sliders simultaneously from min to max");
       for (int i = bleGamepadConfig.getAxesMin(); i < bleGamepadConfig.getAxesMax(); i += (bleGamepadConfig.getAxesMax() / 256) + 1)
        {
            gamepad->setSliders(i, i);
            gamepad->sendGamepadReport();
            delay(10);
        }
        gamepad->setSliders(); // Reset all sliders to zero
        gamepad->sendGamepadReport();

        Serial.println("Send hat switch 1 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            gamepad->setHat1(i);
            gamepad->sendGamepadReport();
            delay(200);
        }

        Serial.println("Send hat switch 2 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            gamepad->setHat2(i);
            gamepad->sendGamepadReport();
            delay(200);
        }

        Serial.println("Send hat switch 3 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            gamepad->setHat3(i);
            gamepad->sendGamepadReport();
            delay(200);
        }

        Serial.println("Send hat switch 4 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            gamepad->setHat4(i);
            gamepad->sendGamepadReport();
            delay(200);
        }

        //    Simulation controls are disabled by default
        //    Serial.println("Move all simulation controls simultaneously from min to max");
        //    for (int i = bleGamepadConfig.getSimulationMin(); i < bleGamepadConfig.getSimulationMax(); i += (bleGamepadConfig.getAxesMax() / 256) + 1)
        //    {
        //      gamepad->setRudder(i);
        //      gamepad->setThrottle(i);
        //      gamepad->setAccelerator(i);
        //      gamepad->setBrake(i);
        //      gamepad->setSteering(i);
        //      gamepad->sendGamepadReport();
        //      delay(10);
        //    }
        //    gamepad->setSimulationControls(); //Reset all simulation controls to zero
        gamepad->sendGamepadReport();
    }
}