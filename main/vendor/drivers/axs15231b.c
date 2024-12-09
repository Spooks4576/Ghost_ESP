#include "vendor/drivers/axs15231b.h"

#ifdef CONFIG_JC3248W535EN_LCD

#pragma message("Compiling JC3248W535EN")

#include "esp_log.h"

#include <axs15231b/esp_bsp.h>
#include <axs15231b/display.h>

#define LVGL_PORT_ROTATION_DEGREE (90)

static const char *TAG = "lcd_panel.axs15231b";

esp_err_t lcd_axs15231b_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing axs15231b LCD panel");

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = (EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES),
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 0
        .rotate = LV_DISP_ROT_NONE,
#endif
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    return ret;
}

#endif
