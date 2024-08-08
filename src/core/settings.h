#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <nvs_flash.h>
#include <nvs.h>

class FSettings {
public:
    enum class RGBMode : uint8_t {
        Stealth = 0,
        Normal = 1,
        Rainbow = 2
    };

    FSettings();
    ~FSettings();

    void setRGBMode(RGBMode mode);
    RGBMode getRGBMode() const;

    void setChannelSwitchDelay(float delay_ms);
    float getChannelSwitchDelay() const;

    void loadSettings();
    void saveSettings();

private:
    RGBMode rgbMode;
    float channelSwitchDelay;
    nvs_handle_t nvsHandle;

    static const char* NVS_RGB_MODE_KEY;
    static const char* NVS_CHANNEL_SWITCH_DELAY_KEY;
};

#endif // SETTINGS_H