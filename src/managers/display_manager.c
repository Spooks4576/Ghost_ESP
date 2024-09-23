// display_manager.c

#include "drivers/display/display_driver.h"
#include "drivers/display/st7789_driver.c"
#include "drivers/display/st7735_driver.c"
#include "managers/display_manager.h"
#include "esp_log.h"
#include <driver/gpio.h>

static const char *TAG = "DISPLAY_MANAGER";

// Global pointer to the current display driver
display_driver_t *display_driver = NULL;

void display_init() {
    ESP_LOGI(TAG, "Initializing display");

    // Select the driver based on configuration
#if defined(CONFIG_LILYGO_T_DISPLAY) || defined(CONFIG_LILYGO_T_DONGLE_S2)
    ESP_LOGI(TAG, "Using ST7789 driver");
    display_driver = &st7789_driver;
#elif defined(CONFIG_LILYGO_T_DONGLE_S3)
    ESP_LOGI(TAG, "Using ST7735 driver");
    display_driver = &st7735_driver;
#else
    ESP_LOGI(TAG, "no display supported...");
    return;
#endif
    
    ESP_ERROR_CHECK(display_driver->init());


    display_driver->invert_color(true);
    display_driver->swap_xy(false);
    display_driver->mirror(false, false);
    display_driver->set_gap(26, 1);

    ESP_ERROR_CHECK(display_driver->disp_on_off(true));

    // Initialize backlight
    ESP_LOGI(TAG, "Turning on LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_TFT_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(BOARD_TFT_BL, 0);
}

void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data) {
    display_driver->draw_bitmap(x, y, width, height, data);
}