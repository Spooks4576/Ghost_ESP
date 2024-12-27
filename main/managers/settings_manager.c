#include "managers/settings_manager.h"
#include "managers/rgb_manager.h"
#include <string.h>
#include <esp_log.h>
#include <time.h>
#include "lvgl.h"
#include "managers/display_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define S_TAG "SETTINGS"

// NVS Keys
static const char* NVS_RGB_MODE_KEY = "rgb_mode";
static const char* NVS_CHANNEL_DELAY_KEY = "channel_delay";
static const char* NVS_BROADCAST_SPEED_KEY = "broadcast_speed";
static const char* NVS_AP_SSID_KEY = "ap_ssid";
static const char* NVS_AP_PASSWORD_KEY = "ap_password";
static const char* NVS_RGB_SPEED_KEY = "rgb_speed";
static const char* NVS_PORTAL_URL_KEY = "portal_url";
static const char* NVS_PORTAL_SSID_KEY = "portal_ssid";
static const char* NVS_PORTAL_PASSWORD_KEY = "portal_password";
static const char* NVS_PORTAL_AP_SSID_KEY = "portal_ap_ssid";
static const char* NVS_PORTAL_DOMAIN_KEY = "portal_domain";
static const char* NVS_PORTAL_OFFLINE_KEY = "portal_offline";
static const char* NVS_PRINTER_IP_KEY = "printer_ip";
static const char* NVS_PRINTER_TEXT_KEY = "printer_text";
static const char* NVS_PRINTER_FONT_SIZE_KEY = "printer_font_size";
static const char* NVS_PRINTER_ALIGNMENT_KEY = "printer_alignment";
static const char* NVS_PRINTER_CONNECTED_KEY = "printer_connected";
static const char* NVS_BOARD_TYPE_KEY = "board_type";
static const char* NVS_CUSTOM_PIN_CONFIG_KEY = "custom_pin_config";
static const char* NVS_FLAPPY_GHOST_NAME = "flap_name";
static const char* NVS_TIMEZONE_NAME = "sel_tz";
static const char* NVS_ACCENT_COLOR = "sel_ac";
static const char* NVS_GPS_RX_PIN = "gps_rx_pin";
static const char* NVS_DISPLAY_TIMEOUT_KEY = "disp_timeout";
static const char* NVS_ENABLE_RTS_KEY = "rts_enable";

extern lv_timer_t *time_update_timer;

static const char* TAG = "SettingsManager";

void settings_init(FSettings* settings) {
    settings_set_defaults(settings);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
        settings_load(settings);
        printf("Settings loaded successfully.\n");
    } else {
        printf("Failed to open NVS handle: %s\n", esp_err_to_name(err));
    }
}

void settings_deinit(void) {
    nvs_close(nvsHandle);
}

void settings_set_defaults(FSettings* settings) {
    settings->rgb_mode = RGB_MODE_NORMAL;
    settings->channel_delay = 1.0f;
    settings->broadcast_speed = 5;
    strcpy(settings->ap_ssid, "GhostNet");
    strcpy(settings->ap_password, "GhostNet");
    settings->rgb_speed = 15;

    // Evil Portal defaults
    strcpy(settings->portal_url, "/default/path");
    strcpy(settings->portal_ssid, "EvilPortal");
    strcpy(settings->portal_password, "EvilPortalPass");
    strcpy(settings->portal_ap_ssid, "EvilAP");
    strcpy(settings->portal_domain, "portal.local");
    settings->portal_offline_mode = false;

    // Power Printer defaults
    strcpy(settings->printer_ip, "192.168.1.100");
    strcpy(settings->printer_text, "Default Text");
    settings->printer_font_size = 12;
    settings->printer_alignment = ALIGNMENT_CM;
    strcpy(settings->flappy_ghost_name, "Bob");
    strcpy(settings->selected_hex_accent_color, "#ffffff");
    strcpy(settings->selected_timezone, "MST7MDT,M3.2.0,M11.1.0");
    settings->gps_rx_pin = 0;
    settings->display_timeout_ms = 10000; // Default 10 seconds
    settings->rts_enabled = false;
}

void settings_load(FSettings* settings) {
    esp_err_t err;
    uint8_t value_u8;
    uint16_t value_u16;
    float value_float;
    size_t str_size;

    // Load RGB Mode
    err = nvs_get_u8(nvsHandle, NVS_RGB_MODE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->rgb_mode = (RGBMode)value_u8;
    }

    size_t required_size = sizeof(value_float); // Set the size of the buffer
    err = nvs_get_blob(nvsHandle, NVS_CHANNEL_DELAY_KEY, &value_float, &required_size);
    if (err == ESP_OK) {
        settings->channel_delay = value_float;
    } else {
        printf("Failed to load Channel Delay: %s\n", esp_err_to_name(err));
    }

    // Load Broadcast Speed
    err = nvs_get_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, &value_u16);
    if (err == ESP_OK) {
        settings->broadcast_speed = value_u16;
    }

    // Load AP SSID
    str_size = sizeof(settings->ap_ssid);
    err = nvs_get_str(nvsHandle, NVS_AP_SSID_KEY, settings->ap_ssid, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load AP SSID\n");
    }

    // Load AP Password
    str_size = sizeof(settings->ap_password);
    err = nvs_get_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->ap_password, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load AP Password\n");
    }

    // Load RGB Speed
    err = nvs_get_u8(nvsHandle, NVS_RGB_SPEED_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->rgb_speed = value_u8;
    }

    // Load Evil Portal settings
    str_size = sizeof(settings->portal_url);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_URL_KEY, settings->portal_url, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Portal URL\n");
    }

    str_size = sizeof(settings->portal_ssid);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_SSID_KEY, settings->portal_ssid, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Portal SSID\n");
    }

    str_size = sizeof(settings->portal_password);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_PASSWORD_KEY, settings->portal_password, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Portal Password\n");
    }

    str_size = sizeof(settings->portal_ap_ssid);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_AP_SSID_KEY, settings->portal_ap_ssid, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Portal AP SSID\n");
    }

    str_size = sizeof(settings->portal_domain);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_DOMAIN_KEY, settings->portal_domain, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Portal Domain\n");
    }

    err = nvs_get_u8(nvsHandle, NVS_PORTAL_OFFLINE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->portal_offline_mode = value_u8;
    }

    // Load Power Printer settings
    str_size = sizeof(settings->printer_ip);
    err = nvs_get_str(nvsHandle, NVS_PRINTER_IP_KEY, settings->printer_ip, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Printer IP\n");
    }

    str_size = sizeof(settings->printer_text);
    err = nvs_get_str(nvsHandle, NVS_PRINTER_TEXT_KEY, settings->printer_text, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Printer Text\n");
    }

    err = nvs_get_u8(nvsHandle, NVS_PRINTER_FONT_SIZE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->printer_font_size = value_u8;
    }

    err = nvs_get_u8(nvsHandle, NVS_PRINTER_ALIGNMENT_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->printer_alignment = (PrinterAlignment)value_u8;
    }

    str_size = sizeof(settings->flappy_ghost_name);
    err = nvs_get_str(nvsHandle, NVS_FLAPPY_GHOST_NAME, settings->flappy_ghost_name, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Flappy Ghost Name\n");
    }

#ifdef CONFIG_HAS_RTC_CLOCK
    str_size = sizeof(settings->selected_timezone);
    err = nvs_get_str(nvsHandle, NVS_TIMEZONE_NAME, settings->selected_timezone, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Timezone String\n");
    }
#endif
    
    str_size = sizeof(settings->selected_hex_accent_color);
    err = nvs_get_str(nvsHandle, NVS_ACCENT_COLOR, settings->selected_hex_accent_color, &str_size);
    if (err != ESP_OK) {
        printf("Failed to load Hex Accent Color String\n");
    }

    err = nvs_get_u8(nvsHandle, NVS_GPS_RX_PIN, &value_u8);
    if (err == ESP_OK) {
        settings->gps_rx_pin = value_u8;
    }

    uint32_t timeout_value;
    err = nvs_get_u32(nvsHandle, NVS_DISPLAY_TIMEOUT_KEY, &timeout_value);
    if (err == ESP_OK) {
        settings->display_timeout_ms = timeout_value;
    } else {
        settings->display_timeout_ms = 10000; // Default 10 seconds if not found
    }

    uint8_t rtsenabledvalue;
    err = nvs_get_u8(nvsHandle, NVS_ENABLE_RTS_KEY, &rtsenabledvalue);
    if (err == ESP_OK)
    {
        settings->rts_enabled = rtsenabledvalue;
    }
    else 
    {
        settings->rts_enabled = false;
    }

    printf("Settings loaded from NVS.\n");
}

void settings_save(const FSettings* settings) {
    ESP_LOGI(TAG, "Starting settings save process");
    ESP_LOGI(TAG, "Current display timeout: %lu ms", settings->display_timeout_ms);
    ESP_LOGI(TAG, "Current timezone: %s", settings->selected_timezone);
    
    esp_err_t err;

    // Save RGB Mode
    err = nvs_set_u8(nvsHandle, NVS_RGB_MODE_KEY, (uint8_t)settings->rgb_mode);
    if (err != ESP_OK) {
        printf("Failed to save RGB Mode\n");
    }

    // Save Channel Delay
    err = nvs_set_blob(nvsHandle, NVS_CHANNEL_DELAY_KEY, &settings->channel_delay, sizeof(settings->channel_delay));
    if (err != ESP_OK) {
        printf("Failed to save Channel Delay\n");
    }

    // Save Broadcast Speed
    err = nvs_set_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, settings->broadcast_speed);
    if (err != ESP_OK) {
        printf("Failed to save Broadcast Speed\n");
    }

    // Save AP SSID
    err = nvs_set_str(nvsHandle, NVS_AP_SSID_KEY, settings->ap_ssid);
    if (err != ESP_OK) {
        printf("Failed to save AP SSID\n");
    }

    // Save AP Password
    err = nvs_set_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->ap_password);
    if (err != ESP_OK) {
        printf("Failed to save AP Password\n");
    }

    // Save RGB Speed
    err = nvs_set_u8(nvsHandle, NVS_RGB_SPEED_KEY, settings->rgb_speed);
    if (err != ESP_OK) {
        printf("Failed to save RGB Speed\n");
    }

    // Save RTS Enabled
    err = nvs_set_u8(nvsHandle, NVS_ENABLE_RTS_KEY, settings->rts_enabled);
    if (err != ESP_OK) {
        printf("Failed to save RTS Enabled\n");
    }

    // Save Evil Portal settings
    err = nvs_set_str(nvsHandle, NVS_PORTAL_URL_KEY, settings->portal_url);
    if (err != ESP_OK) {
        printf("Failed to save Portal URL\n");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_SSID_KEY, settings->portal_ssid);
    if (err != ESP_OK) {
        printf("Failed to save Portal SSID\n");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_PASSWORD_KEY, settings->portal_password);
    if (err != ESP_OK) {
        printf("Failed to save Portal Password\n");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_AP_SSID_KEY, settings->portal_ap_ssid);
    if (err != ESP_OK) {
        printf("Failed to save Portal AP SSID\n");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_DOMAIN_KEY, settings->portal_domain);
    if (err != ESP_OK) {
        printf("Failed to save Portal Domain\n");
    }

    err = nvs_set_u8(nvsHandle, NVS_PORTAL_OFFLINE_KEY, settings->portal_offline_mode);
    if (err != ESP_OK) {
        printf("Failed to save Portal Offline Mode\n");
    }

    // Save Power Printer settings
    err = nvs_set_str(nvsHandle, NVS_PRINTER_IP_KEY, settings->printer_ip);
    if (err != ESP_OK) {
        printf("Failed to save Printer IP\n");
    }

    err = nvs_set_str(nvsHandle, NVS_PRINTER_TEXT_KEY, settings->printer_text);
    if (err != ESP_OK) {
        printf("Failed to save Printer Text\n");
    }

    err = nvs_set_u8(nvsHandle, NVS_PRINTER_FONT_SIZE_KEY, settings->printer_font_size);
    if (err != ESP_OK) {
        printf("Failed to save Printer Font Size\n");
    }

    err = nvs_set_u8(nvsHandle, NVS_PRINTER_ALIGNMENT_KEY, (uint8_t)settings->printer_alignment);
    if (err != ESP_OK) {
        printf("Failed to save Printer Alignment\n");
    }

    err = nvs_set_str(nvsHandle, NVS_FLAPPY_GHOST_NAME, settings->flappy_ghost_name);
    if (err != ESP_OK) {
        printf("Failed to save Flappy Ghost Name\n");
    }

    err = nvs_set_str(nvsHandle, NVS_TIMEZONE_NAME, settings->selected_timezone);
    if (err != ESP_OK) {
        printf("Failed to Save Timezone String %s\n", esp_err_to_name(err));
    }

    err = nvs_set_str(nvsHandle, NVS_ACCENT_COLOR, settings->selected_hex_accent_color);
    if (err != ESP_OK) {
        printf("Failed to Save Hex Accent Color %s", esp_err_to_name(err));
    }

    err = nvs_set_u8(nvsHandle, NVS_GPS_RX_PIN, (uint8_t)settings->gps_rx_pin);
    if (err != ESP_OK) {
        printf("Failed to save Printer Alignment\n");
    }

    err = nvs_set_u32(nvsHandle, NVS_DISPLAY_TIMEOUT_KEY, settings->display_timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save Display Timeout");
    }

    if (settings_get_rgb_mode(&G_Settings) == 0)
    {
        if (rgb_effect_task_handle != NULL)
        {
            vTaskDelete(rgb_effect_task_handle);
            rgb_effect_task_handle = NULL;
        }
        rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
    }
    else 
    {
        if (rgb_effect_task_handle == NULL)
        {
            xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
        }
    }

    // Commit all changes
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        printf("Failed to commit NVS changes\n");
    } else {
        printf("Settings saved to NVS.\n");
    }

#ifdef CONFIG_HAS_RTC_CLOCK
    // Apply timezone change immediately
    ESP_LOGI(TAG, "Applying timezone change: %s", settings->selected_timezone);
    setenv("TZ", settings->selected_timezone, 1);
    tzset();
    
    // Force time display update if it exists
    if (time_update_timer) {
        ESP_LOGI(TAG, "Forcing time display update");
        lv_timer_ready(time_update_timer);
    } else {
        ESP_LOGW(TAG, "Time update timer not available");
    }
#endif

    // Update global settings immediately
    ESP_LOGI(TAG, "Updating global settings");
    ESP_LOGI(TAG, "Old display timeout: %lu ms", G_Settings.display_timeout_ms);
    memcpy(&G_Settings, settings, sizeof(FSettings));
    ESP_LOGI(TAG, "New display timeout: %lu ms", G_Settings.display_timeout_ms);

    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Settings saved to NVS successfully");
    }
}

// Core Settings Getters and Setters
void settings_set_rgb_mode(FSettings* settings, RGBMode mode) {
    settings->rgb_mode = mode;
}


void settings_set_rts_enabled(FSettings* settings, bool enabled) {
    settings->rts_enabled = enabled;
}


bool settings_get_rts_enabled(FSettings* settings) {
    return settings->rts_enabled;
}

RGBMode settings_get_rgb_mode(const FSettings* settings) {
    return settings->rgb_mode;
}

void settings_set_channel_delay(FSettings* settings, float delay_ms) {
    settings->channel_delay = delay_ms;
}

float settings_get_channel_delay(const FSettings* settings) {
    return settings->channel_delay;
}

void settings_set_broadcast_speed(FSettings* settings, uint16_t speed) {
    settings->broadcast_speed = speed;
}

uint16_t settings_get_broadcast_speed(const FSettings* settings) {
    return settings->broadcast_speed;
}

void settings_set_flappy_ghost_name(FSettings *settings, const char *Name)
{
    strncpy(settings->flappy_ghost_name, Name, sizeof(settings->flappy_ghost_name) - 1);
    settings->flappy_ghost_name[sizeof(settings->flappy_ghost_name) - 1] = '\0';
}

const char *settings_get_flappy_ghost_name(const FSettings *settings)
{
    return settings->flappy_ghost_name;
}

void settings_set_timezone_str(FSettings *settings, const char *Name)
{
    strncpy(settings->selected_timezone, Name, sizeof(settings->selected_timezone) - 1);
    settings->selected_timezone[sizeof(settings->selected_timezone) - 1] = '\0';
}

const char *settings_get_timezone_str(const FSettings *settings)
{
    return settings->selected_timezone;
}

void settings_set_accent_color_str(FSettings *settings, const char *Name)
{
    strncpy(settings->selected_hex_accent_color, Name, sizeof(settings->selected_hex_accent_color) - 1);
    settings->selected_hex_accent_color[sizeof(settings->selected_hex_accent_color) - 1] = '\0';
}

const char *settings_get_accent_color_str(const FSettings *settings)
{
    return settings->selected_hex_accent_color;
}

void settings_set_ap_ssid(FSettings *settings, const char *ssid)
{
    strncpy(settings->ap_ssid, ssid, sizeof(settings->ap_ssid) - 1);
    settings->ap_ssid[sizeof(settings->ap_ssid) - 1] = '\0';
}

const char* settings_get_ap_ssid(const FSettings* settings) {
    return settings->ap_ssid;
}

void settings_set_ap_password(FSettings* settings, const char* password) {
    strncpy(settings->ap_password, password, sizeof(settings->ap_password) - 1);
    settings->ap_password[sizeof(settings->ap_password) - 1] = '\0';
}

const char* settings_get_ap_password(const FSettings* settings) {
    return settings->ap_password;
}

void settings_set_gps_rx_pin(FSettings* settings, uint8_t RxPin) {
    settings->gps_rx_pin = RxPin;
}

uint8_t settings_get_gps_rx_pin(const FSettings* settings) {
    return settings->gps_rx_pin;
}

void settings_set_rgb_speed(FSettings* settings, uint8_t speed) {
    settings->rgb_speed = speed;
}

uint8_t settings_get_rgb_speed(const FSettings* settings) {
    return settings->rgb_speed;
}

// Evil Portal Getters and Setters
void settings_set_portal_url(FSettings* settings, const char* url) {
    strncpy(settings->portal_url, url, sizeof(settings->portal_url) - 1);
    settings->portal_url[sizeof(settings->portal_url) - 1] = '\0';
}

const char* settings_get_portal_url(const FSettings* settings) {
    return settings->portal_url;
}

void settings_set_portal_ssid(FSettings* settings, const char* ssid) {
    strncpy(settings->portal_ssid, ssid, sizeof(settings->portal_ssid) - 1);
    settings->portal_ssid[sizeof(settings->portal_ssid) - 1] = '\0';
}

const char* settings_get_portal_ssid(const FSettings* settings) {
    return settings->portal_ssid;
}

void settings_set_portal_password(FSettings* settings, const char* password) {
    strncpy(settings->portal_password, password, sizeof(settings->portal_password) - 1);
    settings->portal_password[sizeof(settings->portal_password) - 1] = '\0';
}

const char* settings_get_portal_password(const FSettings* settings) {
    return settings->portal_password;
}

void settings_set_portal_ap_ssid(FSettings* settings, const char* ap_ssid) {
    strncpy(settings->portal_ap_ssid, ap_ssid, sizeof(settings->portal_ap_ssid) - 1);
    settings->portal_ap_ssid[sizeof(settings->portal_ap_ssid) - 1] = '\0';
}

const char* settings_get_portal_ap_ssid(const FSettings* settings) {
    return settings->portal_ap_ssid;
}

void settings_set_portal_domain(FSettings* settings, const char* domain) {
    strncpy(settings->portal_domain, domain, sizeof(settings->portal_domain) - 1);
    settings->portal_domain[sizeof(settings->portal_domain) - 1] = '\0';
}

const char* settings_get_portal_domain(const FSettings* settings) {
    return settings->portal_domain;
}

void settings_set_portal_offline_mode(FSettings* settings, bool offline_mode) {
    settings->portal_offline_mode = offline_mode;
}

bool settings_get_portal_offline_mode(const FSettings* settings) {
    return settings->portal_offline_mode;
}

// Power Printer Getters and Setters
void settings_set_printer_ip(FSettings* settings, const char* ip) {
    strncpy(settings->printer_ip, ip, sizeof(settings->printer_ip) - 1);
    settings->printer_ip[sizeof(settings->printer_ip) - 1] = '\0';
}

const char* settings_get_printer_ip(const FSettings* settings) {
    return settings->printer_ip;
}

void settings_set_printer_text(FSettings* settings, const char* text) {
    strncpy(settings->printer_text, text, sizeof(settings->printer_text) - 1);
    settings->printer_text[sizeof(settings->printer_text) - 1] = '\0';
}

const char* settings_get_printer_text(const FSettings* settings) {
    return settings->printer_text;
}

void settings_set_printer_font_size(FSettings* settings, uint8_t font_size) {
    settings->printer_font_size = font_size;
}

uint8_t settings_get_printer_font_size(const FSettings* settings) {
    return settings->printer_font_size;
}

void settings_set_printer_alignment(FSettings* settings, PrinterAlignment alignment) {
    settings->printer_alignment = alignment;
}

PrinterAlignment settings_get_printer_alignment(const FSettings* settings) {
    return settings->printer_alignment;
}

void settings_set_display_timeout(FSettings* settings, uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Setting display timeout from %lu to %lu ms", settings->display_timeout_ms, timeout_ms);
    settings->display_timeout_ms = timeout_ms;
}

uint32_t settings_get_display_timeout(const FSettings* settings) {
    return settings->display_timeout_ms;
}