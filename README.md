# Ghost ESP Firmware: Cutting-Edge Wireless Testing for ESP32

Welcome to **Ghost ESP**, a versatile and powerful firmware designed to transform your ESP32 microcontroller into a sophisticated wireless testing tool. With Ghost ESP, your ESP32 can perform comprehensive WiFi and Bluetooth Low Energy (BLE) analysis, execute targeted wireless tests, and explore dynamic wireless environments.

## ⚠️ Notice: Alpha Version (ESP-IDF)

**This is an Alpha release** of Ghost ESP firmware built on the ESP-IDF framework.

**Please note:** certain features, such as **BLE spam** and **display support**, are actively in development and may be incomplete or unstable. Some ESP32 models may experience compatibility or performance limitations when using resource-intensive functionalities. This release aims to gather feedback from early adopters, which will guide improvements toward a stable release.

We appreciate your understanding and contributions as we refine Ghost ESP.

## Key Features

Ghost ESP offers an array of tools for wireless testing, with new functionalities being added in future updates. Here's an overview of current features:

### WiFi Features
- **WiFi Access Point (AP) Scanning:** Identify and display nearby WiFi networks for insights into surrounding network activity
- **Station Scanning:** Monitor WiFi stations (clients) connected to nearby networks, observing active devices
- **Beacon Spam:** Deploy customizable SSID beacons to simulate or disrupt local WiFi environments
- **Deauthentication Attacks:** Execute deauthentication attacks to disconnect clients from specific WiFi networks
- **WiFi Capture:** Capture and store probe requests, beacon frames, deauthentication packets, and raw wireless data for detailed analysis (requires an SD card or compatible storage)
- **Evil Portal:** Create a fake WiFi portal with a custom SSID and domain for controlled network testing

### BLE Features
- **BLE Scanning:** Detect BLE devices, including unique modes for identifying devices like AirTags and Flipper Zeros
- **BLE Packet Capture:** Capture and analyze Bluetooth Low Energy packets, including support for detecting card skimmers
- **BLE Wardriving:** Map and track BLE devices in your area
- **BLE Detectors:** Specialized BLE scan modes to find specific devices like AirTags, Flipper Zeros, and other BLE emitters (upcoming in future versions)

### Additional Features
- **GPS Integration:** Get location information using the "gpsinfo" command (on supported hardware)
- **RGB LED Modes:** Customize RGB lighting for different task feedback with modes such as Stealth, Normal, and Rainbow
- **DIAL & Chromecast V2 Support:** Interact with DIAL-capable devices such as Roku or Chromecast for media control and playback

## Supported ESP32 Models

The following ESP32 models are compatible with Ghost ESP, though specific features may vary by model:

- **ESP32 Wroom**
- **ESP32 S2**
- **ESP32 C3**
- **ESP32 S3**
- **ESP32 C6**

### Important Compatibility Considerations

Ghost ESP is an advanced tool that leverages the ESP32's WiFi and BLE capabilities to their fullest. Certain features, particularly BLE spam (still in development), may exceed the performance limits of some models, such as the ESP32 Wroom, leading to potential crashes or reduced functionality.

Due to the **alpha** status, users should expect potential limitations, feature inconsistencies, and some degree of instability. We encourage early adopters to report issues to help us improve the firmware.

## 🚀 Getting Started

To install and configure Ghost ESP, follow our [Flashing Guide](https://github.com/Spooks4576/Ghost_ESP/wiki) for step-by-step instructions. Be sure to check for any known limitations specific to your ESP32 model before proceeding.

Make sure to check out our [Discord](https://discord.gg/PkdjxqYKe4)

## What Makes this different than ESP32 Marauder 
This table should explain the key differences between ghost esp and ESP32 Marauder

![Ghost_VS_Marauder](https://github.com/user-attachments/assets/84fde8b3-e17e-44d5-9321-04f1f1ae8541)



## Special Acknowledgments

We extend our thanks to the following projects and their developers for their contributions and inspiration in the development of Ghost ESP:

- **[JustCallMeKoKo](https://github.com/justcallmekoko/ESP32Marauder):** For foundational ESP32 development and tools  
- **[thibauts](https://github.com/thibauts/node-castv2-client):** For insights into the CastV2 protocol for media integration  
- **[MarcoLucidi01](https://github.com/MarcoLucidi01/ytcast/tree/master/dial):** For pioneering DIAL protocol integration on ESP32 platforms  
- **[SpacehuhnTech](https://github.com/SpacehuhnTech/esp8266_deauther):** For providing excellent reference code for deauthentication functionality  


## Legal Disclaimer

Ghost ESP is intended strictly for educational and ethical security research. Unauthorized use of this firmware for malicious activities, such as disrupting legitimate network services, is illegal and subject to prosecution. Always secure appropriate permissions before conducting any network testing.

## Open Source Contributions

This project is fully open source and welcomes modifications and improvements from the community. While we may not have access to every ESP32-based device for testing, we encourage users to contribute device-specific support and other enhancements. If you've made modifications or added support for additional devices, please feel free to submit your contributions.
