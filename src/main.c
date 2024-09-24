#include "core/system_manager.h"
#include "managers/display_manager.h"
#include "core/serial_manager.h"
#include "managers/wifi_manager.h"
#include "core/command.h"


void app_main() {
    system_manager_init();
    serial_manager_init();
    wifi_manager_init();

    command_init();

    register_wifi_commands();

#ifdef HAS_SCREEN
    display_init();

    display_draw_text(0, 0, "Suck My Dick Skidda", 10, 10, lv_color_white(), LV_ALIGN_CENTER);
#endif
}