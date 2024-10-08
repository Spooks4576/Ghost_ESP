#include "managers/rgb_manager.h"
#include "esp_log.h"
#include <math.h>
#include "driver/ledc.h"
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

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_RED    LEDC_CHANNEL_0
#define LEDC_CHANNEL_GREEN  LEDC_CHANNEL_1
#define LEDC_CHANNEL_BLUE   LEDC_CHANNEL_2
#define LEDC_DUTY_RES       LEDC_TIMER_8_BIT  // 8-bit resolution (0-255)
#define LEDC_FREQUENCY      5000  // 5 kHz PWM frequency

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
        
        rgb_manager_rainbow_effect(rgb_manager, settings_get_rgb_speed(G_Settings));
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}

void police_task(void *pvParameter)
{
    RGBManager_t* rgb_manager = (RGBManager_t*) pvParameter;
    while (1) {
        
        rgb_manager_policesiren_effect(rgb_manager, settings_get_rgb_speed(G_Settings));
        
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
esp_err_t rgb_manager_init(RGBManager_t* rgb_manager, gpio_num_t pin, int num_leds, led_pixel_format_t pixel_format, led_model_t model, gpio_num_t red_pin, gpio_num_t green_pin, gpio_num_t blue_pin) {
    if (!rgb_manager) return ESP_ERR_INVALID_ARG;

    rgb_manager->pin = pin;
    rgb_manager->num_leds = num_leds;
    rgb_manager->red_pin = red_pin;
    rgb_manager->green_pin = green_pin;
    rgb_manager->blue_pin = blue_pin;

    // Check if separate pins for R, G, B are provided
    if (red_pin != GPIO_NUM_NC && green_pin != GPIO_NUM_NC && blue_pin != GPIO_NUM_NC) {
         rgb_manager->is_separate_pins = true;

        // Configure the LEDC timer
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_MODE,
            .timer_num        = LEDC_TIMER,
            .duty_resolution  = LEDC_DUTY_RES,  // 8-bit duty resolution
            .freq_hz          = LEDC_FREQUENCY,  // Frequency in Hertz
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        // Configure the LEDC channels for Red, Green, Blue
        ledc_channel_config_t ledc_channel_red = {
            .channel    = LEDC_CHANNEL_RED,
            .duty       = 255,
            .gpio_num   = red_pin,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));

        ledc_channel_config_t ledc_channel_green = {
            .channel    = LEDC_CHANNEL_GREEN,
            .duty       = 255,
            .gpio_num   = green_pin,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));

        ledc_channel_config_t ledc_channel_blue = {
            .channel    = LEDC_CHANNEL_BLUE,
            .duty       = 255,
            .gpio_num   = blue_pin,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_blue));

        gpio_set_level(rgb_manager->red_pin, 0);
        gpio_set_level(rgb_manager->green_pin, 0);
        gpio_set_level(rgb_manager->blue_pin, 0);

        ESP_LOGI(TAG, "RGBManager initialized for separate R/G/B pins: %d, %d, %d", red_pin, green_pin, blue_pin);
        return ESP_OK;
    } else {
        // Single pin for LED strip
        rgb_manager->is_separate_pins = false;

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
}

void pulse_once(RGBManager_t* rgb_manager, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t brightness = 0;
    int direction = 1;

    while (brightness <= 255 || brightness > 0) {
        
        float brightness_scale = brightness / 255.0;
        uint8_t adj_red = red * brightness_scale;
        uint8_t adj_green = green * brightness_scale;
        uint8_t adj_blue = blue * brightness_scale;

        
        esp_err_t ret = led_strip_set_pixel(rgb_manager->strip, 0, adj_red, adj_green, adj_blue);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED color");
            return; 
        }

        led_strip_refresh(rgb_manager->strip);
        
        
        brightness += direction * 5; 

        if (brightness >= 255) {
            direction = -1;
        }

        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


esp_err_t rgb_manager_set_color(RGBManager_t* rgb_manager, int led_idx, uint8_t red, uint8_t green, uint8_t blue, bool pulse) {
#ifdef LED_DATA_PIN
    if (!rgb_manager) return ESP_ERR_INVALID_ARG;

    // For LED strip
    if (pulse) {
        pulse_once(rgb_manager, red, green, blue);
        return ESP_OK;
    } else {
        scale_grb_by_brightness(&green, &red, &blue, 0.3);
        esp_err_t ret = led_strip_set_pixel(rgb_manager->strip, led_idx, red, green, blue);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED color");
            return ret;
        }
        return led_strip_refresh(rgb_manager->strip);
    }
#elif CONFIG_IDF_TARGET_ESP32S2

    scale_grb_by_brightness(&green, &red, &blue, 0.3);

    uint8_t ired = (uint8_t)(255 - (red * 255));
    uint8_t igreen = (uint8_t)(255 - (green * 255));
    uint8_t iblue = (uint8_t)(255 - (blue * 255));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RED, ired));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RED));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_GREEN, igreen));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_GREEN));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_BLUE, iblue));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_BLUE));


    return ESP_OK;
#endif
    return ESP_OK;
}

// Rainbow effect
void rgb_manager_rainbow_effect(RGBManager_t* rgb_manager, int delay_ms) {
    double hue = 0.0;  // Start at 0.0

    while (1) {
        for (int i = 0; i < rgb_manager->num_leds; i++) {
RGBSTART:
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

            if (rgb_manager->is_separate_pins)
            {
                uint8_t ired = (uint8_t)(255 - (rgb_color.r * 255));
                uint8_t igreen = (uint8_t)(255 - (rgb_color.g * 255));
                uint8_t iblue = (uint8_t)(255 - (rgb_color.b * 255));
                rgb_manager_set_color(rgb_manager, i, ired, igreen, iblue, false);
                hue = fmod(hue + 1, 360.0);

        
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                goto RGBSTART;
            }

            // Set the color of the LED
            rgb_manager_set_color(rgb_manager, i, red, green, blue, false);
        }

        if (!rgb_manager->is_separate_pins)
        {
            led_strip_refresh(rgb_manager->strip);
        }

        
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
    if (!rgb_manager) return ESP_ERR_INVALID_ARG;

    if (rgb_manager->is_separate_pins) {
        gpio_set_level(rgb_manager->red_pin, 0);
        gpio_set_level(rgb_manager->green_pin, 0);
        gpio_set_level(rgb_manager->blue_pin, 0);
        ESP_LOGI(TAG, "RGBManager deinitialized (separate pins)");
    } else {
        // Clear the LED strip and deinitialize
        led_strip_clear(rgb_manager->strip);
        ESP_LOGI(TAG, "RGBManager deinitialized (LED strip)");
    }

    return ESP_OK;
}