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

    void setChannelSwitchDelay(uint16_t delay_ms);
    uint16_t getChannelSwitchDelay() const;

    void loadSettings();
    void saveSettings();

private:
    uint16_t settings;  // Reduced to 16 bits for simplicity and safety
    nvs_handle_t nvsHandle;

    static const uint8_t RGB_MODE_MASK = 0x03; // 2 bits for RGB mode (0000 0011)
    static const uint8_t RGB_MODE_SHIFT = 0;

    static const uint16_t CHANNEL_SWITCH_DELAY_MASK = 0x3FF; // 10 bits for delay (0000 0011 1111 1111)
    static const uint8_t CHANNEL_SWITCH_DELAY_SHIFT = 2; // Shift right by 2 bits
};

#endif // SETTINGS_H