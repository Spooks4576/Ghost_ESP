#include <Arduino.h>
#include <KeyboardDevice.h>
#include <BleCompositeHID.h>


BleCompositeHID compositeHID("ESP32 Media Keyboard", "Mystfit", 100);
KeyboardDevice* keyboard;

void setup()
{
    Serial.begin(115200);

    // Media keys are not enabled by default
    KeyboardConfiguration config;
    config.setUseMediaKeys(true);

    keyboard = new KeyboardDevice(config);
    compositeHID.addDevice(keyboard);
    compositeHID.begin();

    Serial.println("Waiting for connection");
    delay(3000);
}

void pressReleaseMediaKey(uint32_t keyCode){
    Serial.println("Pressing key " + String(keyCode));
    keyboard->mediaKeyPress(keyCode);
    delay(50);
    keyboard->mediaKeyRelease(keyCode);
    delay(1000);
}

void loop()
{
    if (compositeHID.isConnected())
    {
        pressReleaseMediaKey(KEY_MEDIA_PLAY);
        pressReleaseMediaKey(KEY_MEDIA_PAUSE);
        pressReleaseMediaKey(KEY_MEDIA_RECORD);
        pressReleaseMediaKey(KEY_MEDIA_FASTFORWARD);
        pressReleaseMediaKey(KEY_MEDIA_REWIND);
        pressReleaseMediaKey(KEY_MEDIA_NEXTTRACK);
        pressReleaseMediaKey(KEY_MEDIA_PREVIOUSTRACK);
        pressReleaseMediaKey(KEY_MEDIA_STOP);
        pressReleaseMediaKey(KEY_MEDIA_EJECT);
        pressReleaseMediaKey(KEY_MEDIA_RANDOMPLAY);
        pressReleaseMediaKey(KEY_MEDIA_REPEAT);
        pressReleaseMediaKey(KEY_MEDIA_PLAYPAUSE);
        pressReleaseMediaKey(KEY_MEDIA_MUTE);
        pressReleaseMediaKey(KEY_MEDIA_VOLUMEUP);
        pressReleaseMediaKey(KEY_MEDIA_VOLUMEDOWN);
        pressReleaseMediaKey(KEY_MEDIA_WWWHOME);
        pressReleaseMediaKey(KEY_MEDIA_MYCOMPUTER);
        pressReleaseMediaKey(KEY_MEDIA_CALCULATOR);
        pressReleaseMediaKey(KEY_MEDIA_WWWFAVORITES);
        pressReleaseMediaKey(KEY_MEDIA_WWWSEARCH);
        pressReleaseMediaKey(KEY_MEDIA_WWWSTOP);
        pressReleaseMediaKey(KEY_MEDIA_WWWBACK);
        pressReleaseMediaKey(KEY_MEDIA_MEDIASELECT);
        pressReleaseMediaKey(KEY_MEDIA_MAIL);
    }
}
