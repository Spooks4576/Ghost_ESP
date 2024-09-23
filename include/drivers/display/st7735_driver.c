#include "display_driver.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include <driver/gpio.h>

static const char *DR_TAG = "ST7735_DRIVER";

static esp_err_t st7735_init(void) {
    ESP_LOGI(DR_TAG, "Initializing SPI bus for ST7735");

    spi_bus_config_t bus_config = {
        .sclk_io_num = BOARD_SPI_SCK,        // SPI clock pin
        .mosi_io_num = BOARD_SPI_MOSI,       // SPI MOSI pin
        .miso_io_num = BOARD_SPI_MISO,       // (Optional) SPI MISO pin, -1 if not used
        .quadwp_io_num = -1,                 // Not used in 3-wire SPI
        .quadhd_io_num = -1,                 // Not used in 3-wire SPI
        .max_transfer_sz = 240 * 320 * 2 + 10, // Maximum transfer size (adjust as needed)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO)); // SPI2_HOST is the SPI peripheral (SPI2 in this case)

    
    ESP_LOGI(DR_TAG, "Setting up panel IO for ST7735");

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BOARD_TFT_DC,       // Data/command pin
        .cs_gpio_num = BOARD_TFT_CS,       // Chip select pin
        .pclk_hz = 26 * 1000 * 1000,       // Pixel clock frequency (adjust based on your panel's max speed)
        .spi_mode = 0,                     // SPI mode 0
        .trans_queue_depth = 10,           // Transaction queue depth
        .lcd_cmd_bits = 8,                 // Command bits width
        .lcd_param_bits = 8,               // Parameter bits width
        .on_color_trans_done = NULL,       // (Optional) Callback for transaction done event, or use lvgl callback
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

    
    ESP_LOGI(DR_TAG, "Configuring ST7735 panel");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_TFT_RST,   // Reset pin
        .rgb_endian = LCD_RGB_ENDIAN_BGR,  // RGB color order
        .bits_per_pixel = 16,              // 16 bits per pixel
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    
    ESP_LOGI(DR_TAG, "Resetting ST7735 panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    ESP_LOGI(DR_TAG, "Initializing ST7735 panel");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    
    ESP_LOGI(DR_TAG, "Additional configurations for ST7735 panel");

    
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));

    
    ESP_LOGI(DR_TAG, "Turning on the ST7735 display");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    
    ESP_LOGI(DR_TAG, "Turning on backlight for ST7735");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_TFT_BL,  // Backlight pin
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(BOARD_TFT_BL, 1);

    return ESP_OK;
}

static esp_err_t st7735_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void *data) {
    return esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + width, y + height, data);
}

static esp_err_t st7735_invert_color(bool invert) {
    return esp_lcd_panel_invert_color(panel_handle, invert);
}

static esp_err_t st7735_set_gap(uint16_t x_gap, uint16_t y_gap) {
    return esp_lcd_panel_set_gap(panel_handle, x_gap, y_gap);
}

static esp_err_t st7735_mirror(bool mirror_x, bool mirror_y) {
    return esp_lcd_panel_mirror(panel_handle, mirror_x, mirror_y);
}

static esp_err_t st7735_swap_xy(bool swap_axes) {
    return esp_lcd_panel_swap_xy(panel_handle, swap_axes);
}

static esp_err_t st7735_disp_on_off(bool on_off) {
    return esp_lcd_panel_disp_on_off(panel_handle, on_off);
}

// Instantiate the driver
display_driver_t st7735_driver = {
    .init = st7735_init,
    .draw_bitmap = st7735_draw_bitmap,
    .invert_color = st7735_invert_color,
    .set_gap = st7735_set_gap,
    .mirror = st7735_mirror,
    .swap_xy = st7735_swap_xy,
    .disp_on_off = st7735_disp_on_off,
};