#include "managers/settings_manager.h"
#include "core/system_manager.h"
#include "managers/rgb_manager.h"
#include <stdio.h>  // For debugging purposes

// Define NVS keys
static const char* NVS_RGB_MODE_KEY = "rgb_mode";
static const char* NVS_CHANNEL_SWITCH_DELAY_KEY = "channel_delay";
static const char* NVS_ENABLE_CHANNEL_HOP_KEY = "enable_channel_hop";
static const char* NVS_RANDOM_BLE_MAC = "random_ble_mac";

void settings_init(FSettings* settings) {
    settings->rgbMode = RGB_MODE_STEALTH;
    settings->channelSwitchDelay = 0.0f;
    settings->enableChannelHopping = true;
    settings->randomBLEMacEnabled = true;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err = nvs_open("storage", NVS_READWRITE, &settings->nvsHandle);
    if (err == ESP_OK) {
        settings_load(settings);
    }
}

void settings_deinit(FSettings* settings) {
    nvs_close(settings->nvsHandle);
}

void settings_set_rgb_mode(FSettings* settings, RGBMode mode) {
    if (mode == RGB_MODE_RAINBOW) {
       xTaskCreate(rainbow_task, "Rainbow Task", 2048, &rgb_manager, 1, &rainbow_task_handle);
    } else {
        if (rainbow_task_handle != NULL)
        {
            vTaskDelete(rainbow_task_handle);
        }
        rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0);
    }

    settings->rgbMode = mode;
    settings_save(settings);
}

RGBMode settings_get_rgb_mode(const FSettings* settings) {
    return settings->rgbMode;
}

void settings_set_channel_switch_delay(FSettings* settings, float delay_ms) {
    settings->channelSwitchDelay = delay_ms;
    settings_save(settings);
}

float settings_get_channel_switch_delay(const FSettings* settings) {
    return settings->channelSwitchDelay * 1000;
}

void settings_set_channel_hopping_enabled(FSettings* settings, bool enabled) {
    settings->enableChannelHopping = enabled;
    settings_save(settings);
}

bool settings_get_channel_hopping_enabled(const FSettings* settings) {
    return settings->enableChannelHopping;
}

void settings_set_random_ble_mac_enabled(FSettings* settings, bool enabled) {
    settings->randomBLEMacEnabled = enabled;
    settings_save(settings);
}

bool settings_get_random_ble_mac_enabled(const FSettings* settings) {
    return settings->randomBLEMacEnabled;
}

void settings_load(FSettings* settings) {
    // Load RGB Mode
    uint8_t storedRGBMode = 0;
    esp_err_t err = nvs_get_u8(settings->nvsHandle, NVS_RGB_MODE_KEY, &storedRGBMode);
    if (err == ESP_OK) {
        settings->rgbMode = (RGBMode)storedRGBMode;
    } else {
        settings->rgbMode = RGB_MODE_STEALTH;  // Default value
    }

    size_t required_size = sizeof(float);
    err = nvs_get_blob(settings->nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &settings->channelSwitchDelay, &required_size);
    if (err != ESP_OK) {
        settings->channelSwitchDelay = 1.0f; // Default value
    }

    int8_t enableChannelHopping = 0;
    err = nvs_get_i8(settings->nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, &enableChannelHopping);
    if (err != ESP_OK) {
        settings->enableChannelHopping = true;
    } else {
        settings->enableChannelHopping = (bool)enableChannelHopping;
    }

    int8_t randomBLEMacEnabled = 0;
    err = nvs_get_i8(settings->nvsHandle, NVS_RANDOM_BLE_MAC, &randomBLEMacEnabled);
    if (err != ESP_OK) {
        settings->randomBLEMacEnabled = true;
    } else {
        settings->randomBLEMacEnabled = (bool)randomBLEMacEnabled;
    }
}

void settings_save(FSettings* settings) {
    nvs_set_u8(settings->nvsHandle, NVS_RGB_MODE_KEY, (uint8_t)settings->rgbMode);
    nvs_set_blob(settings->nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &settings->channelSwitchDelay, sizeof(float));
    nvs_set_i8(settings->nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, (int8_t)settings->enableChannelHopping);
    nvs_set_i8(settings->nvsHandle, NVS_RANDOM_BLE_MAC, (int8_t)settings->randomBLEMacEnabled);
    nvs_commit(settings->nvsHandle);
}