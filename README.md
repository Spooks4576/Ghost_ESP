# Ghost ESP Firmware: Empowering Wireless Testing with ESP32

Welcome to **Ghost ESP**, a robust and feature-rich firmware designed for your ESP32 microcontroller, enabling advanced wireless network exploration and security testing. With Ghost ESP installed, you can leverage the power of your ESP32 device to conduct in-depth WiFi and Bluetooth Low Energy (BLE) analysis, launch strategic wireless attacks, and explore a wide range of wireless environments.

## Notice: Beta Version (ESP-IDF)

Please note that this version of Ghost ESP firmware, built on the ESP-IDF framework, is currently in **beta**. Some features, such as **BLE spam** and **screen support**, are still under development and will be introduced in future updates as we approach the full release.

We appreciate your patience and feedback as we continue to refine and expand the functionality of Ghost ESP.

## Key Features

- **WiFi Access Point (AP) Scanning:** Detect and display all nearby WiFi networks, providing detailed visibility into wireless environments.
- **Station Scanning:** Identify and monitor WiFi stations (clients) connected to local networks, providing insight into active devices.
- **Beacon Spam:** Deploy SSID beacons with customizable modes to simulate or disrupt network environments.
- **BLE Scanning** Scan for BLE devices, including specialized modes for detecting devices like AirTags and Flipper Zeros.
- **Deauthentication Attacks:** Launch deauthentication attacks to disconnect clients from WiFi networks.
- **WiFi Capture:** Capture probe requests, beacon frames, deauthentication packets, and raw wireless data for analysis (requires an SD card or external storage device).
- **Evil Portal:** Set up a fake WiFi portal with a custom SSID and domain, enabling targeted network testing.
- **RGB LED Modes:** Customize the RGB lighting of your ESP32 device with various modes such as Stealth, Normal, and Rainbow, creating visual feedback for different tasks.
- **BLE Detectors:** Specialized scanning modes for identifying elusive BLE devices, such as AirTags and Flipper Zeros, or scanning for raw BLE packets in real-time (planned for future versions).
- **DIAL & Chromecast V2 Support:** Interact with DIAL-enabled devices like Roku or Chromecast for seamless media control and playback.

## Compatible ESP32 Boards

Ghost ESP supports the following ESP32 models:
- **ESP32 Wroom**
- **ESP32 S2**
- **ESP32 C3**
- **ESP32 S3**
- **ESP32 C6** (Note: Partial support; BLE advertising not fully functional)

## Important Considerations

Ghost ESP offers powerful BLE and WiFi functionalities, including spam and scanning capabilities. However, certain ESP32 models, such as the ESP32 Wroom, may experience performance limitations when running resource-intensive operations like BLE spam (once available), which could result in system crashes. 

Use these features responsibly and ensure that you operate within the legal boundaries of wireless testing and security research.

## Getting Started

Ready to unlock the full potential of your ESP32 device with Ghost ESP? Follow our comprehensive flashing tutorial to install and configure the firmware:

[![Flashing Tutorial](https://img.shields.io/badge/Tutorial-Flashing-blue)](https://github.com/Spooks4576/Ghost_ESP/blob/main/docs/HOWTOFLASH.md)

## Acknowledgments

We owe the success of Ghost ESP to the contributions and inspiration from the following open-source projects and their developers:
- **[JustCallMeKoKo](https://github.com/justcallmekoko/ESP32Marauder):** For laying the groundwork of ESP32 development and tools.
- **[thibauts](https://github.com/thibauts/node-castv2-client):** For offering crucial insights into the CastV2 protocol, aiding media integration.
- **[MarcoLucidi01](https://github.com/MarcoLucidi01/ytcast/tree/master/dial):** For spearheading the development of DIAL protocol integration on ESP32 platforms.

We express our deepest gratitude to these innovators for their invaluable contributions, which helped shape the Ghost ESP project.

## Legal Disclaimer

Ghost ESP is intended for educational purposes and ethical security research only. Unauthorized use or deployment of this firmware for malicious purposes, such as disrupting legitimate network services, is illegal and punishable under applicable laws. Always obtain proper authorization before conducting wireless security tests.

---

Unleash the true potential of your ESP32 device with Ghost ESP—your ultimate tool for wireless network exploration and testing.