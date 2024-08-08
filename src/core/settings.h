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

    void loadSettings();
    void saveSettings();

private:
    uint8_t settings;
    nvs_handle_t nvsHandle;

    static const uint8_t RGB_MODE_MASK = 0x03; // 2 bits for RGB mode (0000 0011)
    static const uint8_t RGB_MODE_SHIFT = 0;
};

#endif // SETTINGS_H