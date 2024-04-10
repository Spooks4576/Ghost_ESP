/*
 * This example turns the ESP32 into a Bluetooth LE gamepad that presses buttons and moves axis
 *
 * Possible buttons are:
 * BUTTON_1 through to BUTTON_128 (Windows gamepad tester only visualises the first 32)
 ^ Use http://www.planetpointy.co.uk/joystick-test-application/ to visualise all of them
 * Whenever you adjust the amount of buttons/axes etc, make sure you unpair and repair the BLE device
 *
 * Possible DPAD/HAT switch position values are:
 * DPAD_CENTERED, DPAD_UP, DPAD_UP_RIGHT, DPAD_RIGHT, DPAD_DOWN_RIGHT, DPAD_DOWN, DPAD_DOWN_LEFT, DPAD_LEFT, DPAD_UP_LEFT
 *
 * gamepad->setAxes takes the following int16_t parameters for the Left/Right Thumb X/Y, Left/Right Triggers plus slider1 and slider2:
 * (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger, Slider 1, Slider 2);
 *
 * gamepad->setLeftThumb (or setRightThumb) takes 2 int16_t parameters for x and y axes (or z and rZ axes)
 *
 * gamepad->setLeftTrigger (or setRightTrigger) takes 1 int16_t parameter for rX axis (or rY axis)
 *
 * gamepad->setSlider1 (or setSlider2) takes 1 int16_t parameter for slider 1 (or slider 2)
 *
 * gamepad->setHat1 takes a hat position as above (or 0 = centered and 1~8 are the 8 possible directions)
 *
 * setHats, setTriggers and setSliders functions are also available for setting all hats/triggers/sliders at once
 *
 * The example shows that you can set axes/hats independantly, or together.
 *
 * It also shows that you can disable the autoReport feature (enabled by default), and manually call the sendReport() function when wanted
 *
 */

#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID("Controller axis", "lemmingDev", 100);
GamepadDevice* gamepad;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    
    GamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setAutoReport(false); // This is true by default
    bleGamepadConfig.setButtonCount(128);
    bleGamepadConfig.setHatSwitchCount(2);
    
    gamepad = new GamepadDevice(bleGamepadConfig); // Creates a gamepad with 128 buttons, 2 hat switches and x, y, z, rZ, rX, rY and 2 sliders (no simulation controls enabled by default)
    // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again

    compositeHID.addDevice(gamepad); 
	compositeHID.begin();
}

void loop()
{
    if (compositeHID.isConnected())
    {
        Serial.println("Press buttons 1, 32, 64 and 128. Set hat 1 to down right and hat 2 to up left");

        // Press buttons 5, 32, 64 and 128
        gamepad->press(BUTTON_5);
        gamepad->press(BUTTON_32);
        gamepad->press(BUTTON_64);
        gamepad->press(BUTTON_128);

        // Move all axes to max.
        gamepad->setLeftThumb(32767, 32767);  // or gamepad->setX(32767); and gamepad->setY(32767);
        gamepad->setRightThumb(32767, 32767); // or gamepad->setZ(32767); and gamepad->setRZ(32767);
        gamepad->setLeftTrigger(32767);       // or gamepad->setRX(32767);
        gamepad->setRightTrigger(32767);      // or gamepad->setRY(32767);
        gamepad->setSlider1(32767);
        gamepad->setSlider2(32767);

        // Set hat 1 to down right and hat 2 to up left (hats are otherwise centered by default)
        gamepad->setHat1(DPAD_DOWN_RIGHT); // or gamepad->setHat1(HAT_DOWN_RIGHT);
        gamepad->setHat2(DPAD_UP_LEFT);    // or gamepad->setHat2(HAT_UP_LEFT);
        // Or gamepad->setHats(DPAD_DOWN_RIGHT, DPAD_UP_LEFT);

        // Send the gamepad report
        gamepad->sendGamepadReport();
        delay(500);

        Serial.println("Release button 5 and 64. Move all axes to min. Set hat 1 and 2 to centred.");
        gamepad->release(BUTTON_5);
        gamepad->release(BUTTON_64);
        gamepad->setAxes(0, 0, 0, 0, 0, 0, 0, 0);
        gamepad->setHats(DPAD_CENTERED, HAT_CENTERED);
        gamepad->sendGamepadReport();
        delay(500);
    }
}
