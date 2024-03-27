#pragma once
#include <stdint.h>
#include <Arduino.h>
#include "board_config.h"

#include "../core/commandline.h"
#include <LinkedList.h>

#include "../components/wifi_module/wifi_module.h"

class WiFiModule;

inline WiFiModule* wifimodule;
inline bool RainbowLEDActive;

struct SerialCallback {
    std::function<bool(String&)> condition;
    std::function<void(String&)> callback;
    int CallbackID;
};


inline void stringToUint8Array(String inputString, uint8_t* outputArray, int maxOutputLength) {
    int byteIndex = 0;
    int strLength = inputString.length();
    
    for (int i = 0; i < strLength && byteIndex < maxOutputLength; i += 2) {
        String byteString = inputString.substring(i, i + 2);
        
        uint8_t byteValue = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        
        outputArray[byteIndex++] = byteValue;
    }
}

inline LinkedList<SerialCallback> callbacks;

inline void registerCallback(const std::function<bool(String&)>& condition, const std::function<void(String&)>& callback) {
    int CallbackID = callbacks.size() + 1;
    callbacks.add({condition, callback, CallbackID});
}

inline void clearCallbacks() {
    callbacks.clear();
}

#ifdef HAS_BT
#include "../components/ble_module/ble_module.h"
inline BLEModule* BleModule;
#endif

#ifdef SD_CARD_CS_PIN
#include "../components/sdcard_module/sd_card_module.h"
inline SDCardModule* sdCardmodule;
#endif

#ifdef OLD_LED
#include "../components/rgb_led_module/rgb_led_module.h"
inline RGBLedModule* rgbmodule;
#endif

#ifdef NEOPIXEL_PIN
#include "../components/neopixel_module/neopixel_module.h"
inline NeopixelModule* neopixelmodule;
#endif

#ifdef HAS_GPS
#include "../components/gps_module/gps_module.h"
//inline GpsModule* gpsmodule;
#endif

#ifdef DISPLAY_SUPPORT
#include "../components/display_module/display_module.h"
#include "../lib/TFT_eSPI/User_Setup.h"
inline DisplayModule* displaymodule;
#endif

struct LEDThreads
{
TaskHandle_t* RainbowTaskHandle;
TaskHandle_t* BreatheTaskHandle;
int TargetPin;
};

inline LEDThreads Threadinfo;

inline void BreatheTask()
{
#ifdef OLD_LED
    rgbmodule->breatheLED(Threadinfo.TargetPin, 500);
#endif
}

inline void RainbowTask()
{
#ifdef OLD_LED
    rgbmodule->Rainbow(0.1, 4);
#elif SD_CARD_CS_PIN
    neopixelmodule->rainbow(255, 4);
#endif
}

#ifdef SD_CARD_CS_PIN
#define LOG_MESSAGE_TO_SD(message) sdCardmodule->logMessage("GhostESP.txt", "logs", message)
#define LOG_RESULTS(filename, folder, message) sdCardmodule->logMessage(filename, folder, message)
#else
#define LOG_MESSAGE_TO_SD(message) // Not Supported do nothing
#define LOG_SCAN_RESULTS(filename, folder, message)
#endif