# Ghost ESP: Next Generation Wi-Fi Pentesting

**⭐️ Enjoying Ghost ESP? Please give our repo a star!**

Ghost ESP turns your ESP32 into a powerful, cheap and helpful wireless testing tool. Built on ESP-IDF.

> **Note:** This is an **Alpha release**. Some features are still under development and may be unstable.

---

## Getting Started

1. Follow our [Flashing Guide](https://github.com/Spooks4576/Ghost_ESP/wiki) for installation and configuration.
2. Join our community on [Discord](https://discord.gg/PkdjxqYKe4) for support and feedback.
3. Visit our [Official Website](https://ghostesp.net) to stay in touch!

---

## Key Features

### WiFi
- **AP Scanning:** Detect nearby WiFi networks.
- **Station Scanning:** Monitor connected WiFi clients.
- **Beacon Spam:** Broadcast customizable SSID beacons.
- **Deauthentication Attacks:** Disconnect clients from specific networks.
- **WiFi Capture:** Log probe requests, beacon frames, deauth packets, and raw data *(requires SD card or compatible storage)*.
- **Evil Portal:** Set up a fake WiFi portal with a custom SSID and domain.
- **Pineapple Detection** Detect Wi-Fi Pineapples and Evil Twin Attacks
- **Web-UI** In-built Web-UI for changing settings easily.
- **Port Scanning** Scan your local network for open ports!
### BLE
- **BLE Scanning:** Detect BLE devices, including specialized modes for AirTags, Flipper Zeros and more.
- **BLE Packet Capture:** Capture and analyze BLE traffic.
- **BLE Wardriving:** Map and track BLE devices in your vicinity.

### Additional
- **GPS Integration:** Retrieve location info via the `gpsinfo` command *(on supported hardware)*.
- **RGB LED Modes:** Customizable LED feedback (Stealth, Normal, Rainbow).
- **DIAL & Chromecast V2 Support:** Interact with DIAL-capable devices (e.g., Roku, Chromecast).
- **Flappy Ghost and Rave Modes** Extra Apps for boards with displays!
> We do NOT support BLE Spam at this time.
---

## Supported ESP32 Models

- **ESP32 Wroom**
- **ESP32 S2**
- **ESP32 C3**
- **ESP32 S3**
- **ESP32 C6**

> **Note:** Feature availability may vary by model.
> We do NOT support BLE Spam
---

## Ghost ESP vs. ESP32 Marauder
![399090585-84fde8b3-e17e-44d5-9321-04f1f1ae8541](https://github.com/user-attachments/assets/1455802e-390a-4320-a224-aba2bdd412d0)

---

## Acknowledgments

Special thanks to:

- **[JustCallMeKoKo](https://github.com/justcallmekoko/ESP32Marauder):** For foundational ESP32 development.
- **[thibauts](https://github.com/thibauts/node-castv2-client):** For CastV2 protocol insights.
- **[MarcoLucidi01](https://github.com/MarcoLucidi01/ytcast/tree/master/dial):** For DIAL protocol integration.
- **[SpacehuhnTech](https://github.com/SpacehuhnTech/esp8266_deauther):** For reference deauthentication code.

---

## Legal Disclaimer

Ghost ESP is intended solely for educational and ethical security research. Unauthorized or malicious use is illegal. Always obtain proper permissions before conducting any network tests.

---

## Open Source Contributions

This project is open source and welcomes your contributions. If you've added new features or enhanced device support, please submit your changes!
