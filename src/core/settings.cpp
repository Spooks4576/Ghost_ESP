#include "Settings.h"
#include "core/system_manager.h"
#include <iostream>  // For debugging purposes

// Define NVS keys
const char* FSettings::NVS_RGB_MODE_KEY = "rgb_mode";
const char* FSettings::NVS_CHANNEL_SWITCH_DELAY_KEY = "channel_delay";

FSettings::FSettings() : rgbMode(RGBMode::Stealth), channelSwitchDelay(0.0f) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
        loadSettings();
    }
}

FSettings::~FSettings() {
    nvs_close(nvsHandle);
}

void FSettings::setRGBMode(RGBMode mode) {
    if (mode == RGBMode::Rainbow) {
        SystemManager::getInstance().RainbowLEDActive = true;
    } else {
        SystemManager::getInstance().RainbowLEDActive = false;
        SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
    }

    rgbMode = mode;
    saveSettings();
}

FSettings::RGBMode FSettings::getRGBMode() const {
    return rgbMode;
}

void FSettings::setChannelSwitchDelay(float delay_ms) {
    channelSwitchDelay = delay_ms;
    saveSettings();
}

float FSettings::getChannelSwitchDelay() const {
    return channelSwitchDelay * 1000;
}

void FSettings::loadSettings() {
    // Load RGB Mode
    uint8_t storedRGBMode = 0;
    esp_err_t err = nvs_get_u8(nvsHandle, NVS_RGB_MODE_KEY, &storedRGBMode);
    if (err == ESP_OK) {
        rgbMode = static_cast<RGBMode>(storedRGBMode);
    } else {
        rgbMode = RGBMode::Stealth; // Default value
    }

    size_t required_size = sizeof(float);
    err = nvs_get_blob(nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &channelSwitchDelay, &required_size);
    if (err != ESP_OK) {
        channelSwitchDelay = 0.0f; // Default value
    }
}

void FSettings::saveSettings() {
    nvs_set_u8(nvsHandle, NVS_RGB_MODE_KEY, static_cast<uint8_t>(rgbMode));
    
    nvs_set_blob(nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &channelSwitchDelay, sizeof(float));

    nvs_commit(nvsHandle);
}