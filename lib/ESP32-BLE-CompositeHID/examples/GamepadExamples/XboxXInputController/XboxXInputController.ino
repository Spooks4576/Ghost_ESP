#include <BleConnectionStatus.h>

#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>

int ledPin = 5; // LED connected to digital pin 13

XboxGamepadDevice *gamepad;
BleCompositeHID compositeHID("CompositeHID XInput Controller", "Mystfit", 100);

void OnVibrateEvent(XboxGamepadOutputReportData data)
{
    if(data.weakMotorMagnitude > 0 || data.strongMotorMagnitude > 0){
        digitalWrite(ledPin, LOW);
    } else {
        digitalWrite(ledPin, HIGH);
    }
    Serial.println("Vibration event. Weak motor: " + String(data.weakMotorMagnitude) + " Strong motor: " + String(data.strongMotorMagnitude));
}

void setup()
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT); // sets the digital pin as output

    // Uncomment one of the following two config types depending on which controller version you want to use
    // The XBox series X controller only works on linux kernels >= 6.5
    
    XboxOneSControllerDeviceConfiguration* config = new XboxOneSControllerDeviceConfiguration();
    //XboxSeriesXControllerDeviceConfiguration* config = new XboxSeriesXControllerDeviceConfiguration();

    // The composite HID device pretends to be a valid Xbox controller via vendor and product IDs (VID/PID).
    // Platforms like windows/linux need this in order to pick an XInput driver over the generic BLE GATT HID driver. 
    BLEHostConfiguration hostConfig = config->getIdealHostConfiguration();
    Serial.println("Using VID source: " + String(hostConfig.getVidSource(), HEX));
    Serial.println("Using VID: " + String(hostConfig.getVid(), HEX));
    Serial.println("Using PID: " + String(hostConfig.getPid(), HEX));
    Serial.println("Using GUID version: " + String(hostConfig.getGuidVersion(), HEX));
    Serial.println("Using serial number: " + String(hostConfig.getSerialNumber()));
    
    // Set up gamepad
    gamepad = new XboxGamepadDevice(config);

    // Set up vibration event handler

    // Add all child devices to the top-level composite HID device to manage them
    compositeHID.addDevice(gamepad);

    // Start the composite HID device to broadcast HID reports
    Serial.println("Starting composite HID device...");
    compositeHID.begin(hostConfig);
}

void loop()
{
    if(compositeHID.isConnected()){
        testButtons();
        testPads();
        testTriggers();
        testThumbsticks();
    }
}

void testButtons(){
    // Test each button
    uint16_t buttons[] = {
        XBOX_BUTTON_A, 
        XBOX_BUTTON_B, 
        XBOX_BUTTON_X, 
        XBOX_BUTTON_Y, 
        XBOX_BUTTON_LB, 
        XBOX_BUTTON_RB, 
        XBOX_BUTTON_START,
        XBOX_BUTTON_SELECT,
        //XBOX_BUTTON_HOME,   // Uncomment this to test the hom/guide button. Steam will flip out and enter big picture mode when running this sketch though so be warned!
        XBOX_BUTTON_LS, 
        XBOX_BUTTON_RS
    };
    for (uint16_t button : buttons)
    {
        Serial.println("Pressing button " + String(button));
        gamepad->press(button);
        gamepad->sendGamepadReport();
        delay(500);
        gamepad->release(button);
        gamepad->sendGamepadReport();
        delay(100);
    }

    // The share button is a seperate call since it doesn't live in the same 
    // bitflag as the rest of the buttons
    gamepad->pressShare();
    gamepad->sendGamepadReport();
    delay(500);
    gamepad->releaseShare();
    gamepad->sendGamepadReport();
    delay(100);
}

void testPads(){
    XboxDpadFlags directions[] = {
        XboxDpadFlags::NORTH,
        XboxDpadFlags((uint8_t)XboxDpadFlags::NORTH | (uint8_t)XboxDpadFlags::EAST),
        XboxDpadFlags::EAST,
        XboxDpadFlags((uint8_t)XboxDpadFlags::EAST | (uint8_t)XboxDpadFlags::SOUTH),
        XboxDpadFlags::SOUTH,
        XboxDpadFlags((uint8_t)XboxDpadFlags::SOUTH | (uint8_t)XboxDpadFlags::WEST),
        XboxDpadFlags::WEST,
        XboxDpadFlags((uint8_t)XboxDpadFlags::WEST | (uint8_t)XboxDpadFlags::NORTH)
    };

    for (XboxDpadFlags direction : directions)
    {
        Serial.println("Pressing DPad: " + String(direction));
        gamepad->pressDPadDirectionFlag(direction);
        gamepad->sendGamepadReport();
        delay(500);
        gamepad->releaseDPad();
        gamepad->sendGamepadReport();
        delay(100);
    }
}

void testTriggers(){
    for(int16_t val = XBOX_TRIGGER_MIN; val <= XBOX_TRIGGER_MAX; val++){
        if(val % 8 == 0)
            Serial.println("Setting trigger value to " + String(val));
        gamepad->setLeftTrigger(val);
        gamepad->setRightTrigger(val);
        gamepad->sendGamepadReport();
        delay(10);
    }
}

void testThumbsticks(){
    int startTime = millis();
    int reportCount = 0;
    while(millis() - startTime < 8000){
        reportCount++;
        int16_t x = cos((float)millis() / 1000.0f) * XBOX_STICK_MAX;
        int16_t y = sin((float)millis() / 1000.0f) * XBOX_STICK_MAX;

        gamepad->setLeftThumb(x, y);
        gamepad->setRightThumb(x, y);
        gamepad->sendGamepadReport();
        
        if(reportCount % 8 == 0)
            Serial.println("Setting left thumb to " + String(x) + ", " + String(y));
            
        delay(10);
    }
}