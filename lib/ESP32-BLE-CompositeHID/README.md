# ESP32-BLE-CompositeHID

Forked from ESP32-BLE-Gamepad by lemmingDev to provide support support for composite human interface devices.

This library will let your ESP32 microcontroller behave as a bluetooth mouse, keyboard, gamepad (XInput or generic), or a combination of any of these devices.

## License
Published under the MIT license. Please see license.txt.

## XInput gamepad features

 - [x] All buttons and joystick axes available
 - [x] XBox One S and XBox Series X controller support
 - [x] Linux XInput support (Kernel version < 6.5 only supports the XBox One S controller)
 - [x] Haptic feedback callbacks for strong and weak motor rumble support
 - [ ] LED support (pull requests welcome)

## Generic gamepad features (from ESP32-BLE-Gamepad)

 - [x] Button press (128 buttons)
 - [x] Button release (128 buttons)
 - [x] Axes movement (6 axes (configurable resolution up to 16 bit) (x, y, z, rZ, rX, rY) --> (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger))
 - [x] 2 Sliders (configurable resolution up to 16 bit) (Slider 1 and Slider 2)
 - [x] 4 point of view hats (ie. d-pad plus 3 other hat switches)
 - [x] Simulation controls (rudder, throttle, accelerator, brake, steering)
 - [x] Special buttons (start, select, menu, home, back, volume up, volume down, volume mute) all disabled by default

## Mouse features
 - [x] Configurable button count
 - [x] X and Y axes
 - [ ] Configurable axes

## Keyboard features
 - [x] Supports most USB HID scancodes
 - [x] Media key support
 - [x] LED callbacks for caps/num/scroll lock keys

## Composite BLE host features (adapted from ESP32-BLE-Gamepad)
 - [x] Configurable HID descriptors per device
 - [x] Configurable VID and PID values
 - [x] Configurable BLE characteristics (name, manufacturer, model number, software revision, serial number, firmware revision, hardware revision)	
 - [x] Report optional battery level to host
 - [x] Uses efficient NimBLE bluetooth library
 - [x] Compatible with Windows
 - [x] Compatible with Android (Android OS maps default buttons / axes / hats slightly differently than Windows)
 - [x] Compatible with Linux (limited testing)
 - [x] Compatible with MacOS X (limited testing)
 - [ ] Compatible with iOS (No - not even for accessibility switch - This is not a “Made for iPhone” (MFI) compatible device)

## Installation
- (Make sure your IDE of choice has support for ESP32 boards available. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- In the Arduino IDE go to "Tools" -> "Manage Libraries..." -> Filter for "NimBLE-Arduino" by h2zero and install.
- You can now go to "File" -> "Examples" -> "ESP32-BLE-CompositeHID" and select an example to get started.

## Example

``` C++
#include <BleCompositeHID.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <GamepadDevice.h>

GamepadDevice gamepad;
KeyboardDevice keyboard;
MouseDevice mouse;
BleCompositeHID compositeHID("CompositeHID Keyboard Mouse Gamepad", "Mystfit", 100);

void setup() {
    Serial.begin(115200);

     // Add all devices to the composite HID device to manage them
    compositeHID.addDevice(&keyboard);
    compositeHID.addDevice(&mouse);
    compositeHID.addDevice(&gamepad);

    // Start the composite HID device to broadcast HID reports
    compositeHID.begin();

    delay(3000);
}

void loop() {
  if(compositeHID.isConnected()){

    // Test mouse by moving it in a circle
    int startTime = millis();
    int reportCount = 0;

    int8_t lastX = 0;
    int8_t lastY = 0;
    bool gamepadPressed = false;

    while(millis() - startTime < 8000){
        reportCount++;
        int8_t x = round(cos((float)millis() / 1000.0f) * 10.0f);
        int8_t y = round(sin((float)millis() / 1000.0f) * 10.0f);
        mouse->mouseMove(x, y);

        // Test keyboard presses
        if(reportCount % 100 == 0){
            keyboard->keyPress(KEY_A);
            keyboard->keyRelease(KEY_A);
        }

        // Test gamepad button presses
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

```
By default, reports are sent on every button press/release or axis/slider/hat/simulation movement, however this can be disabled, and then you manually call sendReport on each device instance as shown in the IndividualAxes.ino example.

VID and PID values can be set. See TestAll.ino for example.

There is also Bluetooth specific information that you can use (optional):

Instead of `BleCompositeHID bleCompositeHID;` you can do `BleCompositeHID bleCompositeHID("Bluetooth Device Name", "Bluetooth Device Manufacturer", 100);`.
The third parameter is the initial battery level of your device.
By default the battery level will be set to 100%, the device name will be `Composite HID` and the manufacturer will be `Espressif`.

The battery level can be set during operation by calling, for example, `bleCompositeHID.setBatteryLevel(80);`

## Credits for ESP32-BLE-CompositeHID

Credit goes to lemmingDev for his work on [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) which most of the gamepad portion of this library was based upon. 

USB HID codes for keyboards created by MightyPork, 2016 (see KeyboardHIDCodes.h)

## Credits for ESP32-BLE-Gamepad

Credits to [T-vK](https://github.com/T-vK) as this library is based on his ESP32-BLE-Mouse library (https://github.com/T-vK/ESP32-BLE-Mouse) that he provided.

Credits to [chegewara](https://github.com/chegewara) as the ESP32-BLE-Mouse library is based on [this piece of code](https://github.com/nkolban/esp32-snippets/issues/230#issuecomment-473135679) that he provided.

Credits to [wakwak-koba](https://github.com/wakwak-koba) for the NimBLE [code](https://github.com/wakwak-koba/ESP32-NimBLE-Gamepad) that he provided.


You might also be interested in:
- [ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)
- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)

or the NimBLE versions at

- [ESP32-NimBLE-Mouse](https://github.com/wakwak-koba/ESP32-NimBLE-Mouse)
- [ESP32-NimBLE-Keyboard](https://github.com/wakwak-koba/ESP32-NimBLE-Keyboard)
- [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)