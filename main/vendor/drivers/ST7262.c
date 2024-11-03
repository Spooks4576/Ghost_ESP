#include "vendor/drivers/ST7262.h"


#ifdef USE_7_INCHER

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_io_spi.h"
#include "lvgl.h"

static const char *TAG = "lcd_st7262";

// Panel handle
static esp_lcd_panel_handle_t rgb_panel_handle = NULL;

// LVGL display driver
static lv_disp_drv_t disp_drv;

// Semaphores for synchronization
static SemaphoreHandle_t sem_vsync_end = NULL;
static SemaphoreHandle_t sem_gui_ready = NULL;

// Data lines D0 to D15
#ifdef Crowtech_LCD
// Crowtech display (formerly Sasquatch display)
static const int lcd_data_gpio_nums[] = {
    GPIO_NUM_15, // D0 - B0
    GPIO_NUM_7,  // D1 - B1
    GPIO_NUM_6,  // D2 - B2
    GPIO_NUM_5,  // D3 - B3
    GPIO_NUM_4,  // D4 - B4
    GPIO_NUM_9,  // D5 - G0
    GPIO_NUM_46, // D6 - G1
    GPIO_NUM_3,  // D7 - G2
    GPIO_NUM_8,  // D8 - G3
    GPIO_NUM_16, // D9 - G4
    GPIO_NUM_1,  // D10 - G5
    GPIO_NUM_14, // D11 - R0
    GPIO_NUM_21, // D12 - R1
    GPIO_NUM_47, // D13 - R2
    GPIO_NUM_48, // D14 - R3
    GPIO_NUM_45  // D15 - R4
};

// Control signals for Crowtech display
#define LCD_HSYNC_GPIO_NUM   GPIO_NUM_39
#define LCD_VSYNC_GPIO_NUM   GPIO_NUM_40
#define LCD_DE_GPIO_NUM      GPIO_NUM_41
#define LCD_PCLK_GPIO_NUM    GPIO_NUM_0
#define LCD_DISP_GPIO_NUM    -1      // Not used
#define LCD_BACKLIGHT_GPIO   GPIO_NUM_2  
#define LCD_RESET_GPIO       GPIO_NUM_4  // Corrected to GPIO4

#elif Waveshare_LCD
// Waveshare display
static const int lcd_data_gpio_nums[] = {
    GPIO_NUM_14, // D0 - B3
    GPIO_NUM_38, // D1 - B4
    GPIO_NUM_18, // D2 - B5
    GPIO_NUM_17, // D3 - B6
    GPIO_NUM_10, // D4 - B7
    GPIO_NUM_39, // D5 - G2
    GPIO_NUM_0,  // D6 - G3
    GPIO_NUM_45, // D7 - G4
    GPIO_NUM_48, // D8 - G5
    GPIO_NUM_47, // D9 - G6
    GPIO_NUM_21, // D10 - G7
    GPIO_NUM_1,  // D11 - R3
    GPIO_NUM_2,  // D12 - R4
    GPIO_NUM_42, // D13 - R5
    GPIO_NUM_41, // D14 - R6
    GPIO_NUM_40  // D15 - R7
};

// Control signals for Waveshare display
#define LCD_HSYNC_GPIO_NUM   GPIO_NUM_46
#define LCD_VSYNC_GPIO_NUM   GPIO_NUM_3
#define LCD_DE_GPIO_NUM      GPIO_NUM_5
#define LCD_PCLK_GPIO_NUM    GPIO_NUM_7
#define LCD_DISP_GPIO_NUM    -1      // Not used
#define LCD_BACKLIGHT_GPIO   -1      // Not used
#define LCD_RESET_GPIO       GPIO_NUM_4  // Corrected to GPIO4

#elif Sunton_LCD

// Data pins for Waveshare display
static const int lcd_data_gpio_nums[] = {
    GPIO_NUM_8,  // D0 - B3
    GPIO_NUM_3,  // D1 - B4
    GPIO_NUM_46, // D2 - B5
    GPIO_NUM_9,  // D3 - B6
    GPIO_NUM_1,  // D4 - B7
    GPIO_NUM_5,  // D5 - G2
    GPIO_NUM_6,  // D6 - G3
    GPIO_NUM_7,  // D7 - G4
    GPIO_NUM_15, // D8 - G5
    GPIO_NUM_16, // D9 - G6
    GPIO_NUM_4,  // D10 - G7
    GPIO_NUM_45, // D11 - R3
    GPIO_NUM_48, // D12 - R4
    GPIO_NUM_47, // D13 - R5
    GPIO_NUM_21, // D14 - R6
    GPIO_NUM_14  // D15 - R7
};

// Control signals for Waveshare display
#define LCD_HSYNC_GPIO_NUM   GPIO_NUM_39
#define LCD_VSYNC_GPIO_NUM   GPIO_NUM_41
#define LCD_DE_GPIO_NUM      GPIO_NUM_40
#define LCD_PCLK_GPIO_NUM    GPIO_NUM_42
#define LCD_DISP_GPIO_NUM    GPIO_NUM_NC   // Not connected
#define LCD_BACKLIGHT_GPIO   GPIO_NUM_2    // Backlight
#define LCD_RESET_GPIO       GPIO_NUM_4    // Reset


#endif

// SPI pins for control interface (if used)
#define LCD_SPI_CS_GPIO_NUM      GPIO_NUM_13 // Adjust as per your hardware
#define LCD_SPI_SCK_GPIO_NUM     GPIO_NUM_12
#define LCD_SPI_MOSI_GPIO_NUM    GPIO_NUM_11
#define LCD_SPI_CLK_FREQ_HZ      (40 * 1000 * 1000) // 10MHz

static esp_lcd_panel_io_handle_t io_handle = NULL;

// LVGL flush callback
static void lcd_st7262_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_err_t ret;
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;

    
    size_t num_pixels = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    
    for (size_t i = 0; i < num_pixels; i++) {
        uint16_t pixel = color_map[i].full;
        color_map[i].full = (pixel >> 8) | (pixel << 8);
    }

    // Now proceed to draw bitmap
    ret = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to draw bitmap to panel");
    }

    // Inform LVGL that flushing is done
    lv_disp_flush_ready(drv);
}

esp_err_t lcd_st7262_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing ST7262 LCD panel");

    // Initialize reset GPIO
    if (LCD_RESET_GPIO >= 0) {
        gpio_config_t reset_conf = {
            .pin_bit_mask = 1ULL << LCD_RESET_GPIO,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&reset_conf));

        // Perform reset
        gpio_set_level(LCD_RESET_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LCD_RESET_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

#ifdef Crowtech_LCD
    int ClockFrequency = 15;
#elif Waveshare_LCD
    int ClockFrequency = 25;
#elif Sunton_LCD
    int ClockFrequency = 18;
#else 
    int ClockFrequency = 10;
#endif

    // Prepare RGB panel configuration with accurate timings
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = ClockFrequency * 1000 * 1000, // Pixel clock frequency based on the typical 25 MHz from datasheet
            .h_res = 800,
            .v_res = 480,
            .hsync_back_porch = 4,
            .hsync_front_porch = 4,
            .hsync_pulse_width = 2,
            .vsync_back_porch = 4,
            .vsync_front_porch = 4,
            .vsync_pulse_width = 2,
            .flags.pclk_active_neg = true, // Use as per your display’s requirements
        },
        .data_width = 16,
        .psram_trans_align = 64,
        .hsync_gpio_num = LCD_HSYNC_GPIO_NUM,
        .vsync_gpio_num = LCD_VSYNC_GPIO_NUM,
        .de_gpio_num = LCD_DE_GPIO_NUM,
        .pclk_gpio_num = LCD_PCLK_GPIO_NUM,
        .disp_gpio_num = LCD_DISP_GPIO_NUM,
        .data_gpio_nums = {
            [0] = lcd_data_gpio_nums[0],
            [1] = lcd_data_gpio_nums[1],
            [2] = lcd_data_gpio_nums[2],
            [3] = lcd_data_gpio_nums[3],
            [4] = lcd_data_gpio_nums[4],
            [5] = lcd_data_gpio_nums[5],
            [6] = lcd_data_gpio_nums[6],
            [7] = lcd_data_gpio_nums[7],
            [8] = lcd_data_gpio_nums[8],
            [9] = lcd_data_gpio_nums[9],
            [10] = lcd_data_gpio_nums[10],
            [11] = lcd_data_gpio_nums[11],
            [12] = lcd_data_gpio_nums[12],
            [13] = lcd_data_gpio_nums[13],
            [14] = lcd_data_gpio_nums[14],
            [15] = lcd_data_gpio_nums[15],
        },
        .flags.fb_in_psram = true,
        .num_fbs = 2, // Use double buffering
        .bounce_buffer_size_px = 20 * 480,
    };

    // Create RGB panel
    ret = esp_lcd_new_rgb_panel(&panel_config, &rgb_panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to create RGB panel");

    // Initialize the panel
    ret = esp_lcd_panel_reset(rgb_panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to reset panel");

    ret = esp_lcd_panel_init(rgb_panel_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize panel");

    // Turn on the display
    ret = esp_lcd_panel_disp_on_off(rgb_panel_handle, true);

#ifdef Crowtech_LCD || Sunton_LCD
    esp_rom_gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);

    gpio_set_level(2, 1);
#endif

    ESP_LOGI(TAG, "ST7262 LCD panel initialized successfully");
    return ESP_OK;
}

esp_err_t lcd_st7262_deinit(void)
{
    esp_err_t ret = ESP_OK;

    if (rgb_panel_handle) {
        ret = esp_lcd_panel_del(rgb_panel_handle);
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to delete panel");
        rgb_panel_handle = NULL;
    }

    if (io_handle) {
        ret = esp_lcd_panel_io_del(io_handle);
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to delete panel IO");
        io_handle = NULL;
    }

    ret = spi_bus_free(SPI2_HOST);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to free SPI bus");

    // Delete semaphores
    if (sem_vsync_end) {
        vSemaphoreDelete(sem_vsync_end);
        sem_vsync_end = NULL;
    }
    if (sem_gui_ready) {
        vSemaphoreDelete(sem_gui_ready);
        sem_gui_ready = NULL;
    }

    ESP_LOGI(TAG, "ST7262 LCD panel deinitialized successfully");
    return ESP_OK;
}

esp_lcd_panel_handle_t lcd_st7262_get_panel_handle(void)
{
    return rgb_panel_handle;
}

esp_err_t lcd_st7262_lvgl_init(void)
{
    if (rgb_panel_handle == NULL) {
        ESP_LOGE(TAG, "Panel not initialized. Call lcd_st7262_init() first.");
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize LVGL
    lv_init();

    // Create semaphores for synchronization
    sem_vsync_end = xSemaphoreCreateBinary();
    if (sem_vsync_end == NULL) {
        ESP_LOGE(TAG, "Failed to create sem_vsync_end");
        return ESP_ERR_NO_MEM;
    }

    sem_gui_ready = xSemaphoreCreateBinary();
    if (sem_gui_ready == NULL) {
        ESP_LOGE(TAG, "Failed to create sem_gui_ready");
        return ESP_ERR_NO_MEM;
    }

    // Allocate LVGL display buffer
    static lv_color_t *lvgl_disp_buf1 = NULL;
    static lv_color_t *lvgl_disp_buf2 = NULL;

    // Allocate 3 full-frame buffers in SPIRAM
    lvgl_disp_buf1 = heap_caps_malloc(800 * 480 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lvgl_disp_buf2 = heap_caps_malloc(800 * 480 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (lvgl_disp_buf1 == NULL || lvgl_disp_buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate frame buffers");
        return ESP_ERR_NO_MEM;
    }

    
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, lvgl_disp_buf1, lvgl_disp_buf2, 800 * 480);

    // Initialize LVGL display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800; // Adjust based on your display resolution
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = lcd_st7262_lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = (void *)rgb_panel_handle;
    disp_drv.full_refresh = false; // Enable full refresh mode for synchronization

    // Register the display driver with LVGL
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "LVGL initialized successfully");
    return ESP_OK;
}

#endif
