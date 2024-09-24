#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>
#include <nvs_flash.h>
#include <nvs.h>

// Enum for RGB Modes
typedef enum {
    RGB_MODE_STEALTH = 0,
    RGB_MODE_NORMAL = 1,
    RGB_MODE_RAINBOW = 2
} RGBMode;

// Struct for settings
typedef struct {
    RGBMode rgbMode;
    float channelSwitchDelay;
    bool enableChannelHopping;
    bool randomBLEMacEnabled;
    nvs_handle_t nvsHandle;
} FSettings;

// Declare functions
void settings_init(FSettings* settings);
void settings_deinit(FSettings* settings);

void settings_set_rgb_mode(FSettings* settings, RGBMode mode);
RGBMode settings_get_rgb_mode(const FSettings* settings);

void settings_set_channel_switch_delay(FSettings* settings, float delay_ms);
float settings_get_channel_switch_delay(const FSettings* settings);

void settings_set_channel_hopping_enabled(FSettings* settings, bool enabled);
bool settings_get_channel_hopping_enabled(const FSettings* settings);

void settings_set_random_ble_mac_enabled(FSettings* settings, bool enabled);
bool settings_get_random_ble_mac_enabled(const FSettings* settings);

void settings_load(FSettings* settings);
void settings_save(FSettings* settings);

FSettings G_Settings;

#endif // SETTINGS_H