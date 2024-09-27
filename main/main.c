#include "core/system_manager.h"
#include "core/serial_manager.h"
#include "managers/rgb_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ap_manager.h"
#include "managers/ble_manager.h"
#include <esp_log.h>
#include "core/commandline.h"


int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

int custom_vprintf(const char *fmt, va_list args)
{
  char buffer[256];
  int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

  ap_manager_add_log(buffer);

  return len;
}

void app_main(void) {
    system_manager_init();
    serial_manager_init();
    wifi_manager_init();
    ble_init();

    command_init();

    register_commands();

    G_Settings = malloc(sizeof(FSettings));

    settings_init(G_Settings);

    ap_manager_init();

    //esp_log_set_vprintf(custom_vprintf);
#ifdef LED_DATA_PIN
    rgb_manager_init(&rgb_manager, LED_DATA_PIN, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812);

    if (settings_get_rgb_mode(G_Settings) == RGB_MODE_RAINBOW)
    {
    xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
    }
#endif
}