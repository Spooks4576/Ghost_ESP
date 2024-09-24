#include "core/system_manager.h"
#include "core/serial_manager.h"
#include "managers/rgb_manager.h"
#include "managers/wifi_manager.h"
#include "core/command.h"

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

void app_main() {
    system_manager_init();
    serial_manager_init();
    wifi_manager_init();

    command_init();

    register_wifi_commands();

#ifdef NEOPIXEL_PIN
    // Initialize the RGB manager for the LED strip on GPIO 8, 1 LED, GRB format, WS2812 model
    rgb_manager_init(&rgb_manager, GPIO_NUM_8, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812);
#endif
}