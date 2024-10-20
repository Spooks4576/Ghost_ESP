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
#define LEDC_FREQUENCY      10000  // 10 kHz PWM frequency


void calculate_matrix_dimensions(int total_leds, int *rows, int *cols) {
    int side = (int)sqrt(total_leds);

    if (side * side == total_leds) {
        *rows = side;
        *cols = side;
    } else {
        for (int i = side; i > 0; i--) {
            if (total_leds % i == 0) {
                *rows = i;
                *cols = total_leds / i;
                return;
            }
        }
    }
}


rgb hsv2rgb(hsv HSV) {
    rgb RGB;
    double H = HSV.h, S = HSV.s, V = HSV.v;
    double P, Q, T, fract;

    // Ensure hue is wrapped between 0 and 360
    H = fmod(H, 360.0);
    if (H < 0) H += 360.0;

    // Convert hue to a 0-6 range for RGB segment
    H /= 60.0;
    fract = H - floor(H);

    P = V * (1.0 - S);
    Q = V * (1.0 - S * fract);
    T = V * (1.0 - S * (1.0 - fract));

    if (H < 1.0) {
        RGB = (rgb){.r = V, .g = T, .b = P};
    } else if (H < 2.0) {
        RGB = (rgb){.r = Q, .g = V, .b = P};
    } else if (H < 3.0) {
        RGB = (rgb){.r = P, .g = V, .b = T};
    } else if (H < 4.0) {
        RGB = (rgb){.r = P, .g = Q, .b = V};
    } else if (H < 5.0) {
        RGB = (rgb){.r = T, .g = P, .b = V};
    } else {
        RGB = (rgb){.r = V, .g = P, .b = Q};
    }

    // Apply a per-channel scaling factor to reduce white-ish colors
    RGB.r = pow(RGB.r, 0.8);  // Reduce the contribution of each channel slightly
    RGB.g = pow(RGB.g, 0.8);
    RGB.b = pow(RGB.b, 0.8);

    return RGB;
}


void rainbow_task(void* pvParameter) 
{
    RGBManager_t* rgb_manager = (RGBManager_t*) pvParameter;
    
    while (1) {

        if (rgb_manager->num_leds > 1)
        {
            rgb_manager_rainbow_effect_matrix(rgb_manager, settings_get_rgb_speed(&G_Settings));
        }
        else 
        {
            rgb_manager_rainbow_effect(rgb_manager, settings_get_rgb_speed(&G_Settings));
        }
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}

void police_task(void *pvParameter)
{
    RGBManager_t* rgb_manager = (RGBManager_t*) pvParameter;
    while (1) {
        
        rgb_manager_policesiren_effect(rgb_manager, settings_get_rgb_speed(&G_Settings));
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}



void clamp_rgb(uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (*r > 255) ? 255 : *r;
    *g = (*g > 255) ? 255 : *g;
    *b = (*b > 255) ? 255 : *b;
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

        rgb_manager_set_color(rgb_manager, 1, 0, 0, 0, false);

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

int get_pixel_index(int row, int column) {
    // Map 2D grid to 1D index, adjust based on your wiring
    return row * 8 + column;
}

void set_led_column(size_t column, uint8_t height) {
    // Clear the column first
    for (int row = 0; row < 8; ++row) {
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(row, column), 0, 0, 0);
    }

    uint8_t r = 255, g = 1, b = 1;

    scale_grb_by_brightness(&g,&r,&b, 0.1);

    // Light up the required number of LEDs with the selected primary color
    for (int row = 0; row < height; ++row) {
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(7 - row, column), r, g, b);
    }
}

void set_led_square(uint8_t size, uint8_t red, uint8_t green, uint8_t blue) {
    // Size is the 'thickness' of the square from the edges. 
    // Example: size=0 means the outermost 8x8 border, size=1 means one square inward (6x6), and so on.

    // Clear all LEDs first
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            led_strip_set_pixel(rgb_manager.strip, get_pixel_index(row, col), 0, 0, 0);
        }
    }

    // Draw square perimeter based on 'size'
    int start = size;
    int end = 7 - size;

    // Top and Bottom sides of the square
    for (int col = start; col <= end; ++col) {
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(start, col), red, green, blue); // Top side
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(end, col), red, green, blue);   // Bottom side
    }

    // Left and Right sides of the square
    for (int row = start + 1; row < end; ++row) { // Avoid corners since they are already set
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(row, start), red, green, blue); // Left side
        led_strip_set_pixel(rgb_manager.strip, get_pixel_index(row, end), red, green, blue);   // Right side
    }
}

void update_led_visualizer(uint8_t *amplitudes, size_t num_bars, bool square_mode) {
    if (square_mode) {
        // Square visualizer effect
        uint8_t amplitude = amplitudes[0]; // Use the first amplitude value
        uint8_t square_size = (amplitude * 4) / 255; // Map amplitude to square size (0 to 4)

        // Randomly select one primary color for the square
        uint8_t red = 255, green = 0, blue = 0;

        // Draw the square based on the calculated size
        set_led_square(square_size, red, green, blue);
    } else {
        // Original bar visualizer effect
        for (size_t bar = 0; bar < num_bars; ++bar) {
            uint8_t amplitude = amplitudes[bar];
            uint8_t num_pixels_to_light = (amplitude * 8) / 255; // Scale to 8 pixels high
            set_led_column(bar, num_pixels_to_light);
        }
    }

    // Refresh the LED strip
    led_strip_refresh(rgb_manager.strip);
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

    
    if (rgb_manager->num_leds > 1) {
        for (int i = 0; i < rgb_manager->num_leds; i++) {
            uint8_t r = red, g = green, b = blue;
            scale_grb_by_brightness(&g, &r, &b, 0.3);

            esp_err_t ret = led_strip_set_pixel(rgb_manager->strip, i, r, g, b);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set LED %d color", i);
                return ret;
            }
        }
        return led_strip_refresh(rgb_manager->strip);
    }


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
    scale_grb_by_brightness(&green, &red, &blue, -0.3);

    uint8_t ired = (uint8_t)(255 - red);
    uint8_t igreen = (uint8_t)(255 - green);
    uint8_t iblue = (uint8_t)(255 - blue);


    if (ired == 255 && igreen == 255 && iblue == 255) {
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_RED, 1);
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_GREEN, 1);
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_BLUE, 1);
    } else {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RED, ired));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RED));

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_GREEN, igreen));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_GREEN));

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_BLUE, iblue));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_BLUE));
    }

    return ESP_OK;
#endif

    return ESP_OK;
}

void rgb_manager_rainbow_effect_matrix(RGBManager_t* rgb_manager, int delay_ms) {
    double hue = 0.0;

    while (1) {
        for (int i = 0; i < rgb_manager->num_leds; i++) {
            uint8_t red, green, blue;

            double hue_offset = fmod(hue + i * (360.0 / rgb_manager->num_leds), 360.0);


            hsv hsv_color = { .h = hue_offset, .s = 1.0, .v = 0.5 };
            rgb rgb_color = hsv2rgb(hsv_color);

            red = (uint8_t)(fmin(rgb_color.r * 255, 120));
            green = (uint8_t)(fmin(rgb_color.g * 255, 120));
            blue = (uint8_t)(fmin(rgb_color.b * 255, 120));

            
            clamp_rgb(&red, &green, &blue);

            scale_grb_by_brightness(&green, &red, &blue, 0.3);

            esp_err_t ret = led_strip_set_pixel(rgb_manager->strip, i, red, green, blue);
            
            if (!rgb_manager->is_separate_pins) {
                led_strip_refresh(rgb_manager->strip);
            }
            hue = fmod(hue + 0.5, 360.0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

// Rainbow effect
void rgb_manager_rainbow_effect(RGBManager_t* rgb_manager, int delay_ms) {
    double hue = 0.0;

    while (1) {
        for (int i = 0; i < rgb_manager->num_leds; i++) {
RGBSTART:
            uint8_t red, green, blue;

           
            double hue_offset = fmod(hue + i * (360.0 / rgb_manager->num_leds), 360.0);

           
            hsv hsv_color = { .h = hue_offset, .s = 1.0, .v = 1.0 };
            hsv_color.v *= 0.5;
            rgb rgb_color = hsv2rgb(hsv_color);

          
            red = (uint8_t)(rgb_color.r * 180);
            green = (uint8_t)(rgb_color.g * 180);
            blue = (uint8_t)(rgb_color.b * 180);

            
            clamp_rgb(&red, &green, &blue);

            if (rgb_manager->is_separate_pins)
            {
                uint8_t ired = (uint8_t)(180 - (rgb_color.r * 180));
                uint8_t igreen = (uint8_t)(180 - (rgb_color.g * 180));
                uint8_t iblue = (uint8_t)(180 - (rgb_color.b * 180));
                rgb_manager_set_color(rgb_manager, i, ired, igreen, iblue, false);
                hue = fmod(hue + 1, 360.0);

        
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                goto RGBSTART;
            }

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
    bool is_red = true;

    while (1) {
        for (int pulse_step = 0; pulse_step <= 255; pulse_step += 5) {
            if (increasing) {
                brightness = pulse_step;
            } else {
                brightness = 255 - pulse_step;
            }

            
            if (is_red) {
                rgb_manager_set_color(rgb_manager, 0, brightness, 0, 0, false); 
            } else {
                rgb_manager_set_color(rgb_manager, 0, 0, 0, brightness, false); 
            }

            
            led_strip_refresh(rgb_manager->strip);

            
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        
        increasing = !increasing;

        
        if (!increasing) {
            is_red = !is_red;
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