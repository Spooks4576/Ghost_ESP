#pragma once
#include <stdint.h>
#include <Arduino.h>
#include "board_config.h"

#include "../core/commandline.h"
#include <LinkedList.h>

#include "../components/wifi_module/wifi_module.h"

class WiFiModule;

inline WiFiModule* wifimodule;

struct SerialCallback {
    std::function<bool(String&)> condition;
    std::function<void(String&)> callback;
    int CallbackID;
};

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
inline GpsInterface* gpsmodule;
#endif

#ifdef DISPLAY_SUPPORT
#include "../components/display_module/display_module.h"
inline DisplayModule* displaymodule;
#endif