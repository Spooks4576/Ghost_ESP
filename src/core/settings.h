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
    bool ChannelHoppingEnabled() const;

    void setChannelSwitchDelay(float delay_ms);
    void setChannelHoppingEnabled(bool Enabled);
    float getChannelSwitchDelay() const;

    void loadSettings();
    void saveSettings();

private:
    RGBMode rgbMode;
    float channelSwitchDelay;
    bool EnableChannelHopping;
    nvs_handle_t nvsHandle;

    static const char* NVS_RGB_MODE_KEY;
    static const char* NVS_CHANNEL_SWITCH_DELAY_KEY;
    static const char* NVS_ENABLE_CHANNEL_HOP_KEY;
};

#endif // SETTINGS_H