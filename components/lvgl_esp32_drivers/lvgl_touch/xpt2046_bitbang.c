#include "xpt2046_bitbang.h"
#include "esp_log.h"

// Calibration constants
#define STORAGE_NAMESPACE "storage"
#define TAG "XPT2046"

// GPIO pin definitions (replace with your actual GPIO pins if not using CONFIG_)
#define MOSI_PIN CONFIG_LV_TOUCH_SPI_MOSI
#define MISO_PIN CONFIG_LV_TOUCH_SPI_MISO
#define CLK_PIN CONFIG_LV_TOUCH_SPI_CLK
#define CS_PIN CONFIG_LV_TOUCH_SPI_CS

// Static variables for GPIO pins
static gpio_num_t mosi_pin, miso_pin, clk_pin, cs_pin;

// SPI read function
int xpt2046_read_spi(uint8_t command);

esp_err_t xpt2046_init_bitbang() {
    mosi_pin = CONFIG_LV_TOUCH_SPI_MOSI;
    miso_pin = CONFIG_LV_TOUCH_SPI_MISO;
    clk_pin = CONFIG_LV_TOUCH_SPI_CLK;
    cs_pin = CONFIG_LV_TOUCH_SPI_CS;

    // Configure GPIO pins
    gpio_set_direction(mosi_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(miso_pin, GPIO_MODE_INPUT);
    gpio_set_direction(clk_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(cs_pin, GPIO_MODE_OUTPUT);

    gpio_set_level(cs_pin, 1);
    gpio_set_level(clk_pin, 0);

    return ESP_OK;
}

int xpt2046_read_spi(uint8_t command) {
    int result = 0;

    // Pull CS low to start communication
    gpio_set_level(cs_pin, 0);

    // Send command to SPI
    //ESP_LOGI(TAG, "Sending command: 0x%02X", command);
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(mosi_pin, (command & (1 << i)) ? 1 : 0);
        gpio_set_level(clk_pin, 1);
        esp_rom_delay_us(10);
        gpio_set_level(clk_pin, 0);
        esp_rom_delay_us(10);
    }

    // Read response from SPI
    for (int i = 11; i >= 0; i--) {
        gpio_set_level(clk_pin, 1);
        esp_rom_delay_us(10);
        int bit = gpio_get_level(miso_pin);
        result |= (bit << i);
        gpio_set_level(clk_pin, 0);
        esp_rom_delay_us(10);
    }

    // Pull CS high to end communication
    gpio_set_level(cs_pin, 1);

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