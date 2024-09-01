#pragma once
#include <Arduino.h>
#include <components/wifi_module/wifi_module.h>
#include <components/sdcard_module/sd_card_module.h>
#include <components/rgb_led_module/rgb_led_module.h>
#include <components/neopixel_module/neopixel_module.h>
#include <components/ble_module/ble_module.h>
#include <components/display_module/display_module.h>
#include "../lib/TFT_eSPI/User_Setup.h"
#include "settings.h"

enum ENeoColor
{
    Red,
    Green,
    Blue
};

class gps_module;

class SystemManager {
public:
    static SystemManager& getInstance() {
        static SystemManager instance;
        return instance;
    }

    void setup();

    void loop();

    void SetLEDState(ENeoColor NeoColor = ENeoColor::Red, bool FadeOut = false)
    {
if (Settings.getRGBMode() == FSettings::RGBMode::Normal)
{
#ifdef OLD_LED
if (FadeOut)
{
    rgbModule->fadeOutAllPins(500);
}
else 
{
    analogWrite(rgbModule->redPin, FadeOut ? 255 : 0);
}
#endif
#ifdef NEOPIXEL_PIN
neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(NeoColor == ENeoColor::Red && !FadeOut ? 255 : 0, NeoColor == ENeoColor::Green && !FadeOut ? 255 : 0, NeoColor == ENeoColor::Blue && !FadeOut ? 255 : 0), 300, false);
#endif
}
    }

    static void SerialCheckTask(void *pvParameters)
    {
        while (1) {
            if (SystemManager::getInstance().RainbowLEDActive)
            {
#ifdef OLD_LED
    SystemManager::getInstance().rgbModule->Rainbow(0.1, 4);
#elif NEOPIXEL_PIN
    SystemManager::getInstance().neopixelModule->rainbow(255, 4);
#endif
            }
            #ifndef DISPLAY_SUPPORT
            if (HasRanCommand)
            {   
                if (Serial.available() > 0) {
                    String message = Serial.readStringUntil('\n');
                    Serial.println(message);

                    if (message.startsWith("stop"))
                    {
                        if (HasRanCommand)
                        {
                            esp_restart();
                        }
                        else 
                        {
                            SystemManager::getInstance().wifiModule.shutdownWiFi();
                            #ifdef SD_CARD_CS_PIN
                            SystemManager::getInstance().sdCardModule.stopPcapLogging();
                            #endif
                            #ifdef HAS_BT
                            SystemManager::getInstance().bleModule->shutdownBLE();
                            #endif
                        }
                    }
                }
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
            #endif
        }
    }

public:
    WiFiModule wifiModule;
#ifdef DISPLAY_SUPPORT
    DisplayModule* displayModule;
#endif
    SDCardModule sdCardModule;
    gps_module* gpsModule;
    BLEModule* bleModule;
    FSettings Settings;
    RGBLedModule* rgbModule;
    NeopixelModule* neopixelModule;

    SystemManager() {}

    void initDisplay() {
#ifdef DISPLAY_SUPPORT
        displayModule = new DisplayModule();
        displayModule->Init();
#endif
    }

    void initLEDs() {
#ifdef OLD_LED
        rgbModule = new RGBLedModule(LED_R, LED_G, LED_B);
        rgbModule->init();
#elif defined(NEOPIXEL_PIN)
        neopixelModule = new NeopixelModule(Pixels, NEOPIXEL_PIN);
        neopixelModule->init();
#endif
    }

    void initSDCard() {
#ifdef SD_CARD_CS_PIN
        pinMode(SD_CARD_CS_PIN, OUTPUT);
        digitalWrite(SD_CARD_CS_PIN, HIGH);
#endif
        sdCardModule.init();
    }

    void initGPSModule();

    void initBLE() {
#ifdef HAS_BT
        bleModule = new BLEModule();
        bleModule->init();
#endif
    }

    void initWiFi() {
        wifiModule.RunSetup();
    }

    uint32_t lastTick = 0;
    bool RainbowLEDActive;
    SystemManager(SystemManager &other) = delete;
    void operator=(const SystemManager &) = delete;
};

#ifdef SD_CARD_CS_PIN
#define LOG_MESSAGE_TO_SD(message) SystemManager::getInstance().sdCardModule.logMessage("GhostESP.txt", "logs", message)
#define LOG_RESULTS(filename, folder, message) SystemManager::getInstance().sdCardModule.logMessage(filename, folder, message)
#else
#define LOG_MESSAGE_TO_SD(message) // Not Supported do nothing
#define LOG_RESULTS(filename, folder, message) SystemManager::getInstance().sdCardModule.logPacket((const uint8_t*)message, strlen(message))
#endif

namespace G_Utils
{
    void stringToUint8Array(String inputString, uint8_t* outputArray, int maxOutputLength) {
        int byteIndex = 0;
        int strLength = inputString.length();
        
        for (int i = 0; i < strLength && byteIndex < maxOutputLength; i += 2) {
            String byteString = inputString.substring(i, i + 2);
            
            uint8_t byteValue = (uint8_t) strtol(byteString.c_str(), NULL, 16);
            
            outputArray[byteIndex++] = byteValue;
        }
    }

    AccessPoint getSelectedAccessPoint(LinkedList<AccessPoint>* accessPoints) {
        for (int i = 0; i < accessPoints->size(); i++) {
            AccessPoint ap = accessPoints->get(i);
            if (ap.selected) {
                return ap;
            }
        }
        return AccessPoint();
    }

    String bytesToHexString(const uint8_t* bytes, size_t length) {
        String str = "";
        for (size_t i = 0; i < length; ++i) {
            // Convert each byte to Hex and append to the string
            if (bytes[i] < 16) str += '0'; // Add leading zero for values less than 0x10
            str += String(bytes[i], HEX);
        }
        return str;
    }

    size_t calculateAccessPointSize(const AccessPoint& ap) {
        size_t size = sizeof(AccessPoint);

        size += ap.essid.length() + 1;
        size += ap.Manufacturer.length() + 1;


        if (ap.beacon != nullptr) {
            size += sizeof(LinkedList<char>);
            size += ap.beacon->size();
        }

        if (ap.stations != nullptr) {
            size += sizeof(LinkedList<int>);
            size += ap.stations->size() * sizeof(int); 
        }

        return size;
    }

    bool isMemoryLow(size_t requiredMemory) {
        size_t freeHeap = ESP.getFreeHeap();
        size_t safetyMargin = 1024;
        return (freeHeap < requiredMemory + safetyMargin);
    }

    String formatString(const char* format, ...) 
    {
        char buffer[500];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return String(buffer);
    }

    String authTypeToString(wifi_auth_mode_t type)
    {
        switch (type)
        {
            case wifi_auth_mode_t::WIFI_AUTH_OPEN:
                return "OPEN";
            case wifi_auth_mode_t::WIFI_AUTH_WEP:
                return "WEP";
            case wifi_auth_mode_t::WIFI_AUTH_WPA_PSK:
                return "WPA_PSK";
            case wifi_auth_mode_t::WIFI_AUTH_WPA2_PSK:
                return "WPA2_PSK";
            case wifi_auth_mode_t::WIFI_AUTH_WPA_WPA2_PSK:
                return "WPA_WPA2_PSK";
            case wifi_auth_mode_t::WIFI_AUTH_WPA2_ENTERPRISE:
                return "WPA2_ENTERPRISE";
            case wifi_auth_mode_t::WIFI_AUTH_WPA3_PSK:
                return "WPA3_PSK";
            case wifi_auth_mode_t::WIFI_AUTH_WPA2_WPA3_PSK:
                return "WPA2_WPA3_PSK";
            case wifi_auth_mode_t::WIFI_AUTH_WAPI_PSK:
                return "WAPI_PSK";
            default:
                return "UNKNOWN";
        }
    }

    String parseSSID(const uint8_t* payload, int length) {
        String ssid = "";
        int ssidLength = payload[37]; // SSID length is usually at this offset

        if (ssidLength > 0 && ssidLength < 32) { // Valid SSID lengths
            for (int i = 38; i < (38 + ssidLength); i++) {
                ssid += (char)payload[i];
            }
        }

        return ssid;
    }
}