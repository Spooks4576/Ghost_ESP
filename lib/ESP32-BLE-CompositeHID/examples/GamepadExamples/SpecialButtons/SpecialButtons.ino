#include <Arduino.h>
#include <GamepadDevice.h>
#include <BleCompositeHID.h>

BleCompositeHID compositeHID;
GamepadDevice* gamepad;

void setup()
{
    Serial.begin(115200);

    GamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setWhichSpecialButtons(true, true, true, true, true, true, true, true);
    // Can also enable special buttons individually with the following <-- They are all disabled by default
    // bleGamepadConfig.setIncludeStart(true);
    // bleGamepadConfig.setIncludeSelect(true);
    // bleGamepadConfig.setIncludeMenu(true);
    // bleGamepadConfig.setIncludeHome(true);
    // bleGamepadConfig.setIncludeBack(true);
    // bleGamepadConfig.setIncludeVolumeInc(true);
    // bleGamepadConfig.setIncludeVolumeDec(true);
    // bleGamepadConfig.setIncludeVolumeMute(true);

    gamepad = new GamepadDevice(bleGamepadConfig);
    compositeHID.addDevice(gamepad);
    compositeHID.begin();
    // Changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (compositeHID.isConnected())
    {
        Serial.println("Pressing start and select");
        gamepad->pressStart();
        delay(100);
        gamepad->releaseStart();
        delay(100);
        gamepad->pressSelect();
        delay(100);
        gamepad->releaseSelect();
        delay(100);

        Serial.println("Increasing volume");
        gamepad->pressVolumeInc();
        delay(100);
        gamepad->releaseVolumeInc();
        delay(100);
        gamepad->pressVolumeInc();
        delay(100);
        gamepad->releaseVolumeInc();
        delay(100);
        
        Serial.println("Muting volume");
        gamepad->pressVolumeMute();
        delay(100);
        gamepad->releaseVolumeMute();
        delay(1000);
        gamepad->pressVolumeMute();
        delay(100);
        gamepad->releaseVolumeMute();


        Serial.println("Pressing menu and back");
        gamepad->pressMenu();
        delay(100);
        gamepad->releaseMenu();
        delay(100);
        gamepad->pressBack();
        delay(100);
        gamepad->releaseBack();
        delay(100);

        Serial.println("Pressing home");
        gamepad->pressHome();
        delay(100);
        gamepad->releaseHome();
        delay(2000);
    }
}
