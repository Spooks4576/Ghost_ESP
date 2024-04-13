#pragma once
#include <Arduino.h>
#include <components/wifi_module/wifi_module.h>
#include <components/sdcard_module/sd_card_module.h>
#include <components/rgb_led_module/rgb_led_module.h>
#include <components/neopixel_module/neopixel_module.h>
#include <components/gps_module/gps_module.h>
#include <components/controller_module/controller_module.h>
#include <components/ble_module/ble_module.h>
#include <components/display_module/display_module.h>
#include "../lib/TFT_eSPI/User_Setup.h"

class SystemManager {
public:
    static SystemManager& getInstance() {
        static SystemManager instance;
        return instance;
    }

    void setup();

    void loop();

    static void SerialCheckTask(void *pvParameters)
    {
        while (1) {
            SystemManager::getInstance().ControllerModule.loop();
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
    BLEModule* bleModule;
    RGBLedModule* rgbModule;
    NeopixelModule* neopixelModule;
    controller_interface ControllerModule;

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
        sdCardModule.init();
#endif
    }

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
#define LOG_RESULTS(filename, folder, message)
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

    String bytesToHexString(const uint8_t* bytes, size_t length) {
        String str = "";
        for (size_t i = 0; i < length; ++i) {
            // Convert each byte to Hex and append to the string
            if (bytes[i] < 16) str += '0'; // Add leading zero for values less than 0x10
            str += String(bytes[i], HEX);
        }
        return str;
    }
}