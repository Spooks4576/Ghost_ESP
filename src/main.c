#include "core/system_manager.h"
#include "core/serial_manager.h"
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
}