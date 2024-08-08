#include "Settings.h"
#include "core/system_manager.h"

FSettings::FSettings() : settings(0) {
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

    if (mode == RGBMode::Rainbow)
    {
        SystemManager::getInstance().RainbowLEDActive = true;
    }
    else
    {
        SystemManager::getInstance().RainbowLEDActive = false;
        SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
    }

    settings = (settings & ~RGB_MODE_MASK) | (static_cast<uint8_t>(mode) << RGB_MODE_SHIFT);
    saveSettings();
}

FSettings::RGBMode FSettings::getRGBMode() const {
    return static_cast<RGBMode>((settings & RGB_MODE_MASK) >> RGB_MODE_SHIFT);
}

void FSettings::loadSettings() {
    esp_err_t err = nvs_get_u8(nvsHandle, "settings", &settings);
    if (err != ESP_OK) {
        settings = 0; // Default settings if not found
    }
}

void FSettings::saveSettings() {
    nvs_set_u8(nvsHandle, "settings", settings);
    nvs_commit(nvsHandle);
}