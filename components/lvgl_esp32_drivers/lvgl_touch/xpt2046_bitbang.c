#include "xpt2046_bitbang.h"
#include "esp_log.h"

// Calibration constants
#define STORAGE_NAMESPACE "storage"
#define TAG "XPT2046"

// SPI read function
int xpt2046_read_spi(uint8_t command);

esp_err_t xpt2046_init_bitbang() {

    return ESP_OK;
}

int xpt2046_read_spi(uint8_t command) {
    int result = 0;

    // Pull CS low to start communication
    gpio_set_level(CONFIG_LV_TOUCH_SPI_CS, 0);

    // Send command to SPI
    //ESP_LOGI(TAG, "Sending command: 0x%02X", command);
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(CONFIG_LV_TOUCH_SPI_MOSI, (command & (1 << i)) ? 1 : 0);
        gpio_set_level(CONFIG_LV_TOUCH_SPI_CLK, 1);
        esp_rom_delay_us(10);
        gpio_set_level(CONFIG_LV_TOUCH_SPI_CLK, 0);
        esp_rom_delay_us(10);
    }

    // Read response from SPI
    for (int i = 11; i >= 0; i--) {
        gpio_set_level(CONFIG_LV_TOUCH_SPI_CLK, 1);
        esp_rom_delay_us(10);
        int bit = gpio_get_level(CONFIG_LV_TOUCH_SPI_MISO);
        result |= (bit << i);
        gpio_set_level(CONFIG_LV_TOUCH_SPI_CLK, 0);
        esp_rom_delay_us(10);
    }

    // Pull CS high to end communication
    gpio_set_level(CONFIG_LV_TOUCH_SPI_CS, 1);

    //ESP_LOGI(TAG, "Read result: %d", result);
    return result;
}

Point xpt2046_get_touch() {
    Point point;

    // Read X and Y coordinates
    int x = xpt2046_read_spi(CMD_READ_X);
    int y = xpt2046_read_spi(CMD_READ_Y);

    point.x = x;
    point.y = y;
    return point;
}