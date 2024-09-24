#include "managers/display_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"   // For specific panel vendors like ST7789
#include "esp_timer.h"  // For getting system time

// Pin configuration (adjust based on your setup)
#define DISPLAY_WIDTH  160  // Set your display width
#define DISPLAY_HEIGHT 80   // Set your display height
#define TFT_CS_PIN 4
#define TFT_SDA_PIN 3  // SPI MOSI
#define TFT_SCL_PIN 5  // SPI SCLK
#define TFT_DC_PIN 2   // Data/Command control
#define TFT_RES_PIN 1  // Reset pin
#define TFT_LED_PIN 38 // Backlight control

// Define LVGL buffer and display driver
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static const char *TAG = "DISPLAY_MANAGER";

// LVGL flush callback function (push buffer to the display)
static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp_drv); // Indicate flush is complete
}

// Initialize the display and LVGL
void display_init(void) {
    ESP_LOGI(TAG, "Initializing LVGL display");

    // Initialize LVGL
    lv_init();

    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .sclk_io_num = TFT_SCL_PIN,
        .mosi_io_num = TFT_SDA_PIN,
        .miso_io_num = -1,  // Not using MISO
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Configure the SPI interface for the LCD
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = TFT_DC_PIN,
        .cs_gpio_num = TFT_CS_PIN,
        .pclk_hz = 10 * 1000 * 1000,  // 10 MHz
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

    // LCD panel configuration
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TFT_RES_PIN,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Reset and initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Initialize LVGL display buffer
    static lv_color_t buf1[DISPLAY_WIDTH * 10];  // Allocate buffer for 10 lines
    static lv_color_t buf2[DISPLAY_WIDTH * 10];  // Optional second buffer for double buffering
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISPLAY_WIDTH * 10);

    // Initialize LVGL display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;  // Flush callback
    disp_drv.draw_buf = &draw_buf;

    // Register the display driver with LVGL
    lv_disp_drv_register(&disp_drv);

    // Turn on the backlight
    gpio_set_direction(TFT_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TFT_LED_PIN, 1);

    ESP_LOGI(TAG, "Display initialized successfully");
}

// Draw a bitmap on the display
void display_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data) {
    lv_img_dsc_t img;
    img.data = data;
    img.header.w = width;
    img.header.h = height;
    img.header.always_zero = 0;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;

    lv_obj_t *img_obj = lv_img_create(lv_scr_act());
    lv_img_set_src(img_obj, &img);
    lv_obj_set_pos(img_obj, x, y);
}

// Draw text on the display
void display_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t width, uint16_t height, lv_color_t color, lv_align_t align) {
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_obj_set_size(label, width, height);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, align, x, y);
}

// Set backlight brightness
void display_set_backlight(uint8_t brightness) {
    // GPIO-based backlight control
    ESP_LOGI(TAG, "Setting backlight to %d%%", brightness);
    uint32_t level = (brightness > 0) ? 1 : 0;
    gpio_set_level(TFT_LED_PIN, level);
}

// Clear the display with a specified color
void display_clear(lv_color_t color) {
    lv_obj_clean(lv_scr_act()); // Clear all objects
    lv_obj_t *bg = lv_obj_create(lv_scr_act()); // Create a background object
    lv_obj_set_size(bg, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(bg, color, 0);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
}


#define LVGL_TICK_PERIOD_MS 10  // 10ms tick period

// Handle LVGL task refreshing (must be called periodically)
void display_lv_task_handler(void) {
    static uint64_t last_tick_time = 0;
    uint64_t current_time = esp_timer_get_time() / 1000;
    
    if (current_time - last_tick_time >= LVGL_TICK_PERIOD_MS) {
        lv_tick_inc(LVGL_TICK_PERIOD_MS);
        last_tick_time = current_time;
    }

    lv_timer_handler();
}