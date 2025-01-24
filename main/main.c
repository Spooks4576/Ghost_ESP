#include "core/commandline.h"
#include "core/serial_manager.h"
#include "core/system_manager.h"
#include "managers/ap_manager.h"
#include "managers/display_manager.h"
#include "managers/rgb_manager.h"
#include "managers/sd_card_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "managers/ble_manager.h"
#endif
#include <esp_log.h>

#ifdef CONFIG_WITH_ETHERNET
// TODO
#endif

#ifdef CONFIG_WITH_SCREEN
#include "managers/views/splash_screen.h"
#endif

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { return 0; }

void app_main(void) {
    serial_manager_init();
    wifi_manager_init();
#ifndef CONFIG_IDF_TARGET_ESP32S2
    // ble_init();
#endif

#ifdef USB_MODULE
    wifi_manager_auto_deauth();
    return;
#endif

#ifdef CONFIG_WITH_ETHERNET

#endif

    command_init();

    register_commands();

    settings_init(&G_Settings);

    ap_manager_init();

    esp_err_t err = sd_card_init();

#ifdef CONFIG_WITH_SCREEN

#ifdef CONFIG_USE_JOYSTICK

#define L_BTN 13
#define C_BTN 34
#define U_BTN 36
#define R_BTN 39
#define D_BTN 35

    joystick_init(&joysticks[0], L_BTN, HOLD_LIMIT, true);
    joystick_init(&joysticks[1], C_BTN, HOLD_LIMIT, true);
    joystick_init(&joysticks[2], U_BTN, HOLD_LIMIT, true);
    joystick_init(&joysticks[3], R_BTN, HOLD_LIMIT, true);
    joystick_init(&joysticks[4], D_BTN, HOLD_LIMIT, true);

    printf("Joystick GPIO Setup Successfully...\n");
#endif

    display_manager_init();
    display_manager_switch_view(&splash_view);
#endif

#ifdef CONFIG_LED_DATA_PIN
    rgb_manager_init(&rgb_manager, CONFIG_LED_DATA_PIN, CONFIG_NUM_LEDS, LED_ORDER,
                     LED_MODEL_WS2812, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC);

    if (settings_get_rgb_mode(&G_Settings) == 1) {
        xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
    }
#endif
#ifdef CONFIG_RED_RGB_PIN &&CONFIG_GREEN_RGB_PIN &&CONFIG_BLUE_RGB_PIN
    rgb_manager_init(&rgb_manager, GPIO_NUM_NC, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812,
                     CONFIG_RED_RGB_PIN, CONFIG_GREEN_RGB_PIN, CONFIG_BLUE_RGB_PIN);
    if (settings_get_rgb_mode(&G_Settings) == 1) {
        xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
    }
#endif

    printf("Ghost ESP Ready ;)\n");
}