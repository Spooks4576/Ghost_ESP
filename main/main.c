#include "core/system_manager.h"
#include "core/serial_manager.h"
#include "managers/rgb_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ble_manager.h"
#include <esp_log.h>
#include "core/commandline.h"

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

void app_main(void) {
    system_manager_init();
    serial_manager_init();
    wifi_manager_init();
    ble_init();

    command_init();

    register_commands();

    settings_init(&G_Settings);

#ifdef NEOPIXEL_PIN
    rgb_manager_init(&rgb_manager, NEOPIXEL_PIN, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812);

    if (settings_get_rgb_mode(&G_Settings) == RGB_MODE_RAINBOW)
    {
      xTaskCreate(rainbow_task, "Rainbow Task", 2048, &rgb_manager, 1, &rainbow_task_handle);
    }
#endif
}