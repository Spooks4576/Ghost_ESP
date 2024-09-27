#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <nvs_flash.h>
#include "core/utils.h"
#include <nvs.h>

// Enum for RGB Modes
typedef enum {
    RGB_MODE_STEALTH = 0,
    RGB_MODE_NORMAL = 1,
    RGB_MODE_RAINBOW = 2
} RGBMode;

// Struct for static color
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} StaticColor;

// Struct for settings
typedef struct {
    // Existing settings
    RGBMode rgbMode;
    float channelSwitchDelay;
    bool enableChannelHopping;
    bool randomBLEMacEnabled;

    // New settings
    // WiFi/BLE
    bool randomMacEnabled;
    uint16_t broadcastSpeed;

    // AP SSID/PWD
    char apSSID[33];       // Max SSID length is 32 bytes + null terminator
    char apPassword[65];   // Max password length is 64 bytes + null terminator

    const char* mdnsHostname;

    // RGB
    bool breathModeEnabled;
    bool rainbowModeEnabled;
    uint8_t rgbSpeed;
    StaticColor staticColor;
} FSettings;



// Function declarations
void settings_init(FSettings* settings);
void settings_deinit(FSettings* settings);

// Existing functions
void settings_set_rgb_mode(FSettings* settings, RGBMode mode);
RGBMode settings_get_rgb_mode(const FSettings* settings);

void settings_set_channel_switch_delay(FSettings* settings, float delay_ms);
float settings_get_channel_switch_delay(const FSettings* settings);

void settings_set_channel_hopping_enabled(FSettings* settings, bool enabled);
bool settings_get_channel_hopping_enabled(const FSettings* settings);

void settings_set_random_ble_mac_enabled(FSettings* settings, bool enabled);
bool settings_get_random_ble_mac_enabled(const FSettings* settings);

// New getter and setter functions

// WiFi/BLE
void settings_set_random_mac_enabled(FSettings* settings, bool enabled);
bool settings_get_random_mac_enabled(const FSettings* settings);

void settings_set_broadcast_speed(FSettings* settings, uint16_t speed);
uint16_t settings_get_broadcast_speed(const FSettings* settings);

// AP SSID/PWD
void settings_set_ap_ssid(FSettings* settings, const char* ssid);
const char* settings_get_ap_ssid(const FSettings* settings);

void settings_set_ap_password(FSettings* settings, const char* password);
const char* settings_get_ap_password(const FSettings* settings);

// RGB
void settings_set_breath_mode_enabled(FSettings* settings, bool enabled);
bool settings_get_breath_mode_enabled(const FSettings* settings);

void settings_set_rainbow_mode_enabled(FSettings* settings, bool enabled);
bool settings_get_rainbow_mode_enabled(const FSettings* settings);

void settings_set_rgb_speed(FSettings* settings, uint8_t speed);
uint8_t settings_get_rgb_speed(const FSettings* settings);

void settings_set_static_color(FSettings* settings, uint8_t red, uint8_t green, uint8_t blue);
void settings_get_static_color(const FSettings* settings, uint8_t* red, uint8_t* green, uint8_t* blue);

// Load and Save
void settings_load(FSettings* settings);
void settings_save(FSettings* settings);

FSettings* G_Settings;
static nvs_handle_t nvsHandle;

#endif // SETTINGS_MANAGER_H