#include <BleConnectionStatus.h>

#include <BleCompositeHID.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>


KeyboardDevice* keyboard;
MouseDevice* mouse;
BleCompositeHID compositeHID("CompositeHID Keyboard and Mouse", "Mystfit", 100);

void setup() {
    Serial.begin(115200);
  
    // Set up keyboard
    KeyboardConfiguration keyboardConfig;
    keyboardConfig.setAutoReport(false);
    keyboard = new KeyboardDevice(keyboardConfig);

    // Set up mouse
    MouseConfiguration mouseConfig;
    mouseConfig.setAutoReport(false);
    mouse = new MouseDevice(mouseConfig);

     // Add both devices to the composite HID device to manage them
    compositeHID.addDevice(keyboard);
    compositeHID.addDevice(mouse);

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

    while(millis() - startTime < 8000){
        reportCount++;
        int8_t x = round(cos((float)millis() / 1000.0f) * 10.0f);
        int8_t y = round(sin((float)millis() / 1000.0f) * 10.0f);

        mouse->mouseMove(x, y);
        mouse->sendMouseReport();

        // Test keyboard
        if(reportCount % 100 == 0){
            keyboard->keyPress(KEY_A);
            keyboard->sendKeyReport();
            keyboard->keyRelease(KEY_A);
            keyboard->sendKeyReport();
        }
        
        delay(16);
    }
  }
}