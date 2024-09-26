#include "managers/settings_manager.h"
#include "core/system_manager.h"
#include "managers/rgb_manager.h"
#include "managers/ap_manager.h"
#include <string.h>
#include <stdio.h>  // For debugging purposes

// Define NVS keys
// Existing keys
static const char* NVS_RGB_MODE_KEY = "rgb_mode";
static const char* NVS_CHANNEL_SWITCH_DELAY_KEY = "channel_delay";
static const char* NVS_ENABLE_CHANNEL_HOP_KEY = "enable_ch";
static const char* NVS_RANDOM_BLE_MAC_KEY = "random_ble_mac";

// New keys
// WiFi/BLE
static const char* NVS_RANDOM_MAC_KEY = "random_mac";
static const char* NVS_BROADCAST_SPEED_KEY = "broadcast_speed";

// AP SSID/PWD
static const char* NVS_AP_SSID_KEY = "ap_ssid";
static const char* NVS_AP_PASSWORD_KEY = "ap_password";

// RGB
static const char* NVS_BREATH_MODE_KEY = "breath_mode";
static const char* NVS_RAINBOW_MODE_KEY = "rainbow_mode";
static const char* NVS_RGB_SPEED_KEY = "rgb_speed";
static const char* NVS_STATIC_COLOR_KEY = "static_color";


void settings_init(FSettings* settings) {
    // Initialize default values
    settings->rgbMode = RGB_MODE_STEALTH;
    settings->channelSwitchDelay = 1.0f; // Default to 1 second
    settings->enableChannelHopping = true;
    settings->randomBLEMacEnabled = true;

    // New settings
    settings->randomMacEnabled = false;
    settings->broadcastSpeed = 500; // Default broadcast speed

    strcpy(settings->apSSID, "GhostNet");       // Default SSID
    strcpy(settings->apPassword, "GhostNet"); // Default password

    settings->breathModeEnabled = false;
    settings->rainbowModeEnabled = false;
    settings->rgbSpeed = 50; // Default RGB speed

    settings->staticColor.red = 255;
    settings->staticColor.green = 255;
    settings->staticColor.blue = 255;

    // Initialize NVS

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
        settings_load(settings);
        printf("Loaded Settings... %li\n", nvsHandle);
    }
    else
    {
        printf(esp_err_to_name(err));
    }
}

void settings_deinit(FSettings* settings) {
    nvs_close(nvsHandle);
}

// Getter and Setter implementations

// Existing functions
void settings_set_rgb_mode(FSettings* settings, RGBMode mode) {
    settings->rgbMode = mode;
}

RGBMode settings_get_rgb_mode(const FSettings* settings) {
    return settings->rgbMode;
}

void settings_set_channel_switch_delay(FSettings* settings, float delay_ms) {
    settings->channelSwitchDelay = delay_ms;
}

float settings_get_channel_switch_delay(const FSettings* settings) {
    return settings->channelSwitchDelay;
}

void settings_set_channel_hopping_enabled(FSettings* settings, bool enabled) {
    settings->enableChannelHopping = enabled;
}

bool settings_get_channel_hopping_enabled(const FSettings* settings) {
    return settings->enableChannelHopping;
}

void settings_set_random_ble_mac_enabled(FSettings* settings, bool enabled) {
    settings->randomBLEMacEnabled = enabled;
}

bool settings_get_random_ble_mac_enabled(const FSettings* settings) {
    return settings->randomBLEMacEnabled;
}

// New functions

// WiFi/BLE
void settings_set_random_mac_enabled(FSettings* settings, bool enabled) {
    settings->randomMacEnabled = enabled;
}

bool settings_get_random_mac_enabled(const FSettings* settings) {
    return settings->randomMacEnabled;
}

void settings_set_broadcast_speed(FSettings* settings, uint16_t speed) {
    settings->broadcastSpeed = speed;
}

uint16_t settings_get_broadcast_speed(const FSettings* settings) {
    return settings->broadcastSpeed;
}

// AP SSID/PWD
void settings_set_ap_ssid(FSettings* settings, const char* ssid) {
    strncpy(settings->apSSID, ssid, sizeof(settings->apSSID) - 1);
    settings->apSSID[sizeof(settings->apSSID) - 1] = '\0'; // Ensure null termination
}

const char* settings_get_ap_ssid(const FSettings* settings) {
    return settings->apSSID;
}

void settings_set_ap_password(FSettings* settings, const char* password) {
    strncpy(settings->apPassword, password, sizeof(settings->apPassword) - 1);
    settings->apPassword[sizeof(settings->apPassword) - 1] = '\0'; // Ensure null termination
}

const char* settings_get_ap_password(const FSettings* settings) {
    return settings->apPassword;
}

// RGB
void settings_set_breath_mode_enabled(FSettings* settings, bool enabled) {
    settings->breathModeEnabled = enabled;
}

bool settings_get_breath_mode_enabled(const FSettings* settings) {
    return settings->breathModeEnabled;
}

void settings_set_rainbow_mode_enabled(FSettings* settings, bool enabled) {
    settings->rainbowModeEnabled = enabled;
}

bool settings_get_rainbow_mode_enabled(const FSettings* settings) {
    return settings->rainbowModeEnabled;
}

void settings_set_rgb_speed(FSettings* settings, uint8_t speed) {
    settings->rgbSpeed = speed;
}

uint8_t settings_get_rgb_speed(const FSettings* settings) {
    return settings->rgbSpeed;
}

void settings_set_static_color(FSettings* settings, uint8_t red, uint8_t green, uint8_t blue) {
    settings->staticColor.red = red;
    settings->staticColor.green = green;
    settings->staticColor.blue = blue;
}

void settings_get_static_color(const FSettings* settings, uint8_t* red, uint8_t* green, uint8_t* blue) {
    if (red) *red = settings->staticColor.red;
    if (green) *green = settings->staticColor.green;
    if (blue) *blue = settings->staticColor.blue;
}

// Load and Save functions
void settings_load(FSettings* settings) {
    esp_err_t err;

    // Load existing settings
    uint8_t storedRGBMode = 0;
    err = nvs_get_u8(nvsHandle, NVS_RGB_MODE_KEY, &storedRGBMode);
    if (err == ESP_OK) {
        settings->rgbMode = (RGBMode)storedRGBMode;
    }

    size_t required_size = sizeof(float);
    err = nvs_get_blob(nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &settings->channelSwitchDelay, &required_size);

    int8_t enableChannelHopping = 0;
    err = nvs_get_i8(nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, &enableChannelHopping);
    if (err == ESP_OK) {
        settings->enableChannelHopping = (bool)enableChannelHopping;
    }

    int8_t randomBLEMacEnabled = 0;
    err = nvs_get_i8(nvsHandle, NVS_RANDOM_BLE_MAC_KEY, &randomBLEMacEnabled);
    if (err == ESP_OK) {
        settings->randomBLEMacEnabled = (bool)randomBLEMacEnabled;
    }

    // WiFi/BLE
    int8_t randomMacEnabled = 0;
    err = nvs_get_i8(nvsHandle, NVS_RANDOM_MAC_KEY, &randomMacEnabled);
    if (err == ESP_OK) {
        settings->randomMacEnabled = (bool)randomMacEnabled;
    }

    uint16_t broadcastSpeed = 0;
    err = nvs_get_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, &broadcastSpeed);
    if (err == ESP_OK) {
        settings->broadcastSpeed = broadcastSpeed;
    }

    // AP SSID/PWD
    size_t ssid_size = sizeof(settings->apSSID);
    err = nvs_get_str(nvsHandle, NVS_AP_SSID_KEY, settings->apSSID, &ssid_size);

    size_t pwd_size = sizeof(settings->apPassword);
    err = nvs_get_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->apPassword, &pwd_size);

    // RGB
    int8_t breathModeEnabled = 0;
    err = nvs_get_i8(nvsHandle, NVS_BREATH_MODE_KEY, &breathModeEnabled);
    if (err == ESP_OK) {
        settings->breathModeEnabled = (bool)breathModeEnabled;
    }

    int8_t rainbowModeEnabled = 0;
    err = nvs_get_i8(nvsHandle, NVS_RAINBOW_MODE_KEY, &rainbowModeEnabled);
    if (err == ESP_OK) {
        settings->rainbowModeEnabled = (bool)rainbowModeEnabled;
    }

    uint8_t rgbSpeed = 0;
    err = nvs_get_u8(nvsHandle, NVS_RGB_SPEED_KEY, &rgbSpeed);
    if (err == ESP_OK) {
        settings->rgbSpeed = rgbSpeed;
    }
    else 
    {
        settings->rgbSpeed = 15;
    }

    size_t color_size = sizeof(StaticColor);
    err = nvs_get_blob(nvsHandle, NVS_STATIC_COLOR_KEY, &settings->staticColor, &color_size);
}

void settings_save(FSettings* settings) {
    esp_err_t err;

    printf("saving Settings... %li\n", nvsHandle);

    err = nvs_set_u8(nvsHandle, NVS_RGB_MODE_KEY, (uint8_t)settings->rgbMode);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }
    
    err = nvs_set_blob(nvsHandle, NVS_CHANNEL_SWITCH_DELAY_KEY, &settings->channelSwitchDelay, sizeof(float));
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_i8(nvsHandle, NVS_ENABLE_CHANNEL_HOP_KEY, (int8_t)settings->enableChannelHopping);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_i8(nvsHandle, NVS_RANDOM_BLE_MAC_KEY, (int8_t)settings->randomBLEMacEnabled);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err)));
        return;
    }

    // Group 2: Save WiFi/BLE settings
    err = nvs_set_i8(nvsHandle, NVS_RANDOM_MAC_KEY, (int8_t)settings->randomMacEnabled);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, settings->broadcastSpeed);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    // AP SSID/PWD
    err = nvs_set_str(nvsHandle, NVS_AP_SSID_KEY, settings->apSSID);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->apPassword);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    // Commit after second group
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err)));
        return;
    }

    // Group 3: Save RGB settings
    err = nvs_set_i8(nvsHandle, NVS_BREATH_MODE_KEY, (int8_t)settings->breathModeEnabled);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_i8(nvsHandle, NVS_RAINBOW_MODE_KEY, (int8_t)settings->rainbowModeEnabled);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_u8(nvsHandle, NVS_RGB_SPEED_KEY, settings->rgbSpeed);
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    err = nvs_set_blob(nvsHandle, NVS_STATIC_COLOR_KEY, &settings->staticColor, sizeof(StaticColor));
    if (err != ESP_OK) { ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err))); return; }

    
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ap_manager_add_log(WRAP_MESSAGE(esp_err_to_name(err)));
        return;
    }
    
    if (settings_get_rgb_mode(&G_Settings) == RGB_MODE_RAINBOW) {
        xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rainbow_task_handle);
    } else {
        if (rainbow_task_handle != NULL) {
            vTaskDelete(rainbow_task_handle);  // Delete the task
            rainbow_task_handle = NULL;        // Reset the task handle
            rgb_manager_set_color(&rgb_manager, 1, 0, 0, 0, false);
        }

        if ((settings->staticColor.red > 0 && settings->staticColor.red < 255) ||
            (settings->staticColor.green > 0 && settings->staticColor.green < 255) ||
            (settings->staticColor.blue > 0 && settings->staticColor.blue < 255)) {
            rgb_manager_set_color(&rgb_manager, 1, 
                                settings->staticColor.red, 
                                settings->staticColor.green, 
                                settings->staticColor.blue, 
                                false);
        }
    }
}