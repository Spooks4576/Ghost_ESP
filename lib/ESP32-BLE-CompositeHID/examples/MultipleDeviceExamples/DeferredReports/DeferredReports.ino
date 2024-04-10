// This example demonstrates how device reports can be deferred to be sent later 
// in order to avoid overloading the BLE connection.
// You can send queued reports manually by calling the sendDeferredReports() function,
// or send them regularly by using the setThreadedAutoSend(true) option

#include <BleCompositeHID.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <GamepadDevice.h>

// Create variables to hold our devices
GamepadDevice* gamepad = nullptr;
KeyboardDevice* keyboard = nullptr;
MouseDevice* mouse = nullptr;

// Create our composite HID device that will manage our devices
BleCompositeHID compositeHID("CompositeHID Keyboard Mouse Gamepad", "Mystfit", 100);

// Variable to track the last time a report was sent
int lastReportTime = 0;

void setup() {
    Serial.begin(115200);

    // To use deferred reports, we can set the autoDefer flag to true in each device's configuration object
    // This means that any time we tell a device to send a report, we will queue it instead.
    // We can then either send all queued reports at once manually, or send them regularly in the background
    // if we set the autoSendActive flag to true in the BLEHostConfiguration object.

    // Set up gamepad config
    GamepadConfiguration gamepadConfig;
    gamepadConfig.setAutoDefer(true);

    // Set up gamepad device instance
    gamepad = new GamepadDevice(gamepadConfig);
  
    // Set up keyboard config
    KeyboardConfiguration keyboardConfig;
    keyboardConfig.setAutoDefer(true);

    // Set up keyboard device instance
    keyboard = new KeyboardDevice(keyboardConfig);

    // Set up mouse
    MouseConfiguration mouseConfig;
    mouseConfig.setAutoDefer(true);

    // Set up mouse device instance
    mouse = new MouseDevice(mouseConfig);

     // Add all devices to the composite HID device to manage them
    compositeHID.addDevice(keyboard);
    compositeHID.addDevice(mouse);
    compositeHID.addDevice(gamepad);

    // We can also let our composite HID device send reports at a regular interval.
    // This works by creating a background task that will periodically send all queued reports.
    BLEHostConfiguration config;
    config.setQueueSendRate(240);
    config.setQueuedSending(true);

    // Start the composite HID device to broadcast HID reports
    compositeHID.begin(config);

    // Wait a little bit for the connection to start before sending reports
    delay(3000);
}

void loop() {
    // Make sure we;re connected to a client before sending any reports
    if(compositeHID.isConnected())
    {   
        // Button states
        bool gamepadPressed = false;
        bool keyPressed = false;
        
        int currentTime = millis();
        int elapsedTime = currentTime - lastReportTime;

        // Test mouse by moving it in a circle
        mouse->mouseMove(
            round(cos((float)currentTime / 100.0f) * 10.0f), 
            round(sin((float)currentTime / 100.0f) * 10.0f)
        );

        if(elapsedTime > 1000){
            // Test keyboard by pressing and releasing the A key every second
            keyPressed = !keyPressed;
            if(keyPressed){
                keyboard->keyPress(KEY_A);
            } else {
                keyboard->keyRelease(KEY_A);
            }
        
            // Test gamepad by pressing and releasing the A button every second
            gamepadPressed = !gamepadPressed;
            if(gamepadPressed){
                gamepad->press(BUTTON_1);
            } else {
                gamepad->release(BUTTON_1);
            }
        }

        // Instead of using the queued send feature, you can call the sendDeferredReports() function
        // to send all queued reports manually. Uncomment the next line to enable.
        //compositeHID.sendDeferredReports();
        
        lastReportTime = currentTime;
        delay(2);
    }
}