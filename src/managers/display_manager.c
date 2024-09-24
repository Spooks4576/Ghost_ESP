#include "drivers/display/display_driver.h"
#include "drivers/display/st7789_driver.c"
#include "drivers/display/st7735_driver.c"
#include "managers/display_manager.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include "lvgl/lvgl.h"

static const char *D_TAG = "DISPLAY_MANAGER";

// Global pointer to the current display driver
display_driver_t *display_driver = NULL;

// Global display buffer and LVGL driver
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf1[LV_HOR_RES_MAX * 10];  // Buffer size, adjust as needed

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    display_driver->draw_bitmap(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, color_map);
    lv_disp_flush_ready(drv);  // Indicate flushing is done
}

void display_init() {
    ESP_LOGI(D_TAG, "Initializing display");

    // Select the driver based on configuration
#if defined(CONFIG_LILYGO_T_DISPLAY) || defined(CONFIG_LILYGO_T_DONGLE_S2)
    ESP_LOGI(D_TAG, "Using ST7789 driver");
    display_driver = &st7789_driver;
#elif defined(CONFIG_LILYGO_T_DONGLE_S3)
    ESP_LOGI(D_TAG, "Using ST7735 driver");
    display_driver = &st7735_driver;
#else
    ESP_LOGI(D_TAG, "no display supported...");
    return;
#endif

    ESP_ERROR_CHECK(display_driver->init());

    display_driver->invert_color(true);
    display_driver->swap_xy(false);
    display_driver->mirror(false, false);
    display_driver->set_gap(26, 1);

    ESP_ERROR_CHECK(display_driver->disp_on_off(true));

    // Initialize backlight
    ESP_LOGI(D_TAG, "Turning on LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_TFT_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(BOARD_TFT_BL, 0);

    // Initialize LVGL
    ESP_LOGI(D_TAG, "Initializing LVGL");
    lv_init();

    // Initialize the display buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LV_HOR_RES_MAX * 10);

    // Register the display driver in LVGL
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
}

void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data) {
    display_driver->draw_bitmap(x, y, width, height, data);
}

void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t width, uint16_t height, lv_color_t color, lv_align_t align) {
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);  // Style updated for LVGL v8+
    lv_obj_set_width(label, width);
    lv_obj_set_height(label, height);
    lv_obj_align(label, LV_ALIGN_CENTER, x, y);  // Alignment updated for LVGL v8+
}

void display_draw_image(uint16_t x, uint16_t y, const void *image_src) {
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, image_src);
    lv_obj_align(img, LV_ALIGN_CENTER, x, y);  // Updated for LVGL v8+
}

void display_lv_task_handler(void) {
    lv_timer_handler();  // Call LVGL's task handler, this replaces lv_task_handler() in v8+
}