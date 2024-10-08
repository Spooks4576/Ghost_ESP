#include "core/system_manager.h"
#include "core/serial_manager.h"
#include "core/commandline.h"
#include "core/crash_handler.h"
#include "managers/rgb_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ap_manager.h"
#include "managers/sd_card_manager.h"
#ifdef CONFIG_BT_ENABLED
#include "managers/ble_manager.h"
#endif
#include <esp_log.h>


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
#if CONFIG_BT_ENABLED
  ble_init();
#endif

#ifdef USB_MODULE
  wifi_manager_auto_deauth();
  return;
#endif

  command_init();

  register_commands();

  G_Settings = malloc(sizeof(FSettings));

  settings_init(G_Settings);

  ap_manager_init();

  esp_err_t err = sd_card_init();

#ifdef DEBUG
  if (err == ESP_OK)
  {
    setup_custom_panic_handler();
  }
#endif

#ifdef LED_DATA_PIN
  rgb_manager_init(&rgb_manager, LED_DATA_PIN, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812);

  if (settings_get_rgb_mode(G_Settings) == RGB_MODE_RAINBOW)
  {
  xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
  }
#endif
}