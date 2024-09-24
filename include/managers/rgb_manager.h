#ifndef RGB_MANAGER_H
#define RGB_MANAGER_H

#include "driver/gpio.h"
#include "vendor/led_strip.h"

// Struct for the RGB manager (addressable LED strip)
typedef struct {
    gpio_num_t pin;                     // GPIO pin used for the LED strip
    led_strip_handle_t strip;           // Handle to the LED strip object
    int num_leds;                       // Number of LEDs in the strip
} RGBManager_t;

/**
 * @brief Initialize the RGB LED manager
 * @param rgb_manager Pointer to the RGBManager_t structure
 * @param pin GPIO pin used to control the LED
 * @param num_leds Number of LEDs in the strip
 * @param pixel_format Pixel format (GRB or GRBW)
 * @param model LED model (WS2812, SK6812, etc.)
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t rgb_manager_init(RGBManager_t* rgb_manager, gpio_num_t pin, int num_leds, led_pixel_format_t pixel_format, led_model_t model);

/**
 * @brief Set the color of a specific LED in the strip
 * @param rgb_manager Pointer to the RGBManager_t structure
 * @param led_idx Index of the LED to set the color
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t rgb_manager_set_color(RGBManager_t* rgb_manager, int led_idx, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Apply the rainbow effect to the LED strip
 * @param rgb_manager Pointer to the RGBManager_t structure
 * @param delay_ms Delay between hue shifts in milliseconds
 */
void rgb_manager_rainbow_effect(RGBManager_t* rgb_manager, int delay_ms);

void rgb_manager_policesiren_effect(RGBManager_t* rgb_manager, int delay_ms);

/**
 * @brief Deinitialize the RGB LED manager
 * @param rgb_manager Pointer to the RGBManager_t structure
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t rgb_manager_deinit(RGBManager_t* rgb_manager);

void rainbow_task(void* pvParameter);

void police_task(void* pvParameter);

TaskHandle_t rainbow_task_handle;

RGBManager_t rgb_manager;

#endif // RGB_MANAGER_H