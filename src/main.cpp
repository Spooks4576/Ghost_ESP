#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_config.h"
#include <core/system_manager.h>

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

void loop() {
    SystemManager::getInstance().loop();
}

void setup() 
{
    SystemManager::getInstance().setup();
#ifndef DISPLAY_SUPPORT
#ifndef C3
    xTaskCreate(SystemManager::SerialCheckTask, "SerialCheckTask", 2048, NULL, 1, NULL);
#endif
#endif
}