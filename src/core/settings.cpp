#include "settings.h"
#include "core/system_manager.h"
#include <iostream>  // For debugging purposes

// Define NVS keys
const char* FSettings::NVS_RGB_MODE_KEY = "rgb_mode";
const char* FSettings::NVS_CHANNEL_SWITCH_DELAY_KEY = "channel_delay";
const char* FSettings::NVS_ENABLE_CHANNEL_HOP_KEY = "enable_channel_hop";
const char* FSettings::NVS_RANDOM_BLE_MAC = "random_ble_mac";

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

bool FSettings::ChannelHoppingEnabled() const
{
    return EnableChannelHopping;
}

void FSettings::setChannelSwitchDelay(float delay_ms) {
    channelSwitchDelay = delay_ms;
    saveSettings();
}

void FSettings::setChannelHoppingEnabled(bool Enabled)
{
    EnableChannelHopping = Enabled;
    saveSettings();
}

void FSettings::SetRandomBLEMacEnabled(bool NewValue)
{
    RandomBLEMacEnabled = NewValue;
    saveSettings();
}

bool FSettings::getRandomBLEMacEnabled() const 
{
    return RandomBLEMacEnabled;
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
        channelSwitchDelay = 1; // Default value
    }


    err = nvs_get_i8(nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, (int8_t*)&EnableChannelHopping);
    if (err != ESP_OK) {
        EnableChannelHopping = true;
    }

    err = nvs_get_i8(nvsHandle, NVS_RANDOM_BLE_MAC, (int8_t*)&RandomBLEMacEnabled);
    if (err != ESP_OK) {
        RandomBLEMacEnabled = true;
    }
}

void FSettings::saveSettings() {
    nvs_set_u8(nvsHandle, NVS_RGB_MODE_KEY, static_cast<uint8_t>(rgbMode));
    
    nvs_set_blob(nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &channelSwitchDelay, sizeof(float));

    nvs_set_i8(nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, static_cast<int8_t>(EnableChannelHopping));

    nvs_set_i8(nvsHandle, NVS_RANDOM_BLE_MAC,static_cast<int8_t>(RandomBLEMacEnabled));

    nvs_commit(nvsHandle);
}