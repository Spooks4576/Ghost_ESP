# Ghost ESP Firmware - Frequently Asked Questions (FAQ)

## I ran into a boot loop, what should I do?

A boot loop can happen for various reasons, such as configuration errors or memory issues. Here’s how you can troubleshoot:

1. **Check Your Configuration:** Ensure the settings, especially Wi-Fi settings, are properly configured. Misconfigurations can cause continuous reboots.
2. **Reset the Device:** Try resetting the ESP32 by holding the reset button, or flash a fresh copy of the firmware to eliminate any corrupt configurations.
3. **Erase Flash Memory:** If resetting doesn't work, you can erase the flash memory using the following command: esptool.py erase_flash

This will clear any problematic settings.
4. **Check Power Supply:** Ensure that your power supply provides stable and sufficient power for your ESP32 device.

## I'm experiencing a bug or crash, what should I do?

If you're facing unexpected behavior, bugs, or crashes:

1. **Check the Logs:** Use the serial monitor to check for error messages or logs that might indicate what went wrong. This can help narrow down the issue.
2. **Update Firmware:** Ensure you're using the latest version of Ghost ESP firmware. New releases often fix bugs or performance issues.
3. **Reproduce the Bug:** Try to recreate the bug or crash under similar conditions to gather more information about what causes it.
4. **Report the Issue:** If you believe it’s a firmware issue, please report it on the project's GitHub issues page. Include detailed steps to reproduce, your device information, and logs if possible.

## A feature is not working, what should I do?

If a feature is not functioning as expected:

1. **Check the Help Menu:** Verify the usage of the command by running `help` to ensure you're using the correct syntax.
2. **Check Compatibility:** Some features may not be fully supported on all ESP32 models. Ensure your board is compatible with the specific feature.
3. **Check for Beta Features:** Some features (e.g., BLE spam, screen support) are still in development and may not be fully functional. These will be included in future firmware updates.
4. **Update Firmware:** Ensure you are using the latest version of the firmware. Feature fixes and improvements are often released in updates.

## I'm having trouble flashing the firmware, what should I do?

If you're facing issues flashing the firmware onto your ESP32 device:

1. **Verify USB Connection:** Ensure the ESP32 is properly connected to your computer via a USB cable that supports data transfer.
2. **Install Drivers:** Make sure the necessary drivers for your ESP32 board are installed, especially if you're using Windows.
3. **Check Flashing Tool:** Use the recommended flashing tool (e.g., `esptool.py`) and make sure the correct port and settings are selected.
4. **Erase Flash:** You can try erasing the flash before flashing the firmware by using the command: esptool.py erase_flash

5. **Lower Baud Rate:** Sometimes, lowering the baud rate can help if the connection is unstable. Use the `-b 115200` flag with the flashing tool.

## Can I customize the settings for my device?

Yes, you can use the `setsetting` command to modify various settings such as RGB modes, channel hopping, and more. Here are some common settings you can adjust:

- **RGB Mode:** Control the lighting mode of your device.
- **Channel Hopping:** Enable or disable automatic channel hopping for better network scanning.
- **Random BLE MAC:** Toggle randomization of the BLE MAC address for increased privacy.

Check the full list of configurable options using `help` under the `setsetting` command.

## What ESP32 boards are supported?

Ghost ESP supports several ESP32 variants including:

- **ESP32 Wroom**
- **ESP32 S2**
- **ESP32 C3**
- **ESP32 S3**
- **ESP32 C6** (Partial support; BLE advertising is broken)

Refer to the official documentation for the latest compatibility information.

## Can I use this firmware on other platforms?

Currently, Ghost ESP firmware is designed specifically for the ESP32 family of microcontrollers. Support for other platforms may be considered in the future, but it’s not available at this time.

## How do I contribute to this project?

We welcome contributions from the community! Here’s how you can help:

1. **Report Bugs:** If you find a bug, report it on the project's GitHub issues page with detailed information.
2. **Feature Requests:** Feel free to suggest new features or improvements.
3. **Submit Code:** If you’re familiar with ESP-IDF and open-source development, you can submit pull requests for new features or bug fixes.
4. **Testing:** Testing the beta features and providing feedback is immensely helpful as we work toward full releases.


## Why is My Touch Not Working Correctly? Selecting items is impossible?

Currently How The Touch Is Designed is on the main menu you select your item by pressing the item directly 

### However its a little different for the wifi and bluetooth scroll menus 

for those menus the upper half of the screen is to move up a item.
 
bottom half of the screen is to move down a item 

and middle of the screen is to select a item 

Any full screen menu with no navigation is simply click to go back 

I Hope this Clears up the confusion

---

If you have any other questions not covered here, feel free to reach out to the community on the [Ghost ESP GitHub page](https://github.com/Spooks4576/Ghost_ESP).
