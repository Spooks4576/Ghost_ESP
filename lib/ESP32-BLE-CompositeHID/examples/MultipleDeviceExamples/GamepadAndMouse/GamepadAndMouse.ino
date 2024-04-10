#include <BleConnectionStatus.h>

#include <BleCompositeHID.h>
#include <GamepadDevice.h>
#include <MouseDevice.h>


GamepadDevice* gamepad;
MouseDevice* mouse;
BleCompositeHID compositeHID("CompositeHID Gamepad and Mouse", "Mystfit", 100);

void setup() {
  Serial.begin(115200);
  
  // Set up gamepad
  GamepadConfiguration gamepadConfig;
  gamepadConfig.setButtonCount(8);
  gamepadConfig.setHatSwitchCount(0);
  gamepad = new GamepadDevice(gamepadConfig);

  // Set up mouse
  mouse = new MouseDevice();

  // Add both devices to the composite HID device to manage them
  compositeHID.addDevice(gamepad);
  compositeHID.addDevice(mouse);

  // Start the composite HID device to broadcast HID reports
  compositeHID.begin();
}

void loop() {
  if(compositeHID.isConnected()){

    // Test gamepad
    gamepad->press(BUTTON_3);
    delay(500);               
    gamepad->release(BUTTON_3);
    delay(500);

    // Test mouse
    int startTime = millis();
    int reportCount = 0;
    while(millis() - startTime < 8000){
        reportCount++;
        int8_t x = round(cos((float)millis() / 1000.0f) * 10.0f);
        int8_t y = round(sin((float)millis() / 1000.0f) * 10.0f);

        mouse->mouseMove(x, y);
        mouse->sendMouseReport();
        
        if(reportCount % 8 == 0)
            Serial.println("Setting relative mouse to " + String(x) + ", " + String(y));
            
        delay(10);
    }
    delay(1000);
  }
}