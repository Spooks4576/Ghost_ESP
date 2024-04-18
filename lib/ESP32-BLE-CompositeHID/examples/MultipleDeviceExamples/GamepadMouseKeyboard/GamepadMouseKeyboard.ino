#include <BleConnectionStatus.h>

#include <BleCompositeHID.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <GamepadDevice.h>


GamepadDevice* gamepad;
KeyboardDevice* keyboard;
MouseDevice* mouse;
BleCompositeHID compositeHID("CompositeHID Keyboard Mouse Gamepad", "Mystfit", 100);

void setup() {
    Serial.begin(115200);

    // Set up gamepad
    GamepadConfiguration gamepadConfig;
    gamepadConfig.setButtonCount(8);
    gamepadConfig.setHatSwitchCount(0);
    gamepad = new GamepadDevice(gamepadConfig);
  
    // Set up keyboard
    KeyboardConfiguration keyboardConfig;
    keyboard = new KeyboardDevice(keyboardConfig);

    // Set up mouse
    MouseConfiguration mouseConfig;
    mouse = new MouseDevice(mouseConfig);

     // Add all devices to the composite HID device to manage them
    compositeHID.addDevice(keyboard);
    compositeHID.addDevice(mouse);
    compositeHID.addDevice(gamepad);

    // Start the composite HID device to broadcast HID reports
    compositeHID.begin();

    delay(3000);
}

void loop() {
  if(compositeHID.isConnected()){

    // Test mouse
    int startTime = millis();
    int reportCount = 0;

    int8_t lastX = 0;
    int8_t lastY = 0;
    bool gamepadPressed = false;

    while(millis() - startTime < 8000){
        reportCount++;
        int8_t x = round(cos((float)millis() / 1000.0f) * 10.0f);
        int8_t y = round(sin((float)millis() / 1000.0f) * 10.0f);

        // Test mouse
        mouse->mouseMove(x, y);

        // Test keyboard
        if(reportCount % 100 == 0){
            keyboard->keyPress(KEY_A);
            keyboard->keyRelease(KEY_A);
        }

        // Test gamepad
        if(reportCount % 100 == 0){
            gamepadPressed = !gamepadPressed;
            if(gamepadPressed)
                gamepad->press(BUTTON_1);
            else
                gamepad->release(BUTTON_1);
        }
        
        delay(16);
    }
  }
}