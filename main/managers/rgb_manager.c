#include "managers/rgb_manager.h"
#include "esp_log.h"
#include <math.h>
#include "managers/settings_manager.h"
#include "freertos/task.h"

static const char* TAG = "RGBManager";

typedef struct {
    double r;       // ∈ [0, 1]
    double g;       // ∈ [0, 1]
    double b;       // ∈ [0, 1]
} rgb;

typedef struct {
    double h;       // ∈ [0, 360]
    double s;       // ∈ [0, 1]
    double v;       // ∈ [0, 1]
} hsv;

// Converts HSV to RGB with values between [0, 1]
rgb hsv2rgb(hsv HSV)
{
    rgb RGB;
    double H = HSV.h, S = HSV.s, V = HSV.v;
    double P, Q, T, fract;

    (H == 360.0) ? (H = 0.0) : (H /= 60.0);
    fract = H - floor(H);

    P = V * (1.0 - S);
    Q = V * (1.0 - S * fract);
    T = V * (1.0 - S * (1.0 - fract));

    if (0.0 <= H && H < 1.0) {
        RGB = (rgb){.r = V, .g = T, .b = P};
    } else if (1.0 <= H && H < 2.0) {
        RGB = (rgb){.r = Q, .g = V, .b = P};
    } else if (2.0 <= H && H < 3.0) {
        RGB = (rgb){.r = P, .g = V, .b = T};
    } else if (3.0 <= H && H < 4.0) {
        RGB = (rgb){.r = P, .g = Q, .b = V};
    } else if (4.0 <= H && H < 5.0) {
        RGB = (rgb){.r = T, .g = P, .b = V};
    } else if (5.0 <= H && H < 6.0) {
        RGB = (rgb){.r = V, .g = P, .b = Q};
    } else {
        RGB = (rgb){.r = 0.0, .g = 0.0, .b = 0.0};
    }

    return RGB;
}

void rainbow_task(void* pvParameter) 
{
    RGBManager_t* rgb_manager = (RGBManager_t*) pvParameter;
    
    while (1) {
        
        rgb_manager_rainbow_effect(rgb_manager, 15);
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}

void police_task(void *pvParameter)
{
    RGBManager_t* rgb_manager = (RGBManager_t*) pvParameter;
    while (1) {
        
        rgb_manager_policesiren_effect(rgb_manager, 6);
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}


// Clamps an RGB value to the 0-255 range
static void clamp_rgb(uint8_t *r, uint8_t *g, uint8_t *b) {
    if (*r > 255) *r = 255;
    if (*g > 255) *g = 255;
    if (*b > 255) *b = 255;
}

// Initialize the RGB LED manager
esp_err_t rgb_manager_init(RGBManager_t* rgb_manager, gpio_num_t pin, int num_leds, led_pixel_format_t pixel_format, led_model_t model) {
    if (!rgb_manager) return ESP_ERR_INVALID_ARG;

    rgb_manager->pin = pin;
    rgb_manager->num_leds = num_leds;

    // Create LED strip configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,
        .max_leds = num_leds,
        .led_pixel_format = pixel_format,
        .led_model = model,
        .flags.invert_out = 0  // Set to 1 if you need to invert the output signal
    };

    // Create RMT configuration for LED strip
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,      // Default RMT clock source
        .resolution_hz = 10 * 1000 * 1000    // 10 MHz resolution
    };

    // Initialize the LED strip with both configurations
    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &rgb_manager->strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize the LED strip");
        return ret;
    }

    // Clear the strip (turn off all LEDs)
    led_strip_clear(rgb_manager->strip);

    ESP_LOGI(TAG, "RGBManager initialized for pin %d with %d LEDs", pin, num_leds);
    return ESP_OK;
}


esp_err_t rgb_manager_set_color(RGBManager_t* rgb_manager, int led_idx, uint8_t red, uint8_t green, uint8_t blue, bool pulse) {
#ifdef NEOPIXEL_PIN
    if (settings_get_rgb_mode(&G_Settings) != RGB_MODE_STEALTH)
    {
        if (!rgb_manager || !rgb_manager->strip) return ESP_ERR_INVALID_ARG;

        esp_err_t ret = ESP_OK;

        if (pulse) {
            // Pulse the LED: gradually increase and decrease brightness
            for (int i = 0; i <= 255; i += 10) {  // Gradually increase brightness
                uint8_t brightness_red = (red * i) / 255;
                uint8_t brightness_green = (green * i) / 255;
                uint8_t brightness_blue = (blue * i) / 255;
                
                ret = led_strip_set_pixel(rgb_manager->strip, led_idx, brightness_red, brightness_green, brightness_blue);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set LED color during pulse");
                    return ret;
                }
                ret = led_strip_refresh(rgb_manager->strip);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to refresh LED strip during pulse");
                    return ret;
                }
                vTaskDelay(pdMS_TO_TICKS(10));  // Delay for smooth pulsing
            }

            for (int i = 255; i >= 0; i -= 10) {  // Gradually decrease brightness
                uint8_t brightness_red = (red * i) / 255;
                uint8_t brightness_green = (green * i) / 255;
                uint8_t brightness_blue = (blue * i) / 255;
                
                ret = led_strip_set_pixel(rgb_manager->strip, led_idx, brightness_red, brightness_green, brightness_blue);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set LED color during pulse");
                    return ret;
                }
                ret = led_strip_refresh(rgb_manager->strip);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to refresh LED strip during pulse");
                    return ret;
                }
                vTaskDelay(pdMS_TO_TICKS(10));  // Delay for smooth pulsing
            }
        } else {
            // Set the color of the LED at index `led_idx`
            ret = led_strip_set_pixel(rgb_manager->strip, led_idx, red, green, blue);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set LED color");
                return ret;
            }

            // Refresh the strip to apply the changes
            ret = led_strip_refresh(rgb_manager->strip);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to refresh LED strip");
                return ret;
            }
        }

        return ESP_OK;
    }
    else 
    {
        return ESP_OK;
    }
#endif
    return ESP_OK;
}

// Rainbow effect
void rgb_manager_rainbow_effect(RGBManager_t* rgb_manager, int delay_ms) {
    double hue = 0.0;  // Start at 0.0

    while (1) {
        for (int i = 0; i < rgb_manager->num_leds; i++) {
            uint8_t red, green, blue;

            // Calculate the hue for each LED
            double hue_offset = fmod(hue + i * (360.0 / rgb_manager->num_leds), 360.0);

            // Use HSV values (hue: 0-360, saturation: 1.0, brightness: 1.0)
            hsv hsv_color = { .h = hue_offset, .s = 1.0, .v = 1.0 };
            rgb rgb_color = hsv2rgb(hsv_color);

            // Convert RGB from [0.0, 1.0] to [0, 255]
            red = (uint8_t)(rgb_color.r * 255);
            green = (uint8_t)(rgb_color.g * 255);
            blue = (uint8_t)(rgb_color.b * 255);

            // Clamp the RGB values to ensure they stay within 0-255
            clamp_rgb(&red, &green, &blue);

            // Set the color of the LED
            rgb_manager_set_color(rgb_manager, i, red, green, blue, false);
        }

        
        led_strip_refresh(rgb_manager->strip);

        
        hue = fmod(hue + 1, 360.0);

        
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void rgb_manager_policesiren_effect(RGBManager_t *rgb_manager, int delay_ms) {
    uint8_t brightness;
    bool increasing = true;
    bool is_red = true;  // Start with red

    while (1) {
        for (int pulse_step = 0; pulse_step <= 255; pulse_step += 5) {
            // Determine brightness for pulsing effect
            if (increasing) {
                brightness = pulse_step;  // Increase brightness
            } else {
                brightness = 255 - pulse_step;  // Decrease brightness
            }

            // Set the LED to either red or blue based on `is_red` flag
            if (is_red) {
                rgb_manager_set_color(rgb_manager, 0, brightness, 0, 0, false);  // Red color
            } else {
                rgb_manager_set_color(rgb_manager, 0, 0, 0, brightness, false);  // Blue color
            }

            // Refresh the LED strip to apply the new color
            led_strip_refresh(rgb_manager->strip);

            // Delay to control the speed of the pulsing effect
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        // Switch between increasing and decreasing brightness
        increasing = !increasing;

        // Alternate between red and blue
        if (!increasing) {
            is_red = !is_red;  // Switch color only after a full pulse cycle
        }
    }
}

// Deinitialize the RGB LED manager
esp_err_t rgb_manager_deinit(RGBManager_t* rgb_manager) {
    if (!rgb_manager || !rgb_manager->strip) return ESP_ERR_INVALID_ARG;

    
    led_strip_clear(rgb_manager->strip);

    ESP_LOGI(TAG, "RGBManager deinitialized");
    return ESP_OK;
}